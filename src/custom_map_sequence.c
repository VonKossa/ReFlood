#include "reflood_custom_map.h"
#include <stdio.h>
#include <string.h>

bool reflood_custom_next_header(const char *current,char *next,size_t capacity){
    if(!current||!next||!capacity)return false;
    const char *name=strrchr(current,'/'),*backslash=strrchr(current,'\\');
    if(backslash&&(!name||backslash>name))name=backslash;
    name=name?name+1:current;
    if(strlen(name)!=19u||strncmp(name,"level_",6u)!=0||
       strcmp(name+8,"_header.bin")!=0||
       name[6]<'0'||name[6]>'9'||name[7]<'0'||name[7]>'9')return false;
    const unsigned number=(unsigned)(name[6]-'0')*10u+(unsigned)(name[7]-'0');
    if(number<1u||number>=99u)return false;
    const size_t directory_length=(size_t)(name-current);
    if(directory_length+strlen(name)>=capacity)return false;
    const int length=snprintf(next,capacity,"%.*slevel_%02u_header.bin",
        (int)directory_length,current,number+1u);
    if(length<0||(size_t)length>=capacity)return false;
    FILE *file=fopen(next,"rb");
    if(!file)return false;
    fclose(file);
    return true;
}
