# Progetto **Pseudo FAT**
#### Martina D'Amico - 1791820 - damico.1791820@studenti.uniroma1.it

***

## Specifiche del progetto
### _Sistemi Operativi 2021-22, Prof. Grisetti_
Implement a file system that uses a pseudo "FAT" on an mmapped buffer.

The functions to implement are:

* `createFile`
* `eraseFile`
* `write` (potentially extending the file boundaries)
* `read`
* `seek`
* `createDir`
* `eraseDir`
* `changeDir`
* `listDir`

The opening of a file should return a `FileHandle` that stores the position in a `file`.

> https://gitlab.com/grisetti/sistemi_operativi_2021_22/-/blob/main/so_2021_22_projects.txt

## Premessa

Per implementare una versione simile al file system **FAT**, senza l'onere di operare con gli strumenti tipici dei bassi livelli del calcolatore elettronico, si mappa lo spazio fisico su uno spazio di memoria virtuale recuperando i dati dalla memoria in modo grezzo _raw_.

## Modello Pseudo FAT
Nel nostro modello si utilizza un sottoinsieme delle funzionalità e dei dati del classico **FAT**: quelli strettamente necessari per una corretta implementazione del file system. Inoltre si adottanno delle semplificazioni, ad esempio, nel modello reale non si può prescidere da una grandezza costante quella del `disk sector` pari a `512 bytes` (per i classici _hard-disk_), nel nostro sistem, poiché poco interessati allo _storage_ vero e proprio, si fissa tale grandezza a `12 byte`. Inoltre si gestiscono solamente file di testo e i nomi dei files e delle directory avranno una lunghezza massima di 8 caratteri. I files non potranno avere una dimensione maggiore di 255 byte.

## Rappresentazione dello spazio
Lo spazio virtuale, mappato in memoria, che modella il "disco fisico" viene suddiviso come segue

![Rappresentazione del disco](disk.png)

### Boot record
Occupa il primo settore del volume e conserva informazioni generali, come di seguito descritto.

|offset (byte)|size (byte)|descrizione|
|:------------|:----------|:----------|
|0|1|Numero di byte per settore|
|1|1|Numero di settori per cluster|
|2|1|Numero di cluster|
|3|1|Numero di `entries` per le `Directory Table` inclusa la `Root Directory`|
|4|8|Nome del volume|

### FAT

La file allocation table è il cuore del filesystem da cui prende il nome. Questa tabella è di fondamentale importanza per gestire il file system secondo il paradigma della _allocazione per concatenazione_. Attraverso la **FAT** il sistema recupera il contenuto dei files, memorizzati nella `data area` all'interno di uno o più cluster, non necessariamente consecutivi, ma sequenzialmente ordinati nella **FAT**.
La **FAT** occupa un certo numero di settori subito dopo il boot record, lo spazio riservato in memoria espresso in settori è in funzione del numero di cluster definiti nel `Boot record`:

`Spazio FAT (settori) = ⌈(n_cluster / n_byte_per_settore)⌉`


|offset (byte)|size (byte)|descrizione|
|-------------|-----------|:----------|
|0|1|Può assumere i seguenti valori:|
|||0: cluster libero|
|||1: ultimo cluster del file|
|||2-255: cluster successivo del file|

Ovviamente i cluster logici inizieranno dall'indice 2 e secondo questa nostra implementazione non potranno essere più di 253.

### Root Directory
Occupa un certo numero di settori, in funzione dell'impostazione del valore di numero di `entries` contenuto nel `Boot record`, ove viene memorizzata la `Directory Table`. Quest'ultima è una tabella che elenca i file e le subdirectory della root come verrà meglio specificato nel seguito. Si precisa che ogni directory del volume, inclusa la `root`, possiede una propria `Directory Table`.

#### Directory Table
E' un elenco di `directory entry` necessari per rappresentare il contenuto di una directory: `files` e `subdirectory`. Questa, in combinazione con la **FAT**, consente di recuperare i dati presenti nella data area: nella tabella è memorizzato l'entry-point di ogni file o meglio il numero del primo cluster da cui parte la ricerca che proseguirà nella FAT.

Lo spazio occupato in memoria della `Directory Table`, espresso in settori, è dato dalla seguente formula:

`Spazio Directory Table (settori) = ⌈n_directory_entries * lughezza_directory_entry / n_byte_per settore⌉`

Nel nostro modello avendo assunto che il numero di byte per settore viene fissato a 12, osservando la struttura delle `directory_entry` che occupano esattamente 12 byte, si può semplificare il calcolo come segue:

`Spazio Directory Table (settori) = n_directory_entries`

#### Directory Entry
Come già descritto la `directory entry` rappresenta un singolo record della `directory table` dove vengono memorizzate le informazioni relative ai `files` o alle `sub-direcotry` afferenti una determinata `directory`

|offset (byte)|size (byte)|descrizione|
|-------------|-----------|:----------|
|0|8|nome|
|8|2|data di creazione (in millisecondi)|
|10|1|primo cluster|
|11|1|dimensione (se uguale a zero si tratta di una `sub-directory`)|

### Data Area
Spazio riservato allo storage vero e proprio dei files e delle `Directory Table`, rappresentative delle sub-directory della root, e viene suddiviso in cluster.


### Esempio rappresentazione di un volume

```
Parametri del boot record
-byte per settore: 12
-n. settori per cluster: 3
-n. cluster: 100
-n. entries per le Directory Table: 10
-nome del volume: AFRODITE

Spazio Boot record = ⌈12 byte / 12 byte⌉ (1 settore)
Spazio FAT = ⌈10 byte / 12 byte⌉ (1 settore)
Spazio Root Directory = ⌈120 byte / 12 byte⌉ (10 settori)
Spazio Data area = 3600 byte (300 settori)

Il volume totale allocato è dunque di 312 settori pari a 3744 byte 
```
### Interfaccia utente

Il programma si utilizza da terminale, in ambiente **linux**, e si avvia con il seguente comando:

`$ ./PseudoFAT [diskname]`

il parametro `diskname` (max 8 caratteri) indica il nome del disco sul quale lavorare, se non esiste si crea un nuovo disco altrimenti mappa in memoria quello passato come parametro.

|comando |sintassi |descrizione|
|:-------|:--------|:----------|
|`changeDir`|`changeDir [dirname]`|cambia la `directory` di lavoro come indicato dal parametro `dirname`|
|`createDir`|`createDir [dirname]`|crea una sub-directory, della `directory` di lavoro, utilizzando il nome passato nel parametro `dirname`|
|`createFile`|`createFile [filename]`|crea un file di testo vuoto con il nome fornito dal parametro `filename`|
|`eraseDir`|`eraseDir [dirname]`|elimina la directory, indicata dal parametro `dirname`, e ricorsivamente tutto il suo contenuto (`sub-directory` incluse)|
|`eraseFile`|`eraseFile [filename]`|elimina il file indicato dal parametro `filename`|
|`exit`|`exit`|chiude il programma e salva il filesystem di lavoro sul file con il nome del disco|
|`format`|`format`|formatta il disco di lavoro|
|`info`|`info`|elenca a video informazioni sul volume di lavoro|
|`read`|`read [filename]`|legge il contenuto del file passato come parametro e lo visualizza a schermo|
|`help`|`help [command]`|fornisce una descrizione del comando passato come parametro, se non fornito elenca i comandi disponibili|
|`listDir`|`listDir [dirname]`|elenca il contenuto della directory passata come parametro, se non fornita elenca il contenuto della directory di lavoro|
|`seek`|?|?|
|`write`|`write [filename] [content]`|scrive nel file di testo, indicato dal parametro `filename`, il contenuto del parametro `content`. Se il file non esiste lo crea|


Fonti:
* http://osr600doc.sco.com/en/FS_admin/_The_dosfs_File_System_Type.html
* http://www.fandecheng.com/personal/interests/ewindows/msdos_functional/fat.htm
* https://www.markdownguide.org/basic-syntax/
