#include "flood_game.h"
#include <stdio.h>
#include <string.h>

static uint16_t be16(const uint8_t *p){return (uint16_t)((p[0]<<8)|p[1]);}
static uint32_t be32(const uint8_t *p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
static void put_be16(uint8_t *p,uint16_t value){p[0]=(uint8_t)(value>>8);p[1]=(uint8_t)value;}

static bool read_exact(const char *path,uint8_t *out,size_t size){
    FILE *f=fopen(path,"rb");size_t got;if(!f)return false;
    got=fread(out,1,size,f);fclose(f);return got==size;
}

static void intro_blit(FloodIntro *i,uint16_t sx,uint16_t sy,uint16_t dx,
    uint16_t dy,uint16_t width,uint16_t height,uint16_t masked){
    const size_t bytes=(size_t)(width>>3);
    if(!bytes||!height||sx+width>FLOOD_INTRO_BACKING_W||dx+width>FLOOD_INTRO_BACKING_W||
       sy+height>FLOOD_INTRO_BACKING_H||dy+height>FLOOD_INTRO_BACKING_H)return;
    const size_t source_x=sx>>3,dest_x=dx>>3;
    for(unsigned plane=0;plane<5u;plane++)for(unsigned row=0;row<height;row++){
        size_t source=(size_t)plane*FLOOD_INTRO_PLANE_BYTES+
            (size_t)(sy+row)*FLOOD_INTRO_ROW_BYTES+source_x;
        size_t dest=(size_t)plane*FLOOD_INTRO_PLANE_BYTES+
            (size_t)(dy+row)*FLOOD_INTRO_ROW_BYTES+dest_x;
        size_t mask=5u*FLOOD_INTRO_PLANE_BYTES+
            (size_t)(sy+row)*FLOOD_INTRO_ROW_BYTES+source_x;
        for(size_t column=0;column<bytes;column++){
            if(masked)i->planes[dest]=(uint8_t)((i->planes[dest]&i->planes[mask])|i->planes[source]);
            else i->planes[dest]=i->planes[source];
            source++;dest++;mask++;
        }
    }
}

bool flood_intro_load(FloodIntro *i,const char *data_dir,char *err,size_t errcap){
    char path[512];memset(i,0,sizeof(*i));
    snprintf(path,sizeof(path),"%s/presentation/BIG_PIC.bin",data_dir);
    if(!read_exact(path,i->planes,5u*FLOOD_INTRO_PLANE_BYTES)){snprintf(err,errcap,"cannot read %s",path);return false;}
    snprintf(path,sizeof(path),"%s/presentation/INTRO_events.bin",data_dir);
    if(!read_exact(path,i->events,sizeof(i->events))){snprintf(err,errcap,"cannot read %s",path);return false;}
    uint8_t colours[64];snprintf(path,sizeof(path),"%s/presentation/INTRO_palette.bin",data_dir);
    if(!read_exact(path,colours,sizeof(colours))){snprintf(err,errcap,"cannot read %s",path);return false;}
    for(unsigned n=0;n<32u;n++)i->palette[n]=be16(colours+n*2u);
    for(size_t n=0;n<FLOOD_INTRO_PLANE_BYTES;n++){
        uint8_t used=0;for(unsigned plane=0;plane<5u;plane++)used|=i->planes[plane*FLOOD_INTRO_PLANE_BYTES+n];
        i->planes[5u*FLOOD_INTRO_PLANE_BYTES+n]=(uint8_t)~used;
    }
    i->loaded=true;return true;
}

void flood_intro_tick(FloodIntro *i,bool fire){
    if(!i->loaded||i->finished)return;
    i->frame++;
    if(fire){i->skipped=true;i->finished=true;return;}
    for(;;){
        if(i->event_pos+16u>sizeof(i->events)){i->finished=true;return;}
        const uint8_t *event=i->events+i->event_pos;const uint16_t when=be16(event);
        if(when==0xffffu){i->finished=true;return;}
        if(when!=i->frame)break;
        uint16_t width=be16(event+10u);
        if(width&1u)width&=(uint16_t)~1u;
        intro_blit(i,be16(event+2u),be16(event+4u),be16(event+6u),be16(event+8u),
            width,be16(event+12u),be16(event+14u));
        i->event_pos+=16u;
    }
    intro_blit(i,0u,16u,160u,16u,160u,76u,0u);
}

uint8_t flood_intro_pixel(const FloodIntro *i,unsigned x,unsigned y){
    if(!i->loaded||x>=FLOOD_INTRO_W||y>=FLOOD_INTRO_H)return 0u;
    /* Copper DIWSTRT/DIWSTOP at $B66E/$B674 expose a 96-line presentation
       window.  Keep the native right-hand 160x96 work image centred inside
       its 320x240 intro canvas rather than scaling the work buffer itself to
       the whole window. */
    if(x<FLOOD_INTRO_CONTENT_X||x>=FLOOD_INTRO_CONTENT_X+FLOOD_INTRO_CONTENT_W||
       y<FLOOD_INTRO_CONTENT_Y||y>=FLOOD_INTRO_CONTENT_Y+FLOOD_INTRO_CONTENT_H)return 0u;
    x=x-FLOOD_INTRO_CONTENT_X+160u;y-=FLOOD_INTRO_CONTENT_Y;
    const size_t byte=(size_t)y*FLOOD_INTRO_ROW_BYTES+(x>>3);const uint8_t bit=(uint8_t)(0x80u>>(x&7u));
    uint8_t colour=0;for(unsigned plane=0;plane<5u;plane++)if(i->planes[plane*FLOOD_INTRO_PLANE_BYTES+byte]&bit)colour|=(uint8_t)(1u<<plane);
    return colour;
}

bool flood_title_load(FloodTitle *title,const char *data_dir,char *err,size_t errcap){
    char path[512];memset(title,0,sizeof(*title));
    snprintf(path,sizeof(path),"%s/presentation/TITLE_SCR.bin",data_dir);
    if(!read_exact(path,title->planes,sizeof(title->planes))){snprintf(err,errcap,"cannot read %s",path);return false;}
    uint8_t colours[64];snprintf(path,sizeof(path),"%s/presentation/TITLE_palette.bin",data_dir);
    if(!read_exact(path,colours,sizeof(colours))){snprintf(err,errcap,"cannot read %s",path);return false;}
    for(unsigned n=0;n<32u;n++)title->palette[n]=be16(colours+n*2u);
    title->loaded=true;return true;
}

uint8_t flood_title_pixel(const FloodTitle *title,unsigned x,unsigned y){
    /* The five-plane title call at $9704 uses $EE4A's 320x208 DIW. */
    if(!title->loaded||x>=FLOOD_TITLE_W||y>=FLOOD_TITLE_H)return 0u;
    x+=16u;
    const size_t byte=(size_t)y*FLOOD_TITLE_ROW_BYTES+(x>>3);const uint8_t bit=(uint8_t)(0x80u>>(x&7u));
    uint8_t colour=0;for(unsigned plane=0;plane<5u;plane++)if(title->planes[plane*FLOOD_TITLE_PLANE_BYTES+byte]&bit)colour|=(uint8_t)(1u<<plane);
    return colour;
}

static void level_glyph(FloodLevelSelect *s,unsigned cell_x,unsigned cell_y,uint8_t ch){
    /* $14DBA subtracts decimal 20 ($14), not character $20. */
    if(ch<0x14u||ch>=0x8cu||cell_x>=FLOOD_LEVEL_SELECT_ROW_BYTES||cell_y>=30u)return;
    const uint8_t *glyph=s->font+(size_t)(ch-0x14u)*8u;
    const size_t dest=(size_t)cell_y*8u*FLOOD_LEVEL_SELECT_ROW_BYTES+cell_x;
    for(unsigned plane=0;plane<4u;plane++)for(unsigned row=0;row<8u;row++)
        s->planes[(size_t)plane*FLOOD_LEVEL_SELECT_PLANE_BYTES+dest+
            (size_t)row*FLOOD_LEVEL_SELECT_ROW_BYTES]=glyph[row];
}

static void level_line(FloodLevelSelect *s,unsigned row,const uint8_t *text){
    for(unsigned column=0;column<20u;column++)level_glyph(s,12u+column,9u+row,text[column]);
}

static void level_digits(FloodLevelSelect *s){
    s->panel[57]=(uint8_t)('0'+s->selected_level/10u);
    s->panel[58]=(uint8_t)('0'+s->selected_level%10u);
}

bool flood_level_select_load(FloodLevelSelect *s,const char *data_dir,
    unsigned initial_level,char *err,size_t errcap){
    char path[512];uint8_t colours[32];memset(s,0,sizeof(*s));
#define LEVEL_READ(name,dest) do { \
    snprintf(path,sizeof(path),"%s/presentation/" name,data_dir); \
    if(!read_exact(path,(uint8_t *)(dest),sizeof(dest))){ \
        snprintf(err,errcap,"cannot read %s",path);return false; \
    } \
} while(0)
    LEVEL_READ("TEMPFILE_SCR.bin",s->planes);
    LEVEL_READ("LEVEL_font.bin",s->font);
    LEVEL_READ("LEVEL_template.bin",s->panel);
    LEVEL_READ("LEVEL_correct.bin",s->correct);
    LEVEL_READ("LEVEL_incorrect.bin",s->incorrect);
    LEVEL_READ("LEVEL_passwords.bin",s->passwords);
    snprintf(path,sizeof(path),"%s/presentation/LEVEL_palette.bin",data_dir);
    if(!read_exact(path,colours,sizeof(colours))){snprintf(err,errcap,"cannot read %s",path);return false;}
#undef LEVEL_READ
    for(unsigned n=0;n<16u;n++)s->palette[n]=be16(colours+n*2u);
    s->selected_level=(uint8_t)(initial_level>=1u&&initial_level<=48u?initial_level:1u);
    memcpy(s->saved_password,"????",5u);level_digits(s);
    memcpy(s->panel+76u,s->saved_password,4u);
    for(unsigned row=0;row<6u;row++)level_line(s,row,s->panel+row*21u);
    s->loaded=true;return true;
}

void flood_level_select_set_background(FloodLevelSelect *s,
    const uint8_t planes[4*FLOOD_LEVEL_SELECT_PLANE_BYTES]){
    if(!s||!s->loaded||!planes)return;
    /* The original front end draws both screens in TEMPFILE.SCR.  Entering
       the selector after the score/credits display therefore retains those
       planes and draws only the six-row PLAY LEVEL panel over them. */
    memcpy(s->planes,planes,sizeof(s->planes));
    for(unsigned row=0;row<6u;row++)level_line(s,row,s->panel+row*21u);
}

void flood_level_select_tick(FloodLevelSelect *s,uint8_t key,bool fire){
    if(!s->loaded||s->finished)return;
    for(unsigned row=0;row<6u;row++)if(row!=4u||!s->status_timer)
        level_line(s,row,s->panel+row*21u);

    if(s->cursor>0u&&s->cursor<4u)
        for(unsigned n=s->cursor;n<4u;n++)s->panel[76u+n]='?';
    else if(s->cursor==0u)memcpy(s->panel+76u,s->saved_password,4u);

    if(key&&key!=s->last_key){
        if(key==0x7fu){
            if(s->cursor){s->cursor--;s->panel[76u+s->cursor]='?';}
        }else if(key==0x0du){
            if(s->cursor){
                s->result_level=0u;s->status_timer=130u;
                if(s->cursor==4u)for(unsigned n=0;n<FLOOD_LEVEL_SELECT_PASSWORD_COUNT;n++)
                    if(!memcmp(s->panel+76u,s->passwords[n],4u)){
                        s->result_level=(uint8_t)(n+1u);break;
                    }
                if(s->result_level)memcpy(s->saved_password,s->panel+76u,4u);
                else{
                    memcpy(s->saved_password,"????",5u);memcpy(s->panel+76u,"????",4u);
                    s->selected_level=1u;level_digits(s);
                }
                s->cursor=0u;
            }
        }else if(s->cursor<4u)s->panel[76u+s->cursor++]=key;
    }
    s->last_key=key;

    if(s->status_timer){
        if(s->result_level){s->selected_level=s->result_level;level_digits(s);level_line(s,4u,s->correct);}
        else level_line(s,4u,s->incorrect);
        s->status_timer--;
    }
    if(fire)s->finished=true;
}

uint8_t flood_level_select_pixel(const FloodLevelSelect *s,unsigned x,unsigned y){
    /* The selector call at $97B2 retains the same 320x208 display window. */
    if(!s->loaded||x>=FLOOD_LEVEL_SELECT_W||y>=FLOOD_LEVEL_SELECT_H)return 0u;
    x+=16u;
    const size_t byte=(size_t)y*FLOOD_LEVEL_SELECT_ROW_BYTES+(x>>3);
    const uint8_t bit=(uint8_t)(0x80u>>(x&7u));uint8_t colour=0u;
    for(unsigned plane=0;plane<4u;plane++)if(s->planes[(size_t)plane*
        FLOOD_LEVEL_SELECT_PLANE_BYTES+byte]&bit)colour|=(uint8_t)(1u<<plane);
    return colour;
}

static void zap_glyph(FloodZapMessage *message,unsigned column,unsigned row,uint8_t ch){
    if(ch<0x14u||ch>=0x8cu||column>=20u||row>=6u)return;
    const uint8_t *glyph=message->font+(size_t)(ch-0x14u)*8u;
    for(unsigned y=0;y<8u;y++)for(unsigned x=0;x<8u;x++)
        message->pixels[(row*8u+y)*FLOOD_ZAP_W+column*8u+x]=
            (glyph[y]&(uint8_t)(0x80u>>x))?15u:0u;
}

bool flood_zap_message_load(FloodZapMessage *message,const char *data_dir,
    unsigned level,char *err,size_t errcap){
    char path[512];uint8_t colours[32];memset(message,0,sizeof(*message));
#define ZAP_READ(name,dest) do { \
    snprintf(path,sizeof(path),"%s/presentation/" name,data_dir); \
    if(!read_exact(path,(uint8_t *)(dest),sizeof(dest))){ \
        snprintf(err,errcap,"cannot read %s",path);return false; \
    } \
} while(0)
    ZAP_READ("LEVEL_font.bin",message->font);
    ZAP_READ("LEVEL_zap.bin",message->panel);
    ZAP_READ("LEVEL_passwords.bin",message->passwords);
    snprintf(path,sizeof(path),"%s/presentation/PROTECTION_palette.bin",data_dir);
    if(!read_exact(path,colours,sizeof(colours))){snprintf(err,errcap,"cannot read %s",path);return false;}
#undef ZAP_READ
    for(unsigned n=0;n<16u;n++)message->palette[n]=be16(colours+n*2u);
    message->level=(uint8_t)(level>=1u&&level<=48u?level:1u);
    message->panel[57]=(uint8_t)('0'+message->level/10u);
    message->panel[58]=(uint8_t)('0'+message->level%10u);
    memcpy(message->panel+76u,message->passwords[message->level-1u],4u);
    for(unsigned row=0;row<6u;row++)for(unsigned column=0;column<20u;column++)
        zap_glyph(message,column,row,message->panel[row*21u+column]);
    message->loaded=true;return true;
}

void flood_zap_message_tick(FloodZapMessage *message,bool fire){
    if(message->loaded&&!message->finished&&fire)message->finished=true;
}

uint8_t flood_zap_message_pixel(const FloodZapMessage *message,unsigned x,unsigned y){
    if(!message->loaded||x>=FLOOD_ZAP_W||y>=FLOOD_ZAP_H)return 0u;
    return message->pixels[y*FLOOD_ZAP_W+x];
}

static void protection_decode_sprite(FloodSprite *s,const uint8_t *src,
    unsigned width,unsigned row_bytes,unsigned block_size){
    memset(s,0,sizeof(*s));s->width=(uint8_t)width;s->height=(uint8_t)width;
    for(unsigned y=0;y<width;y++)for(unsigned x=0;x<width;x++){
        const unsigned byte=y*row_bytes+x/8u,bit=7u-(x&7u),out=y*FLOOD_MAX_SPRITE_W+x;
        s->mask[out]=(uint8_t)((src[4u*block_size+byte]>>bit)&1u);
        for(unsigned plane=0;plane<4u;plane++)
            s->pixels[out]|=(uint8_t)(((src[plane*block_size+byte]>>bit)&1u)<<plane);
    }
}

static void protection_sprite(FloodProtection *p,const FloodSprite *s,int x,int y){
    for(unsigned sy=0;sy<s->height;sy++)for(unsigned sx=0;sx<s->width;sx++){
        const unsigned source=sy*FLOOD_MAX_SPRITE_W+sx;const int dx=x+(int)sx,dy=y+(int)sy;
        if(s->mask[source]&&dx>=0&&dy>=0&&dx<FLOOD_PROTECTION_W&&dy<FLOOD_PROTECTION_H)
            p->pixels[(size_t)dy*FLOOD_PROTECTION_W+(unsigned)dx]=s->pixels[source];
    }
}

static void protection_glyph(FloodProtection *p,unsigned cell_x,unsigned cell_y,uint8_t ch){
    if(ch<0x14u||ch>=0x8cu)return;
    const uint8_t *glyph=p->font+(size_t)(ch-0x14u)*8u;
    const int origin_x=(int)cell_x*8-16,origin_y=(int)cell_y*8;
    for(unsigned y=0;y<8u;y++)for(unsigned x=0;x<8u;x++)if(glyph[y]&(uint8_t)(0x80u>>x)){
        const int dx=origin_x+(int)x,dy=origin_y+(int)y;
        if(dx>=0&&dy>=0&&dx<FLOOD_PROTECTION_W&&dy<FLOOD_PROTECTION_H)
            p->pixels[(size_t)dy*FLOOD_PROTECTION_W+(unsigned)dx]=15u;
    }
}

static void protection_text(FloodProtection *p,unsigned x,unsigned y,const uint8_t *text){
    for(size_t n=0;text[n];n++){
        if(x>41u){y++;x=2u;}protection_glyph(p,x++,y,text[n]);
    }
}

static const uint8_t *protection_message(const FloodProtection *p,unsigned group){
    const unsigned index=group*4u+p->language;
    return p->messages+p->message_offsets[index];
}

static void protection_render(FloodProtection *p){
    memset(p->pixels,0,sizeof(p->pixels));
    for(unsigned n=0;n<16u;n++)protection_sprite(p,&p->symbols[n],
        p->flag_layout[n][0],p->flag_layout[n][1]);
    protection_sprite(p,&p->symbols[16],72+48*(int)p->language,24);
    if(p->phase>=2u){
        const uint8_t *record=p->challenges+
            ((size_t)p->language*FLOOD_PROTECTION_RECORDS+p->challenge)*FLOOD_PROTECTION_RECORD_BYTES;
        protection_text(p,2u,10u,protection_message(p,0u));
        protection_text(p,2u,12u,record);
        protection_text(p,2u,14u,(const uint8_t *)">");
        protection_text(p,3u,14u,(const uint8_t *)p->answer);
        if(p->phase==3u)protection_text(p,2u,16u,protection_message(p,3u));
        else if(p->phase==4u){
            protection_text(p,2u,16u,protection_message(p,2u));
            protection_text(p,2u,18u,protection_message(p,1u));
        }
    }
}

static void protection_clear_answer(FloodProtection *p){
    memset(p->answer,' ',13u);p->answer[13]='\0';p->cursor=0u;p->last_key=0u;
}

bool flood_protection_load(FloodProtection *p,const char *data_dir,
    uint16_t rng_seed,char *err,size_t errcap){
    char path[512];uint8_t raw16[12800],raw32[640],raw_palette[32],raw_offsets[32],raw_flags[96];
    memset(p,0,sizeof(*p));
#define PROTECT_READ(pathpart,dest) do { \
    snprintf(path,sizeof(path),"%s/" pathpart,data_dir); \
    if(!read_exact(path,(uint8_t *)(dest),sizeof(dest))){ \
        snprintf(err,errcap,"cannot read %s",path);return false; \
    } \
} while(0)
    PROTECT_READ("presentation/LEVEL_font.bin",p->font);
    PROTECT_READ("presentation/PROTECTION_challenges_xor.bin",p->challenges);
    PROTECT_READ("presentation/PROTECTION_messages.bin",p->messages);
    PROTECT_READ("presentation/PROTECTION_palette.bin",raw_palette);
    PROTECT_READ("presentation/PROTECTION_message_offsets.bin",raw_offsets);
    PROTECT_READ("presentation/PROTECTION_flags.bin",raw_flags);
    PROTECT_READ("presentation/PROTECTION_input.bin",p->answer);
    PROTECT_READ("sprites/SPR_16_unpacked.bin",raw16);
    PROTECT_READ("sprites/SPR_32.B",raw32);
#undef PROTECT_READ
    for(size_t n=0;n<sizeof(p->challenges);n++)p->challenges[n]^=0xaau;
    for(unsigned n=0;n<16u;n++){
        p->palette[n]=be16(raw_palette+n*2u);p->message_offsets[n]=be16(raw_offsets+n*2u);
        for(unsigned word=0;word<3u;word++)p->flag_layout[n][word]=be16(raw_flags+n*6u+word*2u);
        const unsigned id=p->flag_layout[n][2];
        protection_decode_sprite(&p->symbols[n],raw16+id*160u,16u,2u,32u);
    }
    protection_decode_sprite(&p->symbols[16],raw32,32u,4u,128u);
    p->rng_state=rng_seed;p->phase=1u;p->loaded=true;protection_render(p);return true;
}

void flood_protection_seed(FloodProtection *p,uint16_t raster_seed){
    if(p->loaded&&p->phase==1u)p->rng_state=raster_seed;
}

void flood_protection_tick(FloodProtection *p,int8_t horizontal,uint8_t key,bool fire){
    if(!p->loaded||p->finished)return;
    if(p->phase==1u){
        if(horizontal&&!p->last_horizontal)
            p->language=(uint8_t)((p->language+(int)horizontal)&3);
        p->last_horizontal=horizontal;
        if(!p->selection_pending&&fire){
            p->selection_pending=true;
        }else if(p->selection_pending&&!fire){
            p->rng_state=(uint16_t)((uint32_t)p->rng_state*0x24a1u+0x24dfu);
            p->challenge=(uint8_t)((p->rng_state&0x7fffu)%FLOOD_PROTECTION_ACTIVE_RECORDS);
            p->selection_pending=false;p->phase=2u;
        }
    }else if(p->phase==2u){
        if(key&&key!=p->last_key){
            if(key==0x7fu){if(p->cursor){p->answer[--p->cursor]=' ';}}
            else if(key==0x0du){
                if(p->cursor){
                    const uint8_t *record=p->challenges+
                        ((size_t)p->language*FLOOD_PROTECTION_RECORDS+p->challenge)*FLOOD_PROTECTION_RECORD_BYTES;
                    if(!memcmp(p->answer,record+61u,13u))p->phase=3u;
                    else{p->phase=4u;p->failures++;}
                    p->ack_fire_seen=false;
                }
            }else if(p->cursor<13u)p->answer[p->cursor++]=(char)key;
        }
        p->last_key=key;
    }else{
        if(fire)p->ack_fire_seen=true;
        else if(p->ack_fire_seen){
            p->ack_fire_seen=false;
            if(p->phase==3u){p->passed=true;p->finished=true;}
            else if(p->failures>=2u){p->exhausted=true;p->finished=true;}
            else{p->phase=2u;protection_clear_answer(p);}
        }
    }
    protection_render(p);
}

uint8_t flood_protection_pixel(const FloodProtection *p,unsigned x,unsigned y){
    return p->loaded&&x<FLOOD_PROTECTION_W&&y<FLOOD_PROTECTION_H?
        p->pixels[(size_t)y*FLOOD_PROTECTION_W+x]:0u;
}

static void highscore_glyph(FloodHighScores *h,unsigned glyph_offset,
    unsigned byte_x,unsigned y,bool swapped,bool restore){
    if(glyph_offset+FLOOD_HIGHSCORE_GLYPH_BYTES>sizeof(h->glyphs)||
       byte_x+1u>=FLOOD_HIGHSCORE_ROW_BYTES||y+24u>FLOOD_HIGHSCORE_H)return;
    static const uint8_t natural[4]={2u,4u,6u,8u};
    static const uint8_t alternate[4]={2u,4u,8u,6u};
    const uint8_t *order=swapped?alternate:natural;
    for(unsigned row=0;row<24u;row++){
        const uint8_t *source=h->glyphs+glyph_offset+row*10u;
        const uint16_t mask=be16(source);
        for(unsigned plane=0;plane<4u;plane++){
            const size_t dest=(size_t)plane*FLOOD_HIGHSCORE_PLANE_BYTES+
                (size_t)(y+row)*FLOOD_HIGHSCORE_ROW_BYTES+byte_x;
            const uint16_t background=be16((restore?h->base_planes:h->planes)+dest);
            put_be16(h->planes+dest,(uint16_t)((background&mask)|be16(source+order[plane])));
        }
    }
}

static unsigned highscore_char_offset(uint8_t ch){
    return ch>=0x30u&&ch<=0x5au?(unsigned)(ch-0x30u)*FLOOD_HIGHSCORE_GLYPH_BYTES:0u;
}

static void highscore_draw_text(FloodHighScores *h,const char *text,unsigned byte_x,unsigned y){
    /* Original fixed-board blitter at $12B4E uses planes +2,+4,+6,+8.
       Only the live editor glyph uses the temporary +2,+4,+8,+6 order. */
    for(unsigned n=0;text[n];n++)highscore_glyph(h,highscore_char_offset((uint8_t)text[n]),
                                                byte_x+n*2u,y,false,false);
}

static void highscore_score_text(uint32_t score,char out[6]){
    static const uint32_t divisor[5]={10000u,1000u,100u,10u,1u};
    for(unsigned n=0;n<5u;n++){
        unsigned digit=0u;while(score>=divisor[n]){score-=divisor[n];digit++;}
        out[n]=(char)('0'+digit);
    }
    out[5]='\0';
}

static void highscore_build(FloodHighScores *h){
    memcpy(h->planes,h->base_planes,sizeof(h->planes));
    for(unsigned row=0;row<FLOOD_HIGHSCORE_ENTRIES;row++){
        const unsigned y=20u+row*32u;char digits[6];highscore_score_text(h->scores[row],digits);
        highscore_draw_text(h,h->names[row],2u,y);
        highscore_draw_text(h,digits,28u,y);
    }
}

static void highscore_active(FloodHighScores *h,bool swapped){
    if(h->entry_index<0)return;
    highscore_glyph(h,h->glyph_offset,2u+h->cursor*2u,
        20u+(unsigned)h->entry_index*32u,swapped,true);
}

void flood_high_scores_enter(FloodHighScores *h,uint32_t current_score){
    h->entry_index=-1;h->editing=false;h->finished=false;h->cursor=0u;
    h->horizontal_ready=false;h->fire_seen=false;h->glyph_step=0;
    unsigned rank=FLOOD_HIGHSCORE_ENTRIES;
    for(unsigned n=0;n<FLOOD_HIGHSCORE_ENTRIES;n++)
        if((int32_t)current_score>=(int32_t)h->scores[n]){rank=n;break;}
    if(rank<FLOOD_HIGHSCORE_ENTRIES){
        for(unsigned n=FLOOD_HIGHSCORE_ENTRIES-1u;n>rank;n--){
            h->scores[n]=h->scores[n-1u];memcpy(h->names[n],h->names[n-1u],12u);
        }
        h->scores[rank]=current_score;memcpy(h->names[rank],"===========",12u);
        h->entry_index=(int8_t)rank;h->editing=true;
        h->glyph_offset=13u*FLOOD_HIGHSCORE_GLYPH_BYTES;
        h->target_offset=h->glyph_offset;
    }
    highscore_build(h);
}

bool flood_high_scores_load(FloodHighScores *h,const char *data_dir,
    uint32_t current_score,char *err,size_t errcap){
    char path[512];uint8_t raw_scores[20],raw_palette[32];memset(h,0,sizeof(*h));h->entry_index=-1;
#define HISCORE_READ(name,dest) do { \
    snprintf(path,sizeof(path),"%s/presentation/" name,data_dir); \
    if(!read_exact(path,(uint8_t *)(dest),sizeof(dest))){ \
        snprintf(err,errcap,"cannot read %s",path);return false; \
    } \
} while(0)
    HISCORE_READ("TEMPFILE_SCR.bin",h->base_planes);
    HISCORE_READ("HSCORE_DAT.bin",h->glyphs);
    HISCORE_READ("HIGHSCORE_scores.bin",raw_scores);
    HISCORE_READ("HIGHSCORE_names.bin",h->names);
    HISCORE_READ("LEVEL_palette.bin",raw_palette);
#undef HISCORE_READ
    for(unsigned n=0;n<FLOOD_HIGHSCORE_ENTRIES;n++)h->scores[n]=be32(raw_scores+n*4u);
    for(unsigned n=0;n<16u;n++)h->palette[n]=be16(raw_palette+n*2u);
    h->loaded=true;flood_high_scores_enter(h,current_score);return true;
}

static void highscore_animate(FloodHighScores *h){
    const int32_t next=(int32_t)h->glyph_offset+h->glyph_step;
    if(next<0||next>0x2760){h->glyph_step=0;h->target_offset=h->glyph_offset;}
    else{
        h->glyph_offset=(uint16_t)next;
        if(h->glyph_offset==h->target_offset)h->glyph_step=0;
    }
    highscore_active(h,true);
}

void flood_high_scores_tick(FloodHighScores *h,int8_t horizontal,
    int8_t vertical,bool fire){
    if(!h->loaded||h->finished)return;
    if(!h->editing){
        if(fire)h->fire_seen=true;
        else if(h->fire_seen){h->fire_seen=false;h->finished=true;}
        return;
    }
    if(!horizontal)h->horizontal_ready=true;
    if(h->glyph_step){highscore_animate(h);return;}
    if(fire){h->fire_seen=true;highscore_active(h,true);return;}
    if(h->fire_seen){
        h->fire_seen=false;highscore_active(h,false);
        const uint8_t glyph=(uint8_t)(h->glyph_offset/FLOOD_HIGHSCORE_GLYPH_BYTES);
        if(h->cursor==FLOOD_HIGHSCORE_NAME_CHARS-1u){
            h->names[(unsigned)h->entry_index][h->cursor]=(char)('0'+glyph);h->editing=false;return;
        }
        if(glyph==10u){
            memset(h->names[(unsigned)h->entry_index]+h->cursor,'=',
                FLOOD_HIGHSCORE_NAME_CHARS-h->cursor);h->editing=false;return;
        }
        h->names[(unsigned)h->entry_index][h->cursor]=(char)('0'+glyph);
        h->names[(unsigned)h->entry_index][h->cursor+1u]=(char)('0'+glyph);
        h->cursor++;highscore_active(h,true);return;
    }
    if(horizontal&&h->horizontal_ready){
        highscore_active(h,false);
        h->names[(unsigned)h->entry_index][h->cursor]=
            (char)('0'+h->glyph_offset/FLOOD_HIGHSCORE_GLYPH_BYTES);
        if(horizontal<0&&h->cursor)h->cursor--;
        else if(horizontal>0&&h->cursor<FLOOD_HIGHSCORE_NAME_CHARS-1u)h->cursor++;
        h->glyph_offset=(uint16_t)highscore_char_offset(
            (uint8_t)h->names[(unsigned)h->entry_index][h->cursor]);
        h->target_offset=h->glyph_offset;h->horizontal_ready=false;highscore_active(h,true);return;
    }
    if(vertical){
        h->target_offset=(uint16_t)((int32_t)h->glyph_offset+(vertical>0?240:-240));
        h->glyph_step=(int16_t)(vertical>0?20:-20);highscore_animate(h);return;
    }
    highscore_active(h,true);
}

uint8_t flood_high_scores_pixel(const FloodHighScores *h,unsigned x,unsigned y){
    /* The score-screen setup in the $9FF8 path also uses $EE4A's DIW. */
    if(!h->loaded||x>=FLOOD_HIGHSCORE_W||y>=FLOOD_HIGHSCORE_H)return 0u;
    x+=16u;
    const size_t byte=(size_t)y*FLOOD_HIGHSCORE_ROW_BYTES+(x>>3);
    const uint8_t bit=(uint8_t)(0x80u>>(x&7u));uint8_t colour=0u;
    for(unsigned plane=0;plane<4u;plane++)if(h->planes[(size_t)plane*
        FLOOD_HIGHSCORE_PLANE_BYTES+byte]&bit)colour|=(uint8_t)(1u<<plane);
    return colour;
}
