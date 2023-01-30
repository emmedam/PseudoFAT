#include "fat.c"

void print_path();

int main(int argc, char** argv){
    init();
    char input[50];
    char dir_name[12];    
    
    if(argc == 1){
        printf("PseudoFAT, versione 1.0\n" 
               "Utilizzo: $./PseudoFAT [Disk Name]\n\n"
               "-help per consultare la lista dei comandi;\n" 
               "-help [command] per maggiori informazioni sul comando;\n\n"
               "Realizzato da Martina D\'Amico (damico.1791820@studenti.uniroma1.it)\n" 
               "Progetto Sistemi Operativi 2021-22, Prof. Grisetti\n");    
        exit(0);
    }
    char *f_name = argv[1];
    
    //controllo se file è gia esistente
    if (access(f_name, F_OK) == 0) {
        printf("il file è già esistente in memoria\n");
    } else {
        printf("disco non esistente, formattazione in corso del volume %s...\n", f_name);
        format(f_name);
    }
    
    //mappo il disco in memoria
    readDisk(f_name);
    readBootRecord();
    char* token;
    while(1){
        print_path(working_dir);
        printf(COLOR_BOLD_BLUE "> " COLOR_OFF);
        scanf(" %[^\n]%*c", input);
        token = strtok(input, " ");

        if(strcmp(input, "info") == 0){
            info();
        }
        
        else if(strcmp(token, "createDir") == 0 || strcmp(token, "md") == 0){
            token = strtok(NULL, " ");
            createDir(token);
            
        }
    
        else if(strcmp(token, "changeDir") == 0|| strcmp(token, "cd") == 0){
            token = strtok(NULL, " ");
            changeDir(token);
        }

        else if(strcmp(input, "listDir") == 0 || strcmp(input, "ld") == 0){
            listDir(working_dir->dir_entry);
        }

        else if(strcmp(token, "createFile") == 0 || strcmp(token, "cf") == 0){
            token = strtok(NULL, " ");
            char* tmp_token = token;
            token = strtok(NULL, " ");
            createFile(tmp_token, atoi(token));
        }

        

        else if (strcmp(input, "exit") == 0)
            break;
        
        else{
            printf("comando sconosciuto\n");
        }


    }

    if(munmap(disk, disk_length()) == -1){
        perror("Rimozione mappatura fallita: "); 
        exit(EXIT_FAILURE);
        
    }

    return 0;

}


// void print_path(){
//     if(path_size(working_dir) == 1){
//         printf(COLOR_BOLD_BLUE "root> " COLOR_OFF);
//         return;
//     }
//     while(working_dir != NULL){
//         char *name = ((DirectoryEntry*)working_dir->dir_entry)->name;
//         printf(COLOR_BOLD_BLUE "%s/> " COLOR_OFF, name);
//         working_dir = working_dir->next; 

//     }
// }