#include "flood_game.h"
#include <stdio.h>
#include <string.h>

typedef struct { uint16_t dest,source; uint8_t width,height,hold; } EndingEvent;

/* Events 39-74 are the literal parameters passed to $163A4 by $15AD2.
   The first 39 are the preceding regular climbing loop. */
static const EndingEvent tail_events[] = {
    {0x6f6,0x010,2,32,4},{0x6f6,0x014,2,32,4},
    {0x596,0x018,2,32,4},{0x597,0x01c,2,32,4},
    {0x437,0x020,2,32,4},{0x437,0x17cc,2,32,0},
    {0x179,0x024,2,32,4},{0x179,0x500,2,32,10},
    {0x000,0xb40,2,40,3},{0x000,0xb44,2,40,3},
    {0x000,0x1180,3,40,3},{0x000,0x17c0,4,40,3},
    {0x000,0x1e00,5,40,3},{0x000,0x2440,6,40,3},
    {0x000,0x2a80,7,40,3},{0x000,0x3700,8,40,3},
    {0x000,0x3d40,9,40,3},{0x002,0x4380,9,40,3},
    {0x004,0x49c0,9,40,3},{0x006,0xb52,9,40,3},
    {0x008,0x1192,11,40,3},{0x00a,0x17d2,10,40,3},
    {0x00c,0x1e12,9,40,3},{0x00e,0x2452,9,40,3},
    {0x010,0x2a92,11,40,3},{0x012,0x30d2,10,40,3},
    {0x014,0x3712,9,40,3},{0x014,0x3d52,10,40,3},
    {0x014,0x4392,10,40,3},{0x01a,0x49d2,7,40,3},
    {0x01c,0x504,6,40,3},{0x01e,0x510,5,40,3},
    {0x020,0x51a,4,40,3},{0x022,0x522,3,40,3},
    {0x024,0xb4c,2,40,3},{0x024,0x118c,2,40,62}
};

static EndingEvent event_at(unsigned index){
    EndingEvent e;
    if(index>=39u)return tail_events[index-39u];
    e.dest=(uint16_t)(0x21c6u-index*0xb0u);
    e.source=(uint16_t)((0x0cu+index*4u)&0x0cu);
    e.width=2u;e.height=(uint8_t)(4u+index*4u);
    if(e.height>32u)e.height=32u;
    e.hold=4u;return e;
}

static int read_file(const char *path,uint8_t *out,size_t size){
    FILE *f=fopen(path,"rb");size_t got;
    if(!f)return 0;
    got=fread(out,1,size,f);fclose(f);return got==size;
}

bool flood_ending_load(FloodEnding *ending,const char *data_dir,char *err,size_t errcap){
    char path[512];uint8_t palette_bytes[32];
    memset(ending,0,sizeof(*ending));
    snprintf(path,sizeof(path),"%s/ending/END_SCRE.bin",data_dir);
    if(!read_file(path,ending->source,sizeof(ending->source))){
        snprintf(err,errcap,"cannot read exact ending asset %s",path);return false;
    }
    snprintf(path,sizeof(path),"%s/ending/ENDING_palette.bin",data_dir);
    if(!read_file(path,palette_bytes,sizeof(palette_bytes))){
        snprintf(err,errcap,"cannot read ending palette %s",path);return false;
    }
    for(unsigned n=0;n<16u;n++)ending->palette[n]=(uint16_t)((palette_bytes[n*2u]<<8)|palette_bytes[n*2u+1u]);
    memcpy(ending->screen,ending->source,FLOOD_ENDING_SCREEN_BYTES);
    ending->hold=1u;ending->loaded=true;return true;
}

static void apply_event(FloodEnding *ending,EndingEvent e){
    const uint8_t *atlas=ending->source+FLOOD_ENDING_SCREEN_BYTES;
    for(unsigned plane=0;plane<4u;plane++)for(unsigned row=0;row<e.height;row++){
        const size_t from=plane*0x5000u+e.source+row*40u;
        const size_t to=plane*FLOOD_ENDING_PLANE_BYTES+e.dest+row*FLOOD_ENDING_ROW_BYTES;
        memcpy(ending->screen+to,atlas+from,(size_t)e.width*2u);
    }
}

void flood_ending_tick(FloodEnding *ending){
    if(!ending->loaded||ending->finished)return;
    ending->frames++;
    if(ending->hold){
        ending->hold--;
        if(!ending->hold&&ending->event_index==FLOOD_ENDING_EVENT_COUNT)ending->finished=true;
        return;
    }
    while(ending->event_index<FLOOD_ENDING_EVENT_COUNT){
        EndingEvent e=event_at(ending->event_index++);apply_event(ending,e);ending->hold=e.hold;
        if(ending->hold){
            ending->hold--;
            if(!ending->hold&&ending->event_index==FLOOD_ENDING_EVENT_COUNT)ending->finished=true;
            return;
        }
    }
    ending->finished=true;
}

uint8_t flood_ending_pixel(const FloodEnding *ending,unsigned x,unsigned y){
    uint8_t colour=0;
    /* $EE9A-$EEA0 installs DIWSTRT=$2C81 and DIWSTOP=$FCC1: the ending
       exposes 208 rows.  END_SCRE retains 240 backing rows for its blits,
       including off-window working pixels below the display. */
    if(x>=FLOOD_ENDING_W||y>=FLOOD_ENDING_H)return 0;
    const size_t byte=y*FLOOD_ENDING_ROW_BYTES+x/8u;const uint8_t mask=(uint8_t)(0x80u>>(x&7u));
    for(unsigned plane=0;plane<4u;plane++)if(ending->screen[plane*FLOOD_ENDING_PLANE_BYTES+byte]&mask)
        colour|=(uint8_t)(1u<<plane);
    return colour;
}
