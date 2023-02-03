#include "fat.h"

BootRecord *boot_record;

void save(char*);

//lista concatenata che rappresenta la working_dir
ListPath* path;



void init(){
       
    boot_record = (BootRecord*) malloc(sizeof(BootRecord));
 
}

void* format(char *name){
    
    time_t creation_time = time(NULL); 
    // printf("sizeof bootrecord %ld\n", sizeof(*boot_record));
    boot_record->byte_per_sector        = (u_int16_t)BYTE_PER_SECTOR;
    boot_record->sector_per_cluster     = (u_int16_t)SECTOR_PER_CLUSTER;
    boot_record->n_cluster              = (u_int16_t)NUMBER_OF_CLUSTER;
    boot_record->n_directory_entries    = (u_int16_t)NUMBER_OF_DIRECTORY_ENTRIES;
    boot_record->date                   = (time_t)creation_time;
    strcpy(boot_record->name, name);
       
    // printf("%d\n", boot_record->byte_per_sector);
    // printf("%d\n", boot_record->sector_per_cluster);
    // printf("%d\n", boot_record->n_cluster);
    // printf("%d\n", boot_record->n_directory_entries);
    // printf("name: %s\n", boot_record->name);

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
    //printf("%s\n", dir_entry->name);
    
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

    path = list_init(root);
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
    printf("numero di files e/o directory: %d\n",             n_files);
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
// int get_free_cluster(){
//     for(int i = 0; i < fatSectorNumber(); i++){
//         // char *sector = readSector(i + 1);
//         for(int j = i * (boot_record->byte_per_sector / 2); 
//             j < (boot_record->byte_per_sector) / 2 * (i + 1); j++){
//             if(*(u_int16_t*)((readSector(i + 1) + j * 2))== 0)
//                 return j;
//         }
//     }
//     //FAT piena nessun cluster libero
//     return 0;

// }

u_int16_t get_free_cluster(){
    void *sector = readSector(1);
    for(int i = 0; i < boot_record -> n_cluster; i++){
        if((*(u_int16_t*)(sector + (i * 2))) == 0)
            return (u_int16_t)i;
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
    if (current_dir(path)->first_cluster == 0)
        n_sector = 1 + fatSectorNumber();
    else
        n_sector = first_sector_of_cluster(current_dir(path)->first_cluster);

    for(int i = 0; i < boot_record -> n_directory_entries; i++){
        readDirEntry(dir_entry, readSector(n_sector+ i));
        if(strcmp((dir_entry -> name), dirname) == 0){
            printf(COLOR_RED "Directory già esistente\n" COLOR_DEFAULT );
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


    //controllo se ho cluster liberi, altrimenti non posso creare nuove directory
    u_int16_t free_cluster = get_free_cluster();
    if(free_cluster == 0){
        printf("nessun cluster disponibile, impossibile creare nuove directory\n");
        free(dir_entry);
        return;
    }
    printf("Primo cluster disponibile: %d\n", free_cluster);

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
    //if (current_dir(working_dir)->first_cluster == 0)
    if (dir->first_cluster == 0)
        n_sector = 1 + fatSectorNumber();
    else
        //n_sector = first_sector_of_cluster(current_dir(working_dir)->first_cluster);
        n_sector = first_sector_of_cluster(dir->first_cluster);
    

    for(int i = 0; i < boot_record -> n_directory_entries; i++){
        readDirEntry(tmp_dir, readSector(n_sector + i));
        
        //si tratta di un file oppure
        if(tmp_dir->creation_date != 0){
            
            //char *date = (char*)malloc(20 * sizeof(char));
            char date[20];
            printf("%s\t", formatTime(date,(tmp_dir->creation_date))); 
            printf("%d\t", tmp_dir->dimension);
            
            // si tratta di un file
            if(tmp_dir->update_date != 0){
                printf("%s", tmp_dir->name);
                //stampo la sequenza dei cluster memorizzati nella FAT 
                u_int16_t curr_cluster = tmp_dir->first_cluster;
                u_int16_t next_cluster = 0;
                void* sector = readSector(1); 
                printf("\t\t%d", curr_cluster);
                while(1){
                    next_cluster = *((u_int16_t*)(sector+ curr_cluster * 2));
                    if(next_cluster == 1){
                        printf("\n");
                        break;
                    }
                    printf("-%d", next_cluster);
                    curr_cluster = next_cluster;
                }
            }
            //si tratta di una directory
            else{
                printf(COLOR_GREEN "%s" COLOR_DEFAULT, tmp_dir->name);
                printf("\t\t%d\n", tmp_dir->first_cluster);
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
        remove_last(path);
        return;
    }

    if(strcmp(dir_name, ".") == 0){
        path = reset_path(path);
        // init_root();
        return;
    }


    DirectoryEntry *tmp_dir = (DirectoryEntry*)malloc(sizeof(DirectoryEntry));
    
    u_int16_t n_sector;
    if (current_dir(path)->first_cluster == 0)
        n_sector = 1 + fatSectorNumber();
    else
        n_sector = first_sector_of_cluster(current_dir(path)->first_cluster);

    for(int i = 0; i < boot_record -> n_directory_entries; i++){
        readDirEntry(tmp_dir, readSector(n_sector+i));

        //controllo se nome corrisponde e che sia una direcotry e non un file
        if( strcmp(tmp_dir->name, dir_name) == 0 && tmp_dir->update_date == 0 ){
            //aggiorno la lista
            ListPath* new_item = (ListPath*)malloc(sizeof(ListPath));
            new_item->dir_entry = tmp_dir;
            list_insert(path, new_item);
            return;
        }
    }
    
    //directory non esiste nella working dir   
    printf("Directory insesistente\n"); 
    free(tmp_dir);
}
int remaining_space(){
    char *sector= readSector(1);
    int free_cluster = 0;
    for(int i=0; i < (boot_record->n_cluster); i++){
        if(*(sector+i*2) == 0)
            free_cluster++;
    }
    return free_cluster * boot_record->byte_per_sector * boot_record->sector_per_cluster;

}
void createFile(char* file_name, int dimension){
    if (dimension <= 0)
        dimension = 1;

    
    //verifico se la lunghezza di file_name è minore o uguale di 11 caratteri
    if(strlen(file_name) > 11){
        printf(COLOR_RED "Nome file non consentito, massimo 11 caratteri\n" COLOR_DEFAULT);
        return;
    }
    
    if(dimension > remaining_space()){
        printf(COLOR_RED "spazio non disponibile\n" COLOR_DEFAULT);
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
    if (current_dir(path)->first_cluster == 0)
        n_sector = 1 + fatSectorNumber();
    else
        n_sector = first_sector_of_cluster(current_dir(path)->first_cluster);

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
    if(free_cluster == 0){
        printf("nessun cluster disponibile, impossibile creare nuovi file\n");
        free(dir_entry);
        return;
    }
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

    printf("Primo cluster disponibile: %d\n", free_cluster);
    int prev = 0;
    count = 0;
    u_int16_t data = 0;
    void *sector = readSector(1);
    for(int j = 0; j < boot_record -> n_cluster; j++){
        if( (*(u_int16_t*)(sector + (j * 2))) == 0 ){
           
            void* dest = readSector(1 + fatSectorNumber() + dirTableSectorNumber() + j * boot_record->sector_per_cluster);
            
            if(count < n_cluster - 1)
                //scrivo nella data area
                memcpy(dest, 
                    data_file + (count * boot_record->byte_per_sector * boot_record->sector_per_cluster),
                    boot_record->byte_per_sector * boot_record->sector_per_cluster);
            
            if (n_cluster == 1){
                 //scrivo nella data area
                memcpy(dest, 
                    data_file + (count * boot_record->byte_per_sector * boot_record->sector_per_cluster), 
                    dimension);
                    //dimension-1
                count++;
                data = 1;
                write_on_fat(j, &data);
                break;
            }

            count++;
            if(prev != 0){
                data = j;
                write_on_fat(prev, &data);
                if(count == n_cluster){
                     //scrivo nella data area
                    memcpy( dest, 
                        data_file + ((count - 1) * boot_record->sector_per_cluster *  boot_record->byte_per_sector),
                        dimension - ((count - 1) * boot_record->sector_per_cluster *  boot_record->byte_per_sector));
                    data = 1;
                    write_on_fat(j, &data);
                    break;
                }
            }
            prev = j;        
        }
    
    }
    free(dir_entry);
    free(data_file);

}

// pos indica l'elemento i-esimo della tabella/vettore
// che rappresenta la FAT
void write_on_fat(int pos, u_int16_t* data){
    memcpy(readSector(1) + (pos * 2), data, sizeof(u_int16_t));
}

void read_file(char* file_name){
    if(!file_name){
        printf(COLOR_RED "fornire nome file da leggere\n" COLOR_DEFAULT);
        return;
    }

    //trovo directoryentry con nome file_name 
    DirectoryEntry *tmp_dir = (DirectoryEntry*)malloc(sizeof(DirectoryEntry));
    
    u_int16_t n_sector;
    if (current_dir(path)->first_cluster == 0)
        n_sector = 1 + fatSectorNumber();
    else
        n_sector = first_sector_of_cluster(current_dir(path)->first_cluster);

    int i;
    for(i= 0; i < boot_record -> n_directory_entries; i++){
        readDirEntry(tmp_dir, readSector(n_sector + i));
        if(strcmp(tmp_dir->name, file_name) == 0 && tmp_dir->update_date == 0){
            printf(COLOR_RED "non posso leggere una directory\n" COLOR_DEFAULT);
            free(tmp_dir);
            return;
        }
        else if (strcmp(tmp_dir->name, file_name) == 0 && tmp_dir->update_date != 0)
            break;
    }

    if (i == boot_record -> n_directory_entries){
        printf(COLOR_RED "file non trovato\n" COLOR_DEFAULT);
        free(tmp_dir);
        return;
    }
    
    //recupero i cluster dalla fat e in contemporanea recupero dati dalla data area
    u_int16_t curr_cluster = tmp_dir->first_cluster;
    u_int16_t next_cluster = 0;
    void* sector_fat = readSector(1); 
    void* sector_data_area;
    u_int16_t dim = boot_record->byte_per_sector * boot_record->sector_per_cluster;
    char* res = (char*)malloc(sizeof(char)*dim);
    
    printf("***inizio file\n");
    while(1){
        sector_data_area = readSector(1 + fatSectorNumber() + dirTableSectorNumber() +
         curr_cluster * boot_record->sector_per_cluster ); 
        
        strncpy(res, (char*)sector_data_area, dim);
        
        printf("%s", res);

        next_cluster = *((u_int16_t*)(sector_fat + curr_cluster * 2));
        if(next_cluster == 1){
            // strncpy(res, "\n", dim);
            printf("\n***fine file\n");
            break;
        }
        curr_cluster = next_cluster;
        
    }
    
    free(tmp_dir);
    free(res);
}

void erease_file(char* file_name){
    if(!file_name){
        printf(COLOR_RED "indicare file da rimuovere\n" COLOR_DEFAULT);
        return;
    }

    //controllo se file_name esiste su dir corrente
    DirectoryEntry *dir_entry = (DirectoryEntry*)malloc(sizeof(DirectoryEntry));
    
    u_int16_t n_sector;
    if (current_dir(path)->first_cluster == 0)
        n_sector = 1 + fatSectorNumber();
    else
        n_sector = first_sector_of_cluster(current_dir(path)->first_cluster);

    int i;
    u_int16_t sector;
    for(i = 0; i < boot_record -> n_directory_entries; i++){
        readDirEntry(dir_entry, readSector(n_sector+ i));

        if(strcmp((dir_entry -> name), file_name) == 0 ){
            sector = n_sector + i;
            printf("sector: %d\n", sector);
            break;
        }
    }   
    if(i >= boot_record->n_directory_entries){
        printf(COLOR_RED "file inesistente du directory corrente\n" COLOR_DEFAULT);
        return;
    }

    char* answer = (char*)malloc(sizeof(char));
    printf("Sei sicuro di voler eliminare il file?[Si/No]\n");
    scanf("%[^\n]%*c", answer);
    
    if(strcmp(answer, "Si") == 0 || strcmp(answer, "S") == 0 || strcmp(answer, "s") == 0){
        //recupero i cluster dalla fat e in contemporanea recupero dati dalla data area
        u_int16_t curr_cluster = dir_entry->first_cluster;
        u_int16_t next_cluster = 0;
        void* sector_fat = readSector(1); 
        //void* sector_data_area = readSector(1 + fatSectorNumber() + dirTableSectorNumber() + curr_cluster); 
        //u_int16_t dim = boot_record->byte_per_sector * boot_record->sector_per_cluster;
        u_int16_t free = 0;
        while(1){
            next_cluster = *((u_int16_t*)(sector_fat + curr_cluster * 2));
            write_on_fat(curr_cluster, &free);
            if(next_cluster == 1){
                break;
            }
            curr_cluster = next_cluster;
            //sector_data_area = readSector(1 + fatSectorNumber() + dirTableSectorNumber() + curr_cluster ); 
            
        }
        memcpy(readSector(sector) , calloc(sizeof(char), 32), sizeof(DirectoryEntry));

    
    }
    
    else if(strcmp(answer, "No") == 0){
        free(dir_entry);
        free(answer);
        return;
    } 
    
    else{
        printf(COLOR_RED "Non è una risposta valida\n" COLOR_DEFAULT);
        free(dir_entry);
        free(answer);
        return;
    }
    free(answer);
    free(dir_entry);

}

void erease_dir(char* dir_name){
    if(!dir_name){
        printf(COLOR_RED "indicare directory da rimuovere\n" COLOR_DEFAULT);
        return;
    }

    //controllo se dir_name esiste su dir corrente
    DirectoryEntry *dir_entry = (DirectoryEntry*)malloc(sizeof(DirectoryEntry));
    
    u_int16_t n_sector;
    if (current_dir(path)->first_cluster == 0)
        n_sector = 1 + fatSectorNumber();
    else
        n_sector = first_sector_of_cluster(current_dir(path)->first_cluster);

    int i;
    u_int16_t sector;
    for(i = 0; i < boot_record -> n_directory_entries; i++){
        readDirEntry(dir_entry, readSector(n_sector+ i));

        if(strcmp((dir_entry -> name), dir_name) == 0){
            sector = n_sector + i;
            break;
        }
    }   
    if(i >= boot_record->n_directory_entries){
        printf(COLOR_RED "directory inesistente su directory corrente\n" COLOR_DEFAULT);
        free(dir_entry);
        return;
    }

    int count = 0;
    for(int i = 0; i < boot_record -> n_directory_entries; i++){
        readDirEntry(dir_entry, readSector(n_sector + i));
        if(dir_entry->creation_date != 0)
            count++;
    }
    char* answer = (char*)malloc(sizeof(char));
    u_int16_t free_cluster = 0;
    if(count > 0){
        printf("Sono presenti dei file e/o directory nella cartella, sei sicuro di voler eliminare la direcotry?[Si/No]\n");
        scanf("%[^\n]%*c", answer);
        if(strcmp(answer, "Si") == 0 || strcmp(answer, "S") == 0 || strcmp(answer, "s") == 0){
            
        }
        
        else if(strcmp(answer, "No") == 0){
            free(dir_entry);
            free(answer);
            return;
        }
        else{
            printf(COLOR_RED "Non è una risposta valida\n" COLOR_DEFAULT);
            free(dir_entry);
            free(answer);
            return;
        }
    } 
    else{
        printf("sei sicuro di voler eliminare la direcotry?[Si/No]\n");
        scanf("%[^\n]%*c", answer);
        if(strcmp(answer, "Si") == 0 || strcmp(answer, "S") == 0 || strcmp(answer, "s") == 0){
            write_on_fat(dir_entry->first_cluster, &free_cluster);
            memcpy(readSector(dir_entry->first_cluster), &free_cluster, sizeof(*dir_entry));

        }
        
        else if(strcmp(answer, "No") == 0){
            free(dir_entry);
            free(answer);
            return;
        }
        else{
            printf(COLOR_RED "Non è una risposta valida\n" COLOR_DEFAULT);
            free(dir_entry);
            free(answer);
            return;
        }

    }
    free(answer);
    
    

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

ListPath* reset_path(ListPath *list){
    ListPath* next = list -> next;
    ListPath* curr = list -> next;
    while(next != NULL){
        next = curr -> next;
        free(curr);
        curr = next;
    }
    list -> next = NULL;
    return list;
}