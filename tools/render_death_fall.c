#include "flood_game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const unsigned char rgb[16][3]={
    {0,0,0},{51,34,51},{85,68,136},{102,136,153},
    {102,17,0},{119,51,0},{187,102,0},{204,153,34},
    {51,68,17},{68,85,17},{85,102,0},{102,119,0},
    {34,51,102},{51,102,119},{136,153,170},{204,187,153}
};

static void sprite(unsigned char *back,const FloodGame *g,unsigned id,int x,int y){
    const FloodSprite *s=flood_sprite(g,id);if(!s)return;
    for(unsigned sy=0;sy<s->height;sy++)for(unsigned sx=0;sx<s->width;sx++){
        unsigned pi=sy*FLOOD_MAX_SPRITE_W+sx;if(!s->mask[pi])continue;
        int dx=x+(int)sx+FLOOD_BACKING_X,dy=y+(int)sy;
        if(dx>=0&&dx<FLOOD_BACKING_W&&dy>=0&&dy<FLOOD_VIEW_H)
            back[dy*FLOOD_BACKING_W+dx]=(unsigned char)(s->pixels[pi]&15u);
    }
}

static int save(const char *path,FloodGame *g){
    unsigned char back[FLOOD_BACKING_W*FLOOD_VIEW_H];
    flood_build_tile_backing(g,back);sprite(back,g,flood_player_sprite_id(g),g->player.x,g->player.y);
    FILE *f=fopen(path,"wb");if(!f)return 0;
    fprintf(f,"P6\n%u %u\n255\n",FLOOD_VIEW_W,FLOOD_VIEW_H);
    for(unsigned y=0;y<FLOOD_VIEW_H;y++)for(unsigned x=0;x<FLOOD_VIEW_W;x++)
        fwrite(rgb[back[y*FLOOD_BACKING_W+x+FLOOD_BACKING_X]&15u],1,3,f);
    fclose(f);return 1;
}

int main(int argc,char **argv){
    if(argc!=3){fprintf(stderr,"usage: %s DATA_DIR OUTPUT_PREFIX\n",argv[0]);return 2;}
    FloodGame g;flood_game_init(&g);char err[160];
    if(!flood_game_load_level(&g,argv[1],1u,err,sizeof(err))){fprintf(stderr,"%s\n",err);return 2;}
    memset(g.world.terrain,0,sizeof(g.world.terrain));
    memset(g.world.render_terrain,0,sizeof(g.world.render_terrain));g.world.render_terrain_valid=true;
    g.world.attr[7]=FLOOD_ATTR_SOLID;
    for(unsigned y=1;y<8;y++)g.world.terrain[y*FLOOD_MAP_W+7]=7u;
    for(unsigned x=4;x<10;x++)g.world.terrain[8*FLOOD_MAP_W+x]=7u;
    memcpy(g.world.render_terrain,g.world.terrain,sizeof(g.world.terrain));
    g.player.x=80;g.player.y=24;g.player.dx=0;g.player.dy=0;
    g.player.life_force=0;g.player.air=63;g.player.invulnerable_timer=0;
    FloodInput in={0};in.x=1;in.y=-1;
    static const unsigned target[4]={0u,4u,8u,20u};unsigned frame=0u;
    for(unsigned n=0;n<4;n++){
        while(frame<target[n]&&g.player.death_mode!=2u){flood_game_tick(&g,in);frame++;}
        char path[512];snprintf(path,sizeof(path),"%s%u.ppm",argv[2],n);
        if(!save(path,&g))return 3;
    }
    return 0;
}
