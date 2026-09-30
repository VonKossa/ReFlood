#include "reflood_editor_level.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
#include <direct.h>
#define mkdir_one(p) _mkdir(p)
#else
#include <sys/stat.h>
#define mkdir_one(p) mkdir(p,0755)
#endif

static void fail(char *error,size_t capacity,const char *message){
    if(error&&capacity)snprintf(error,capacity,"%s",message);
}
static unsigned be16(const uint8_t *p){return (unsigned)p[0]*256u+p[1];}
unsigned editor_level_bank(const EditorLevel *l){return be16(l->header);}
unsigned editor_level_trigger_count(const EditorLevel *l){return be16(l->header+2);}
static bool path(char *dst,size_t cap,const char *dir,const char *name,const char *part){
    int count=snprintf(dst,cap,"%s/%s_%s.bin",dir,name,part);
    return count>0&&(size_t)count<cap;
}
static bool valid_name(const char *name){
    size_t n=strlen(name);
    if(!n||n>63u)return false;
    for(size_t i=0;i<n;i++){
        char c=name[i];
        if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||
             (c>='0'&&c<='9')||c=='_'||c=='-'))return false;
    }
    return true;
}
static bool read_exact(const char *p,void *dst,size_t bytes){
    FILE *f=fopen(p,"rb");if(!f)return false;
    bool ok=fread(dst,1,bytes,f)==bytes&&fgetc(f)==EOF;
    fclose(f);return ok;
}
void editor_level_new(EditorLevel *l,unsigned number){
    memset(l,0,sizeof(*l));l->level=number;
    l->header[1]=1; /* BLOCK A */
    l->map[4u*EDITOR_W+4u]=22; /* Quiffy start */
    l->changed=true;
}
bool editor_level_validate(const EditorLevel *l,char *error,size_t cap){
    if(l->level<1||l->level>99){fail(error,cap,"level number must be 1..99");return false;}
    if(editor_level_bank(l)<1||editor_level_bank(l)>3){fail(error,cap,"BLOCK bank must be A, B or C");return false;}
    if(editor_level_trigger_count(l)>64){fail(error,cap,"trigger count exceeds 64");return false;}
    unsigned starts=0;
    for(size_t i=0;i<EDITOR_MAP_BYTES;i++)if(l->map[i]==22)starts++;
    if(!starts){fail(error,cap,"place a Quiffy start marker (tile 22)");return false;}
    return true;
}
static bool load_named(EditorLevel *l,const char *directory,const char *name,
    unsigned n,char *error,size_t cap){
    EditorLevel temp;memset(&temp,0,sizeof(temp));temp.level=n;
    const char *parts[]={"header","tilemap","trigger_payload"};
    void *dest[]={temp.header,temp.map,temp.triggers};
    const size_t sizes[]={sizeof(temp.header),sizeof(temp.map),sizeof(temp.triggers)};
    char p[1024];
    for(unsigned i=0;i<3;i++){
        if(!path(p,sizeof(p),directory,name,parts[i])||!read_exact(p,dest[i],sizes[i])){
            fail(error,cap,"cannot read complete level header, tilemap or trigger payload");return false;
        }
    }
    if(!editor_level_validate(&temp,error,cap))return false;
    /* The fourth extractor file is a copy of active records, when present. */
    if(!path(p,sizeof(p),directory,name,"triggers")){fail(error,cap,"level path too long");return false;}
    FILE *f=fopen(p,"rb");
    if(f){
        size_t count=editor_level_trigger_count(&temp)*10u;
        uint8_t records[EDITOR_TRIGGER_BYTES];
        bool matches=fread(records,1,count,f)==count&&fgetc(f)==EOF&&
            memcmp(records,temp.triggers,count)==0;
        fclose(f);
        if(!matches){fail(error,cap,"trigger file differs from trigger payload");return false;}
    }
    *l=temp;return true;
}
bool editor_level_load(EditorLevel *l,const char *directory,unsigned n,char *error,size_t cap){
    char dir[1024],name[16];
    if(snprintf(dir,sizeof(dir),"%s/levels",directory)>=(int)sizeof(dir)){
        fail(error,cap,"level path too long");return false;
    }
    snprintf(name,sizeof(name),"level_%02u",n);
    return load_named(l,dir,name,n,error,cap);
}
bool editor_level_load_header(EditorLevel *l,const char *header_path,char *error,size_t cap){
    const char *slash=strrchr(header_path,'/');
    const char *backslash=strrchr(header_path,'\\');
    if(backslash&&(!slash||backslash>slash))slash=backslash;
    if(!slash){fail(error,cap,"choose a header file from a folder");return false;}
    const char *name=slash+1;
    size_t length=strlen(name);
    if(length<=11u||strcmp(name+length-11u,"_header.bin")!=0||length-11u>63u){
        fail(error,cap,"choose a *_header.bin file");return false;
    }
    char stem[64],dir[1024];
    memcpy(stem,name,length-11u);stem[length-11u]='\0';
    if(!valid_name(stem)){fail(error,cap,"invalid level file name");return false;}
    size_t dir_len=(size_t)(slash-header_path);
    if(dir_len>=sizeof(dir)){fail(error,cap,"level path too long");return false;}
    if(!dir_len){dir[0]=header_path[0];dir[1]='\0';}
    else {memcpy(dir,header_path,dir_len);dir[dir_len]='\0';}
    unsigned number=1u;
    if(strlen(stem)==8u&&strncmp(stem,"level_",6u)==0&&
       stem[6]>='0'&&stem[6]<='9'&&stem[7]>='0'&&stem[7]<='9')
        number=(unsigned)(stem[6]-'0')*10u+(unsigned)(stem[7]-'0');
    return load_named(l,dir,stem,number,error,cap);
}
static bool write_exact(const char *p,const void *data,size_t bytes){
    FILE *f=fopen(p,"wb");if(!f)return false;
    bool ok=fwrite(data,1,bytes,f)==bytes;
    if(fclose(f)!=0)ok=false;
    if(!ok)remove(p);
    return ok;
}
bool editor_level_ensure_output_directory(const char *directory,char *error,size_t cap){
    char p[1024];
    if(snprintf(p,sizeof(p),"%s/levels",directory)>=(int)sizeof(p)){
        fail(error,cap,"output path too long");return false;
    }
    if(mkdir_one(directory)!=0&&errno!=EEXIST){fail(error,cap,"cannot create output directory");return false;}
    if(mkdir_one(p)!=0&&errno!=EEXIST){fail(error,cap,"cannot create levels directory");return false;}
    return true;
}
bool editor_level_named_exists(const char *directory,const char *name){
    if(!valid_name(name))return false;
    const char *parts[]={"header","tilemap","trigger_payload","triggers"};
    char p[1024];
    for(unsigned i=0;i<4u;i++)if(path(p,sizeof(p),directory,name,parts[i])){
        FILE *f=fopen(p,"rb");
        if(f){fclose(f);return true;}
    }
    return false;
}
bool editor_level_save_named(const EditorLevel *l,const char *directory,
    const char *name,char *error,size_t cap){
    if(!editor_level_validate(l,error,cap))return false;
    if(!valid_name(name)){fail(error,cap,"use 1..63 letters, digits, _ or - for the file name");return false;}
    char p[1024];
    const char *parts[]={"header","tilemap","trigger_payload","triggers"};
    const void *data[]={l->header,l->map,l->triggers,l->triggers};
    const size_t sizes[]={sizeof(l->header),sizeof(l->map),sizeof(l->triggers),
        editor_level_trigger_count(l)*10u};
    for(unsigned i=0;i<4;i++){
        if(!path(p,sizeof(p),directory,name,parts[i])||!write_exact(p,data[i],sizes[i])){
            fail(error,cap,"cannot write all four level files");return false;
        }
    }
    return true;
}
bool editor_level_save(const EditorLevel *l,const char *directory,char *error,size_t cap){
    char path[1024],name[16];
    if(!editor_level_ensure_output_directory(directory,error,cap))return false;
    if(snprintf(path,sizeof(path),"%s/levels",directory)>=(int)sizeof(path)){
        fail(error,cap,"output path too long");return false;
    }
    snprintf(name,sizeof(name),"level_%02u",l->level);
    return editor_level_save_named(l,path,name,error,cap);
}
