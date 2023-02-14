#pragma once
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
#include "utils.h"
#include "linked_list.h"


#define COLOR_RED       "\e[0;31m"
#define COLOR_DEFAULT   "\e[0m"
#define COLOR_GREEN     "\e[0;32m"
#define COLOR_BLUE      "\x1b[34m"
#define COLOR_BOLD_BLUE "\e[1;34m"


#define BYTE_PER_SECTOR             32 //rimane fisso
#define SECTOR_PER_CLUSTER          8
#define NUMBER_OF_CLUSTER           100
#define NUMBER_OF_DIRECTORY_ENTRIES 8

enum type {FILE_FAT = 0, DIRECTORY_FAT = 1};

typedef struct BootRecord{
    u_int16_t   byte_per_sector;
    u_int16_t   sector_per_cluster;
    u_int16_t   n_cluster;
    u_int16_t   n_directory_entries;
    time_t      date;
    char        name[12];
}BootRecord;

typedef struct DirectoryEntry{
    time_t      creation_date;
    time_t      update_date;
    u_int16_t   first_cluster;
    u_int16_t   dimension;
    char        name[12];
}DirectoryEntry;

typedef struct FileHandle{
  //indica la posizione della dir_entry
  DirectoryEntry* entry;
  u_int16_t seek;
}FileHandle;

typedef struct ListPath ListPath;

extern BootRecord *boot_record;

//riferimento al volume di lavoro mappato in memoria
extern void *disk;


//lista concatenata che rappresenta il percorso
// della directory di lavoro o working dir
extern ListPath* path;


/*********************************************/

void init();

void save(char*);

//calcolo numero di settori necessari per allocare la Directory Table
int dir_table_sector_number();

//calcolo numero di settori necessari per allocare la FAT table
int fat_sector_number();

//calcolo numero di settori necessari per allocare la Data Area
long data_area_sector_number();

//allocazione BootRecord
BootRecord* initBootRecord(char*);

//leggo contenuto del boot record sulla memoria
void read_boot_record();

//elenca a video informazioni sul volume di lavoro
void info();

//formatta il disco di lavoro
void format(char*);

//leggo un settore 
void* read_sector(int);

//quanti cluster liberi ci sono nella fat e numero di files
void cluster_info(int *, int*);

//crea una sub-directory, della directory di lavoro, 
//utilizzando il nome passato nel parametro dirname
void create_dir(char*);

//lunghezza del disco
size_t disk_length();

//scrivo su disco 
void write_on_disk(void*, int);


//elenca il contenuto della directory passata come parametro, 
//se non fornita elenca il contenuto della directory di lavoro
void list_dir();

//cambia la directory di lavoro, modifico la working dir
void change_dir(char*);


//ritorna il primo settore di un cluster
u_int16_t first_sector_of_cluster(u_int16_t);

void init_root();

FileHandle* create_file(char*, int);

void write_on_fat(int, u_int16_t*);

void write_on_data_area();

//legge il contenuto del file passato come parametro e lo visualizza a schermo
char* read_file(char*);

DirectoryEntry* write_file(char*, FileHandle*);

//ritorna lo spazio rimanente nella data area
int remaining_space();

//elimina il file indicato dal parametro
void erase_file(char*);

//elimina la directory, indicata dal parametro
void erase_dir(char*);


FileHandle* get_file_handle(DirectoryEntry*);


void read_disk(char*);

int seek(FileHandle*, u_int16_t);

u_int16_t sector_current_dir();

DirectoryEntry* set_dir_entry(char*, enum type);

void clear_fat(u_int16_t);

void* get_dir_entry(char* name);
