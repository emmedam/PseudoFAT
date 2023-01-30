#include "fat.h"

BootRecord *boot_record;

void save(char*);

//lista concatenata che rappresenta la working_dir
ListPath* working_dir;



void init(){
       
    boot_record = (BootRecord*) malloc(sizeof(BootRecord));
    
    

 
}

void* format(char *name){
    
    time_t creation_time = time(NULL); 
    printf("sizeof bootrecord %ld\n", sizeof(*boot_record));
    boot_record->byte_per_sector        = (u_int16_t)BYTE_PER_SECTOR;
    boot_record->sector_per_cluster     = (u_int16_t)SECTOR_PER_CLUSTER;
    boot_record->n_cluster              = (u_int16_t)NUMBER_OF_CLUSTER;
    boot_record->n_directory_entries    = (u_int16_t)NUMBER_OF_DIRECTORY_ENTRIES;
    boot_record->date                   = (time_t)creation_time;
    strcpy(boot_record->name, name);
       
    printf("%d\n", boot_record->byte_per_sector);
    printf("%d\n", boot_record->sector_per_cluster);
    printf("%d\n", boot_record->n_cluster);
    printf("%d\n", boot_record->n_directory_entries);
    printf("name: %s\n", boot_record->name);

    save(name);
    
}


//legge il contenuto del disco, legge il file e lo mappa in memoria
void readDisk(char* f_name){
    
    int fd_in = open(f_name, O_RDWR);
    if(fd_in < 0 ) {
        perror("impossbile aprire file:");
        exit(1);
    }
    //mappo il contenuto del disco rappresentato dal file in memoria
    disk = mmap(NULL, disk_length(), PROT_WRITE|PROT_READ, MAP_SHARED, fd_in, 0);
 
    if(disk == MAP_FAILED){
        perror("Mapping Failed: "); 
        fprintf(stderr, "Value of errno: %d\n", errno);
        printf( "%s\n", strerror( errno)); 
        exit(EXIT_FAILURE);
        
    }

    close(fd_in);

}

// n indica il numero i-esimo del settore, ove il
// primo settore s'intende uguale a 0
void* readSector(int n){
    return (disk + (BYTE_PER_SECTOR * n));
}

void readCluster(){

}

u_int16_t first_sector_of_cluster(u_int16_t n_cluster){
    return 1 + fatSectorNumber() + 
            boot_record->n_directory_entries + 
            (boot_record->sector_per_cluster * n_cluster);
}

void readDirEntry(DirectoryEntry *dir_entry, void* sector){
    dir_entry->creation_date    = *(time_t*)(sector + 0);
    dir_entry->update_date      = *(time_t*)(sector + 8);
    dir_entry->first_cluster    = *(u_int16_t*)(sector + 16);
    dir_entry->dimension        = *(u_int16_t*)(sector + 18);
    strcpy(dir_entry->name, (char*)(sector + 20));
}


void readBootRecord(){
    char *s = readSector(0);
    boot_record->byte_per_sector        = *(s+0);
    boot_record->sector_per_cluster     = *(u_int16_t*)(s+2);
    boot_record->n_cluster              = *(u_int16_t*)(s+4);
    boot_record->n_directory_entries    = *(u_int16_t*)(s+6);
    boot_record->date                   = *(time_t*)(s+8);
    strcpy(boot_record->name, (char*)(s+16));

    init_root();
    
}

//inizializzo la working dir a root
void init_root(){
    DirectoryEntry* root = (DirectoryEntry*)malloc(sizeof(DirectoryEntry));
    strcpy(root->name, "root");
    root->creation_date = boot_record->date;
    root->update_date = 0;
    root->first_cluster = 0;
    root->dimension = 0;

    working_dir = list_init(root);
}
void cluster_info(int *free_cluster, int *n_files){
    char *s = readSector(1);
    int data = 0;
    for(int i=0; i < (boot_record->n_cluster); i++){
        data = *(s+i*2);
        if( data == 0 ) (*free_cluster)++; 
        else if(data == 1) (*n_files)++;
    }

    // sottraggo 2 poiché i primi due elementi della FAT
    // sono inutilizzati, ma valorizzati con il valore 1
    (*n_files) -= 2;
}

size_t disk_length(){
    return (1+fatSectorNumber()+ dirTableSectorNumber()+
            dataAreaSectorNumber())*BYTE_PER_SECTOR;
}

void info(){
    
    printf("n. byte per sector: %d\n",          (boot_record->byte_per_sector));
    printf("n. sector per cluster: %d\n",       (boot_record->sector_per_cluster));
    printf("n. cluster: %d\n",                  (boot_record->n_cluster));
    printf("n. entries directory table: %d\n",  (boot_record->n_directory_entries));
    char *date = (char*)malloc(20 * sizeof(char));
    printf("creation time: %s\n",             formatTime(date,(boot_record->date)));
    printf("volume name: %s\n",                 (char*)(boot_record->name));
    int free_cluster = 0, n_files = 0;
    cluster_info(&free_cluster, &n_files);
    printf("numero di cluster liberi: %d\n",    free_cluster);
    printf("numero di files: %d\n",             n_files);
    printf("dimensione: %ld Bytes\n",           disk_length());

    free(date);                                                    

}


int fatSectorNumber(){
    return ceil((NUMBER_OF_CLUSTER * 2 / (double)BYTE_PER_SECTOR));
  
}

int dirTableSectorNumber(){
    return NUMBER_OF_DIRECTORY_ENTRIES;
}

long dataAreaSectorNumber(){
    return NUMBER_OF_CLUSTER * SECTOR_PER_CLUSTER;
}


//trovo primo cluster libero nella FAT 
int get_free_cluster(){
    for(int i = 0; i < fatSectorNumber(); i++){
        char *sector = readSector(i + 1);
        for(int j = i * (boot_record->byte_per_sector / 2); 
            j < (boot_record->byte_per_sector) / 2 * (i + 1); j++){
            if((*(sector+j*2)) == 0)
                return j;
        }
    }
    //FAT piena nessun cluster libero
    return 0;
}




void printDirectoryEntry(DirectoryEntry *d){
    printf("name: %s\n", d -> name);
    char *date = (char*)malloc(20 * sizeof(char));
    printf("creation date: %s\n", formatTime(date,(d -> creation_date)));
    printf("update date: %s\n", formatTime(date,(d -> update_date)));
    printf("first cluster: %d\n", d -> first_cluster);
    printf("dimension: %d\n", d -> dimension);
    free(date);
}

void createDir(char* dirname){
    //controllo se validità dirname, se non NULL
    if(!dirname){
        printf(COLOR_RED "impossibile creare directory, nome non valido\n" COLOR_DEFAULT);
        return;
    }
    //verifico se la lunghezza di dirname è minore o uguale di 11 caratteri
    if(strlen(dirname) > 11){
        printf(COLOR_RED "Nome directory non consentito, massimo 11 caratteri\n" COLOR_DEFAULT);
        return;
    }
    //controllo se directory già esiste
    DirectoryEntry *dir_entry = (DirectoryEntry*)malloc(sizeof(DirectoryEntry));
    
    int first_free_sector = 0;
    
    u_int16_t n_sector;
    if (current_dir(working_dir)->first_cluster == 0)
        n_sector = 1 + fatSectorNumber();
    else
        n_sector = first_sector_of_cluster(current_dir(working_dir)->first_cluster);

    for(int i = 0; i < boot_record -> n_directory_entries; i++){
        readDirEntry(dir_entry, readSector(n_sector+ i));
        if(strcmp((dir_entry -> name), dirname) == 0){
            printf(COLOR_RED "Directory già esistente\n" COLOR_DEFAULT );
            printDirectoryEntry(dir_entry);
            free(dir_entry);
            return;
        }
        // //per discriminare se settore sia vuoto, quindi non contiene 
        //nessuna dir entry, è sufficiente valutare se abbia valorizzato
        //l'attributo primo cluster
        if(first_free_sector == 0  &&  dir_entry -> first_cluster == 0){
            first_free_sector = n_sector + i;
        
        }
    }   
    
    //se first_free_sector è ancora uguale a zero
    //vuol dire che la directory table è piena
    if(first_free_sector == 0){
        printf("La directory table è piena, impossibile aggiungere nuovi elementi\n");
        free(dir_entry);
        return;
    }


    //controllo se ho cluster liberi, altrimenti non posso creare nuove directory
    u_int16_t free_cluster = get_free_cluster();
    if(free_cluster == 0){
        printf("nessun cluster disponibile, impossibile creare nuove directory\n");
        free(dir_entry);
        return;
    }

    //creo nuova direcotry
    printf("Creo nuova directory, settore disponibile nella dir-entry: %d\n", first_free_sector);

    time_t creation_time        = time(NULL); 
    dir_entry->creation_date    = creation_time;
    dir_entry->update_date      = 0;
    dir_entry->first_cluster    = free_cluster;
    dir_entry->dimension        = (u_int16_t)0;
    strcpy(dir_entry->name, dirname);
    

    printf("Write on disk %ld bytes\n", sizeof(*dir_entry));
    printf("directory name: %s\n", dir_entry -> name);
    // printf("cluster: %d\n", dir_entry -> first_cluster);

    //scrivo sul disco la directory entry sullo spazio di memoria mappato
    memcpy(readSector(first_free_sector), dir_entry, sizeof(*dir_entry));

   
    
    //Scrivo nella FAT il cluster della directory appena creata
    u_int16_t last = 1;
    memcpy( (disk + (boot_record->byte_per_sector + free_cluster * 2)) , &last, sizeof(u_int16_t));

    
    free(dir_entry);
}

// void printFAT(){
//     for(int i=0; i < fatSectorNumber(); i++){
//         char *sector = readSector(i+1);
//         for(int j = 2; j < (boot_record->byte_per_sector) / 2; j++){
//             if( *(sector + j  * 2) != 0){
//                 printf("[%d]: %d\n", j, *(sector + j  * 2));
//             }
//         }
//     }
// }

void save(char *name){
    FILE * fd_out=fopen(name, "wb");
    
    fwrite(boot_record, sizeof(char)*BYTE_PER_SECTOR, 1, fd_out);
    
    //inizializzo a 1 i primi due byte della FAT che non verranno utilizzati
    u_int16_t one = 1;
    fwrite( &one ,sizeof(u_int16_t), 1, fd_out);
    fwrite( &one ,sizeof(u_int16_t), 1, fd_out);

    //uso calloc in modo da inizializzare a 0 tutti gli elementi della Fat table e allocare spazio per la fat table
    fwrite(calloc(fatSectorNumber() * BYTE_PER_SECTOR -  2 * sizeof(u_int16_t), 1), 
        fatSectorNumber() * BYTE_PER_SECTOR - 2 * sizeof(u_int16_t), 1, fd_out);
    
    //alloco spazio per la directory table della root ( o root directory) 
    fwrite(calloc(dirTableSectorNumber() * BYTE_PER_SECTOR, 1), dirTableSectorNumber() * BYTE_PER_SECTOR, 1, fd_out);
    
    //alloco spazio per la data area
    fwrite(calloc(dataAreaSectorNumber() * BYTE_PER_SECTOR, 1), dataAreaSectorNumber() * BYTE_PER_SECTOR, 1, fd_out);

    fflush(fd_out);
    fclose(fd_out);
}

void listDir(DirectoryEntry* dir){
    
    //controlla se parametro dir è stato fornito, altrimenti si assume come dir la working_dir
    // if(!dir){
    //     dir = working_dir;
    // }
    DirectoryEntry *tmp_dir = (DirectoryEntry*)malloc(sizeof(DirectoryEntry));
    tmp_dir->creation_date = dir->creation_date;
    tmp_dir->dimension = dir->dimension;
    tmp_dir->first_cluster = dir->first_cluster;
    strcpy(tmp_dir->name, dir->name);
    tmp_dir->update_date = dir->update_date;
    
    u_int16_t n_sector;
    if (current_dir(working_dir)->first_cluster == 0)
        n_sector = 1 + fatSectorNumber();
    else
        n_sector = first_sector_of_cluster(current_dir(working_dir)->first_cluster);


    for(int i = 0; i < boot_record -> n_directory_entries; i++){
        readDirEntry(tmp_dir, readSector(n_sector+ i));
        
        if(tmp_dir->creation_date != 0){
            
            char *date = (char*)malloc(20 * sizeof(char));
            
            printf("%s\t", formatTime(date,(tmp_dir->creation_date))); 
            printf("%d\t", tmp_dir->dimension);
            
            
            if(tmp_dir->update_date != 0){
                printf("%s", tmp_dir->name);
            }
            else{
                printf(COLOR_GREEN "%s" COLOR_DEFAULT, tmp_dir->name);
            }

            //stampo la sequenza dei cluster memorizzati nella FAT 
            u_int16_t curr_cluster = tmp_dir->first_cluster;
            int sector = ceil(curr_cluster / (double)boot_record->byte_per_sector/2); 
            printf("\t\t%d", curr_cluster);
            while(1){
                curr_cluster = *((u_int16_t*)(readSector( sector ) + curr_cluster * 2));
                if(curr_cluster == 1){
                    printf("\n");
                   break;
                }
                printf("-%d", curr_cluster);
                sector = ceil(curr_cluster / (double)boot_record->byte_per_sector/2); 
            }
            
        }

            
        
    }   
    free(tmp_dir);
}


void changeDir(char* dir_name){
    
    
     if(!dir_name){
        printf(COLOR_RED "fornire directory\n" COLOR_DEFAULT);
        return;
    }

    if(strcmp(dir_name, "..") == 0){
        remove_last(working_dir);
        return;
    }

    if(strcmp(dir_name, ".") == 0){
        reset_path(working_dir);
        init_root();
        return;
    }


    DirectoryEntry *tmp_dir = (DirectoryEntry*)malloc(sizeof(DirectoryEntry));
    
    u_int16_t n_sector;
    if (current_dir(working_dir)->first_cluster == 0)
        n_sector = 1 + fatSectorNumber();
    else
        n_sector = first_sector_of_cluster(current_dir(working_dir)->first_cluster);

    for(int i = 0; i < boot_record -> n_directory_entries; i++){
        readDirEntry(tmp_dir, readSector(n_sector+i));

        //controllo se nome corrisponde e che sia una direcotry e non un file
        if( strcmp(tmp_dir->name, dir_name) == 0 && tmp_dir->update_date == 0 ){
            //aggiorno la lista
            ListPath* new_item = (ListPath*)malloc(sizeof(ListPath));
            new_item->dir_entry = tmp_dir;
            list_insert(working_dir, new_item);
            return;
        }
    }
    
    //directory non esiste nella working dir   
    printf("Directory insesistente\n");

}

void createFile(char* file_name, int dimension){
    //verifico se la lunghezza di file_name è minore o uguale di 11 caratteri
    if(strlen(file_name) > 11){
        printf(COLOR_RED "Nome file non consentito, massimo 11 caratteri\n" COLOR_DEFAULT);
        return;
    }
    
    //conto il numero di cluster necessari per memorizzare il file
    u_int16_t n_cluster = ceil( dimension / (double)(boot_record->sector_per_cluster * boot_record->byte_per_sector));

    //verifico se ho n_cluster liberi nella FAT necessari per memorizzare file
    int count = 0;
    for(int i = 2; i < fatSectorNumber(); i++){
        if((disk + (fatSectorNumber() + i)) == 0){
            count++;
        }
        
    }
    if(count > n_cluster){
        printf("impossibile memorizzare file, FAT piena\n");
        return;
    }

    //controllo se file già esiste
    DirectoryEntry *dir_entry = (DirectoryEntry*)malloc(sizeof(DirectoryEntry));
    
    int first_free_sector = 0;
    
    u_int16_t n_sector;
    if (current_dir(working_dir)->first_cluster == 0)
        n_sector = 1 + fatSectorNumber();
    else
        n_sector = first_sector_of_cluster(current_dir(working_dir)->first_cluster);

    for(int i = 0; i < boot_record -> n_directory_entries; i++){
        readDirEntry(dir_entry, readSector(n_sector+ i));
        
        if(strcmp((dir_entry -> name), file_name) == 0){
            printf(COLOR_RED "file già esistente\n" COLOR_DEFAULT );
            printDirectoryEntry(dir_entry);
            free(dir_entry);
            return;
        }
        
        //per discriminare se settore sia vuoto, quindi non contiene 
        //nessuna dir entry, è sufficiente valutare se abbia valorizzato
        //l'attributo primo cluster
        if(first_free_sector == 0  &&  dir_entry -> first_cluster == 0){
            first_free_sector = n_sector + i;
        }
        
    }
    
    //se first_free_sector è ancora uguale a zero
    //vuol dire che la directory table è piena
    if(first_free_sector == 0){
        printf("La directory table è piena, impossibile aggiungere nuovi elementi\n");
        free(dir_entry);
        return;
    }

    //alloco una porzione di memoria di pari alla dimensione, che verrà scritta nel file
    char* data_file = (char*)malloc(dimension * sizeof(char));
    for (int i = 0; i < dimension-1; i++) {
        *(data_file + i) =  "Fatti non fummo a viver come bruti, ma a seguir virtude e canoscenza."[i % 69];
    }
    *(data_file + dimension - 1) = '\0'; 
    printf("contenuto del file: \n%s\n", data_file);

    u_int16_t free_cluster = get_free_cluster();
    time_t creation_time        = time(NULL); 
    dir_entry->creation_date    = creation_time;
    dir_entry->update_date      = creation_time;
    dir_entry->first_cluster    = free_cluster;
    dir_entry->dimension        = (u_int16_t)dimension;
    strcpy(dir_entry->name, file_name);

    //scrivo sul disco la directory entry sullo spazio di memoria mappato
    memcpy(readSector(first_free_sector), dir_entry, sizeof(*dir_entry));
    
    printf("Dimension: %d\n", dimension);
    printf("n_cluster: %d\n", n_cluster);


    int prev = 0;
    count = 0;
    u_int16_t data = 0;
    // ciclo tra i settori della FAT
    for(int i = 0; i < fatSectorNumber(); i++){ 
        // ciclo tra gli elementi j del settore i-esimo della FAT
        // j coincide con il numero di cluster
        for(u_int16_t j = i * (boot_record->byte_per_sector/2); 
            j < (boot_record->byte_per_sector)/2 * (i+1); j++){   

            //printf("j: %d\n", j);

            //printf("readSector(%d): %d\n", j, *((int*)readSector(j))  + i*2 );
            if( *(u_int16_t*)((readSector(i + 1) + j * 2)) == 0 ){
                //scrivo nella data area
                memcpy( readSector(1 + fatSectorNumber() + dirTableSectorNumber() 
                + j * boot_record->sector_per_cluster) , 
                data_file + (count * dimension/n_cluster), dimension/n_cluster);
                
                count++;
                if (n_cluster == 1){
                    data = 1;
                    write_on_fat(j, &data);
                    break;
                }

                if(prev != 0){
                    data = j;
                    //printf("prev: %d, data: %d\n", prev, data);
                    write_on_fat(prev, &data);
                    //scrivo nella fat
                    if(count == n_cluster){
                        data = 1;
                        write_on_fat(j, &data);
                        break;
                    }
                }
                prev = j;        
            }
    
        }
        if(count == n_cluster)
            break;
    
    }
    free(dir_entry);
    free(data_file);

}

// pos indica l'elemento i-esimo della tabella/vettore
// che rappresenta la FAT
void write_on_fat(int pos, u_int16_t* data){
    memcpy(readSector(1) + (pos * 2), data, sizeof(u_int16_t));
}

/**********************LISTA************************/
ListPath* list_init(DirectoryEntry *dir_entry) {
  ListPath* list = (ListPath*)malloc(sizeof(ListPath));
  list->dir_entry = dir_entry;
  list->next = NULL;
  return list;
}

void list_insert(ListPath* list, ListPath* new_item) {
    if (list->next == NULL){
        list->next = new_item;
        new_item->next = NULL;
        return;
    }
   list_insert(list->next, new_item);
}

DirectoryEntry* current_dir(ListPath* list){
    if(list->next == NULL)
        return list->dir_entry;
    current_dir(list->next);
}

void print_path(ListPath* list){
    if(list == NULL)
        return;
    printf(COLOR_BOLD_BLUE "/" COLOR_OFF);
    printf(COLOR_BOLD_BLUE "%s" COLOR_OFF, list->dir_entry->name);
    print_path(list->next);
}

void remove_last(ListPath *list){
    if(list->next == NULL)
        return;
    
    if(list->next->next == NULL){
        free(list->next);
        list->next = NULL;
        return;
    }
    remove_last(list->next);
}

int lenght(ListPath *list){
    if(list == NULL)
        return 0;
    return 1 + lenght(list->next);
}

void reset_path(ListPath *list){
    if(list->next == NULL){
        free(list);
        return;
    }
    reset_path(list->next);

}