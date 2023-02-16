#include "fat.c"

int main(int argc, char** argv){
    
    char input[500];
    
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
    init();
    //controllo se file è gia esistente
    if (access(f_name, F_OK) == 0) {
        printf("il file è già esistente in memoria\n");
    } else {
        printf("disco non esistente, formattazione in corso del volume %s...\n", f_name);
        format(f_name);
    }
    
    //mappo il disco in memoria
    read_disk(f_name);
    read_boot_record();
    
    char* token;
    char* read_text = NULL;

    FileHandle* global_handle = NULL;

    
    while(1){
        print_path(path);
        printf(COLOR_BOLD_BLUE "> " COLOR_DEFAULT);
        scanf(" %[^\n]%*c", input);
        token = strtok(input, " ");
        
        if(strcmp(input, "info") == 0 || strcmp(input, "i") == 0){
            info();
        }
        
        else if(strcmp(token, "help") == 0 || strcmp(token, "h") == 0){
            token = strtok(NULL, " ");
            help(token);
        }
        
        else if(strcmp(token, "createDir") == 0 || strcmp(token, "md") == 0){
            token = strtok(NULL, " ");
            create_dir(token);
            
        }
    
        else if(strcmp(token, "changeDir") == 0|| strcmp(token, "cd") == 0){
            token = strtok(NULL, " ");
            change_dir(token);
        }

        else if(strcmp(input, "listDir") == 0 || strcmp(input, "ld") == 0){
            list_dir(current_dir(path));
        }

        else if(strcmp(token, "createFile") == 0 || strcmp(token, "cf") == 0){
            token = strtok(NULL, " ");
            char* tmp_token = token;
            token = strtok(NULL, " ");
            FileHandle* fe;
            if(!token)
                fe = create_file(tmp_token, 0);
            else
                fe = create_file(tmp_token, atoi(token));
            free(fe);
        }

        else if(strcmp(token, "read") == 0 || strcmp(token, "r") == 0 ){
            token = strtok(NULL, " ");
            read_text = read_file(token);
            if (read_text){
                FileHandle* fe = get_file_handle(get_dir_entry(token));
                if(global_handle != NULL && fe->entry == global_handle->entry){
                    if(global_handle->seek > strlen(read_text))
                        printf("\n");
                    else
                        printf("%s\n", read_text + global_handle->seek);
                    
                    free(global_handle);
                    global_handle = NULL;
                }
                else{
                    printf("%s\n", read_text);
                    
                }
                free(fe);
                free(read_text);
                read_text = NULL;

                
            }
        }

        else if(strcmp(token, "write") == 0 || strcmp(token, "w") == 0 ){
            char* name = strtok(NULL, " ");
            if(!name){
                printf( "inserire nome\n" );
            }
            else{
                DirectoryEntry* dir_entry = get_dir_entry(name);
                if(!dir_entry){
                    printf(COLOR_RED "file inesistente\n" COLOR_DEFAULT);
                }
                else{
                    token = strtok(NULL, "");
                    if(!token){
                        printf(COLOR_RED "inserire contenuto da scrivere\n" COLOR_DEFAULT);
                    }
                    else{
                        FileHandle* fe = get_file_handle(dir_entry);
                        
                        read_text = read_file(fe->entry->name);
                        if (strlen(read_text) == 0){
                            write_file(token, fe);
                        }else{
                            if(global_handle != NULL && fe->entry == global_handle->entry){
                                int dim = global_handle->seek + strlen(token);
                                
                                if( dim > strlen(read_text)){
                                    read_text = realloc(read_text, dim + 1);
                                    // write_file(strcat(read_text, token), fe);
                                    *(read_text + dim) = '\0';
                                }
                                
                                int i = global_handle->seek;
                                for(int j = 0; j < strlen(token); j++){
                                    *(read_text + i++) = *(token+j);
                                }

                                write_file(read_text, fe);
                            
                                free(global_handle);
                                global_handle = NULL;
                            }
                            else{
                                read_text = realloc(read_text, strlen(read_text) + strlen(token) + 1);
                                write_file(strcat(read_text, token), fe);
                            }
                            
                        }
                    
                        free(fe);
                        free(read_text);
                        read_text = NULL;
                    }
                }
            }
        }

        else if(strcmp(token, "seek") == 0 || strcmp(token, "s") == 0){
            token = strtok(NULL, " ");
            if(token){
                DirectoryEntry* dir_entry = get_dir_entry(token);
                if(!dir_entry){
                    printf(COLOR_RED "file inesistente\n" COLOR_DEFAULT);
                }
                else{
                    global_handle = get_file_handle(dir_entry);
                    token = strtok(NULL, " ");
                    if(!token)
                        printf("inserire offset\n");
                    else{
                        if(seek(global_handle, atoi(token)) == 1){
                            printf(COLOR_RED "offset non valido\n" COLOR_DEFAULT);
                            free(global_handle);
                            global_handle = NULL;
                        }
                    }
                }
            }
            else{
                printf("Inserire nome file\n");
            }
            

        }

        else if(strcmp(token, "eraseFile") == 0 || strcmp(token, "rf") == 0){
            token = strtok(NULL, " ");
            erase_file(token);
        }

        else if(strcmp(token, "eraseDir") == 0 || strcmp(token, "rd") == 0){
            token = strtok(NULL, " ");
            erase_dir(token);
        }

        else if(strcmp(token, "format") == 0 || strcmp(token, "f") == 0){
            char answer[50];
            printf("Sei sicuro di voler formattare il disco? tutti i tuoi dati andranno persi[si/no]\n");
            scanf("%[^\n]%*c", answer);
            if(strcmp(answer, "Si") == 0 ||
             strcmp(answer, "si") == 0 || 
             strcmp(answer, "s") == 0 ||
             strcmp(answer, "S") == 0){
                format(boot_record->name);
            }
        }

        else if (strcmp(input, "exit") == 0 || strcmp(input, "e") == 0)
            break;
        
        else{
            printf("comando sconosciuto, per maggiori informazioni consultare help\n");
        }


    }


    if(munmap(disk, disk_length()) == -1){
        perror("Rimozione mappatura fallita: ");
        free(boot_record);
        list_free(path);
        exit(EXIT_FAILURE);
    }
    
    if(global_handle)
        free(global_handle);

    free(boot_record);
    list_free(path);
    return 0;
}