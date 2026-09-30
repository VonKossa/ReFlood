#include "flood_game.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void le16(FILE *f,uint16_t v){fputc(v&255u,f);fputc(v>>8,f);}
static void le32(FILE *f,uint32_t v){le16(f,(uint16_t)v);le16(f,(uint16_t)(v>>16));}

int main(int argc,char **argv){
    if(argc!=3){fprintf(stderr,"usage: %s DATA_DIR OUTPUT.wav\n",argv[0]);return 2;}
    const uint32_t rate=44100u,frames=rate*4u,bytes=frames*4u;
    FloodAudio audio;char err[160];
    if(!flood_audio_load(&audio,argv[1],rate,err,sizeof(err))){fprintf(stderr,"%s\n",err);return 2;}
    FILE *f=fopen(argv[2],"wb");if(!f)return 3;
    fwrite("RIFF",1,4,f);le32(f,36u+bytes);fwrite("WAVEfmt ",1,8,f);le32(f,16u);
    le16(f,1u);le16(f,2u);le32(f,rate);le32(f,rate*4u);le16(f,4u);le16(f,16u);
    fwrite("data",1,4,f);le32(f,bytes);flood_audio_trigger(&audio,24u);
    int16_t buffer[1024u*2u];uint32_t done=0u;
    while(done<frames){uint32_t count=frames-done;if(count>1024u)count=1024u;
        flood_audio_mix(&audio,buffer,count);fwrite(buffer,sizeof(*buffer)*2u,count,f);done+=count;}
    fclose(f);return 0;
}
