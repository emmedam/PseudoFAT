#include "utils.h"

char* formatTime(char * str, time_t t){
    if(t == 0){
        return "\t\t\t";
    }
    struct tm *ptm = localtime(&t);
    sprintf(str, "%02d/%02d/%04d %02d:%02d:%02d", 
        ptm->tm_mday, (ptm->tm_mon) + 1, (ptm->tm_year) + 1900,
        ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
    return str;
}