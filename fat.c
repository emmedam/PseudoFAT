#include "fat.h"

BootRecord *boot_record;

void save(char*);

//indica in quale directory ci troviamo
void *working_dir;

void init(){
       
    boot_record = (BootRecord*) malloc(sizeof(BootRecord));
 
}

void* format(char *name){
    // BootRecord *boot_record = initBootRecord(name);
    
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


void* readSector(int n){
    return (disk + (BYTE_PER_SECTOR * n));
}

void readCluster(){

}

void readBootRecord(){
    char *s = readSector(0);
    boot_record->byte_per_sector        = *(s+0);
    boot_record->sector_per_cluster     = *(u_int16_t*)(s+2);
    boot_record->n_cluster              = *(u_int16_t*)(s+4);
    boot_record->n_directory_entries    = *(u_int16_t*)(s+6);
    boot_record->date                   = *(time_t*)(s+8);
    strcpy(boot_record->name, (char*)(s+16));
}

void cluster_info(int *free_cluster, int *n_files){
    char *s = readSector(1);
    int data = 0;
    for(int i=0; i < (boot_record->n_cluster); i++){
        data = *(s+i*2);
        if( data == 0 ) (*free_cluster)++; 
        else if(data == 1) (*n_files)++;
    }
    

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

void readDirEntry(DirectoryEntry *dir_entry, void* sector){
    strcpy(dir_entry->name, (char*)(sector+0));
    dir_entry->creation_date    = *(time_t*)(sector + 12);
    dir_entry->update_date      = *(time_t*)(sector + 20);
    dir_entry->first_cluster    = *(u_int16_t*)(sector + 14);
    dir_entry->dimension        = *(u_int16_t*)(sector + 30);

    printf("NAME: %s\n", dir_entry->name);

}

//trovo primo cluster libero nella FAT 
int get_free_cluster(){
    for(int i=0; i < fatSectorNumber(); i++){
        char *sector = readSector(i+1);
        for(int j=2; j < (boot_record->byte_per_sector)/2; j++){
            if( (*(sector+j)*2) == 0)
                return j;
        }
    }
    //FAT piena nessun cluster libero
    return 0;
}


void createDir(char* dirname){
    //controllo se directory già esiste
    DirectoryEntry *dir_entry = (DirectoryEntry*)malloc(sizeof(DirectoryEntry));
    int first_free_sector = 0; 
    for(int i = 0; i < boot_record -> n_directory_entries; i++){
        printf("name: %s\n", (char*)readSector(fatSectorNumber() + 1 + i));
        readDirEntry(dir_entry, readSector(fatSectorNumber() + 1+ i));
        //printf("directory name[%d]: %s\n", i , (char*)dir_entry->name);
        if(strcmp((char*)dir_entry -> name, dirname) == 0){
            printf("Directory già esistente\n");
            return;
        }
        // //per discriminare se settore sia vuoto, quindi non contiene 
        //nessuna dir entry, è sufficiente valutare se abbia valorizzato
        //l'attributo primo cluster
        if(first_free_sector == 0  &&  dir_entry->first_cluster == 0){
            first_free_sector = fatSectorNumber() + 1 + i;
            //printf("first free sector: %d\n", first_free_sector);
            //printf("first cluster dir: %d\n", dir_entry->first_cluster);
        
        }
    }   
    
    //se first_free_sector è ancora uguale a zero
    //vuol dire che la directory table è piena
    if(first_free_sector == 0){
        printf("La directory table è piena, impossibile aggiungere nuovi elementi\n");
        return;
    }

    //creo nuova direcotry

    //controllo se ho cluster liberi, altrimenti non posso creare nuove directory
    u_int16_t free_cluster = get_free_cluster();
    if(free_cluster == 0){
        printf("nessun cluster disponibile, impossibile creare nuove directory\n");
        return;
    }

    strcpy(dir_entry->name, dirname);
    time_t creation_time        = time(0); 
    dir_entry->creation_date    = dir_entry->update_date = creation_time;
    dir_entry->first_cluster    = free_cluster;
    dir_entry->dimension        = 0;
    
    //printf("name: %s\n", dir_entry->name);
    
    write_on_disk(dir_entry, first_free_sector);
    
}

void write_on_disk(void *data, int offset){
    strcpy(disk+offset, data);
    // memcpy((disk + offset), data, sizeof(data));

    //sincronizzo il file con la memoria
    // if(msync(disk, disk_length(),  MS_SYNC) == -1){
    //     perror("Sincronizzazione fallita: "); 
    //     exit(EXIT_FAILURE);
        
    // }
}

void save(char *name){
    FILE * fd_out=fopen(name, "wb");
    
    fwrite(boot_record, sizeof(char)*BYTE_PER_SECTOR, 1, fd_out);
    fflush(fd_out);
    
    //uso calloc in modo da inizializzare a 0 tutti gli elementi della Fat table e allocare spazio per la fat table
    fwrite(calloc(fatSectorNumber() * BYTE_PER_SECTOR, 1), fatSectorNumber() * BYTE_PER_SECTOR, 1, fd_out);
    fflush(fd_out);
    
    //alloco spazio per la directory table della root ( o root directory) 
    fwrite(calloc(dirTableSectorNumber() * BYTE_PER_SECTOR, 1), dirTableSectorNumber() * BYTE_PER_SECTOR, 1, fd_out);
    fflush(fd_out);
    
    //alloco spazio per la data area
    fwrite(calloc(dataAreaSectorNumber() * BYTE_PER_SECTOR, 1), dataAreaSectorNumber() * BYTE_PER_SECTOR, 1, fd_out);
    fflush(fd_out);
   
    fclose(fd_out);
}