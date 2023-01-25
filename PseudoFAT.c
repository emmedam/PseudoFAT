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
        printf(COLOR_BOLD_BLUE "root> " COLOR_OFF);
        // fgets(input, sizeof input, stdin);
        scanf(" %[^\n]%*c", input);
    
        if(strcmp(input, "info") == 0){
            info();
        }
        
        else if(strcmp(token = strtok(input, " "), "createDir") == 0){
            //int i = 0;
            token = strtok(NULL, " ");
            createDir(token);
            
            // printf("%s\n", token);
            // if(token == NULL){ 
            //     printf("inserire nome directory\n");
            // }
            // printf("%s\n", dir_name);
            // // while( token != NULL ) {
            //     if(strcmp(token,"") == 0){ 
            //         printf("inserire nome directory\n");
            //         break;
            //     }
            //     if(strlen(token) > 12){
            //         printf("stringa troppo lunga\n");
            //         break;
            //     }
            //     strcpy(dir_name, token);
                
            //     // i++;
            //     // if(i>2){
            //     //     printf("troppi argomenti");
            //     //     break;
            //     // }

            //     token = strtok(NULL, " ");
            // }

            
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