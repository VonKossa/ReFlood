#include "flood_game.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc,char **argv){
    if(argc!=4){fprintf(stderr,"usage: %s DATA_DIR FRAME OUTPUT.ppm\n",argv[0]);return 2;}
    unsigned target=(unsigned)strtoul(argv[2],NULL,10);FloodIntro intro;char err[160];
    if(!flood_intro_load(&intro,argv[1],err,sizeof(err))){fprintf(stderr,"%s\n",err);return 2;}
    while(intro.frame<target&&!intro.finished)flood_intro_tick(&intro,false);
    FILE *f=fopen(argv[3],"wb");if(!f)return 3;
    fprintf(f,"P6\n%u %u\n255\n",FLOOD_INTRO_W,FLOOD_INTRO_H);
    for(unsigned y=0;y<FLOOD_INTRO_H;y++)for(unsigned x=0;x<FLOOD_INTRO_W;x++){
        const uint16_t c=intro.palette[flood_intro_pixel(&intro,x,y)];
        fputc(((c>>8)&15u)*17u,f);fputc(((c>>4)&15u)*17u,f);fputc((c&15u)*17u,f);
    }
    fclose(f);return 0;
}
