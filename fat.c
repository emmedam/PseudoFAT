#include "fat.h"

BootRecord *boot_record;

void *disk;

ListPath *path;



void init(){
    boot_record = (BootRecord *)calloc(sizeof(BootRecord), 1);
}


void format(char *name){

    time_t creation_time = time(NULL);
    boot_record->byte_per_sector = (u_int16_t)BYTE_PER_SECTOR;
    boot_record->sector_per_cluster = (u_int16_t)SECTOR_PER_CLUSTER;
    boot_record->n_cluster = (u_int16_t)NUMBER_OF_CLUSTER;
    boot_record->n_directory_entries = (u_int16_t)NUMBER_OF_DIRECTORY_ENTRIES;
    boot_record->date = (time_t)creation_time;
    if (boot_record->name != name){
        strcpy(boot_record->name, name);
    }

    save(name);
}

void save(char *name){
    FILE *fd_out = fopen(name, "wb");

    fwrite(boot_record, sizeof(char) * BYTE_PER_SECTOR, 1, fd_out);

    // inizializzo a 1 i primi due byte della FAT che non verranno utilizzati
    u_int16_t one = 1;
    fwrite(&one, sizeof(u_int16_t), 1, fd_out);
    fwrite(&one, sizeof(u_int16_t), 1, fd_out);

    // uso calloc in modo da inizializzare a 0 tutti gli elementi della Fat table e allocare spazio per la fat table
    void * data = calloc(fat_sector_number() * BYTE_PER_SECTOR - 2 * sizeof(u_int16_t), 1); 
    fwrite(data,fat_sector_number() * BYTE_PER_SECTOR - 2 * sizeof(u_int16_t), 1, fd_out);
    free(data);
    
    // alloco spazio per la directory table della root ( o root directory)
    data = calloc(dir_table_sector_number() * BYTE_PER_SECTOR, 1);
    fwrite(data, dir_table_sector_number() * BYTE_PER_SECTOR, 1, fd_out);
    free(data);

    // alloco spazio per la data area
    data = calloc(data_area_sector_number() * BYTE_PER_SECTOR, 1);
    fwrite(data, data_area_sector_number() * BYTE_PER_SECTOR, 1, fd_out);
    free(data);

    fflush(fd_out);
    fclose(fd_out);
    
}

// legge il contenuto del disco, legge il file e lo mappa in memoria
void read_disk(char *f_name){

    int fd_in = open(f_name, O_RDWR);
    if (fd_in < 0){
        perror("impossbile aprire file:");
        exit(1);
    }
    // mappo il contenuto del disco rappresentato dal file in memoria
    disk = mmap(NULL, disk_length(), PROT_WRITE | PROT_READ, MAP_SHARED, fd_in, 0);

    if (disk == MAP_FAILED){
        perror("Mapping Failed: ");
        fprintf(stderr, "Value of errno: %d\n", errno);
        printf("%s\n", strerror(errno));
        close(fd_in);
        exit(EXIT_FAILURE);
    }

    close(fd_in);
}

// n indica il numero i-esimo del settore, ove il
// primo settore s'intende uguale a 0
void *read_sector(int n){
    return (disk + (BYTE_PER_SECTOR * n));
}

u_int16_t first_sector_of_cluster(u_int16_t n_cluster){

    return 1 + fat_sector_number() +
           boot_record->n_directory_entries +
           (boot_record->sector_per_cluster * n_cluster);
}

void read_dir_entry(DirectoryEntry *dir_entry, void *sector){
    dir_entry->creation_date = *(time_t *)(sector + 0);
    dir_entry->update_date = *(time_t *)(sector + 8);
    dir_entry->first_cluster = *(u_int16_t *)(sector + 16);
    dir_entry->dimension = *(u_int16_t *)(sector + 18);
    strcpy(dir_entry->name, (char *)(sector + 20));
}

void read_boot_record(){

    char *s = read_sector(0);
    boot_record->byte_per_sector = *(s + 0);
    boot_record->sector_per_cluster = *(u_int16_t *)(s + 2);
    boot_record->n_cluster = *(u_int16_t *)(s + 4);
    boot_record->n_directory_entries = *(u_int16_t *)(s + 6);
    boot_record->date = *(time_t *)(s + 8);
    strcpy(boot_record->name, (char *)(s + 16));

    init_root();
}

// inizializzo la working dir a root
void init_root(){
    DirectoryEntry *root = (DirectoryEntry *)malloc(sizeof(DirectoryEntry));
    strcpy(root->name, "root");
    root->creation_date = boot_record->date;
    root->update_date = 0;
    root->first_cluster = 0;
    root->dimension = 0;

    path = list_init(root);
}
void cluster_info(int *free_cluster, int *n_files){
    char *s = read_sector(1);
    int data = 0;
    for (int i = 0; i < (boot_record->n_cluster); i++)
    {
        data = *(s + i * 2);
        if (data == 0)
            (*free_cluster)++;
        else if (data == 1)
            (*n_files)++;
    }

    // sottraggo 2 poiché i primi due elementi della FAT
    // sono inutilizzati, ma valorizzati con il valore 1
    (*n_files) -= 2;
}

size_t disk_length(){
    return (1 + fat_sector_number() + dir_table_sector_number() +
            data_area_sector_number()) *
           BYTE_PER_SECTOR;
}

void info(){

    printf("n. byte per sector: %d\n", (boot_record->byte_per_sector));
    printf("n. sector per cluster: %d\n", (boot_record->sector_per_cluster));
    printf("n. cluster: %d\n", (boot_record->n_cluster));
    printf("n. entries directory table: %d\n", (boot_record->n_directory_entries));
    char *date = (char *)malloc(20 * sizeof(char));
    printf("creation time: %s\n", formatTime(date, (boot_record->date)));
    printf("volume name: %s\n", (char *)(boot_record->name));
    int free_cluster = 0, n_files = 0;
    cluster_info(&free_cluster, &n_files);
    printf("numero di cluster liberi: %d\n", free_cluster);
    printf("dimensione: %ld Bytes\n", disk_length());

    free(date);
}

int fat_sector_number(){
    return ceil((NUMBER_OF_CLUSTER * 2 / (double)BYTE_PER_SECTOR));
}

int dir_table_sector_number(){
    return NUMBER_OF_DIRECTORY_ENTRIES;
}

long data_area_sector_number(){
    return NUMBER_OF_CLUSTER * SECTOR_PER_CLUSTER;
}



u_int16_t get_free_cluster(){
    void *sector = read_sector(1);
    for (int i = 0; i < boot_record->n_cluster; i++)
    {
        if ((*(u_int16_t *)(sector + (i * 2))) == 0)
            return (u_int16_t)i;
    }
    // FAT piena nessun cluster libero
    return 0;
}

int get_n_free_cluster(){
    void *sector = read_sector(1);
    int count = 0;
    for (int i = 0; i < boot_record->n_cluster; i++){
        if ((*(u_int16_t *)(sector + (i * 2))) == 0)
            count++;
    }
    return count;
}


void print_directory_entry(DirectoryEntry *d){
    printf("name: %s\n", d->name);
    char *date = (char *)malloc(20 * sizeof(char));
    printf("creation date: %s\n", formatTime(date, (d->creation_date)));
    printf("update date: %s\n", formatTime(date, (d->update_date)));
    printf("first cluster: %d\n", d->first_cluster);
    printf("dimension: %d\n", d->dimension);
    free(date);
}


// crea una entry nella directory di lavoro
// se si tratta di una sub-dir allora assegna anche il primo cluster
// se si tratta di un nuovo file (vuoto per definizione) non assegna
// alcun cluster
DirectoryEntry *set_dir_entry(char *name, enum type type){
    u_int16_t free_cluster = 0;
    if (type == DIRECTORY_FAT){
        free_cluster = get_free_cluster();
        if (free_cluster == 0){
            printf(COLOR_RED
                "nessun cluster disponibile, impossibile creare nuove directory\n" COLOR_DEFAULT);
            return NULL;
        }
    }

    DirectoryEntry *dir_entry = (DirectoryEntry *)malloc(sizeof(DirectoryEntry));
    int first_free_sector = 0;

    u_int16_t n_sector = sector_current_dir();
    for (int i = 0; i < boot_record->n_directory_entries; i++){
        read_dir_entry(dir_entry, read_sector(n_sector + i));

        if (strcmp((dir_entry->name), name) == 0){
            printf(COLOR_RED
                   "nome già esistente, impossibile creare elemento\n" COLOR_DEFAULT);
            print_directory_entry(dir_entry);
            free(dir_entry);
            return NULL;
        }

        // per discriminare se settore sia vuoto, quindi non contiene
        // nessuna dir entry, è sufficiente valutare se abbia valorizzato
        // l'attributo primo cluster
        if (dir_entry -> creation_date == 0){
            first_free_sector = n_sector + i;
            break;
        }
    }

    // se first_free_sector è ancora uguale a zero
    // vuol dire che la directory table è piena
    if (first_free_sector == 0){
        printf(COLOR_RED "La directory table è piena, impossibile aggiungere nuovi elementi\n" COLOR_DEFAULT);
        free(dir_entry);
        return NULL;
    }

    time_t creation_time = time(NULL);
    dir_entry->creation_date = creation_time;
    if (type == FILE_FAT)
        dir_entry->update_date = creation_time;
    else
        dir_entry->update_date = 0;
    dir_entry->first_cluster = free_cluster;
    dir_entry->dimension = 0;
    strcpy(dir_entry->name, name);

    void* result = memcpy(read_sector(first_free_sector), dir_entry, sizeof(*dir_entry));
    free(dir_entry);
    return result;
}

void create_dir(char *dirname){
    // controllo se validità dirname, se non NULL
    if (!dirname){
        printf(COLOR_RED "impossibile creare directory, nome non valido\n" COLOR_DEFAULT);
        return;
    }
    // verifico se la lunghezza di dirname è minore o uguale di 11 caratteri
    if (strlen(dirname) > 11){
        printf(COLOR_RED "Nome directory non consentito, massimo 11 caratteri\n" COLOR_DEFAULT);
        return;
    }

    DirectoryEntry *dir_entry = set_dir_entry(dirname, DIRECTORY_FAT);
    if (dir_entry == NULL)
        return;

    // Scrivo nella FAT il cluster della directory appena creata
    u_int16_t last = 1;
    write_on_fat(dir_entry->first_cluster, &last);

}



void list_dir(){
    DirectoryEntry *tmp_dir = (DirectoryEntry *)malloc(sizeof(DirectoryEntry));
    u_int16_t n_sector = sector_current_dir();

    for (int i = 0; i < boot_record->n_directory_entries; i++){
        read_dir_entry(tmp_dir, read_sector(n_sector + i));

        // si tratta di una entry valida
        if (tmp_dir->creation_date != 0){
            char date[20];
            printf("%s  ", formatTime(date, (tmp_dir->creation_date)));
            printf("%s  ", formatTime(date, (tmp_dir->update_date)));
            printf("   %d  ", tmp_dir->dimension);

            printf("\t");
            // si tratta di una entry che rappresenta un file
            if (tmp_dir->update_date != 0){
                printf("%s\t\t", tmp_dir->name);
                // stampo la sequenza dei cluster memorizzati nella FAT
                u_int16_t curr_cluster = tmp_dir->first_cluster;
                u_int16_t next_cluster = curr_cluster;
                void *sector = read_sector(1);
                while (curr_cluster != 0){
                    printf("%d;", next_cluster);
                    next_cluster = *((u_int16_t *)(sector + curr_cluster * 2));
                    if (next_cluster == 1){
                        break;
                    }
                    curr_cluster = next_cluster;
                }
                printf("\n");
            }
            // si tratta di una directory
            else
            {
                printf(COLOR_GREEN "%s\t\t" COLOR_DEFAULT, tmp_dir->name);
                printf("%d\n", tmp_dir->first_cluster);
            }
        }
    }
    free(tmp_dir);
}

void change_dir(char *dir_name){

    if (!dir_name){
        printf(COLOR_RED "fornire directory\n" COLOR_DEFAULT);
        return;
    }

    if (strcmp(dir_name, "..") == 0){
        remove_last(path);
        return;
    }

    if (strcmp(dir_name, ".") == 0){
        path = reset_path(path);
        return;
    }

    DirectoryEntry *tmp_dir = (DirectoryEntry *)malloc(sizeof(DirectoryEntry));
    u_int16_t n_sector = sector_current_dir();

    for (int i = 0; i < boot_record->n_directory_entries; i++){
        read_dir_entry(tmp_dir, read_sector(n_sector + i));

        // controllo se nome corrisponde e che sia una direcotry e non un file
        if (strcmp(tmp_dir->name, dir_name) == 0 && tmp_dir->update_date == 0){
            // aggiorno la lista
            ListPath *new_item = (ListPath *)malloc(sizeof(ListPath));
            new_item->dir_entry = tmp_dir;
            list_insert(path, new_item);
            return;
        }
    }

    // directory non esiste nella working dir
    printf(COLOR_RED "Directory insesistente\n" COLOR_DEFAULT);
    free(tmp_dir);
}

// recupera il primo settore della directory table della
// path corrente
u_int16_t sector_current_dir(){
    if (current_dir(path)->first_cluster == 0)
        return 1 + fat_sector_number();
    else
        return first_sector_of_cluster(current_dir(path)->first_cluster);
}

int remaining_space(){
    char *sector = read_sector(1);
    int free_cluster = 0;
    for (int i = 0; i < (boot_record->n_cluster); i++){
        if (*(sector + i * 2) == 0)
            free_cluster++;
    }
    return free_cluster * boot_record->byte_per_sector * boot_record->sector_per_cluster;
}

// crea una nuova entry per il file passato come parametro
// nella directory di lavoro corrente se il file non esiste
DirectoryEntry *set_file(char *file_name){
    // verifico se la lunghezza di file_name è minore o uguale di 11 caratteri
    if (strlen(file_name) > 11){
        printf(COLOR_RED "Nome file non consentito, massimo 11 caratteri\n" COLOR_DEFAULT);
        return NULL;
    }

    return set_dir_entry(file_name, FILE_FAT);
}


FileHandle* create_file(char *file_name, int dimension){
    // verifico se la lunghezza di file_name è minore o uguale di 11 caratteri
    if (strlen(file_name) > 11){
        printf(COLOR_RED "Nome file non consentito, massimo 11 caratteri\n" COLOR_DEFAULT);
        return NULL;
    }

    char *data_file;
    if (dimension != 0){
        // alloco una porzione di memoria di pari alla dimensione, che verrà scritta nel file
        data_file = (char *)malloc(dimension * sizeof(char));
        for (int i = 0; i < dimension - 1; i++){
            *(data_file + i) = "Fatti non fummo a viver come bruti, ma a seguir virtude e canoscenza."[i % 69];
        }
        *(data_file + dimension - 1) = '\0';
        printf("contenuto del file: \n%s\n", data_file);
    }

    DirectoryEntry *de = set_dir_entry(file_name, FILE_FAT);

    if (de && dimension > 0){
        FileHandle *fe = get_file_handle(de);
        write_file(data_file, fe);
        if (data_file)
            free(data_file);
        return fe;
    }
    return NULL;
   
}

//torno dir entry di un FILE
void* get_dir_entry(char* name){
    DirectoryEntry* dir_entry;
    u_int16_t n_sector = sector_current_dir();
    
    for (int i = 0; i < boot_record->n_directory_entries; i++){
        dir_entry = (DirectoryEntry*) read_sector(n_sector + i);
        if(strcmp(dir_entry->name, name) == 0 && dir_entry->update_date != 0){
            return read_sector(n_sector + i);
        }
            
    }
    return NULL;
}

// pos indica l'elemento i-esimo della tabella/vettore
// che rappresenta la FAT
void write_on_fat(int pos, u_int16_t *data){
    memcpy(read_sector(1) + (pos * 2), data, sizeof(u_int16_t));
}

// legge il contenuto del file passato come parametro
// presente nella dir di lavoro. Se il file e' vuoto
// ritorna una stringa vuota se non esistente ritorna NULL
char* read_file(char *file_name){
    if (!file_name){
        printf(COLOR_RED "fornire nome file da leggere\n" COLOR_DEFAULT);
        return NULL;
    }

    DirectoryEntry* dir_entry = (DirectoryEntry*)get_dir_entry(file_name);

    if(!dir_entry){
        printf(COLOR_RED "file non trovato\n" COLOR_DEFAULT);
        return NULL;
    }
    // recupero i cluster dalla fat e in contemporanea recupero dati dalla data area
    u_int16_t curr_cluster = dir_entry->first_cluster;
    
    if(curr_cluster == 0){
        return  (char *)calloc(sizeof(char), 1);
    }


    u_int16_t cluster_dim = boot_record->byte_per_sector * boot_record->sector_per_cluster;
    char *res = (char *)calloc(sizeof(char) * (dir_entry->dimension + 1), 1);
    u_int16_t next_cluster = 0;
    void *sector_fat = read_sector(1);
    void *sector_data_area;
    char *res_tmp = (char *)calloc(sizeof(char) * (cluster_dim + 1), 1);
    
    while (1)
    {
        sector_data_area = read_sector(1 + fat_sector_number() + dir_table_sector_number() +
            curr_cluster * boot_record->sector_per_cluster);
        strncpy(res_tmp, (char *)sector_data_area, cluster_dim);

        
        strcat(res, res_tmp);

        next_cluster = *((u_int16_t *)(sector_fat + curr_cluster * 2));
        
        if (next_cluster == 1){
            break;
        } 
        curr_cluster = next_cluster;
    }
    free(res_tmp);
    // strcat(res, " ");
    
    return res;
}

// precondizione: file deve esistere
// scrive il contenuto del file passato come parametero
DirectoryEntry *write_file(char *file_content, FileHandle *file_handle){

    int dimension = strlen(file_content) + 1;
    // conto il numero di cluster necessari per memorizzare il file
    u_int16_t n_cluster = ceil(dimension / 
        (double)(boot_record->sector_per_cluster * 
        boot_record->byte_per_sector));

    // verifico se ho n_cluster liberi nella FAT necessari per memorizzare file
    if (get_n_free_cluster() < n_cluster){
        printf(COLOR_RED"impossibile scrivere su file, FAT o spazio non disponibile\n"COLOR_DEFAULT);
        return NULL;
    }

    // se il file e' stato gia' scritto allora resetto la fat
    if (file_handle->entry->first_cluster != 0){
        clear_fat(file_handle->entry->first_cluster);
    }

    file_handle->entry->first_cluster = get_free_cluster();

    // scrivo nella FAT e nella data area
    int prev = 0;
    int count = 0;
    u_int16_t data = 0;
    void *sector = read_sector(1);

    for (int i = 0; i < boot_record->n_cluster; i++){
        if ((*(u_int16_t *)(sector + (i * 2))) == 0){

            void *dest = read_sector(1 + fat_sector_number() + dir_table_sector_number() + 
                i * boot_record->sector_per_cluster);
            
            if (n_cluster == 1){
                memcpy(dest,
                       file_content + (count * boot_record->byte_per_sector * 
                       boot_record->sector_per_cluster),
                       dimension);
                count++;
                data = 1;
                write_on_fat(i, &data);
                break;
            }

            if (count < n_cluster - 1)
                memcpy(dest,
                       file_content + (count * boot_record->byte_per_sector * 
                       boot_record->sector_per_cluster),
                       boot_record->byte_per_sector * boot_record->sector_per_cluster);

            count++;
            if (prev != 0){
                data = i;
                write_on_fat(prev, &data);
                if (count == n_cluster){
                    // scrivo nella data area
                    memcpy(dest,
                           file_content + ((count - 1) * 
                           boot_record->sector_per_cluster * boot_record->byte_per_sector),
                           dimension - ((count - 1) * boot_record->sector_per_cluster * 
                           boot_record->byte_per_sector));
                    data = 1;
                    write_on_fat(i, &data);
                    break;
                }
            }
            prev = i;
        }
    }

    // aggiorno data di modifica e dimensione file
    file_handle->entry->dimension = strlen(file_content) + 1;
    file_handle->entry->update_date = time(NULL);

    return file_handle->entry;
}

void clear_fat(u_int16_t first_cluster){
    u_int16_t curr_cluster = first_cluster;
    u_int16_t next_cluster = 0;
    void* sector_fat = read_sector(1); 
    u_int16_t free = 0;
    while(1){
            next_cluster = *((u_int16_t*)(sector_fat + curr_cluster * 2));
            write_on_fat(curr_cluster, &free);
            if(next_cluster == 1){
                break;
            }
            curr_cluster = next_cluster;
            
        }
}


FileHandle *get_file_handle(DirectoryEntry* entry){
    FileHandle *fe = (FileHandle *)malloc(sizeof(FileHandle));

    fe->entry = entry;
    fe->seek = 0;
    return fe;
}


int seek(FileHandle* fe, u_int16_t offset){
    if(offset < 0 || offset > fe->entry->dimension-1)
        return 1;
    fe->seek = offset;
    return 0;
}


void erase_file(char *file_name){
    if (!file_name){
        printf(COLOR_RED "indicare file da rimuovere\n" COLOR_DEFAULT);
        return;
    }

    DirectoryEntry* dir_entry = (DirectoryEntry*)get_dir_entry(file_name);

    if(!dir_entry){
        printf(COLOR_RED "file non trovato\n" COLOR_DEFAULT);
        return;
    }

    char answer[50];
    printf("Sei sicuro di eliminare il file? [Si/No]\n");
    scanf("%[^\n]%*c", answer);

    if (strcmp(answer, "Si") == 0 ||
     strcmp(answer, "SI") == 0 || 
     strcmp(answer, "S") == 0 ||
     strcmp(answer, "s") == 0){
        // recupero i cluster dalla fat e in contemporanea recupero dati dalla data area
        u_int16_t curr_cluster = dir_entry->first_cluster;
        

        //se file NON vuoto devo settare a 0 i cluster della FAT
        if(curr_cluster != 0){
            u_int16_t next_cluster = 0;
            void *sector_fat = read_sector(1);
            u_int16_t free = 0;
            
            while (1){
                next_cluster = *((u_int16_t *)(sector_fat + curr_cluster * 2));
                write_on_fat(curr_cluster, &free);
                if (next_cluster == 1){
                    break;
                }
                curr_cluster = next_cluster;
            }
        }
        // uso calloc in modo da inizializzare a 0 tutti gli elementi della DirTable e allocare spazio per la fat table
        void * data = calloc(sizeof(char), sizeof(DirectoryEntry)); 
        memcpy((void*)dir_entry, data, sizeof(DirectoryEntry));
        free(data);
    }
}

void erase_dir(char *dir_name){

    if (!dir_name){
        printf(COLOR_RED "indicare il nome della directory da rimuovere\n" COLOR_DEFAULT);
        return;
    }

    // controllo se dir_name esiste su dir corrente
    DirectoryEntry *dir_entry = (DirectoryEntry *)malloc(sizeof(DirectoryEntry));

    u_int16_t n_sector = sector_current_dir();
    
    int i;
    u_int16_t sector;
    for (i = 0; i < boot_record->n_directory_entries; i++){
        read_dir_entry(dir_entry, read_sector(n_sector + i));

        if (strcmp((dir_entry->name), dir_name) == 0){
            sector = n_sector + i;
            break;
        }
    }

    if (i >= boot_record->n_directory_entries){
        printf(COLOR_RED "directory inesistente su directory corrente\n" COLOR_DEFAULT);
        free(dir_entry);
        return;
    }

    u_int16_t tmp_sector = sector;

    int count = 0;

    DirectoryEntry *tmp_dir = (DirectoryEntry *)malloc(sizeof(DirectoryEntry));

    sector = first_sector_of_cluster((dir_entry)->first_cluster);
    for (int i = 0; i < boot_record->n_directory_entries; i++){
        read_dir_entry(tmp_dir, read_sector(sector + i));
        if (tmp_dir->creation_date != 0)
            count++;
    }


    if (count > 0){
        printf(COLOR_RED "Impossibile eliminare directory piena\n" COLOR_DEFAULT);
    }else{
        char answer[50];
        printf("Sei sicuro di voler eliminare la directory? [Si/No]\n");
        scanf("%[^\n]%*c", answer);
        if(strcmp(answer, "Si") == 0 ||
            strcmp(answer, "si") == 0 || 
            strcmp(answer, "s") == 0 ||
            strcmp(answer, "S") == 0){
            u_int16_t free_cluster = 0;
            write_on_fat(dir_entry->first_cluster, &free_cluster);

            void *dest;
            if (current_dir(path)->first_cluster == 0)
                dest = read_sector(tmp_sector);
            else
                dest = read_sector(1 + fat_sector_number() + dir_table_sector_number() 
                    + (dir_entry->first_cluster - 1) * boot_record->sector_per_cluster);

            void * data = calloc(sizeof(char), sizeof(DirectoryEntry));
            memcpy(dest, data, sizeof(DirectoryEntry));
            free(data);
            printf("directory rimossa!\n");
        }
    }
    free(dir_entry);
    free(tmp_dir);
}


void help(char* command){
    if(!command){
        printf("Comandi disponibili:\n");
        printf("%-28s %s\n", "info|i", "elenca a video informazioni sul volume di lavoro");
        printf("%-28s %s\n", "createDir|md <dirname>", "crea una sub-directory, della directory di lavoro");
        printf("%-28s %s\n", "changeDir|cd <dirname> " ,"cambia la directory di lavoro" );
        printf("%-28s %s\n","listDir|ld" ,"elenca il contenuto della directory di lavoro" );
        printf("%-28s %s\n","createFile|cf <filename>" ,"crea un file di testo vuoto" );
        printf("%-28s %s\n","read|r <filename> " ,"legge il contenuto del file" );
        printf("%-28s %s\n","write|w <filename><content>" ,"scrive nel file di testo" );
        printf("%-28s %s\n","eraseFile|rf <filename>" ,"elimina il file" );
        printf("%-28s %s\n","eraseDir|rd <filename>" ,"elimina la directory, se vuota" );
        printf("%-28s %s\n","seek|s <filename><offset>" ,"sposta la posizione del puntatore del file su un offset specificato" );
        printf("%-28s %s\n","exit|e" ,"chiude il programma e salva il filesystem" );
    }
    else{
        if(strcmp(command, "info") == 0){
            printf("info|i  telenca a video informazioni sul volume di lavoro\n");
        }
        
        else if(strcmp(command, "createDir") == 0 || strcmp(command, "md") == 0){
           printf("createDir|md<dirname>  crea una sub-directory, della directory di"
                "lavoro, utilizzando il nome passato nel parametro dirname\n");
            
        }
    
        else if(strcmp(command, "changeDir") == 0|| strcmp(command, "cd") == 0){
            printf("changeDir|cd<dirname>  cambia la directory di lavoro come indicato "
                "dal parametro dirname\n");
        }

        else if(strcmp(command, "listDir") == 0 || strcmp(command, "ld") == 0){
            printf("listDir|ld  elenca il contenuto della directory di lavoro\n");
        }

        else if(strcmp(command, "createFile") == 0 || strcmp(command, "cf") == 0){
            printf("createFile|cf<filename>  crea un file di testo vuoto con il nome"
                "fornito dal parametro filename\n");
        }

        else if(strcmp(command, "read") == 0 ){
            printf("read|r<filename>  legge il contenuto del file passato come parametro e lo visualizza a schermo\n");
        }

        else if(strcmp(command, "write") == 0 ){
            printf("write|w<filename><content>  scrive nel file di testo, indicato dal parametro filename, "
                "il contenuto del parametro content\n");
        }

        else if(strcmp(command, "eraseFile") == 0 || strcmp(command, "rm") == 0){
           printf("eraseFile|rf<filename>  elimina il file indicato dal parametro filename\n");
        }

        else if(strcmp(command, "eraseDir") == 0 || strcmp(command, "rmdir") == 0){
            printf("eraseDir|rd<filename>  rmdir[dirname] \telimina la directory, indicata dal parametro dirname, solamente nel caso in cui sia vuota\n");
        }

        else if (strcmp(command, "exit") == 0){
            printf("exit|e  chiude il programma e salva il filesystem di lavoro sul file con il nome del disco\n");
        }

        else if (strcmp(command, "seek") == 0){
            printf("seek|s <filename><offset> sposta la posizione del puntatore del file indicato da filename su un offset specificato\n");
        }
            
        
        else{
            printf("comando sconosciuto, help per maggiori informazioni\n");
        }
    }

}

