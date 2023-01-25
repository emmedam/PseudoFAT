#include "fat.c"


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
        if(strcmp(working_dir->name, "root") != 0)
            printf(COLOR_BOLD_BLUE "root/%s> " COLOR_OFF, working_dir->name);  
        else  
            printf(COLOR_BOLD_BLUE "%s> " COLOR_OFF, working_dir->name);
        // fgets(input, sizeof input, stdin);
        scanf(" %[^\n]%*c", input);
        token = strtok(input, " ");

        if(strcmp(input, "info") == 0){
            info();
        }
        
        else if(strcmp(token, "createDir") == 0){
            token = strtok(NULL, " ");
            createDir(token);
            
        }
    
        else if(strcmp(token, "changeDir") == 0){
            token = strtok(NULL, " ");
            changeDir(token);
        }

        else if(strcmp(input, "listDir") == 0){
            listDir(working_dir);
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

// int cmpInput(char *command, char *input){
//     char* token;

//     token = strtok(input, " ");
//     if(strcmp(token, command) == 0){
//         strcpy( strtok(NULL, " "), dir_name);
//         return 1;
//     }
        
//     return 0;
// }