#include "reflood_editor_browser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

static void message(char *error,size_t capacity,const char *s){
    if(error&&capacity)snprintf(error,capacity,"%s",s);
}
static bool header_name(const char *s){
    size_t n=strlen(s);
    return n>11u&&n<=74u&&strcmp(s+n-11u,"_header.bin")==0;
}
static bool join(char *path,size_t capacity,const char *dir,const char *name){
    size_t n=strlen(dir);
    int count=snprintf(path,capacity,"%s%s%s",dir,
        n&&(dir[n-1]=='/'||dir[n-1]=='\\')?"":"/",name);
    return count>0&&(size_t)count<capacity;
}
static int compare(const void *a,const void *b){
    const EditorBrowserEntry *left=a,*right=b;
    if(left->directory!=right->directory)return left->directory?-1:1;
    return strcmp(left->name,right->name);
}
static void add(EditorBrowser *b,const char *name,bool directory){
    if(b->count>=EDITOR_BROWSER_ENTRIES||strlen(name)>=sizeof(b->entries[0].name))return;
    if(!directory&&!header_name(name))return;
    EditorBrowserEntry *item=&b->entries[b->count++];
    strcpy(item->name,name);item->directory=directory;
}
bool editor_browser_open(EditorBrowser *b,const char *directory,char *error,size_t capacity){
    if(!directory||!*directory||strlen(directory)>=sizeof(b->directory)){
        message(error,capacity,"directory path is too long");return false;
    }
    EditorBrowser next;memset(&next,0,sizeof(next));
    strcpy(next.directory,directory);
#ifdef _WIN32
    char pattern[EDITOR_BROWSER_PATH];
    if(!join(pattern,sizeof(pattern),directory,"*")){
        message(error,capacity,"directory path is too long");return false;
    }
    struct _finddata_t found;
    intptr_t handle=_findfirst(pattern,&found);
    if(handle==-1){message(error,capacity,"cannot open directory");return false;}
    do {
        if(!strcmp(found.name,".")||!strcmp(found.name,".."))continue;
        add(&next,found.name,(found.attrib&_A_SUBDIR)!=0);
    }while(_findnext(handle,&found)==0);
    _findclose(handle);
#else
    DIR *dir=opendir(directory);
    if(!dir){message(error,capacity,"cannot open directory");return false;}
    struct dirent *found;
    while((found=readdir(dir))!=NULL){
        if(!strcmp(found->d_name,".")||!strcmp(found->d_name,".."))continue;
        char path[EDITOR_BROWSER_PATH];struct stat info;
        if(!join(path,sizeof(path),directory,found->d_name)||stat(path,&info)!=0)continue;
        if(S_ISDIR(info.st_mode)||S_ISREG(info.st_mode))
            add(&next,found->d_name,S_ISDIR(info.st_mode));
    }
    closedir(dir);
#endif
    qsort(next.entries,next.count,sizeof(next.entries[0]),compare);
    *b=next;return true;
}
bool editor_browser_parent(EditorBrowser *b,char *error,size_t capacity){
    char parent[EDITOR_BROWSER_PATH];
    size_t n=strlen(b->directory);
    if(!n)return false;
    if(n==3u&&b->directory[1]==':'&&
       (b->directory[2]=='/'||b->directory[2]=='\\'))return true;
    bool ascending=true;
    for(size_t i=0;i<n;){
        if(b->directory[i]!='.'||i+1u>=n||b->directory[i+1u]!='.'){
            ascending=false;break;
        }
        i+=2u;
        if(i<n&&b->directory[i++]!='/'){ascending=false;break;}
    }
    if(ascending){
        if(!join(parent,sizeof(parent),b->directory,"..")){
            message(error,capacity,"directory path is too long");return false;
        }
        return editor_browser_open(b,parent,error,capacity);
    }
    memcpy(parent,b->directory,n+1);
    while(n>1&&(parent[n-1]=='/'||parent[n-1]=='\\'))parent[--n]='\0';
    char *slash=strrchr(parent,'/'),*backslash=strrchr(parent,'\\');
    if(backslash&&(!slash||backslash>slash))slash=backslash;
    if(!slash){
        if(!strcmp(parent,"."))strcpy(parent,"..");
        else strcpy(parent,".");
    }
    else if(slash==parent)parent[1]='\0';
    else if(slash==parent+2&&parent[1]==':')slash[1]='\0';
    else *slash='\0';
    return editor_browser_open(b,parent,error,capacity);
}
bool editor_browser_activate(EditorBrowser *b,char *selected_path,size_t path_capacity,
    char *error,size_t capacity){
    if(!b->count||b->selected>=b->count){message(error,capacity,"select a level header");return false;}
    const EditorBrowserEntry *item=&b->entries[b->selected];
    char path[EDITOR_BROWSER_PATH];
    if(!join(path,sizeof(path),b->directory,item->name)){
        message(error,capacity,"selected path is too long");return false;
    }
    if(item->directory)return editor_browser_open(b,path,error,capacity);
    if(strlen(path)>=path_capacity){message(error,capacity,"selected path is too long");return false;}
    strcpy(selected_path,path);return true;
}
