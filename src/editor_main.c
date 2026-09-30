#include "reflood_editor_level.h"
#include "reflood_editor_browser.h"
#include "reflood_editor_tile_help.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 1280
#define HEIGHT 840
#define MAP_LEFT 12
#define MAP_WIDTH 800
#define PANEL_X 840
#define MAP_TOP 48
#define MAP_BOTTOM 788
#define PALETTE_TOP 78
#define PALETTE_SIZE 24
#define PALETTE_STEP 25
#define FONT_BYTES 960u
#define MAX_LABELS 80u
#define TILE_HELP_ROWS 21u
static const unsigned zoom_sizes[]={4u,6u,8u,12u,16u,24u,32u,48u,64u};
#define ZOOM_STEPS (sizeof(zoom_sizes)/sizeof(zoom_sizes[0]))
typedef struct {int x,y;char text[160];} EditorLabel;
typedef enum {DIALOG_NONE,DIALOG_QUIT,DIALOG_LOAD_WARNING,DIALOG_BROWSER,
    DIALOG_SAVE,DIALOG_OVERWRITE,DIALOG_TILES} EditorDialog;
static const uint32_t colours[16]={
    0xFF000000u,0xFF332233u,0xFF554488u,0xFF668899u,
    0xFF661100u,0xFF773300u,0xFFBB6600u,0xFFCC9922u,
    0xFF334411u,0xFF445511u,0xFF556600u,0xFF667700u,
    0xFF223366u,0xFF336677u,0xFF8899AAu,0xFFCCBB99u};
typedef struct {
    EditorLevel level;
    uint8_t undo[EDITOR_MAP_BYTES],font[FONT_BYTES],attributes[256];
    uint32_t atlas[256u*16u*16u];
    SDL_Texture *tiles,*font_texture;
    unsigned selected,zoom,scroll_x,scroll_y;
    bool undo_available,painting,dirty,fullscreen,grid,attributes_valid;
    EditorDialog dialog;
    EditorBrowser browser;
    char save_name[64],save_directory[EDITOR_BROWSER_PATH];
    bool save_name_focus;
    EditorLabel labels[MAX_LABELS];
    unsigned label_count,tile_help_offset;
    char message[160];
    const char *output,*data,*input;
    SDL_Window *window;
    SDL_Renderer *renderer;
} Editor;
static bool file_exact(const char *name,void *dst,size_t n){
    FILE *f=fopen(name,"rb");if(!f)return false;
    bool good=fread(dst,1,n,f)==n&&fgetc(f)==EOF;fclose(f);return good;
}
static SDL_Texture *make_font_texture(Editor *e){
    uint32_t pixels[FONT_BYTES*8u]={0};
    for(unsigned ch=0;ch<FONT_BYTES/8u;ch++)for(unsigned row=0;row<8u;row++)
        for(unsigned col=0;col<8u;col++){
            unsigned character=ch+0x14u;
            bool bit=character==':'?
                ((row==2u||row==6u)&&(col==3u||col==4u)):
                (e->font[ch*8u+row]&(0x80u>>col))!=0u;
            if(bit)pixels[(ch*8u+row)*8u+col]=0xfff0e8d2u;
        }
    SDL_Texture *texture=SDL_CreateTexture(e->renderer,SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STATIC,8,FONT_BYTES);
    if(!texture)return NULL;
    if(SDL_UpdateTexture(texture,NULL,pixels,8*4)!=0||
       SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND)!=0){
        SDL_DestroyTexture(texture);return NULL;
    }
    return texture;
}
static bool load_graphics(Editor *e){
    char path[1024];uint8_t raw[32768];
    snprintf(path,sizeof(path),"%s/blocks/BLOCK%c_tiles.bin",e->data,
        'A'+(int)editor_level_bank(&e->level)-1);
    if(!file_exact(path,raw,sizeof(raw)))return false;
    snprintf(path,sizeof(path),"%s/presentation/LEVEL_font.bin",e->data);
    if(!file_exact(path,e->font,sizeof(e->font)))return false;
    for(unsigned t=0;t<256;t++)for(unsigned y=0;y<16;y++){
        const uint8_t *src=raw+t*128u;
        for(unsigned x=0;x<16;x++){
            unsigned c=0,bit=15u-x;
            for(unsigned p=0;p<4;p++){
                unsigned off=p*32u+y*2u;
                c|=((((unsigned)src[off]<<8)|src[off+1])>>bit&1u)<<p;
            }
            e->atlas[((t/16u)*16u+y)*256u+(t%16u)*16u+x]=colours[c];
        }
    }
    SDL_Texture *new_tiles=SDL_CreateTexture(e->renderer,SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STATIC,256,256);
    if(!new_tiles)return false;
    if(SDL_UpdateTexture(new_tiles,NULL,e->atlas,256*4)!=0){SDL_DestroyTexture(new_tiles);return false;}
    SDL_Texture *new_font=e->font_texture?NULL:make_font_texture(e);
    if(!e->font_texture&&!new_font){SDL_DestroyTexture(new_tiles);return false;}
    if(e->tiles)SDL_DestroyTexture(e->tiles);
    e->tiles=new_tiles;
    if(new_font)e->font_texture=new_font;
    snprintf(path,sizeof(path),"%s/blocks/BLOCK%c_attrs.bin",e->data,
        'A'+(int)editor_level_bank(&e->level)-1);
    e->attributes_valid=file_exact(path,e->attributes,sizeof(e->attributes));
    return true;
}
static void rect(Editor *e,int x,int y,int w,int h,Uint8 r,Uint8 g,Uint8 b){
    SDL_Rect box={x,y,w,h};SDL_SetRenderDrawColor(e->renderer,r,g,b,255);
    SDL_RenderFillRect(e->renderer,&box);
}
static void label(Editor *e,int x,int y,const char *s){
    if(e->fullscreen){
        if(e->label_count<MAX_LABELS){
            EditorLabel *queued=&e->labels[e->label_count++];
            queued->x=x;queued->y=y;
            snprintf(queued->text,sizeof(queued->text),"%s",s);
        }
        return;
    }
    for(;*s;s++,x+=8){
        unsigned ch=(unsigned char)*s;
        if(ch<0x14u||ch>=0x8cu)continue;
        SDL_Rect src={0,(int)(ch-0x14u)*8,8,8},dst={x,y,8,8};
        SDL_RenderCopy(e->renderer,e->font_texture,&src,&dst);
    }
}
static void draw_tile(Editor *e,unsigned tile,int x,int y,int size){
    SDL_Rect src={(int)(tile%16u)*16,(int)(tile/16u)*16,16,16},dst={x,y,size,size};
    SDL_RenderCopy(e->renderer,e->tiles,&src,&dst);
}
static unsigned tile_size(const Editor *e){return zoom_sizes[e->zoom];}
static void draw_grid(Editor *e,int size){
    const int map_right=MAP_LEFT+(int)((EDITOR_W-e->scroll_x)*(unsigned)size)<MAP_LEFT+MAP_WIDTH?
        MAP_LEFT+(int)((EDITOR_W-e->scroll_x)*(unsigned)size):MAP_LEFT+MAP_WIDTH;
    const int map_bottom=MAP_TOP+(int)((EDITOR_H-e->scroll_y)*(unsigned)size)<MAP_BOTTOM?
        MAP_TOP+(int)((EDITOR_H-e->scroll_y)*(unsigned)size):MAP_BOTTOM;
    /* An opaque colour outside the game's palette keeps every boundary
       visible, including boundaries that cross bright terrain tiles. */
    SDL_SetRenderDrawBlendMode(e->renderer,SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(e->renderer,42,221,235,255);
    for(int x=MAP_LEFT;x<=map_right;x+=size)
        SDL_RenderDrawLine(e->renderer,x,MAP_TOP,x,map_bottom);
    for(int y=MAP_TOP;y<=map_bottom;y+=size)
        SDL_RenderDrawLine(e->renderer,MAP_LEFT,y,map_right,y);
}
static int screen_pixel(int logical,int viewport_offset,float scale){
    return (int)(((float)(logical+viewport_offset)*scale)+0.5f);
}
static void draw_fullscreen_labels(Editor *e){
    SDL_Rect viewport;
    float sx,sy;
    SDL_RenderGetViewport(e->renderer,&viewport);
    SDL_RenderGetScale(e->renderer,&sx,&sy);
    unsigned font_scale=(unsigned)(sx<sy?sx:sy);
    if(font_scale<1u)font_scale=1u;
    const int glyph_size=(int)(8u*font_scale);
    SDL_RenderSetClipRect(e->renderer,NULL);
    SDL_RenderSetLogicalSize(e->renderer,0,0);
    for(unsigned n=0;n<e->label_count;n++){
        const EditorLabel *queued=&e->labels[n];
        int x=screen_pixel(queued->x,viewport.x,sx);
        int y=screen_pixel(queued->y,viewport.y,sy);
        for(const char *s=queued->text;*s;s++,x+=glyph_size){
            unsigned ch=(unsigned char)*s;
            if(ch<0x14u||ch>=0x8cu)continue;
            SDL_Rect src={0,(int)(ch-0x14u)*8,8,8};
            SDL_Rect dst={x,y,glyph_size,glyph_size};
            SDL_RenderCopy(e->renderer,e->font_texture,&src,&dst);
        }
    }
    SDL_RenderSetLogicalSize(e->renderer,WIDTH,HEIGHT);
}
static void draw_fullscreen_grid(Editor *e,int size){
    SDL_Rect viewport;
    float scale_x,scale_y;
    SDL_RenderGetViewport(e->renderer,&viewport);
    SDL_RenderGetScale(e->renderer,&scale_x,&scale_y);
    const int logical_right=MAP_LEFT+(int)((EDITOR_W-e->scroll_x)*(unsigned)size)<MAP_LEFT+MAP_WIDTH?
        MAP_LEFT+(int)((EDITOR_W-e->scroll_x)*(unsigned)size):MAP_LEFT+MAP_WIDTH;
    const int logical_bottom=MAP_TOP+(int)((EDITOR_H-e->scroll_y)*(unsigned)size)<MAP_BOTTOM?
        MAP_TOP+(int)((EDITOR_H-e->scroll_y)*(unsigned)size):MAP_BOTTOM;
    const int left=screen_pixel(MAP_LEFT,viewport.x,scale_x);
    const int top=screen_pixel(MAP_TOP,viewport.y,scale_y);
    const int right=screen_pixel(logical_right,viewport.x,scale_x);
    const int bottom=screen_pixel(logical_bottom,viewport.y,scale_y);

    SDL_RenderSetClipRect(e->renderer,NULL);
    SDL_RenderSetLogicalSize(e->renderer,0,0);
    SDL_Rect clip={left,top,right-left,bottom-top};
    SDL_RenderSetClipRect(e->renderer,&clip);
    SDL_SetRenderDrawBlendMode(e->renderer,SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(e->renderer,42,221,235,255);
    for(int x=MAP_LEFT;x<=logical_right;x+=size){
        SDL_Rect line={screen_pixel(x,viewport.x,scale_x),top,1,bottom-top};
        SDL_RenderFillRect(e->renderer,&line);
    }
    for(int y=MAP_TOP;y<=logical_bottom;y+=size){
        SDL_Rect line={left,screen_pixel(y,viewport.y,scale_y),right-left,1};
        SDL_RenderFillRect(e->renderer,&line);
    }
    SDL_RenderSetClipRect(e->renderer,NULL);
    SDL_RenderSetLogicalSize(e->renderer,WIDTH,HEIGHT);
}
static void render(Editor *e){
    e->label_count=0;
    SDL_SetRenderDrawColor(e->renderer,17,19,24,255);SDL_RenderClear(e->renderer);
    rect(e,MAP_LEFT,MAP_TOP,MAP_WIDTH,MAP_BOTTOM-MAP_TOP,4,7,12);
    SDL_Rect map_clip={MAP_LEFT,MAP_TOP,MAP_WIDTH,MAP_BOTTOM-MAP_TOP};
    SDL_RenderSetClipRect(e->renderer,&map_clip);
    const int size=(int)tile_size(e);
    const unsigned cols=(MAP_WIDTH+(unsigned)size-1u)/(unsigned)size;
    const unsigned rows=((unsigned)(MAP_BOTTOM-MAP_TOP)+(unsigned)size-1u)/(unsigned)size;
    for(unsigned row=0;row<=rows&&e->scroll_y+row<EDITOR_H;row++)
        for(unsigned col=0;col<=cols&&e->scroll_x+col<EDITOR_W;col++){
            int x=MAP_LEFT+(int)col*size,y=MAP_TOP+(int)row*size;
            draw_tile(e,e->level.map[(e->scroll_y+row)*EDITOR_W+e->scroll_x+col],x,y,size);
        }
    if(e->grid){
        if(e->fullscreen)draw_fullscreen_grid(e,size);
        else draw_grid(e,size);
    }
    SDL_RenderSetClipRect(e->renderer,NULL);
    rect(e,PANEL_X-2,MAP_TOP,402,1,140,140,140);
    label(e,PANEL_X,18,"REFLOOD-EDITOR");
    label(e,PANEL_X,54,"TILE PALETTE");
    for(unsigned t=0;t<256;t++){
        int x=PANEL_X+(int)(t%16u)*PALETTE_STEP,y=PALETTE_TOP+(int)(t/16u)*PALETTE_STEP;
        draw_tile(e,t,x,y,PALETTE_SIZE);
        if(t==e->selected){
            SDL_Rect border={x-1,y-1,PALETTE_SIZE+2,PALETTE_SIZE+2};
            SDL_SetRenderDrawColor(e->renderer,255,255,255,255);SDL_RenderDrawRect(e->renderer,&border);
        }
    }
    char info[80];
    snprintf(info,sizeof(info),"TILE %03u  BANK %c",e->selected,'A'+(int)editor_level_bank(&e->level)-1);
    label(e,PANEL_X,495,info);
    snprintf(info,sizeof(info),"LEVEL %02u  TRIGGERS %02u",e->level.level,
        editor_level_trigger_count(&e->level));
    label(e,PANEL_X,513,info);
    snprintf(info,sizeof(info),"ZOOM %u PX   GRID %s",tile_size(e),e->grid?"ON":"OFF");
    label(e,PANEL_X,531,info);
    rect(e,1039,555,1,185,68,78,88);
    label(e,PANEL_X,556,"LEFT: PAINT / SELECT");
    label(e,PANEL_X,580,"RIGHT: PICK MAP TILE");
    label(e,PANEL_X,604,"ARROWS / WHEEL: PAN");
    label(e,PANEL_X,628,"+/-: ZOOM");
    label(e,PANEL_X,652,"CTRL+WHEEL: ZOOM");
    label(e,PANEL_X,676,"B: PALETTE BANK");
    label(e,PANEL_X,700,"T: TILE DESCRIPTION");
    label(e,1054,556,"CTRL+G: GRID ON / OFF");
    label(e,1054,580,"CTRL+S: SAVE");
    label(e,1054,604,"CTRL+L: LOAD HEADER");
    label(e,1054,628,"CTRL+F: FULLSCREEN");
    label(e,1054,652,"CTRL+Q: QUIT");
    label(e,1054,676,"CTRL+Z: UNDO");
    label(e,MAP_LEFT,18,e->message);
    if(e->dialog!=DIALOG_NONE){
        if(e->fullscreen){
            e->label_count=0; /* The modal covers underlying labels. */
            label(e,MAP_LEFT,18,e->message);
        }
        const bool browsing=e->dialog==DIALOG_BROWSER||e->dialog==DIALOG_SAVE||
            e->dialog==DIALOG_TILES;
        rect(e,72,browsing?55:285,976,browsing?650:180,42,47,59);
        rect(e,76,browsing?59:289,968,browsing?642:172,10,16,24);
        if(e->dialog==DIALOG_QUIT){
            label(e,110,318,"EXIT REFLOOD-EDITOR?");
            label(e,110,350,e->dirty?"UNSAVED CHANGES WILL BE LOST.":"THE EDITOR WILL CLOSE.");
            label(e,110,390,"Y = EXIT     N OR ESC = CANCEL");
        }else if(e->dialog==DIALOG_LOAD_WARNING){
            label(e,110,318,"LOAD ANOTHER LEVEL?");
            label(e,110,350,"UNSAVED CHANGES WILL BE LOST.");
            label(e,110,390,"Y = CONTINUE     N OR ESC = CANCEL");
        }else if(e->dialog==DIALOG_OVERWRITE){
            label(e,110,318,"OVERWRITE EXISTING LEVEL FILES?");
            label(e,110,350,"THE FOUR MATCHING FILES MAY BE REPLACED.");
            label(e,110,390,"Y = SAVE     N OR ESC = BACK");
        }else if(e->dialog==DIALOG_TILES){
            char heading[80];
            snprintf(heading,sizeof(heading),"TILES - BANK %c - %03u TO %03u / 255",
                'A'+(int)editor_level_bank(&e->level)-1,e->tile_help_offset,
                e->tile_help_offset+TILE_HELP_ROWS-1u);
            label(e,104,80,heading);
            label(e,110,105,"ID    DESCRIPTION");
            label(e,912,105,"GRAPHIC");
            for(unsigned row=0;row<TILE_HELP_ROWS;row++){
                unsigned tile=e->tile_help_offset+row;
                if(tile>=256u)break;
                int y=139+(int)row*23;
                if(tile==e->selected)rect(e,100,y-4,920,22,38,54,73);
                draw_tile(e,tile,930,y-4,20);
                char description[112],number[12];
                editor_tile_description(tile,e->attributes[tile],e->attributes_valid,
                    description,sizeof(description));
                snprintf(number,sizeof(number),"%03u :",tile);
                label(e,110,y,number);
                label(e,166,y,description);
            }
            rect(e,880,650,140,32,49,78,57);
            label(e,925,661,"CLOSE");
            label(e,104,633,"MARKER GRAPHICS ARE NOT RUNTIME SPRITES");
            label(e,104,664,"UP/DOWN PAGE HOME END WHEEL  CLICK: SELECT  T/ESC: CLOSE");
        }else if(e->dialog==DIALOG_SAVE){
            label(e,104,81,"SAVE LEVEL - FOUR MATCHING FILES");
            size_t n=strlen(e->browser.directory);
            label(e,104,108,n>112u?e->browser.directory+n-112u:e->browser.directory);
            rect(e,100,132,920,29,e->save_name_focus?43:25,e->save_name_focus?63:37,
                e->save_name_focus?82:49);
            label(e,110,142,"BASE NAME (NO EXTENSION):");
            label(e,346,142,e->save_name);
            label(e,110,190,"[..] PARENT DIRECTORY");
            for(unsigned row=0;row<16u&&e->browser.offset+row<e->browser.count;row++){
                unsigned index=e->browser.offset+row;
                const EditorBrowserEntry *item=&e->browser.entries[index];
                int y=232+(int)row*22;
                if(index==e->browser.selected&&!e->save_name_focus)
                    rect(e,100,y-4,920,20,43,63,82);
                char entry[272];
                snprintf(entry,sizeof(entry),"%s %.110s",item->directory?"[DIR]":"     ",item->name);
                label(e,110,y,entry);
            }
            if(!e->browser.count)label(e,110,232,"NO LEVEL FILES HERE - ENTER A NEW NAME");
            rect(e,885,620,135,30,49,78,57);
            label(e,925,631,"SAVE");
            label(e,104,632,"TAB: NAME / FILES   ENTER: SAVE / OPEN");
            label(e,104,666,"C: CUSTOM    D: DATA    ESC: CANCEL");
        }else if(browsing){
            label(e,104,81,"LOAD LEVEL HEADER");
            size_t n=strlen(e->browser.directory);
            label(e,104,108,n>112u?e->browser.directory+n-112u:e->browser.directory);
            label(e,110,152,"[..] PARENT DIRECTORY");
            for(unsigned row=0;row<17u&&e->browser.offset+row<e->browser.count;row++){
                unsigned index=e->browser.offset+row;
                const EditorBrowserEntry *item=&e->browser.entries[index];
                int y=180+(int)row*25;
                if(index==e->browser.selected)rect(e,100,y-4,920,21,43,63,82);
                char entry[272];
                snprintf(entry,sizeof(entry),"%s %.110s",item->directory?"[DIR]":"     ",item->name);
                label(e,110,y,entry);
            }
            if(!e->browser.count)label(e,110,200,"NO LEVEL HEADERS OR DIRECTORIES HERE");
            label(e,104,640,"ENTER / DOUBLE CLICK: OPEN    BACKSPACE / [..]: UP");
            label(e,104,664,"C: CUSTOM LEVELS    D: GAME DATA LEVELS    ESC: CANCEL");
        }
    }
    if(e->fullscreen)draw_fullscreen_labels(e);
    SDL_RenderPresent(e->renderer);
}
static void clamp_scroll(Editor *e){
    unsigned size=tile_size(e);
    unsigned cols=MAP_WIDTH/size,rows=(MAP_BOTTOM-MAP_TOP)/size;
    unsigned max_x=cols>=EDITOR_W?0u:EDITOR_W-cols;
    unsigned max_y=rows>=EDITOR_H?0u:EDITOR_H-rows;
    if(e->scroll_x>max_x)e->scroll_x=max_x;
    if(e->scroll_y>max_y)e->scroll_y=max_y;
}
static void change_zoom(Editor *e,int direction){
    unsigned previous=e->zoom;
    if(direction>0&&e->zoom+1u<ZOOM_STEPS)e->zoom++;
    else if(direction<0&&e->zoom)e->zoom--;
    if(previous==e->zoom)return;
    int focus_x=(int)e->scroll_x+MAP_WIDTH/2/(int)zoom_sizes[previous];
    int focus_y=(int)e->scroll_y+(MAP_BOTTOM-MAP_TOP)/2/(int)zoom_sizes[previous];
    int next_x=focus_x-MAP_WIDTH/2/(int)tile_size(e);
    int next_y=focus_y-(MAP_BOTTOM-MAP_TOP)/2/(int)tile_size(e);
    e->scroll_x=(unsigned)(next_x>0?next_x:0);
    e->scroll_y=(unsigned)(next_y>0?next_y:0);
    clamp_scroll(e);
}
static void paint(Editor *e,int mouse_x,int mouse_y,bool pick){
    int size=(int)tile_size(e);
    if(mouse_x>=PANEL_X&&mouse_x<PANEL_X+16*PALETTE_STEP&&
       mouse_y>=PALETTE_TOP&&mouse_y<PALETTE_TOP+16*PALETTE_STEP){
        unsigned col=(unsigned)(mouse_x-PANEL_X)/PALETTE_STEP;
        unsigned row=(unsigned)(mouse_y-PALETTE_TOP)/PALETTE_STEP;
        if((unsigned)(mouse_x-PANEL_X)%PALETTE_STEP<PALETTE_SIZE&&
           (unsigned)(mouse_y-PALETTE_TOP)%PALETTE_STEP<PALETTE_SIZE&&!pick)
            e->selected=row*16u+col;
        return;
    }
    if(mouse_x<MAP_LEFT||mouse_x>=MAP_LEFT+MAP_WIDTH||mouse_y<MAP_TOP||mouse_y>=MAP_BOTTOM)return;
    unsigned x=e->scroll_x+(unsigned)(mouse_x-MAP_LEFT)/(unsigned)size;
    unsigned y=e->scroll_y+(unsigned)(mouse_y-MAP_TOP)/(unsigned)size;
    if(x>=EDITOR_W||y>=EDITOR_H)return;
    unsigned index=y*EDITOR_W+x;
    if(pick){e->selected=e->level.map[index];return;}
    if(!e->painting){memcpy(e->undo,e->level.map,sizeof(e->undo));e->undo_available=true;e->painting=true;}
    if(e->selected==22)for(unsigned i=0;i<EDITOR_MAP_BYTES;i++)
        if(e->level.map[i]==22)e->level.map[i]=0;
    e->level.map[index]=(uint8_t)e->selected;e->dirty=true;
}
static void browser_open(Editor *e,const char *directory){
    char error[160];
    if(!editor_browser_open(&e->browser,directory,error,sizeof(error)))
        snprintf(e->message,sizeof(e->message),"BROWSER: %.148s",error);
}
static void begin_load(Editor *e){
    char directory[EDITOR_BROWSER_PATH],error[160];
    if(e->browser.directory[0]&&editor_browser_open(&e->browser,e->browser.directory,error,sizeof(error))){}
    else {
        snprintf(directory,sizeof(directory),"%s/levels",e->output);
        if(!editor_browser_open(&e->browser,directory,error,sizeof(error))){
            snprintf(directory,sizeof(directory),"%s/levels",e->input?e->input:e->data);
            if(!editor_browser_open(&e->browser,directory,error,sizeof(error)))
                editor_browser_open(&e->browser,".",error,sizeof(error));
        }
    }
    e->dialog=DIALOG_BROWSER;
}
static void begin_save(Editor *e){
    char directory[EDITOR_BROWSER_PATH],error[160];
    if(!e->save_name[0])snprintf(e->save_name,sizeof(e->save_name),"level_%02u",e->level.level);
    if(!editor_level_ensure_output_directory(e->output,error,sizeof(error))){
        snprintf(e->message,sizeof(e->message),"SAVE DIALOG: %.140s",error);return;
    }
    snprintf(directory,sizeof(directory),"%s/levels",e->output);
    if(!(e->save_directory[0]&&editor_browser_open(&e->browser,e->save_directory,error,sizeof(error)))&&
       !editor_browser_open(&e->browser,directory,error,sizeof(error))){
        snprintf(e->message,sizeof(e->message),"SAVE DIALOG: %.140s",error);return;
    }
    e->save_name_focus=true;
    e->dialog=DIALOG_SAVE;
    SDL_StartTextInput();
}
static void save_write(Editor *e){
    char error[160];
    if(!editor_level_save_named(&e->level,e->browser.directory,e->save_name,error,sizeof(error))){
        snprintf(e->message,sizeof(e->message),"SAVE FAILED: %.140s",error);
        e->dialog=DIALOG_SAVE;return;
    }
    snprintf(e->save_directory,sizeof(e->save_directory),"%s",e->browser.directory);
    snprintf(e->message,sizeof(e->message),"SAVED %.63s IN %.80s",e->save_name,e->browser.directory);
    e->dirty=false;e->dialog=DIALOG_NONE;
    SDL_StopTextInput();
}
static void save_submit(Editor *e){
    if(!e->save_name[0]){
        snprintf(e->message,sizeof(e->message),"SAVE FAILED: ENTER A BASE NAME");return;
    }
    if(editor_level_named_exists(e->browser.directory,e->save_name)){
        e->dialog=DIALOG_OVERWRITE;SDL_StopTextInput();
    }else save_write(e);
}
static void save_browser_activate(Editor *e){
    char path[EDITOR_BROWSER_PATH]={0},error[160];
    if(!editor_browser_activate(&e->browser,path,sizeof(path),error,sizeof(error))){
        snprintf(e->message,sizeof(e->message),"BROWSER: %.148s",error);return;
    }
    if(path[0]){
        const char *name=strrchr(path,'/');
        const char *backslash=strrchr(path,'\\');
        if(backslash&&(!name||backslash>name))name=backslash;
        name=name?name+1:path;
        size_t n=strlen(name);
        if(n>11u&&n-11u<sizeof(e->save_name)){
            memcpy(e->save_name,name,n-11u);e->save_name[n-11u]='\0';
            e->save_name_focus=true;
        }
    }
}
static void load_header(Editor *e,const char *path){
    EditorLevel next;
    char error[160];
    if(!editor_level_load_header(&next,path,error,sizeof(error))){
        snprintf(e->message,sizeof(e->message),"LOAD FAILED: %.140s",error);
        return;
    }
    EditorLevel old=e->level;e->level=next;
    if(!load_graphics(e)){
        e->level=old;
        snprintf(e->message,sizeof(e->message),"LOAD FAILED: BLOCK GRAPHICS MISSING");
        return;
    }
    e->dirty=false;e->undo_available=false;e->painting=false;
    e->scroll_x=e->scroll_y=0;
    const char *name=strrchr(path,'/');
    const char *backslash=strrchr(path,'\\');
    if(backslash&&(!name||backslash>name))name=backslash;
    name=name?name+1:path;
    size_t length=strlen(name);
    if(length>11u&&length-11u<sizeof(e->save_name)){
        memcpy(e->save_name,name,length-11u);e->save_name[length-11u]='\0';
    }
    snprintf(e->message,sizeof(e->message),"LOADED %.63s",e->save_name);
    e->dialog=DIALOG_NONE;
}
static void browser_activate(Editor *e){
    char path[EDITOR_BROWSER_PATH]={0},error[160];
    if(!editor_browser_activate(&e->browser,path,sizeof(path),error,sizeof(error))){
        snprintf(e->message,sizeof(e->message),"BROWSER: %.148s",error);
        return;
    }
    if(path[0])load_header(e,path);
}
static void browser_scroll(Editor *e,int delta){
    unsigned *offset=&e->browser.offset;
    if(delta>0)*offset=*offset>(unsigned)delta?*offset-(unsigned)delta:0;
    else *offset+=(unsigned)(-delta);
    if(*offset>=e->browser.count)*offset=e->browser.count?e->browser.count-1u:0u;
    e->browser.selected=*offset;
}
static void save_text(Editor *e,const char *text){
    size_t n=strlen(e->save_name);
    for(;*text&&n<sizeof(e->save_name)-1u;text++){
        unsigned char c=(unsigned char)*text;
        if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
           (c>='0'&&c<='9')||c=='_'||c=='-')e->save_name[n++]=(char)c;
    }
    e->save_name[n]='\0';
}
static void scroll_tile_help(Editor *e,int delta){
    int next=(int)e->tile_help_offset+delta;
    if(next<0)next=0;
    if(next>256-(int)TILE_HELP_ROWS)next=256-(int)TILE_HELP_ROWS;
    e->tile_help_offset=(unsigned)next;
}
static void grab_editor_keyboard(Editor *e,bool focused){
#if SDL_VERSION_ATLEAST(2,0,16)
    /* Keep XFCE window-manager shortcuts from consuming editor shortcuts. */
    SDL_SetWindowKeyboardGrab(e->window,focused?SDL_TRUE:SDL_FALSE);
#else
    (void)e;(void)focused;
#endif
}
static bool key(Editor *e,SDL_Keycode k,SDL_Keymod mod){
    if((mod&KMOD_CTRL)&&k==SDLK_f){
        bool new_mode=!e->fullscreen;
        if(SDL_SetWindowFullscreen(e->window,new_mode?SDL_WINDOW_FULLSCREEN_DESKTOP:0)==0){
            e->fullscreen=new_mode;
            if(!new_mode)SDL_SetWindowPosition(e->window,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED);
        }else snprintf(e->message,sizeof(e->message),"FULLSCREEN FAILED: %.130s",SDL_GetError());
        return true;
    }
    if(e->dialog==DIALOG_QUIT||e->dialog==DIALOG_LOAD_WARNING){
        if(k==SDLK_y){
            if(e->dialog==DIALOG_QUIT)return false;
            begin_load(e);
        }else if(k==SDLK_n||k==SDLK_ESCAPE)e->dialog=DIALOG_NONE;
        return true;
    }
    if(e->dialog==DIALOG_OVERWRITE){
        if(k==SDLK_y)save_write(e);
        else if(k==SDLK_n||k==SDLK_ESCAPE){
            e->dialog=DIALOG_SAVE;SDL_StartTextInput();
        }
        return true;
    }
    if(e->dialog==DIALOG_TILES){
        if(k==SDLK_t||k==SDLK_ESCAPE)e->dialog=DIALOG_NONE;
        else if(k==SDLK_UP)scroll_tile_help(e,-1);
        else if(k==SDLK_DOWN)scroll_tile_help(e,1);
        else if(k==SDLK_PAGEUP)scroll_tile_help(e,-(int)TILE_HELP_ROWS);
        else if(k==SDLK_PAGEDOWN)scroll_tile_help(e,(int)TILE_HELP_ROWS);
        else if(k==SDLK_HOME)e->tile_help_offset=0;
        else if(k==SDLK_END)e->tile_help_offset=256u-TILE_HELP_ROWS;
        return true;
    }
    if(e->dialog==DIALOG_SAVE){
        if(k==SDLK_ESCAPE){e->dialog=DIALOG_NONE;SDL_StopTextInput();}
        else if(k==SDLK_TAB)e->save_name_focus=!e->save_name_focus;
        else if((mod&KMOD_CTRL)&&k==SDLK_s)save_submit(e);
        else if(e->save_name_focus){
            if((mod&KMOD_CTRL)&&k==SDLK_a)e->save_name[0]='\0';
            else if(k==SDLK_BACKSPACE){
                size_t n=strlen(e->save_name);if(n)e->save_name[n-1]='\0';
            }else if(k==SDLK_RETURN||k==SDLK_KP_ENTER)save_submit(e);
            else if((mod&KMOD_CTRL)&&k==SDLK_v){
                char *clip=SDL_GetClipboardText();
                if(clip){save_text(e,clip);SDL_free(clip);}
            }
        }else{
            if(k==SDLK_BACKSPACE){char error[160];
                if(!editor_browser_parent(&e->browser,error,sizeof(error)))
                    snprintf(e->message,sizeof(e->message),"BROWSER: %.148s",error);
            }else if(k==SDLK_UP&&e->browser.selected)e->browser.selected--;
            else if(k==SDLK_DOWN&&e->browser.selected+1u<e->browser.count)e->browser.selected++;
            else if(k==SDLK_PAGEUP)browser_scroll(e,16);
            else if(k==SDLK_PAGEDOWN)browser_scroll(e,-16);
            else if(k==SDLK_RETURN||k==SDLK_KP_ENTER)save_browser_activate(e);
            else if(k==SDLK_c){char directory[EDITOR_BROWSER_PATH];
                snprintf(directory,sizeof(directory),"%s/levels",e->output);browser_open(e,directory);
            }else if(k==SDLK_d){char directory[EDITOR_BROWSER_PATH];
                snprintf(directory,sizeof(directory),"%s/levels",e->data);browser_open(e,directory);
            }
            if(e->browser.selected<e->browser.offset)e->browser.offset=e->browser.selected;
            if(e->browser.selected>=e->browser.offset+16u)e->browser.offset=e->browser.selected-15u;
        }
        return true;
    }
    if(e->dialog==DIALOG_BROWSER){
        if(k==SDLK_ESCAPE)e->dialog=DIALOG_NONE;
        else if(k==SDLK_BACKSPACE){char error[160];
            if(!editor_browser_parent(&e->browser,error,sizeof(error)))
                snprintf(e->message,sizeof(e->message),"BROWSER: %.148s",error);
        }else if(k==SDLK_UP&&e->browser.selected)e->browser.selected--;
        else if(k==SDLK_DOWN&&e->browser.selected+1u<e->browser.count)e->browser.selected++;
        else if(k==SDLK_PAGEUP)browser_scroll(e,17);
        else if(k==SDLK_PAGEDOWN)browser_scroll(e,-17);
        else if(k==SDLK_RETURN||k==SDLK_KP_ENTER)browser_activate(e);
        else if(k==SDLK_c){char path[EDITOR_BROWSER_PATH];
            snprintf(path,sizeof(path),"%s/levels",e->output);browser_open(e,path);
        }else if(k==SDLK_d){char path[EDITOR_BROWSER_PATH];
            snprintf(path,sizeof(path),"%s/levels",e->data);browser_open(e,path);
        }
        if(e->browser.selected<e->browser.offset)e->browser.offset=e->browser.selected;
        if(e->browser.selected>=e->browser.offset+17u)e->browser.offset=e->browser.selected-16u;
        return true;
    }
    if((mod&KMOD_CTRL)&&k==SDLK_q){e->dialog=DIALOG_QUIT;return true;}
    if(k==SDLK_t&&!(mod&KMOD_CTRL)){
        e->tile_help_offset=0;e->dialog=DIALOG_TILES;return true;
    }
    if((mod&KMOD_CTRL)&&k==SDLK_g){e->grid=!e->grid;return true;}
    if((mod&KMOD_CTRL)&&k==SDLK_l){
        if(e->dirty)e->dialog=DIALOG_LOAD_WARNING;
        else begin_load(e);
        return true;
    }
    if(k==SDLK_ESCAPE){e->dialog=DIALOG_QUIT;return true;}
    if((mod&KMOD_CTRL)&&k==SDLK_s)begin_save(e);
    else if((mod&KMOD_CTRL)&&k==SDLK_z&&e->undo_available){
        uint8_t swap[EDITOR_MAP_BYTES];memcpy(swap,e->level.map,sizeof(swap));
        memcpy(e->level.map,e->undo,sizeof(e->undo));memcpy(e->undo,swap,sizeof(swap));e->dirty=true;
    }else if(k==SDLK_LEFT&&e->scroll_x)e->scroll_x--;
    else if(k==SDLK_UP&&e->scroll_y)e->scroll_y--;
    else if(k==SDLK_RIGHT)e->scroll_x++;
    else if(k==SDLK_DOWN)e->scroll_y++;
    else if(k==SDLK_EQUALS||k==SDLK_PLUS||k==SDLK_KP_PLUS)change_zoom(e,1);
    else if(k==SDLK_MINUS||k==SDLK_KP_MINUS)change_zoom(e,-1);
    else if(k==SDLK_b){
        unsigned previous=editor_level_bank(&e->level);
        unsigned bank=previous%3u+1u;
        e->level.header[0]=0;e->level.header[1]=(uint8_t)bank;
        if(!load_graphics(e)){
            e->level.header[0]=0;e->level.header[1]=(uint8_t)previous;
            snprintf(e->message,sizeof(e->message),"BANK GRAPHICS MISSING");
        }
        else e->dirty=true;
    }
    clamp_scroll(e);return true;
}
int main(int argc,char **argv){
    Editor e;memset(&e,0,sizeof(e));e.zoom=6;e.data="data";e.output="custom";
    unsigned number=1;bool blank,explicit_new=false;const char *input=NULL;
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"--data")&&i+1<argc)e.data=argv[++i];
        else if(!strcmp(argv[i],"--input")&&i+1<argc)input=argv[++i];
        else if(!strcmp(argv[i],"--output")&&i+1<argc)e.output=argv[++i];
        else if(!strcmp(argv[i],"--level")&&i+1<argc)number=(unsigned)strtoul(argv[++i],NULL,10);
        else if(!strcmp(argv[i],"--new"))explicit_new=true;
        else {fprintf(stderr,"Usage: reflood-editor [--new] [--level 1..99] [--data DIR] [--input DIR] [--output DIR]\n");return 2;}
    }
    blank=explicit_new||!input;
    if(number<1||number>99){fprintf(stderr,"Level number must be 1..99\n");return 2;}
    char error[160];
    if(blank)editor_level_new(&e.level,number);
    else if(!editor_level_load(&e.level,input?input:e.data,number,error,sizeof(error))){
        fprintf(stderr,"Cannot import level: %s (use --new for a blank level)\n",error);return 1;
    }
    if(SDL_Init(SDL_INIT_VIDEO)!=0){fprintf(stderr,"SDL: %s\n",SDL_GetError());return 1;}
    e.window=SDL_CreateWindow("ReFlood-Editor",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
        WIDTH,HEIGHT,SDL_WINDOW_SHOWN);
    e.renderer=e.window?SDL_CreateRenderer(e.window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC):NULL;
    if(!e.renderer&&e.window)e.renderer=SDL_CreateRenderer(e.window,-1,SDL_RENDERER_SOFTWARE);
    if(e.renderer)SDL_RenderSetLogicalSize(e.renderer,WIDTH,HEIGHT);
    if(!e.renderer||!load_graphics(&e)){
        fprintf(stderr,"Editor needs extracted BLOCK graphics and LEVEL font in %s: %s\n",e.data,SDL_GetError());
        if(e.renderer)SDL_DestroyRenderer(e.renderer);
        if(e.window)SDL_DestroyWindow(e.window);
        SDL_Quit();return 1;
    }
    e.dirty=blank;
    e.input=input;
    snprintf(e.save_name,sizeof(e.save_name),"level_%02u",number);
    snprintf(e.message,sizeof(e.message),"LEVEL %02u",number);
    grab_editor_keyboard(&e,(SDL_GetWindowFlags(e.window)&SDL_WINDOW_INPUT_FOCUS)!=0);
    bool running=true;
    while(running){
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            if(event.type==SDL_QUIT)e.dialog=DIALOG_QUIT;
            else if(event.type==SDL_WINDOWEVENT&&event.window.windowID==SDL_GetWindowID(e.window)){
                if(event.window.event==SDL_WINDOWEVENT_FOCUS_GAINED)grab_editor_keyboard(&e,true);
                else if(event.window.event==SDL_WINDOWEVENT_FOCUS_LOST)grab_editor_keyboard(&e,false);
            }
            else if(event.type==SDL_KEYDOWN)running=key(&e,event.key.keysym.sym,event.key.keysym.mod);
            else if(e.dialog==DIALOG_SAVE&&event.type==SDL_TEXTINPUT&&e.save_name_focus&&
                    !(SDL_GetModState()&KMOD_CTRL))save_text(&e,event.text.text);
            else if(e.dialog==DIALOG_TILES&&event.type==SDL_MOUSEWHEEL)
                scroll_tile_help(&e,-event.wheel.y*3);
            else if(e.dialog==DIALOG_TILES&&event.type==SDL_MOUSEBUTTONDOWN&&
                    event.button.button==SDL_BUTTON_LEFT){
                int x=event.button.x,y=event.button.y;
                if(x>=880&&x<1020&&y>=650&&y<682)e.dialog=DIALOG_NONE;
                else if(x>=100&&x<1020&&y>=135&&y<135+(int)TILE_HELP_ROWS*23){
                    unsigned tile=e.tile_help_offset+(unsigned)(y-135)/23u;
                    if(tile<256u){e.selected=tile;e.dialog=DIALOG_NONE;}
                }
            }
            else if(e.dialog==DIALOG_SAVE&&event.type==SDL_MOUSEBUTTONDOWN&&
                    event.button.button==SDL_BUTTON_LEFT){
                int x=event.button.x,y=event.button.y;
                if(x>=100&&x<1020&&y>=132&&y<162)e.save_name_focus=true;
                else if(x>=100&&x<1020&&y>=184&&y<211){
                    char error[160];
                    if(!editor_browser_parent(&e.browser,error,sizeof(error)))
                        snprintf(e.message,sizeof(e.message),"BROWSER: %.148s",error);
                    e.save_name_focus=false;
                }else if(x>=885&&x<1020&&y>=620&&y<650)save_submit(&e);
                else if(x>=100&&x<1020&&y>=228&&y<580){
                    unsigned index=e.browser.offset+(unsigned)(y-228)/22u;
                    if(index<e.browser.count){
                        e.browser.selected=index;e.save_name_focus=false;
                        if(event.button.clicks>=2u)save_browser_activate(&e);
                    }
                }
            }else if(e.dialog==DIALOG_SAVE&&event.type==SDL_MOUSEWHEEL){
                browser_scroll(&e,-event.wheel.y*3);e.save_name_focus=false;
            }
            else if(e.dialog==DIALOG_BROWSER&&event.type==SDL_MOUSEBUTTONDOWN&&
                    event.button.button==SDL_BUTTON_LEFT){
                int x=event.button.x,y=event.button.y;
                if(x>=100&&x<1020&&y>=146&&y<170){
                    char error[160];
                    if(!editor_browser_parent(&e.browser,error,sizeof(error)))
                        snprintf(e.message,sizeof(e.message),"BROWSER: %.148s",error);
                }else if(x>=100&&x<1020&&y>=176&&y<605){
                    unsigned index=e.browser.offset+(unsigned)(y-176)/25u;
                    if(index<e.browser.count){
                        e.browser.selected=index;
                        if(event.button.clicks>=2u)browser_activate(&e);
                    }
                }
            }else if(e.dialog==DIALOG_BROWSER&&event.type==SDL_MOUSEWHEEL)
                browser_scroll(&e,-event.wheel.y*3);
            else if(e.dialog!=DIALOG_NONE)continue;
            else if(event.type==SDL_MOUSEBUTTONDOWN){
                if(event.button.button==SDL_BUTTON_LEFT||event.button.button==SDL_BUTTON_RIGHT)
                    paint(&e,event.button.x,event.button.y,event.button.button==SDL_BUTTON_RIGHT);
            }else if(event.type==SDL_MOUSEMOTION&&(event.motion.state&SDL_BUTTON_LMASK))
                paint(&e,event.motion.x,event.motion.y,false);
            else if(event.type==SDL_MOUSEBUTTONUP&&event.button.button==SDL_BUTTON_LEFT)e.painting=false;
            else if(event.type==SDL_MOUSEWHEEL){
                int delta=event.wheel.y;
                if(SDL_GetModState()&KMOD_CTRL){
                    if(delta>0)change_zoom(&e,1);
                    else if(delta<0)change_zoom(&e,-1);
                    continue;
                }
                unsigned *position=(SDL_GetModState()&KMOD_SHIFT)?&e.scroll_x:&e.scroll_y;
                if(delta>0)*position=*position>(unsigned)delta?*position-(unsigned)delta:0;
                else *position+=(unsigned)(-delta);
                clamp_scroll(&e);
            }
        }
        render(&e);SDL_Delay(10);
    }
    grab_editor_keyboard(&e,false);
    SDL_DestroyTexture(e.font_texture);SDL_DestroyTexture(e.tiles);
    SDL_DestroyRenderer(e.renderer);SDL_DestroyWindow(e.window);
    SDL_Quit();return 0;
}
