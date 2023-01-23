#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include "utils.c"

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
    char        name[12];
    time_t      creation_date;
    time_t      update_date;
    u_int16_t   first_cluster;
    u_int16_t   dimension;
}DirectoryEntry;

// typedef struct{
//     char data[BYTE_PER_SECTOR];
// }disk_sector;




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
char* readSector(int);


//leggo un cluster
void readCluster();

//quanti cluster liberi ci sono nella fat e numero di files
void cluster_info(int *, int*);

//crea una sub-directory, della directory di lavoro, 
//utilizzando il nome passato nel parametro dirname
void createDir(char*);

//lunghezza del disco
size_t disk_length();

//scrivo su disco 
void write_on_disk(void*, int);