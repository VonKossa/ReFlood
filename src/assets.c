#include "flood_game.h"
#include <stdio.h>
#include <string.h>

static int read_file_exact(const char *path, void *dst, size_t n){
    FILE *f=fopen(path,"rb"); if(!f) return 0;
    size_t got=fread(dst,1,n,f); int extra=fgetc(f); fclose(f);
    return got==n && extra==EOF;
}
static uint16_t be16(const uint8_t *p){return (uint16_t)((p[0]<<8)|p[1]);}
static void seterr(char *err,size_t cap,const char *msg){if(err&&cap){snprintf(err,cap,"%s",msg);}}

static void decode_tiles(FloodWorld *w,const uint8_t raw[32768]){
    for(unsigned t=0;t<256;t++){
        const uint8_t *src=raw+t*128;
        for(unsigned y=0;y<16;y++){
            uint16_t plane[4];
            for(unsigned p=0;p<4;p++) plane[p]=(uint16_t)((src[p*32+y*2]<<8)|src[p*32+y*2+1]);
            for(unsigned x=0;x<16;x++){
                unsigned bit=15-x, c=0;
                for(unsigned p=0;p<4;p++) c|=((plane[p]>>bit)&1u)<<p;
                w->tile_pixels[t*256+y*16+x]=(uint8_t)c;
            }
        }
    }
}

/* W_BLOCKS contains two $0A80-byte display-buffer banks.  Each bank has
   42 16x16 records: 32 bytes for colour bit 2 followed by 32 for bit 3. */
static void decode_overlay(FloodWorld *w,const uint8_t raw[FLOOD_OVERLAY_BYTES]){
    for(unsigned bank=0;bank<FLOOD_OVERLAY_BANK_COUNT;bank++){
        for(unsigned tile=0;tile<FLOOD_OVERLAY_TILE_COUNT;tile++){
            const uint8_t *src=raw+bank*FLOOD_OVERLAY_BANK_BYTES+tile*64u;
            uint8_t *dst=&w->overlay_pixels[
                (bank*FLOOD_OVERLAY_TILE_COUNT+tile)*FLOOD_OVERLAY_TILE_PIXELS];
            for(unsigned y=0;y<16u;y++)for(unsigned x=0;x<16u;x++){
                const unsigned byte=y*2u+x/8u,bit=7u-(x&7u);
                dst[y*16u+x]=(uint8_t)(((src[byte]>>bit)&1u)|
                    (((src[32u+byte]>>bit)&1u)<<1));
            }
        }
    }
}

static void decode_sprite_record(FloodSprite *s,const uint8_t *src,unsigned width,unsigned row_bytes,unsigned block_size){
    memset(s,0,sizeof(*s)); s->width=(uint8_t)width; s->height=(uint8_t)width;
    for(unsigned y=0;y<width;y++) for(unsigned x=0;x<width;x++){
        const unsigned bi=y*row_bytes+x/8u, bit=7u-(x&7u);
        const unsigned out=y*FLOOD_MAX_SPRITE_W+x;
        /* $E5B4 computes mask = image + four plane blocks. */
        s->mask[out]=(uint8_t)((src[4u*block_size+bi]>>bit)&1u);
        unsigned c=0;
        for(unsigned p=0;p<4;p++) c|=((src[p*block_size+bi]>>bit)&1u)<<p;
        s->pixels[out]=(uint8_t)c;
    }
}

static int load_sprites(FloodGame *g,const char *data_dir){
    char path[512]; uint8_t spr16[12800],spr32[96000];
    snprintf(path,sizeof(path),"%s/sprites/SPR_16_unpacked.bin",data_dir);
    if(!read_file_exact(path,spr16,sizeof(spr16))) return 0;
    snprintf(path,sizeof(path),"%s/sprites/SPR_32.B",data_dir);
    if(!read_file_exact(path,spr32,sizeof(spr32))) return 0;
    for(unsigned gid=0;gid<80;gid++) decode_sprite_record(&g->sprites[gid],spr16+gid*160u,16,2,32);
    for(unsigned i=0;i<150;i++) decode_sprite_record(&g->sprites[80u+i],spr32+i*640u,32,4,128);
    return 1;
}

static int load_hud(FloodGame *g,const char *data_dir){
    char path[512];
    snprintf(path,sizeof(path),"%s/hud/HUD_template.bin",data_dir);
    if(!read_file_exact(path,g->hud_template,sizeof(g->hud_template)))return 0;
    snprintf(path,sizeof(path),"%s/hud/HUD_glyphs.bin",data_dir);
    return read_file_exact(path,g->hud_glyphs,sizeof(g->hud_glyphs));
}

static int load_overlay(FloodGame *g,const char *data_dir){
    char path[512];uint8_t raw[FLOOD_OVERLAY_BYTES];
    snprintf(path,sizeof(path),"%s/water/W_BLOCKS.bin",data_dir);
    if(!read_file_exact(path,raw,sizeof(raw)))return 0;
    decode_overlay(&g->world,raw);
    return 1;
}

uint8_t flood_tile_pixel(const FloodGame *g,uint8_t tile,unsigned x,unsigned y){
    return g->world.tile_pixels[(unsigned)tile*256+(y&15u)*16+(x&15u)];
}
const FloodSprite *flood_sprite(const FloodGame *g,unsigned sprite_id){
    return sprite_id<FLOOD_SPRITE_COUNT?&g->sprites[sprite_id]:NULL;
}

/* $DF54-$E10C followed by $A5A4: level entry restores Quiffy's gauges,
   protection, and motion state, clears both runtime record banks and the
   Matilda history, and drops carried/effect/transport state.  Score, lives,
   RNG stream, and the global frame phase survive the handoff (marker setup
   can consume that continuing RNG stream). */
static void reset_level_runtime(FloodGame *g){
    memset(&g->player,0,sizeof(g->player));
    g->player.life_force=511;g->player.air=63;
    g->player.invulnerable_timer=100u;g->player.surface_y=1;
    g->player.pose_code=0u;
    g->escape_life_force=0;g->escape_death_active=false;
    memset(&g->matilda,0,sizeof(g->matilda));
    /* $E0A8/$E0D8: the writer starts at byte offset 0 and the reader at
       byte offset 4, i.e. one 4-byte history record ahead. */
    g->matilda.read_pos=1u;
    memset(g->objects,0,sizeof(g->objects));
    memset(g->actions,0,sizeof(g->actions));
    memset(&g->quiffy_effect,0,sizeof(g->quiffy_effect));
    g->selected_weapon_state=0u;g->fire_ticks=0u;
    g->weapon_main_offset=0u;g->weapon_tip_offset=0u;
    g->transport_timer=0u;g->transport_effect_stage=0u;g->transport_x=0;g->transport_y=0;
    g->transport_render_x=0;g->transport_render_y=0;g->transport_render_prejump=false;
    g->zap_message_pending=false;
    g->action_target_x=0;g->action_target_y=0;g->action_render_count=0u;
    g->weapon_pose_active=false;g->dispatch_suppressed=false;
    g->render_dispatch_suppressed=false;g->quiffy_effect_pose_active=false;
    g->quiffy_effect_render_active=false;g->player_blink_active=false;
    g->special_entry_offset=false;
}

static bool level_path(char *path,size_t capacity,const char *prefix,const char *part,
    char *err,size_t errcap){
    const int length=snprintf(path,capacity,"%s_%s.bin",prefix,part);
    if(length<0||(size_t)length>=capacity){seterr(err,errcap,"level path too long");return false;}
    return true;
}
static bool flood_game_load_prefix(FloodGame *g,const char *data_dir,
    const char *prefix,unsigned level,char *err,size_t errcap){
    char path[1100]; uint8_t header[16], raw[32768];
    if(!level_path(path,sizeof(path),prefix,"header",err,errcap))return false;
    if(!read_file_exact(path,header,sizeof(header))){seterr(err,errcap,"cannot read level header");return false;}
    unsigned bank=be16(header); if(bank<1||bank>3){seterr(err,errcap,"invalid BLOCK bank in level header");return false;}
    if(!level_path(path,sizeof(path),prefix,"tilemap",err,errcap))return false;
    if(!read_file_exact(path,g->world.terrain,sizeof(g->world.terrain))){seterr(err,errcap,"cannot read level tilemap");return false;}
    if(!level_path(path,sizeof(path),prefix,"trigger_payload",err,errcap))return false;
    if(!read_file_exact(path,g->world.trigger_data,sizeof(g->world.trigger_data))){seterr(err,errcap,"cannot read trigger payload");return false;}
    g->world.trigger_count=be16(header+2);
    if(g->world.trigger_count>64u){seterr(err,errcap,"invalid trigger count");return false;}
    const char bankc=(char)('A'+bank-1);
    snprintf(path,sizeof(path),"%s/blocks/BLOCK%c_tiles.bin",data_dir,bankc);
    if(!read_file_exact(path,raw,sizeof(raw))){seterr(err,errcap,"cannot read block graphics");return false;}
    snprintf(path,sizeof(path),"%s/blocks/BLOCK%c_attrs.bin",data_dir,bankc);
    if(!read_file_exact(path,g->world.attr,sizeof(g->world.attr))){seterr(err,errcap,"cannot read block attributes");return false;}
    if(!load_sprites(g,data_dir)){seterr(err,errcap,"cannot read sprite banks");return false;}
    if(!load_hud(g,data_dir)){seterr(err,errcap,"cannot read HUD assets");return false;}
    if(!load_overlay(g,data_dir)){seterr(err,errcap,"cannot read W_BLOCKS overlay graphics");return false;}
    reset_level_runtime(g);
    decode_tiles(&g->world,raw);
    g->world.block_bank=(uint8_t)bank; g->world.level_number=(uint8_t)level;
    g->last_pickup_tile=UINT16_MAX;g->last_special_tile=UINT16_MAX;
    g->level_complete=false;g->game_complete=false;g->running=true;
    g->world.mechanism_enabled=false;g->world.mechanisms_paused=false;
    g->world.water_active=false;
    g->world.water_source_x=0;g->world.water_source_y=0;
    g->world.water_pause=0;g->world.water_speed=1;
    flood_reset_water(g);

    /* $E726-$E732 installs full-map camera maxima before marker 23 can
       replace them for a smaller cavern. */
    g->world.bounds_y=0x570;
    g->world.bounds_x=0x6c0;

    /* $E6BC counts the five edible/replicable map tiles before marker
       initialization mutates the level map. */
    g->world.remaining_food=0;
    for(unsigned i=0;i<FLOOD_MAP_W*FLOOD_MAP_H;i++){
        const uint8_t tile=g->world.terrain[i];
        if(tile==0x94u||(tile>=0x24u&&tile<=0x27u))g->world.remaining_food++;
    }

    int found_start=0;
    /* $E73C-$E75C starts D1 at $31FF and uses DBF, so marker records are
       initialized from the last map byte back to the first.  Level 5 relies
       on this: a second byte value 22 lies beyond its small-cavern bounds,
       but the lower-index Quiffy marker is visited last and is the real start. */
    for(unsigned reverse=FLOOD_MAP_W*FLOOD_MAP_H;reverse>0u;reverse--){
        const unsigned i=reverse-1u;
        uint8_t v=g->world.terrain[i];
        if(v==22)found_start=1;
        if(v>=1&&v<=25)flood_initialize_marker(g,(uint16_t)i,v);
    }
    if(!found_start){seterr(err,errcap,"level has no Quiffy start marker (22)");return false;}
    g->camera_x=0;g->camera_y=0;
    g->render_tile_phase=(uint8_t)(g->ticks&3u);
    g->render_buffer_index=(uint8_t)(g->ticks&1u);
    memcpy(g->world.render_terrain,g->world.terrain,sizeof(g->world.render_terrain));
    g->world.render_terrain_valid=true;
    memcpy(g->world.render_water,g->world.water,sizeof(g->world.render_water));
    flood_update_hud(g);
    return true;
}

bool flood_game_load_level(FloodGame *g,const char *data_dir,unsigned level,char *err,size_t errcap){
    if(level<1||level>42){seterr(err,errcap,"level must be 1..42");return false;}
    char prefix[1024];
    if(snprintf(prefix,sizeof(prefix),"%s/levels/level_%02u",data_dir,level)>=(int)sizeof(prefix)){
        seterr(err,errcap,"level path too long");return false;
    }
    return flood_game_load_prefix(g,data_dir,prefix,level,err,errcap);
}

bool flood_game_load_custom_level(FloodGame *g,const char *data_dir,
    const char *header_path,char *err,size_t errcap){
    if(!header_path){seterr(err,errcap,"select a level header");return false;}
    const size_t length=strlen(header_path),suffix_length=strlen("_header.bin");
    if(length<=suffix_length||length-suffix_length>=1024u||
       strcmp(header_path+length-suffix_length,"_header.bin")!=0){
        seterr(err,errcap,"select a *_header.bin level file");return false;
    }
    char prefix[1024];memcpy(prefix,header_path,length-suffix_length);
    prefix[length-suffix_length]='\0';
    const char *stem=strrchr(prefix,'/');
    const char *backslash=strrchr(prefix,'\\');
    if(backslash&&(!stem||backslash>stem))stem=backslash;
    stem=stem?stem+1:prefix;
    unsigned number=1u;
    if(strlen(stem)==8u&&strncmp(stem,"level_",6u)==0&&
       stem[6]>='0'&&stem[6]<='9'&&stem[7]>='0'&&stem[7]<='9'){
        unsigned parsed=(unsigned)(stem[6]-'0')*10u+(unsigned)(stem[7]-'0');
        if(parsed>=1u&&parsed<=99u)number=parsed;
    }
    return flood_game_load_prefix(g,data_dir,prefix,number,err,errcap);
}

/* $D33C-$D362: R first performs the ordinary runtime reset and cavern setup.
   The caller presents the entry fade and complete level banner between these
   two stages; only after that banner does the original subtract the life. */
bool flood_game_restart_begin(FloodGame *g,const char *data_dir,char *err,size_t errcap){
    return flood_game_load_level(g,data_dir,g->world.level_number,err,errcap);
}

/* $B206-$B22A and $B2E0-$B2F2.  The exit handler leaves remaining_food at
   -1 for the outer frame loop.  Levels 1..41 immediately enter the following
   cavern; completing level 42 advances the original counter to 43 and exits
   gameplay through control value -3 into the dedicated ending sequence. */
bool flood_game_advance_level(FloodGame *g,const char *data_dir,char *err,size_t errcap){
    if(g->world.remaining_food!=-1)return true;
    if(g->world.level_number>=42u){
        g->level_complete=true;g->game_complete=true;g->running=false;
        return true;
    }
    return flood_game_load_level(g,data_dir,(unsigned)g->world.level_number+1u,
        err,errcap);
}
