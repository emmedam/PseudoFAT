#include "fat.c"


int main(int argc, char** argv){
    init();
    char input[20];
    
    if(argc == 1){
        printf("PseudoFAT, versione 1.0\nSe si desidera creare un nuovo disco digitare o recuperare un disco mappato in memeoria digitare il nome tramite il comando ./main [diskname]\nDigitare help per consultare la lista dei comandi\ndigitare help[command] per saperne di più sulla funzione [command]\n");    
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

    while(1){
        printf("root> ");
        scanf(" %[^\n]%*c", input);
        
    
        if(strcmp(input, "info") == 0){
            info();
        }

        else if(strcmp(input, "createDir") == 0){
            createDir("pippo");
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