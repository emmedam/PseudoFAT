#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include "utils.c"

#define COLOR_RED       "\x1b[31m"
#define COLOR_DEFAULT   "\x1b[0m"
#define COLOR_GREEN     "\e[0;32m"
#define COLOR_BOLD      "\e[1m"
#define COLOR_OFF       "\e[m"
#define COLOR_BLUE      "\x1b[34m"
#define COLOR_BOLD_BLUE "\e[1;5;34m"


#define BYTE_PER_SECTOR             32 //rimane fisso
#define SECTOR_PER_CLUSTER          100
#define NUMBER_OF_CLUSTER           600
#define NUMBER_OF_DIRECTORY_ENTRIES 50
#define VOLUME_NAME                 "AFRODITE.fat"

typedef enum {FREE = 0, LAST = 1} statusCluster;

//riferimento al volume di lavoro mappato in memoria
void *disk;

typedef struct{
    u_int16_t   byte_per_sector;
    u_int16_t   sector_per_cluster;
    u_int16_t   n_cluster;
    u_int16_t   n_directory_entries;
    time_t      date;
    char        name[12];
}BootRecord;

typedef struct{
    time_t      creation_date;
    time_t      update_date;
    u_int16_t   first_cluster;
    u_int16_t   dimension;
    char        name[12];
}DirectoryEntry;

// typedef struct{
//     char data[BYTE_PER_SECTOR];
// }disk_sector;


void init();

//alloca directoryTable
// void* initDirTable();

//calcolo numero di settori necessari per allocare la Directory Table
int dirTableSectorNumber();

//calcolo numero di settori necessari per allocare la FAT table
int fatSectorNumber();

//calcolo numero di settori necessari per allocare la Data Area
long dataAreaSectorNumber();

//allocazione BootRecord
BootRecord* initBootRecord(char*);

//leggo contenuto del boot record sulla memoria
void readBootRecord();

//stampo elementi contenuti nel bootRecord
void print_bootRecord(BootRecord*);

//****dovrei deallocare BootRecord... e tutti gli malloc(credo)****

//elenca a video informazioni sul volume di lavoro
void info();

//formatta il disco di lavoro
void* format(char*);

//leggo un settore 
void* readSector(int);

//quanti cluster liberi ci sono nella fat e numero di files
void cluster_info(int *, int*);

//crea una sub-directory, della directory di lavoro, 
//utilizzando il nome passato nel parametro dirname
void createDir(char*);

//lunghezza del disco
size_t disk_length();

//scrivo su disco 
void write_on_disk(void*, int);

void printFAT();

int cmpInput(char*, char *);

//elenca il contenuto della directory passata come parametro, 
//se non fornita elenca il contenuto della directory di lavoro
void listDir(DirectoryEntry*);

//cambia la directory di lavoro, modifico la working dir
void changeDir(char*);


//ritorna il primo settore di un cluster
u_int16_t first_sector_of_cluster(u_int16_t);

void init_root();

void createFile(char*, int);

void write_on_fat(int, u_int16_t*);

void write_on_data_area();

//legge il contenuto del file passato come parametro e lo visualizza a schermo
void read_file(char*);

//ritorna lo spazio rimanente nella data area
int remaining_space();

//elimina il file indicato dal parametro
void erase_file(char*);

//elimina la directory, indicata dal parametro
void erease_dir(char*);
/**********************LISTA**************************/
typedef struct ListPath {
  DirectoryEntry* dir_entry; 
  struct ListPath* next;
} ListPath;

ListPath* list_init(DirectoryEntry*);

void list_insert(ListPath* , ListPath* );

DirectoryEntry* current_dir(ListPath* );

int path_size(ListPath* );

void print_path(ListPath*);

void remove_last(ListPath *);

int lenght(ListPath *);

ListPath* reset_path(ListPath*);