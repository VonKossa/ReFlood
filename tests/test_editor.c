#include "reflood_editor_level.h"
#include "reflood_editor_browser.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#define make_dir(p) _mkdir(p)
#else
#include <sys/stat.h>
#define make_dir(p) mkdir(p,0755)
#endif
int main(int argc,char **argv){
    EditorLevel source,copy;char error[160];
    editor_level_new(&source,77);
    source.header[2]=0;source.header[3]=2;
    source.triggers[0]=0x37;source.triggers[19]=0x91;
    source.map[100]=0x94;
    make_dir("editor-test-output");
    assert(editor_level_save(&source,"editor-test-output",error,sizeof(error)));
    assert(editor_level_load(&copy,"editor-test-output",77,error,sizeof(error)));
    assert(!memcmp(copy.header,source.header,16));
    assert(!memcmp(copy.map,source.map,EDITOR_MAP_BYTES));
    assert(!memcmp(copy.triggers,source.triggers,EDITOR_TRIGGER_BYTES));
    assert(editor_level_save_named(&source,"editor-test-output","my_cavern",error,sizeof(error)));
    assert(editor_level_named_exists("editor-test-output","my_cavern"));
    assert(editor_level_load_header(&copy,"editor-test-output/my_cavern_header.bin",error,sizeof(error)));
    assert(!memcmp(copy.map,source.map,EDITOR_MAP_BYTES));
    assert(!editor_level_save_named(&source,"editor-test-output","../escape",error,sizeof(error)));
    assert(editor_level_load_header(&copy,"editor-test-output/levels/level_77_header.bin",error,sizeof(error)));
    assert(!memcmp(copy.map,source.map,EDITOR_MAP_BYTES));
    assert(!editor_level_load_header(&copy,"editor-test-output/levels/level_77_tilemap.bin",error,sizeof(error)));
    assert(!editor_level_load_header(&copy,"editor-test-output/level_77_header.bin",error,sizeof(error)));
    EditorBrowser browser;
    assert(editor_browser_open(&browser,"editor-test-output/levels",error,sizeof(error)));
    bool found=false;
    for(unsigned i=0;i<browser.count;i++)
        if(!strcmp(browser.entries[i].name,"level_77_header.bin")){
            browser.selected=i;found=true;break;
        }
    assert(found);
    char selected_path[EDITOR_BROWSER_PATH]={0};
    assert(editor_browser_activate(&browser,selected_path,sizeof(selected_path),error,sizeof(error)));
    assert(!strcmp(selected_path,"editor-test-output/levels/level_77_header.bin"));
    assert(editor_level_load_header(&copy,selected_path,error,sizeof(error)));
    assert(editor_browser_parent(&browser,error,sizeof(error)));
    assert(!strcmp(browser.directory,"editor-test-output"));
    assert(editor_browser_parent(&browser,error,sizeof(error)));
    assert(!strcmp(browser.directory,"."));
    assert(editor_browser_parent(&browser,error,sizeof(error)));
    assert(!strcmp(browser.directory,".."));
    assert(editor_browser_parent(&browser,error,sizeof(error)));
    assert(!strcmp(browser.directory,"../.."));
    copy.map[4u*EDITOR_W+4u]=0;
    assert(!editor_level_save(&copy,"editor-test-output",error,sizeof(error)));
    assert(strstr(error,"start marker")!=NULL);
    if(argc>1){
        for(unsigned level=1;level<=42;level++){
            assert(editor_level_load(&copy,argv[1],level,error,sizeof(error)));
            assert(editor_level_save(&copy,"editor-test-output",error,sizeof(error)));
            EditorLevel again;
            assert(editor_level_load(&again,"editor-test-output",level,error,sizeof(error)));
            assert(!memcmp(copy.header,again.header,sizeof(copy.header)));
            assert(!memcmp(copy.map,again.map,sizeof(copy.map)));
            assert(!memcmp(copy.triggers,again.triggers,sizeof(copy.triggers)));
        }
    }
    puts("editor level round trip and start marker validation passed");
    return 0;
}
