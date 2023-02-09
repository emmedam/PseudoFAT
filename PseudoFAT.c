#include "fat.c"



int main(int argc, char** argv){
    init();
    char input[50];
    
    if(argc == 1){
        printf("PseudoFAT\n" 
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
    char* read_text;
    while(1){
        print_path(path);
        printf(COLOR_BOLD_BLUE "> " COLOR_OFF);
        scanf(" %[^\n]%*c", input);
        token = strtok(input, " ");

        if(strcmp(input, "info") == 0){
            info();
        }
        
        else if(strcmp(token, "help") == 0 || strcmp(token, "h") == 0){
            token = strtok(NULL, " ");
            help(token);
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
            //listDir(path->dir_entry);
            listDir(current_dir(path));
        }

        else if(strcmp(token, "createFile") == 0 || strcmp(token, "cf") == 0){
            token = strtok(NULL, " ");
            char* tmp_token = token;
            token = strtok(NULL, " ");
            if(!token)
                createFile(tmp_token, 0);
            else
                createFile(tmp_token, atoi(token));
        }

        else if(strcmp(token, "read") == 0 ){
            token = strtok(NULL, " ");
            read_text = read_file(token);
            if (read_text){
                printf("%s\n", read_text);
                free(read_text);
            }
        }

        else if(strcmp(token, "write") == 0 ){
    
            DirectoryEntry* dir_entry = get_dir_entry(strtok(NULL, " "));
            if(!dir_entry){
                printf("file inesistente\n");
            }
            else{
                token = strtok(NULL, "");
                FileHandle* fe = get_file_handle(dir_entry);
                read_text = read_file(fe->entry->name);
                write_file(strcat(read_text, token), fe);
                free(fe);
                free(read_text);
            }
        }

        else if(strcmp(token, "eraseFile") == 0 || strcmp(token, "rm") == 0){
            token = strtok(NULL, " ");
            erase_file(token);
        }

        else if(strcmp(token, "eraseDir") == 0 || strcmp(token, "rmdir") == 0){
            token = strtok(NULL, " ");
            erase_dir(token);
        }

        else if(strcmp(token, "format") == 0){
            char *answer = (char *)malloc(sizeof(char));
            // TODO chiedere conferma prima di format
            printf("Sei sicuro di voler formattare il disco? tutti i tuoi dati andranno persi[si/no]\n");
            scanf("%[^\n]%*c", answer);
            if(strcmp(answer, "si") == 0){
                format(boot_record->name);
            }
            else if(strcmp(answer, "no") != 0 || strcmp(answer, "si") != 0 ){
                printf("comando non valido\n");
            }
            free(answer);
        }

        else if (strcmp(input, "exit") == 0)
            break;
        
        else{
            printf("comando sconosciuto, per maggiori informazioni consultare help\n");
        }


    }

    if(munmap(disk, disk_length()) == -1){
        perror("Rimozione mappatura fallita: "); 
        exit(EXIT_FAILURE);
        
    }

    return 0;

}


