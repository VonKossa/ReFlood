#include "flood_game.h"
#include <string.h>

static void request_sound(FloodGame *g,uint8_t sound){
    g->last_sound_id=sound;g->sound_pending=true;
    if(g->sound_queue_count<FLOOD_SOUND_QUEUE_MAX)
        g->sound_queue[g->sound_queue_count++]=sound;
}

static uint8_t fill_lut(uint8_t s){
    /* Exact 42-byte table at $109E6.  State 40 is the fully settled 16-pixel
       fill; omitting the last two entries makes deep static water look dry to
       Quiffy's oxygen path. */
    static const uint8_t v[42]={
        0,0,0,0,1,0,0,0,1,1,1,0,
        2,2,2,0,4,4,4,0,6,6,6,0,
        8,8,8,0,10,10,10,0,12,12,12,0,
        14,14,14,0,16,0
    };
    return s<42u?v[s]:0;
}

uint16_t flood_tile_index_at(int16_t x,int16_t y){
    const uint16_t ux=(uint16_t)x,uy=(uint16_t)y;
    return (uint16_t)(((uy>>4)<<7)+(ux>>4));
}

uint8_t flood_tile_attr_at(const FloodGame *g,int16_t x,int16_t y){
    if(x<0||y<0||x>=FLOOD_MAP_W*16||y>=FLOOD_MAP_H*16) return FLOOD_ATTR_SOLID|FLOOD_ATTR_WATER_BLOCK;
    return g->world.attr[g->world.terrain[flood_tile_index_at(x,y)]];
}

static uint16_t add_word(uint16_t a,int16_t b){return (uint16_t)(a+(uint16_t)b);}
static uint16_t span_minus_one(uint16_t position,int16_t delta,uint16_t extent){
    const uint16_t start=add_word(position,delta),end=(uint16_t)(start+extent-1u);
    return (uint16_t)((end>>4)-(start>>4));
}
static uint8_t flags_for_xy(const FloodGame *g,uint16_t tx,uint16_t ty){
    if(tx>=FLOOD_MAP_W||ty>=FLOOD_MAP_H) return FLOOD_ATTR_SOLID|FLOOD_ATTR_WATER_BLOCK;
    return g->world.attr[g->world.terrain[ty*FLOOD_MAP_W+tx]];
}

FloodCollisionResult flood_query_collision(const FloodGame *g,uint16_t x,uint16_t y,int16_t dx,int16_t dy,uint16_t width,uint16_t height){
    FloodCollisionResult result={0,0,0};
    const uint16_t shifted_y_span=span_minus_one(y,dy,height);
    const uint16_t current_y_span=span_minus_one(y,0,height);
    const uint16_t shifted_x_span=span_minus_one(x,dx,width);
    const uint16_t current_x_span=span_minus_one(x,0,width);
    if(dx!=0){
        const uint16_t moved_x=add_word(x,dx);
        uint16_t tx=(uint16_t)(moved_x>>4),ty=(uint16_t)(y>>4);
        if(dx>=0) tx=(uint16_t)(tx+shifted_x_span);
        for(uint16_t n=0;n<=current_y_span;n++) result.x_flags|=flags_for_xy(g,tx,(uint16_t)(ty+n));
    }
    if(dy!=0){
        const uint16_t moved_y=add_word(y,dy);
        uint16_t tx=(uint16_t)(x>>4),ty=(uint16_t)(moved_y>>4);
        if(dy>=0) ty=(uint16_t)(ty+shifted_y_span);
        for(uint16_t n=0;n<=current_x_span;n++) result.y_flags|=flags_for_xy(g,(uint16_t)(tx+n),ty);
    }
    if(dx!=0&&dy!=0){
        const uint16_t moved_x=add_word(x,dx),moved_y=add_word(y,dy);
        uint16_t tx=(uint16_t)(moved_x>>4),ty=(uint16_t)(moved_y>>4);
        if(dx>=0) tx=(uint16_t)(tx+shifted_x_span);
        if(dy>=0) ty=(uint16_t)(ty+shifted_y_span);
        result.corner_flags=flags_for_xy(g,tx,ty);
    }
    return result;
}

uint8_t flood_water_fill_at(const FloodGame *g,int16_t x,int16_t y){
    if(x<0||y<0||x>=FLOOD_MAP_W*16||y>=FLOOD_MAP_H*16)return 0;
    return fill_lut(g->world.water[flood_tile_index_at(x,y)]);
}

/* $F55A: reset the secondary map and the two private propagation cursors. */
void flood_reset_water(FloodGame *g){
    memset(g->world.water,0,sizeof(g->world.water));
    g->world.water_age=1u;
    g->world.water_scan_row=99u;
}

static bool water_destination_open(const FloodGame *g,int index){
    if(index<0||index>=FLOOD_MAP_W*FLOOD_MAP_H)return false;
    const uint8_t tile=g->world.terrain[(unsigned)index];
    return !(g->world.attr[tile]&FLOOD_ATTR_WATER_BLOCK)&&
        g->world.water[(unsigned)index]==0u;
}

static bool water_terrain_open(const FloodGame *g,int index){
    if(index<0||index>=FLOOD_MAP_W*FLOOD_MAP_H)return false;
    return !(g->world.attr[g->world.terrain[(unsigned)index]]&FLOOD_ATTR_WATER_BLOCK);
}

/* $F630-$F696: scan the current row left-to-right for its lowest positive
   state below 40.  Empty rows move the persistent scan cursor upward. */
static int water_select_cell(FloodGame *g){
    while(g->world.water_scan_row<FLOOD_MAP_H){
        const unsigned base=g->world.water_scan_row*FLOOD_MAP_W;
        uint8_t lowest=40u;int selected=-1;
        for(unsigned x=0;x<FLOOD_MAP_W;x++){
            const uint8_t state=g->world.water[base+x];
            if(state>0u&&state<lowest){lowest=state;selected=(int)(base+x);}
        }
        if(selected>=0)return selected;
        if(g->world.water_scan_row==0u)return -1;
        g->world.water_scan_row--;
    }
    /* The original relies on solid map borders before this can happen. */
    g->world.water_scan_row=FLOOD_MAP_H-1u;
    return -1;
}

/* $F698-$F840: exact 41-way state dispatcher. */
bool flood_water_step(FloodGame *g){
    const int index=water_select_cell(g);
    if(index<0)return false;
    uint8_t state=g->world.water[(unsigned)index];
    switch(state){
    case 1u:
    case 2u:
        if(water_destination_open(g,index+FLOOD_MAP_W)){
            g->world.water[(unsigned)(index+FLOOD_MAP_W)]=(uint8_t)(state+4u);
            g->world.water_scan_row++;
        }else state=8u;
        break;
    case 4u:state=8u;break;
    case 5u:
    case 6u:
        if(water_destination_open(g,index+FLOOD_MAP_W)){
            g->world.water[(unsigned)(index+FLOOD_MAP_W)]=state;
            g->world.water_scan_row++;
        }else state=(uint8_t)(state+4u);
        break;
    case 8u:case 9u:case 10u:state=(uint8_t)(state+4u);break;
    case 12u:case 13u:case 14u:
        if(water_destination_open(g,index-1)){
            g->world.water[(unsigned)(index-1)]=(uint8_t)(
                water_destination_open(g,index+FLOOD_MAP_W-1)?1u:12u);
        }else if(water_destination_open(g,index+1)){
            g->world.water[(unsigned)(index+1)]=(uint8_t)(
                water_destination_open(g,index+FLOOD_MAP_W+1)?2u:12u);
        }else state=(uint8_t)(state+4u);
        break;
    case 16u:case 17u:case 18u:
    case 20u:case 21u:case 22u:
    case 24u:case 25u:case 26u:
    case 28u:case 29u:case 30u:
    case 32u:case 33u:case 34u:
        state=(uint8_t)(state+4u);break;
    case 36u:case 37u:case 38u:
        state=40u;
        if(water_terrain_open(g,index-FLOOD_MAP_W)){
            uint8_t *above=&g->world.water[(unsigned)(index-FLOOD_MAP_W)];
            *above=*above<3u?4u:(uint8_t)(*above+4u);
        }
        break;
    default:break;
    }
    g->world.water[(unsigned)index]=state;
    g->world.water_age++;
    return true;
}

/* $F5CE-$F62E: after an ordinary trigger changes terrain, clear and replay
   the flood for its saved age from the last marker-21 source. */
void flood_rebuild_water(FloodGame *g){
    const uint32_t age=g->world.water_age;
    flood_reset_water(g);
    if(g->world.water_source_x!=0){
        const uint16_t index=flood_tile_index_at(
            g->world.water_source_x,g->world.water_source_y);
        if(index<FLOOD_MAP_W*FLOOD_MAP_H)g->world.water[index]=4u;
        for(uint32_t n=0;n<age;n++)if(!flood_water_step(g))break;
    }
    g->world.water_age=age;
}

/* $AF2E-$AF74: pause countdown, speed-squared step budget, then the original
   alternating-buffer speed decay for accelerated flood pickups. */
void flood_update_water(FloodGame *g){
    if(!g->world.water_active)return;
    if(g->world.water_pause)g->world.water_pause--;
    else{
        const uint32_t count=(uint32_t)g->world.water_speed*g->world.water_speed;
        for(uint32_t n=0;n<count;n++)flood_water_step(g);
    }
    if(g->world.water_speed>1u)
        g->world.water_speed=(uint16_t)(g->world.water_speed-(g->render_buffer_index&1u));
}

uint16_t flood_player_sprite_id(const FloodGame *g){
    if(g->player.death_mode==2u){
        const uint8_t phase=g->player.death_phase>7u?7u:g->player.death_phase;
        return (uint16_t)(0x78u+phase);
    }
    if(g->quiffy_effect_pose_active)return g->quiffy_effect_sprite;
    if(g->weapon_pose_active&&g->fire_ticks){
        uint16_t phase=g->fire_ticks-1u;if(phase>3u)phase=3u;
        return (uint16_t)(0xc8u+phase+g->player.hbank+
            (phase==3u?g->weapon_main_offset:0u));
    }
    /* $CE80-$D17A always draws $50 plus D4F2's current/preserved pose.
       $C8-$CF is the separate fire pose, not the free-movement fallback. */
    return (uint16_t)(0x50u+g->player.pose_code);
}

uint16_t flood_weapon_tip_sprite_id(const FloodGame *g){
    return (uint16_t)(0xd0u+g->weapon_tip_offset+(g->player.hbank>>2));
}

/* Exact $D460-$D4F0 single-diagonal selector.  It returns the pose code
   consumed by the special $50-based renderer, or $55 as its sentinel. */
uint8_t flood_select_quiffy_corner_pose(uint8_t c,uint8_t h,uint8_t v){
    if(c&0x02u){
        if(v==0u)return 0x5bu;
        if(h!=0u)return 0x57u;
    }
    if(c&0x08u){
        if(h!=0u)return 0x5au;
        if(v!=0u)return 0x56u;
    }
    if(c&0x20u){
        if(v!=0u)return 0x5du;
        if(h==0u)return 0x59u;
    }
    if(c&0x80u){
        if(h==0u)return 0x5cu;
        if(v==0u)return 0x58u;
    }
    return 0x55u;
}

/* Exact state transformation at $D4F2-$D60C.  $17E68 is the final contact
   word; $17E6A is the raw word captured before attr-7 support fallback. */
FloodPoseSelection flood_select_quiffy_pose(uint16_t contacts,uint8_t raw,
    uint8_t count,int8_t raw_y,uint8_t phase,uint8_t h,uint8_t v,
    uint8_t previous,bool material){
    FloodPoseSelection out={previous,h,v,false};
    phase&=3u;h=h?4u:0u;v=v?4u:0u;out.hbank=h;out.vbank=v;
    if(count==1u){
        uint8_t corner=flood_select_quiffy_corner_pose((uint8_t)contacts,h,v);
        if(corner!=0x55u){out.code=corner;out.active=true;return out;}
    }
    if((contacts&0xffu)!=0u){
        out.active=true;
        if(contacts&0x200u){v=0;out.vbank=0;}
        if(raw&0x01u){out.code=(uint8_t)(8u+h+phase);out.vbank=4;}
        if(raw&0x10u){
            out.code=(uint8_t)((raw_y>0?0x4cu:0u)+h+phase);out.vbank=0;
            return out;
        }
        if(raw&0x40u){out.code=(uint8_t)(0x10u+v+phase);return out;}
        if(raw&0x04u){out.code=(uint8_t)(0x18u+v+phase);return out;}
        return out;
    }
    if(material){out.code=(uint8_t)(0x20u+h+phase);out.active=true;}
    return out;
}

static unsigned popcount8(uint8_t v){
    unsigned n=0; for(;v;v&=(uint8_t)(v-1u))n++; return n;
}

static uint16_t trigger_be16(const uint8_t *p){return (uint16_t)((p[0]<<8)|p[1]);}

static void initialize_marker_slot(FloodObject *o,uint16_t index,uint8_t state){
    memset(o,0,sizeof(*o));o->active=true;o->state=state;
    o->x=(int16_t)((index%FLOOD_MAP_W)*16u);o->y=(int16_t)((index/FLOOD_MAP_W)*16u);
    o->render_x=o->x;o->render_y=o->y;o->timer=10;
}

static FloodObject *allocate_marker_object(FloodGame *g,uint16_t index,uint8_t state){
    for(unsigned n=0;n<FLOOD_OBJECT_MAX;n++)if(!g->objects[n].active){
        initialize_marker_slot(&g->objects[n],index,state);return &g->objects[n];
    }
    return NULL;
}

/* Marker 1's $E7FE path consumes two consecutive 20-byte runtime records. */
static FloodObject *allocate_marker_pair(FloodGame *g,uint16_t index,
    FloodObject **companion){
    for(unsigned n=0;n+1u<FLOOD_OBJECT_MAX;n++)
        if(!g->objects[n].active&&!g->objects[n+1u].active){
            initialize_marker_slot(&g->objects[n],index,1u);
            initialize_marker_slot(&g->objects[n+1u],index,0u);
            g->objects[n].aux=(uint16_t)(n+1u);
            *companion=&g->objects[n+1u];return &g->objects[n];
        }
    return NULL;
}

static uint16_t flood_random16(FloodGame *g){
    g->rng_state=(uint16_t)((uint32_t)g->rng_state*0x24a1u+0x24dfu);
    return g->rng_state;
}

/* Complete jump-table translation of marker dispatcher $E7B0-$EC08. */
bool flood_initialize_marker(FloodGame *g,uint16_t index,uint8_t marker){
    if(index>=FLOOD_MAP_W*FLOOD_MAP_H)return false;
    FloodObject *o;
    switch(marker){
    case 0:return true;
    case 1:
        {FloodObject *companion;
        o=allocate_marker_pair(g,index,&companion);if(!o)return false;
        o->dx=2;o->dy=4;o->sprite_base=0xe2u;g->world.terrain[index]=0;return true;
        }
    case 2:case 12:case 24:case 25:
        o=allocate_marker_object(g,index,12);if(!o)return false;
        o->sprite_base=marker==2u?0x66u:marker==12u?0x30u:marker==24u?0x6eu:0x38u;
        o->dx=marker==2u||marker==12u?2:4;o->state_flags=marker==25u?0:4;
        g->world.terrain[index]=0;return true;
    case 3:
        memset(&g->world.tile_pixels[3u*FLOOD_TILE_PIXELS],0,FLOOD_TILE_PIXELS);return true;
    case 4:case 6:case 7:case 9:case 10:case 15:
        g->world.terrain[index]=0;return true;
    case 5:case 14:
        o=allocate_marker_object(g,index,marker);if(!o)return false;
        o->dx=4;o->dy=16;g->world.terrain[index]=0;return true;
    case 8:case 13:
        if(!allocate_marker_object(g,index,marker))return false;
        g->world.terrain[index]=0;return true;
    case 11:case 16:case 17:case 18:
        o=allocate_marker_object(g,index,marker);if(!o)return false;
        o->sprite_base=0x4eu;o->origin_x=o->x;o->origin_y=o->y;o->anim=0;
        if(marker==11u||marker==18u){o->aux=0;o->timer=0;o->state_flags=3;g->world.terrain[index]=0xcbu;}
        else{o->timer=(uint16_t)(1u+flood_random16(g)%100u);o->state_flags=0;g->world.terrain[index]=marker==16u?0xc9u:0xcdu;}
        return true;
    case 19:
        o=allocate_marker_object(g,index,19);if(!o)return false;
        g->world.terrain[index]=0xe6u;return true;
    case 20:
        o=allocate_marker_object(g,index,22);if(!o)return false;
        o->origin_x=o->x;o->origin_y=o->y;o->state_flags=0;
        o->sprite_base=0x14u;o->sprite=0x14u;g->world.terrain[index]=0;return true;
    case 21:
        g->world.water_active=true;g->world.water[index]=4;
        g->world.water_source_x=(int16_t)((index%FLOOD_MAP_W)*16u);
        g->world.water_source_y=(int16_t)((index/FLOOD_MAP_W)*16u);return true;
    case 22:
        g->player.x=(int16_t)((index%FLOOD_MAP_W)*16u);
        g->player.y=(int16_t)((index/FLOOD_MAP_W)*16u);
        g->player.anim_phase=0;g->player.hbank=0;g->player.vbank=0;
        g->world.terrain[index]=0;return true;
    case 23:
        g->world.bounds_y=(int16_t)((index/FLOOD_MAP_W)*16u-0xd0);
        if(g->world.bounds_y<0)g->world.bounds_y=0;
        g->world.bounds_x=(int16_t)((index%FLOOD_MAP_W)*16u-0x140);
        if(index>0)g->world.bounds_tile=g->world.terrain[index-1u];
        if(index>1&&g->world.terrain[index-2u]==0x10u)g->world.mechanism_enabled=true;
        g->world.terrain[index]=0;return true;
    default:return false;
    }
}

/* $12DA6/$12E12: matching records are processed from last to first.  Each
   map byte is exchanged with mutable trigger payload, not overwritten by the
   record's final word.  That word is the payload offset. */
unsigned flood_activate_trigger(FloodGame *g,uint16_t event_id,uint16_t mode){
    unsigned changed=0;
    for(unsigned reverse=0;reverse<g->world.trigger_count;reverse++){
        const unsigned record=(unsigned)g->world.trigger_count-1u-reverse;
        uint8_t *r=&g->world.trigger_data[record*10u];
        if(trigger_be16(r)!=event_id)continue;
        const uint16_t map_offset=trigger_be16(r+2),width=trigger_be16(r+4);
        const uint16_t height=trigger_be16(r+6),payload_offset=trigger_be16(r+8);
        unsigned payload=payload_offset;
        for(unsigned y=0;y<height;y++)for(unsigned x=0;x<width;x++){
            const unsigned map=(unsigned)map_offset+y*FLOOD_MAP_W+x;
            if(map>=FLOOD_MAP_W*FLOOD_MAP_H||payload>=FLOOD_TRIGGER_DATA_SIZE)continue;
            const uint8_t old=g->world.terrain[map],next=g->world.trigger_data[payload];
            g->world.terrain[map]=next;g->world.trigger_data[payload]=old;payload++;changed++;
            if(next<26u)flood_initialize_marker(g,(uint16_t)map,next);
        }
    }
    if(g->world.water_active&&mode==0u)flood_rebuild_water(g);
    return changed;
}

/* Binary reconstruction of $F22C.  Ordinary Quiffy uses a 16x16 collision
   core at sprite offset (+8,+8).  The Space Hopper path at $101EA temporarily
   changes only the vertical geometry to offset 0 and height 32. */
static uint16_t build_quiffy_contacts_geometry(FloodGame *g,
    int16_t y_offset,uint16_t height){
    const int16_t x=(int16_t)(g->player.x+8);
    const int16_t y=(int16_t)(g->player.y+y_offset);
    uint16_t mask=0;
    bool slope_correction=false;
    static const int8_t dx[4]={-1,1,1,-1},dy[4]={-1,1,-1,1};
    static const uint8_t xb[4]={6,2,2,6},yb[4]={0,4,0,4},cb[4]={7,3,1,5};
    for(unsigned i=0;i<4;i++){
        FloodCollisionResult q=flood_query_collision(g,(uint16_t)x,(uint16_t)y,
            dx[i],dy[i],16,height);
        /* $F4B2/$F4DE: two diagonal support probes make one-unit horizontal
           corrections for the opposed slope attributes (bits 4 and 5). */
        if(i==0u && (q.y_flags&FLOOD_ATTR_SOLID)){
            if(q.y_flags&0x10u){g->player.raw_x++;slope_correction=true;}
            if(q.y_flags&0x20u){g->player.raw_x--;slope_correction=true;}
        }else if(i==1u && (q.y_flags&FLOOD_ATTR_SOLID)){
            if(q.y_flags&0x10u){g->player.raw_x--;slope_correction=true;}
            if(q.y_flags&0x20u){g->player.raw_x++;slope_correction=true;}
        }
        if(q.x_flags&FLOOD_ATTR_FATAL)mask|=0x100u;
        if(q.x_flags&0x02u){mask|=(uint16_t)(1u<<xb[i]);if(q.x_flags&0x80u)mask|=0x200u;}
        if(q.y_flags&FLOOD_ATTR_FATAL)mask|=0x100u;
        if(q.y_flags&0x02u){mask|=(uint16_t)(1u<<yb[i]);if(q.y_flags&0x80u)mask|=0x200u;}
        if(q.corner_flags&0x02u){mask|=(uint16_t)(1u<<cb[i]);if(q.corner_flags&0x80u)mask|=0x200u;}
    }
    g->player.raw_contact_mask=(uint8_t)mask;
    /* $F420-$F476: attribute bit 7 requests a straight-down confirmation and
       converts it to the complete S/SE/SW support triplet. */
    if(mask&0x200u){
        mask=0;
        FloodCollisionResult q=flood_query_collision(g,(uint16_t)x,(uint16_t)y,
            0,1,16,height);
        if(q.y_flags&FLOOD_ATTR_SOLID)mask|=0x38u;
    }
    g->player.contact_mask=(uint8_t)mask;
    g->player.contact_count=(uint8_t)popcount8((uint8_t)mask);
    g->player.contact_aux=(uint16_t)(mask&0x300u);
    /* $F482-$F4A2: the correction scrape is edge-triggered from the two
       persistent latch words at $F228/$F22A. */
    if(slope_correction&&!g->slope_correction_previous)request_sound(g,22u);
    g->slope_correction_previous=slope_correction;
    return mask;
}

uint16_t flood_build_quiffy_contacts(FloodGame *g){
    return build_quiffy_contacts_geometry(g,8,16);
}

/* $D99E-$DAEA: cardinal priority is S,N,E,W.  At an exposed end of a
   cardinal surface, the original enters the attachment cache only when the
   tangent points toward that end; otherwise it returns without attaching. */
void flood_resolve_quiffy_contacts(FloodPlayer *p,uint8_t c){
    int8_t sx=p->raw_x,sy=p->raw_y;
    if(c&0x10u){
        if(sy<=0)return;
        if(!(c&0x20u)){if(sx>=0)return;sx=-1;}
        if(!(c&0x08u)){if(sx<=0)return;sx=1;}
        sy=1;p->attachment_mode=1;
    }else if(c&0x01u){
        if(sy>=0)return;
        if(!(c&0x80u)){if(sx>=0)return;sx=-1;}
        if(!(c&0x02u)){if(sx<=0)return;sx=1;}
        sy=-1;p->attachment_mode=1;
    }else if(c&0x04u){
        if(sx<0)return;
        if(!(c&0x02u)){if(sy>=0)return;sy=-1;}
        if(!(c&0x08u)){if(sy<=0)return;sy=1;}
        sx=1;p->attachment_mode=2;
    }else if(c&0x40u){
        if(sx>0)return;
        if(!(c&0x80u)){if(sy>=0)return;sy=-1;}
        if(!(c&0x20u)){if(sy<=0)return;sy=1;}
        sx=-1;p->attachment_mode=2;
    }else return;
    p->surface_x=sx;p->surface_y=sy;
}

/* $DAEC-$DBE6: maintain the original ten-update attachment cache.  The
   captured directions are deliberately not refreshed from current input.
   While the attachment remains active they are copied back to the live
   direction words before $D99E resolves the new contact geometry.  That
   ordering is what carries Quiffy through a convex corner. */
void flood_maintain_quiffy_attachment(FloodPlayer *p){
    if(p->attachment_mode==1){
        if(p->contact_count<=1)p->surface_x=0;
        if(p->surface_x==0 && p->contact_count>=2)p->attachment_mode=0;
        /* $DB4A is reached even when the test above just cleared mode 1. */
        p->raw_x=p->surface_x;
        p->raw_y=p->surface_y;
    }else if(p->attachment_mode==2){
        p->y=(int16_t)(p->y+(p->y&3));
        if(p->contact_count<=1)p->surface_y=0;
        if(p->surface_y==0 && p->contact_count>=2)
            p->attachment_mode=0;
        else{
            p->raw_x=p->surface_x;
            p->raw_y=p->surface_y;
        }
    }
    if(++p->attachment_ticks==10)p->attachment_mode=0;
    if(!p->attachment_mode)p->attachment_ticks=0;
}

void flood_game_init(FloodGame *g){
    memset(g,0,sizeof(*g)); g->running=true; g->lives=3;
    g->player.x=64;g->player.y=64;g->player.life_force=511;g->player.air=63;
    g->world.water_speed=1;g->player.surface_y=1;g->player.pose_code=0u;
    g->matilda.read_pos=1u;
    g->last_pickup_tile=UINT16_MAX;
    g->last_special_tile=UINT16_MAX;
}
void flood_matilda_add_death_delay(FloodGame *g){g->matilda.hold=20;}

static void update_matilda(FloodGame *g){
    FloodMatilda *m=&g->matilda;
    /* $CEE0-$CFFE uses byte offsets into 256 four-byte records.  The writer
       advances before the old reader sample is consumed, and catching the
       reader cancels a death hold before that sample is drawn. */
    m->write_pos=(m->write_pos+1u)&255u;
    if(m->read_pos==m->write_pos)m->hold=0u;
    FloodHistorySample old=m->history[m->read_pos];
    m->visible=(old.x||old.y);m->x=old.x;m->y=old.y;m->sprite_id=(uint16_t)(0xD2+old.frame);
    if(m->visible&&m->x<g->player.x+16&&m->x+16>g->player.x&&m->y<g->player.y+16&&m->y+16>g->player.y)g->player.life_force-=10;
    if(m->hold)m->hold--; else m->read_pos=(m->read_pos+1u)&255u;
    /* $CF9C-$CFC6 runs even while the ordinary reader advance is held.  At
       each 32-record boundary it consumes one extra sample unless only the
       original eight-record safety gap remains. */
    if((m->read_pos&31u)==0u){
        const unsigned stop=(m->write_pos-8u)&255u;
        if(m->read_pos!=stop)m->read_pos=(m->read_pos+1u)&255u;
    }
    m->history[m->write_pos]=(FloodHistorySample){g->player.x,g->player.y,(uint8_t)((g->player.anim_phase&3)+g->player.hbank)};
}

static bool rectangles_overlap(int x0,int y0,int w0,int h0,int x1,int y1,int w1,int h1){
    return x0<x1+w1&&x0+w0>x1&&y0<y1+h1&&y0+h0>y1;
}

static uint8_t object_frame_step(const FloodGame *g){
    /* $17E86 alternates 1,0 at the end of successive gameplay frames. */
    return (uint8_t)((g->ticks&1u)==0u);
}

static uint8_t *hud_fixed_decimal(uint8_t *out,uint32_t value,unsigned digits){
    uint32_t divisor=1u;
    for(unsigned n=1;n<digits;n++)divisor*=10u;
    for(unsigned n=0;n<digits;n++){
        uint8_t digit='0';
        while(value>=divisor){value-=divisor;digit++;}
        *out++=digit;if(divisor>1u)divisor/=10u;
    }
    *out=0u;return out;
}

static void hud_copy_glyph(FloodGame *g,unsigned glyph,unsigned column){
    if(glyph>=FLOOD_HUD_GLYPH_COUNT||column>=FLOOD_HUD_ROW_BYTES)return;
    for(unsigned y=0;y<FLOOD_HUD_H;y++)
        g->hud_mask[y*FLOOD_HUD_ROW_BYTES+column]=g->hud_glyphs[glyph*8u+y];
}

static void hud_gauge(FloodGame *g,unsigned value,unsigned first_column){
    for(unsigned y=0;y<FLOOD_HUD_H;y++)
        memset(&g->hud_mask[y*FLOOD_HUD_ROW_BYTES+first_column],0,4u);
    unsigned remaining=value;
    for(unsigned segment=0;segment<4u;segment++){
        const unsigned threshold=24u-segment*8u;
        const unsigned part=remaining>threshold?remaining-threshold:0u;
        remaining-=part;hud_copy_glyph(g,0x67u+part,first_column+3u-segment);
    }
}

/* $14388-$14552 builds the original 320x8 one-bit status strip. */
void flood_update_hud(FloodGame *g){
    memcpy(g->hud_mask,g->hud_template,sizeof(g->hud_mask));
    uint8_t *cursor=g->hud_digits;
    cursor=hud_fixed_decimal(cursor,g->score,5u);
    cursor=hud_fixed_decimal(cursor,g->world.remaining_food<0?0u:
        (uint16_t)g->world.remaining_food,2u);
    hud_fixed_decimal(cursor,g->lives<0?0u:(uint16_t)g->lives,2u);
    for(unsigned n=0;n<5u;n++)hud_copy_glyph(g,g->hud_digits[n]-0x14u,11u+n);
    for(unsigned n=0;n<2u;n++)hud_copy_glyph(g,g->hud_digits[5u+n]-0x14u,23u+n);
    for(unsigned n=0;n<2u;n++)hud_copy_glyph(g,g->hud_digits[7u+n]-0x14u,33u+n);
    /* Negative counters are terminal states, not a request to retain the
       template artwork.  The template's gauge bytes are filled, so skipping
       either draw made an exhausted meter appear full again. */
    hud_gauge(g,g->player.life_force>0?(uint16_t)g->player.life_force>>4:0u,36u);
    hud_gauge(g,g->player.air>0?(uint16_t)g->player.air>>1:0u,0u);
}

/* $A87E-$A92E and $1455E-$145E4.  The status mask is written into the
   352-pixel backing row at the camera phase.  The complementary dual-playfield
   delay makes its fetch position invariant at X=16, the first pixel of the
   320-pixel visible window. */
FloodHudPlacement flood_hud_placement(uint16_t camera_x){
    FloodHudPlacement p;
    p.camera_phase=(uint8_t)(camera_x&15u);
    p.bplcon1_shift=(uint8_t)((16u-p.camera_phase)&15u);
    p.bplcon1=(uint8_t)(p.bplcon1_shift|(p.bplcon1_shift<<4));
    p.compositor_base_bias=(int16_t)(p.camera_phase?-704:-702);
    p.frame_byte_offset=(uint8_t)(p.camera_phase?0u:2u);
    p.backing_x=(uint8_t)(p.frame_byte_offset*8u+p.camera_phase);
    p.fetch_x=(uint8_t)(p.backing_x+p.bplcon1_shift);
    p.visible_x=(uint8_t)(p.fetch_x-FLOOD_HUD_FETCH_X);
    return p;
}

/* $D94A-$D990: Quiffy is held at (152,92) until a cavern edge forces an
   independent signed clamp on either camera axis. */
void flood_update_camera(FloodGame *g){
    int16_t x=(int16_t)(g->player.x-152);
    int16_t y=(int16_t)(g->player.y-92);
    if(x<=0)x=0;
    if(x>=g->world.bounds_x)x=g->world.bounds_x;
    if(y<=0)y=0;
    if(y>=g->world.bounds_y)y=g->world.bounds_y;
    g->camera_x=x;g->camera_y=y;
}

/* $ECB4-$ECF2: the tile blitter substitutes three animated families before
   looking up the 128-byte planar tile record. */
uint8_t flood_render_tile_id(const FloodGame *g,uint8_t tile){
    if(tile==0x1du)return (uint8_t)(tile+g->render_tile_phase);
    if(tile==0x20u)return (uint8_t)(0x1du+3u-g->render_tile_phase);
    if(tile==0x47u)return (uint8_t)(tile+g->render_buffer_index);
    return tile;
}

/* $A860-$A92E and $EC4E-$ED64: 22x14 tile overfetch from the coarse camera
   cell, followed by the fine-phase crop to the 320x208 display window. */
static void build_tile_region(const FloodGame *g,uint8_t *out,unsigned width,int x_bias){
    memset(out,0,(size_t)width*FLOOD_VIEW_H);
    const unsigned coarse_x=(uint16_t)g->camera_x>>4;
    const unsigned coarse_y=(uint16_t)g->camera_y>>4;
    const int phase_x=g->camera_x&15;
    const int phase_y=g->camera_y&15;
    for(unsigned row=0;row<FLOOD_TILE_OVERFETCH_ROWS;row++){
        const unsigned map_y=coarse_y+row;
        if(map_y>=FLOOD_MAP_H)continue;
        for(unsigned column=0;column<FLOOD_TILE_OVERFETCH_COLS;column++){
            const unsigned map_x=coarse_x+column;
            if(map_x>=FLOOD_MAP_W)continue;
            const uint8_t *terrain=g->world.render_terrain_valid?
                g->world.render_terrain:g->world.terrain;
            const uint8_t tile=flood_render_tile_id(g,
                terrain[map_y*FLOOD_MAP_W+map_x]);
            const uint8_t *pixels=&g->world.tile_pixels[(unsigned)tile*FLOOD_TILE_PIXELS];
            const int destination_x=(int)column*16-phase_x+x_bias;
            const int destination_y=(int)row*16-phase_y;
            for(unsigned py=0;py<16u;py++){
                const int dy=destination_y+(int)py;
                if(dy<0||dy>=FLOOD_VIEW_H)continue;
                for(unsigned px=0;px<16u;px++){
                    const int dx=destination_x+(int)px;
                    if(dx>=0&&(unsigned)dx<width)
                        out[(unsigned)dy*width+(unsigned)dx]=pixels[py*16u+px];
                }
            }
        }
    }
}

void flood_build_tile_window(const FloodGame *g,uint8_t out[FLOOD_VIEW_PIXELS]){
    build_tile_region(g,out,FLOOD_VIEW_W,0);
}

void flood_build_tile_backing(const FloodGame *g,uint8_t out[FLOOD_BACKING_PIXELS]){
    build_tile_region(g,out,FLOOD_BACKING_W,FLOOD_BACKING_X);
}

/* $ED66-$EE48: the secondary 128x100 state map selects one of 42 two-plane
   W_BLOCKS records.  BLTCON0=$0DFC ORs those source bits into display planes
   2 and 3, preserving the terrain's two low colour bits. */
static void composite_overlay_region(const FloodGame *g,uint8_t *out,unsigned width,int x_bias){
    if(!g->world.water_active)return;
    const unsigned coarse_x=(uint16_t)g->camera_x>>4;
    const unsigned coarse_y=(uint16_t)g->camera_y>>4;
    const int phase_x=g->camera_x&15;
    const int phase_y=g->camera_y&15;
    const unsigned bank=g->render_buffer_index&1u;
    for(unsigned row=0;row<FLOOD_TILE_OVERFETCH_ROWS;row++){
        const unsigned map_y=coarse_y+row;
        if(map_y>=FLOOD_MAP_H)continue;
        for(unsigned column=0;column<FLOOD_TILE_OVERFETCH_COLS;column++){
            const unsigned map_x=coarse_x+column;
            if(map_x>=FLOOD_MAP_W)continue;
            const uint8_t state=g->world.render_water[map_y*FLOOD_MAP_W+map_x];
            if(state==0u||state>=FLOOD_OVERLAY_TILE_COUNT)continue;
            const uint8_t *pixels=&g->world.overlay_pixels[
                (bank*FLOOD_OVERLAY_TILE_COUNT+(unsigned)state)*FLOOD_OVERLAY_TILE_PIXELS];
            const int destination_x=(int)column*16-phase_x+x_bias;
            const int destination_y=(int)row*16-phase_y;
            for(unsigned py=0;py<16u;py++){
                const int dy=destination_y+(int)py;
                if(dy<0||dy>=FLOOD_VIEW_H)continue;
                for(unsigned px=0;px<16u;px++){
                    const int dx=destination_x+(int)px;
                    if(dx>=0&&(unsigned)dx<width)
                        out[(unsigned)dy*width+(unsigned)dx]|=(uint8_t)(pixels[py*16u+px]<<2);
                }
            }
        }
    }
}

void flood_composite_overlay_window(const FloodGame *g,uint8_t out[FLOOD_VIEW_PIXELS]){
    composite_overlay_region(g,out,FLOOD_VIEW_W,0);
}

void flood_composite_overlay_backing(const FloodGame *g,uint8_t out[FLOOD_BACKING_PIXELS]){
    composite_overlay_region(g,out,FLOOD_BACKING_W,FLOOD_BACKING_X);
}

/* $13FC4-$14198 applies the same horizontal mask and vertical duplication to
   each of four separate $2940-byte bitplanes.  Consequently every mosaic
   stage preserves the full colour index while growing its sample block from
   2x2 through 32x32 pixels. */
uint8_t flood_transport_effect_pixel(const uint8_t backing[FLOOD_BACKING_PIXELS],
    unsigned x,unsigned y,unsigned stage){
    if(x>=FLOOD_VIEW_W||y>=FLOOD_VIEW_H)return 0u;
    unsigned source_x=x+FLOOD_BACKING_X,source_y=y;
    if(stage>5u)stage=5u;
    if(stage){
        source_x&=~((1u<<stage)-1u);
        source_y&=~((1u<<stage)-1u);
    }
    return backing[source_y*FLOOD_BACKING_W+source_x];
}

uint16_t flood_fade_colour(uint16_t colour,unsigned steps){
    uint16_t out=0u;if(steps>15u)steps=15u;
    for(unsigned shift=0;shift<=8u;shift+=4u){
        const unsigned component=(colour>>shift)&15u;
        if(component>steps)out|=(uint16_t)((component-steps)<<shift);
    }
    return out;
}

/* The original 20-byte object record stores independent bytes at +8 and +9,
   but some handlers also access them together as a big-endian word. */
static bool object_word8_nonzero(const FloodObject *o){
    return o->state_flags!=0u||o->anim!=0u;
}

static void object_set_word8(FloodObject *o,uint16_t value){
    o->state_flags=(uint8_t)(value>>8);o->anim=(uint8_t)value;
}

static bool map_index_for_point(int16_t x,int16_t y,uint16_t *index){
    if(x<0||y<0||x>=FLOOD_MAP_W*16||y>=FLOOD_MAP_H*16)return false;
    *index=flood_tile_index_at(x,y);return true;
}

static bool state12_hits_player(const FloodGame *g,int16_t x,int16_t y,
    int width,int height){
    return rectangles_overlap(x,y,width,height,
        g->player.x+8,g->player.y+8,16,16);
}

/* $10D5E: collision wrapper shared by the $30/$66 State-12 walkers. */
static uint8_t state12_walker_contacts(const FloodGame *g,int16_t x,int16_t y,
    int16_t dx,int16_t *dy){
    FloodCollisionResult q=flood_query_collision(g,(uint16_t)x,(uint16_t)y,
        dx,*dy,32,32);
    uint8_t contacts=0;
    if(q.y_flags&FLOOD_ATTR_SOLID)*dy=0;
    if(q.x_flags&FLOOD_ATTR_SOLID)contacts|=(uint8_t)(dx<0?0x40u:0x04u);
    if(q.corner_flags&FLOOD_ATTR_SOLID)contacts|=(uint8_t)(dx<0?0x20u:0x08u);
    return contacts;
}

/* $13BA8 and $13C0A use the cell at object position (+16,+16). */
static void state12_teddy_eat(FloodGame *g,int16_t x,int16_t y){
    uint16_t index;if(!map_index_for_point((int16_t)(x+16),(int16_t)(y+16),&index))return;
    const uint8_t tile=g->world.terrain[index];
    if(tile==0x94u||(tile>=0x24u&&tile<=0x27u)){
        g->world.terrain[index]=0;
        g->world.remaining_food--;
    }
}

static int16_t random_signed_remainder(FloodGame *g,int16_t divisor){
    return (int16_t)((int16_t)flood_random16(g)%divisor);
}

static void state12_vong_spawn(FloodGame *g,int16_t x,int16_t y){
    static const uint8_t food[5]={0x94u,0x24u,0x25u,0x26u,0x27u};
    uint16_t index;if(!map_index_for_point((int16_t)(x+16),(int16_t)(y+16),&index))return;
    if(g->world.terrain[index]!=0)return;
    const int16_t choice=random_signed_remainder(g,5);
    if(choice>=0&&choice<5){g->world.terrain[index]=food[choice];g->world.remaining_food++;}
}

static void update_state12_walker(FloodGame *g,FloodObject *o,uint8_t step){
    int16_t x=o->x,y=o->y,dx=o->dx,dy=(int16_t)(o->dy+1);
    const uint8_t phase=(uint8_t)((o->anim+step)&3u);o->anim=phase;
    const uint8_t contacts=state12_walker_contacts(g,x,y,dx,&dy);
    if(dx<0){
        if(!((contacts&0x20u)&&!(contacts&0x40u))){dx=(int16_t)-dx;o->state_flags=4;}
    }else if(!((contacts&0x08u)&&!(contacts&0x04u))){
        dx=(int16_t)-dx;o->state_flags=0;
    }
    o->sprite=(uint8_t)(0x50u+o->sprite_base+phase+o->state_flags);
    o->render_x=x;o->render_y=(int16_t)(y+2);
    x=(int16_t)(x+dx);y=(int16_t)(y+dy);
    o->x=x;o->y=y;o->dx=dx;o->dy=dy;
    if(state12_hits_player(g,x,y,32,32))g->player.life_force=(int16_t)(g->player.life_force-16);
}

static void update_state12_teddy(FloodGame *g,FloodObject *o,uint8_t step){
    int16_t x=o->x,y=o->y,dx=o->dx,dy=(int16_t)(o->dy+2);
    if(dy>16)dy=16;
    const uint8_t phase=(uint8_t)((o->anim+step)&3u);o->anim=phase;
    FloodCollisionResult q=flood_query_collision(g,(uint16_t)(x+8),
        (uint16_t)(y+16),dx,dy,16,16);
    if(q.y_flags&FLOOD_ATTR_SOLID){
        if(!(q.corner_flags&FLOOD_ATTR_SOLID)&&dy>=0)dx=(int16_t)-dx;
        dy=0;
    }
    if(q.x_flags&FLOOD_ATTR_SOLID){
        if(o->state_flags&&dy>=0&&(q.y_flags&FLOOD_ATTR_SOLID))o->state_flags++;
        if(o->state_flags<=1u){
            if(o->state_flags==0u){dy=-16;o->state_flags=1;}
            dx=0;
        }else{
            if(o->state_flags<=2u)dx=(int16_t)-dx;
            o->state_flags=0;
        }
    }
    if(state12_hits_player(g,(int16_t)(x+4),(int16_t)(y+8),24,24))
        g->player.life_force=-1;
    if(g->world.remaining_food>0)state12_teddy_eat(g,x,y);
    if(!(flood_tile_attr_at(g,(int16_t)(x+16),(int16_t)(y+28))&FLOOD_ATTR_SOLID)){
        x=(int16_t)(x+dx);y=(int16_t)(y+dy);o->x=x;o->y=y;
        if(dx)o->dx=dx;
    }
    o->dy=dy;
    const int16_t bank=(int16_t)((o->dx>>1)+2);
    o->sprite=(uint8_t)(0x50u+o->sprite_base+phase+bank);
    o->render_x=x;o->render_y=y;
}

static void update_state12_vong(FloodGame *g,FloodObject *o,uint8_t step){
    int16_t x=o->x,y=o->y,dx=o->dx,dy=(int16_t)(o->dy+2);
    if(dy>16)dy=16;
    const uint8_t phase=(uint8_t)((o->anim+step)&3u);o->anim=phase;
    FloodCollisionResult q=flood_query_collision(g,(uint16_t)(x+4),(uint16_t)y,
        dx,1,24,32);
    if(q.y_flags&FLOOD_ATTR_SOLID){
        dy=0;
        if(g->world.remaining_food>0&&g->world.remaining_food<49&&
            random_signed_remainder(g,9)==0)state12_vong_spawn(g,x,y);
    }
    q=flood_query_collision(g,(uint16_t)(x+4),(uint16_t)y,dx,dy,24,32);
    if(q.y_flags&FLOOD_ATTR_SOLID)dy=0;
    if(q.x_flags&FLOOD_ATTR_SOLID){
        o->dx=(int16_t)-o->dx;
        o->state_flags=(uint8_t)(-(int8_t)o->state_flags+4);
    }
    o->sprite=(uint8_t)(0x50u+o->sprite_base+phase+o->state_flags);
    o->render_x=x;o->render_y=y;
    x=(int16_t)(x+dx);y=(int16_t)(y+dy);o->x=x;o->y=y;o->dy=dy;
    if(state12_hits_player(g,(int16_t)(x+4),y,24,32))g->player.life_force=-1;
}

/* Complete State-12 dispatcher: $10A92 plus subtype routines $10B86,
   $13A4C, and $13CA0. */
void flood_update_state12(FloodGame *g){
    const uint8_t step=object_frame_step(g);
    for(unsigned n=0;n<FLOOD_OBJECT_MAX;n++){
        FloodObject *o=&g->objects[n];if(!o->active||o->state!=12u)continue;
        if(o->sprite_base==0x38u)update_state12_teddy(g,o,step);
        else if(o->sprite_base==0x6eu)update_state12_vong(g,o,step);
        else update_state12_walker(g,o,step);
    }
}

/* $10C46-$10D5C: Plonkin Donkin (runtime state 14).  Byte/word +8 selects
   the initial deceleration phase (zero) or ordinary gravity (nonzero).
   Rendering precedes the position commit and uses the sprite's recovered
   (-6,-14) registration offset. */
static void update_state14_object(FloodGame *g,FloodObject *o,uint8_t step){
        int16_t x=o->x,y=o->y,dx=o->dx,dy=o->dy;
        if(o->state_flags)dy=(int16_t)(dy+1);
        else dy=(int16_t)(dy-1);
        if(dy>16)dy=16;
        if(dy<-12){dy=-12;o->state_flags=1;}

        FloodCollisionResult q=flood_query_collision(g,(uint16_t)x,
            (uint16_t)y,dx,dy,16,16);
        if(q.x_flags&FLOOD_ATTR_SOLID)dx=(int16_t)-dx;

        /* $F50A samples the current centre cell.  A solid centre suppresses
           the whole velocity/position commit, but animation and contact
           damage still run. */
        if(!(flood_tile_attr_at(g,(int16_t)(x+8),(int16_t)(y+8))&FLOOD_ATTR_SOLID)){
            if(q.y_flags&FLOOD_ATTR_SOLID){
                if(dy<0)dy=(int16_t)-dy;
                else{
                    dy=(int16_t)(dy>>1);
                    if(dy<=2)dy=12;
                    dy=(int16_t)-dy;
                    o->state_flags=1;
                }
            }
            x=(int16_t)(x+dx);y=(int16_t)(y+dy);
            o->x=x;o->y=y;o->dx=dx;o->dy=dy;
        }

        o->anim=(uint8_t)((o->anim+step)&3u);
        o->sprite=(uint8_t)(0xdeu+o->anim);
        o->render_x=(int16_t)(x-6);o->render_y=(int16_t)(y-14);
        if(state12_hits_player(g,x,y,16,16))
            g->player.life_force=(int16_t)(g->player.life_force-8);
}

void flood_update_state14(FloodGame *g){
    const uint8_t step=object_frame_step(g);
    for(unsigned n=0;n<FLOOD_OBJECT_MAX;n++){
        FloodObject *o=&g->objects[n];if(!o->active||o->state!=14u)continue;
        update_state14_object(g,o,step);
    }
}

static void update_doctor_dusty(FloodGame *g,FloodObject *o,uint8_t step){
    int16_t x=o->x,y=o->y,dx=o->dx,dy=(int16_t)(o->dy+1);
    o->anim=(uint8_t)((o->anim+step)&3u);
    const uint8_t contacts=state12_walker_contacts(g,x,y,dx,&dy);
    if(dx<0){
        if(!((contacts&0x20u)&&!(contacts&0x40u))){dx=(int16_t)-dx;o->state_flags=4u;}
    }else if(!((contacts&0x08u)&&!(contacts&0x04u))){
        dx=(int16_t)-dx;o->state_flags=0u;
    }
    o->sprite=(uint8_t)(0xe2u+o->anim);o->render_x=x;o->render_y=y;
    x=(int16_t)(x+dx);y=(int16_t)(y+dy);
    o->x=x;o->y=y;o->dx=dx;o->dy=dy;
    if(state12_hits_player(g,x,y,32,32))
        g->player.life_force=(int16_t)(g->player.life_force-16);

    if(o->aux<FLOOD_OBJECT_MAX){
        FloodObject *dust=&g->objects[o->aux];
        if(dust->active&&dust->state==0u){
            dust->state=10u;dust->dy=0;dust->anim=20u;
            dust->x=(int16_t)(x+12);dust->y=y;dust->sprite=0;
        }
    }
}

/* $1385A-$13922: the 20-update object emitted into Doctor Dusty's reserved
   adjacent slot.  It oscillates vertically and then becomes state 9. */
static void update_doctor_dust(FloodGame *g,FloodObject *o){
    int16_t countdown=(int16_t)(uint8_t)o->anim-1;
    o->anim=(uint8_t)countdown;
    if(countdown<0){
        o->y=(int16_t)(o->y-12);o->state=9u;o->anim=0;o->sprite=0;return;
    }
    int16_t x=o->x,y=o->y,dy=o->dy;
    /* $13890 TST.W reads the complete +8 word.  The countdown byte at +9
       therefore makes a newly spawned stick accelerate downward even while
       the direction byte at +8 is still zero. */
    if(object_word8_nonzero(o))dy=(int16_t)(dy+1);else dy=(int16_t)(dy-1);
    if(dy>8)dy=8;
    if(dy<-8){dy=-8;o->state_flags=1u;}
    FloodCollisionResult q=flood_query_collision(g,(uint16_t)(x+8),
        (uint16_t)(y+12),0,dy,1,8);
    if(q.y_flags&FLOOD_ATTR_SOLID)dy=0;
    y=(int16_t)(y+dy);o->x=x;o->y=y;o->dx=0;o->dy=dy;
    o->sprite=(uint8_t)(0x28u+((uint16_t)countdown&3u));
    o->render_x=(int16_t)(x+4);o->render_y=(int16_t)(y+4);
}

/* $10A10-$10A90: the shared six-update State 6/9/15 burst.  State 9 is
   reached by Doctor Dusty's companion; States 6 and 15 are dormant. */
static void update_shared_burst(FloodGame *g,FloodObject *o){
    uint16_t phase=o->anim;
    if((uint8_t)phase==0u)request_sound(g,7u);
    phase++;o->anim=(uint8_t)phase;
    if(phase==7u){o->state=0u;o->aux=0;o->anim=0;o->sprite=0;return;}
    const uint16_t frame=phase>>1;
    o->sprite=(uint8_t)(0x98u+frame);o->render_x=o->x;o->render_y=o->y;
    if(frame==0u&&state12_hits_player(g,o->x,o->y,32,32))
        g->player.life_force=(int16_t)(g->player.life_force-40);
}

void flood_update_states_6_9_15(FloodGame *g){
    for(unsigned n=0;n<FLOOD_OBJECT_MAX;n++){
        FloodObject *o=&g->objects[n];if(!o->active)continue;
        if(o->state==6u||o->state==9u||o->state==15u)update_shared_burst(g,o);
    }
}

/* State 1 plus its exact State-10 companion lifecycle.  Iterating in
   slot order preserves the original same-frame spawn into the next record. */
void flood_update_state1_chain(FloodGame *g){
    const uint8_t step=object_frame_step(g);
    for(unsigned n=0;n<FLOOD_OBJECT_MAX;n++){
        FloodObject *o=&g->objects[n];if(!o->active)continue;
        /* State 1 can activate the following slot during this loop, after the
           global prepass has already visited it.  Apply the dispatcher's y
           guard here as well for that exact same-pass companion case. */
        if(o->y>=1600){o->state=0u;continue;}
        if(o->state==1u)update_doctor_dusty(g,o,step);
        else if(o->state==10u)update_doctor_dust(g,o);
    }
}

static uint8_t state8_direction_toward_player(const FloodGame *g,
    int16_t x,int16_t y){
    const int sx=x<g->player.x?1:x>g->player.x?-1:0;
    const int sy=y<g->player.y?1:y>g->player.y?-1:0;
    static const uint8_t map[3][3]={{7u,0u,1u},{6u,0u,2u},{5u,4u,3u}};
    return map[sy+1][sx+1];
}

static void state8_choose_velocity(FloodGame *g,FloodObject *o,
    int16_t x,int16_t y){
    static const int8_t vectors[8][2]={
        {0,-1},{1,-1},{1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1}
    };
    const uint16_t random_mode=(uint16_t)(flood_random16(g)&1u);
    o->aux=random_mode;
    const uint8_t direction=random_mode?
        (uint8_t)(flood_random16(g)&7u):state8_direction_toward_player(g,x,y);
    o->state_flags=direction;
    o->dx=(int16_t)(vectors[direction][0]*4);
    o->dy=(int16_t)(vectors[direction][1]*4);
}

/* $10834-$10918: Vacuous Gombo.  The collision body starts at x+4 and is
   24x32.  Blocked components affect the local motion only; stored velocity is
   replaced solely when both components are stopped and $12990 chooses a new
   eight-way heading. */
static void update_state8_object(FloodGame *g,FloodObject *o,uint8_t step){
        int16_t x=o->x,y=o->y,dx=o->dx,dy=o->dy;
        FloodCollisionResult q=flood_query_collision(g,(uint16_t)(x+4),
            (uint16_t)y,dx,dy,24,32);
        if(q.x_flags&FLOOD_ATTR_SOLID)dx=0;
        if(q.y_flags&FLOOD_ATTR_SOLID)dy=0;
        if(dx==0&&dy==0){
            state8_choose_velocity(g,o,x,y);
            dx=0;dy=0;
        }
        o->anim=(uint8_t)((o->anim+step)&3u);
        x=(int16_t)(x+dx);y=(int16_t)(y+dy);o->x=x;o->y=y;
        if(state12_hits_player(g,(int16_t)(x+4),y,24,32))
            g->player.life_force=(int16_t)(g->player.life_force-8);
        o->sprite=(uint8_t)(0x90u+o->anim);o->render_x=x;o->render_y=y;
}

void flood_update_state8(FloodGame *g){
    const uint8_t step=object_frame_step(g);
    for(unsigned n=0;n<FLOOD_OBJECT_MAX;n++){
        FloodObject *o=&g->objects[n];if(!o->active||o->state!=8u)continue;
        update_state8_object(g,o,step);
    }
}

/* State 5 Beady Ball, exact control flow from $13972-$13A4A. */
static void update_state5_object(FloodGame *g,FloodObject *o){
        int16_t x=o->x,y=o->y,dx=o->dx,dy=o->dy;
        dy=(int16_t)(dy+(object_word8_nonzero(o)?1:-1));
        if(dy>16)dy=16;
        if(dy<-12){dy=-12;object_set_word8(o,1u);}
        FloodCollisionResult q=flood_query_collision(g,(uint16_t)x,
            (uint16_t)y,dx,dy,32,32);
        if(q.x_flags&FLOOD_ATTR_SOLID)dx=(int16_t)-dx;
        if(q.y_flags&FLOOD_ATTR_SOLID){
            if(dy>=0){
                dy=(int16_t)((uint16_t)dy>>1);
                if(dy<=2)dy=12;
                dy=(int16_t)-dy;object_set_word8(o,1u);
            }else dy=(int16_t)-dy;
        }
        x=(int16_t)(x+dx);y=(int16_t)(y+dy);
        o->x=x;o->y=y;o->dx=dx;o->dy=dy;
        o->sprite=0xaeu;o->render_x=x;o->render_y=y;
        if(rectangles_overlap(x,y,32,32,g->player.x+8,g->player.y+8,16,16))
            g->player.life_force=(int16_t)(g->player.life_force-8);
}

void flood_update_state5(FloodGame *g){
    for(unsigned n=0;n<FLOOD_OBJECT_MAX;n++){
        FloodObject *o=&g->objects[n];if(!o->active||o->state!=5u)continue;
        update_state5_object(g,o);
    }
}

static void mechanism_sound(FloodGame *g,uint8_t sound){
    request_sound(g,sound);
}

static bool mechanism_visible(const FloodGame *g,const FloodObject *o){
    const int camx=g->camera_x,camy=g->camera_y;
    return o->x>=camx-32&&o->x<=camx+384&&
           o->y>=camy-32&&o->y<=camy+272;
}

static bool mechanism_cell(int16_t x,int16_t y,uint16_t *index){
    return map_index_for_point(x,y,index);
}

static int16_t mechanism_place_horizontal(FloodGame *g,FloodObject *o,
    int16_t candidate,int16_t y,int16_t previous,unsigned *blocked){
    if(state12_hits_player(g,candidate,y,16,16)){
        g->player.life_force=(int16_t)(g->player.life_force-16);
        *blocked=13u;candidate=previous;
    }
    uint16_t index;
    if(!mechanism_cell(candidate,y,&index)){candidate=previous;return candidate;}
    const uint8_t tile=g->world.terrain[index];
    if(tile!=0u){
        candidate=previous;
        if(g->world.mechanism_enabled&&tile==0xffu)
            (void)flood_activate_trigger(g,index,0u);
    }else g->world.terrain[index]=(uint8_t)(0xc8u+(o->state==11u?4u:0u));
    return candidate;
}

static int16_t mechanism_place_vertical(FloodGame *g,FloodObject *o,
    int16_t x,int16_t candidate,int16_t previous,unsigned *blocked){
    if(state12_hits_player(g,x,candidate,16,16)){
        g->player.life_force=(int16_t)(g->player.life_force-16);
        *blocked=13u;candidate=previous;
    }
    uint16_t index;
    if(!mechanism_cell(x,candidate,&index)){candidate=previous;return candidate;}
    const uint8_t tile=g->world.terrain[index];
    if(tile!=0u){
        candidate=previous;
        if(g->world.mechanism_enabled&&tile==0xffu)
            (void)flood_activate_trigger(g,index,0u);
    }else g->world.terrain[index]=(uint8_t)(0xcau+(o->state==18u?2u:0u));
    return candidate;
}

static int16_t mechanism_remove_horizontal(FloodGame *g,int16_t candidate,
    int16_t y,int16_t previous){
    uint16_t index;
    if(!mechanism_cell(candidate,y,&index)||g->world.terrain[index]!=0xc8u)
        return previous;
    g->world.terrain[index]=0;return candidate;
}

static int16_t mechanism_remove_vertical(FloodGame *g,int16_t x,
    int16_t candidate,int16_t previous){
    uint16_t index;
    if(!mechanism_cell(x,candidate,&index)||g->world.terrain[index]!=0xcau)
        return previous;
    g->world.terrain[index]=0;return candidate;
}

static void update_horizontal_mechanism(FloodGame *g,FloodObject *o){
    unsigned blocked=0;
    if(o->state_flags==3u){
        const int16_t old_hi=o->origin_x,old_lo=o->x;
        int16_t hi=mechanism_place_horizontal(g,o,(int16_t)(old_hi+16),
            o->origin_y,old_hi,&blocked);
        if(hi==old_hi)blocked++;
        o->origin_x=hi;
        int16_t lo=mechanism_place_horizontal(g,o,(int16_t)(old_lo-16),
            o->y,old_lo,&blocked);
        if(lo==old_lo)blocked++;
        o->x=lo;
        if(blocked>=2u){
            o->state_flags=2u;o->timer=50u;
            o->origin_x=(int16_t)(o->origin_x+16);o->x=(int16_t)(o->x-16);
            if(blocked>=13u)o->state_flags=1u;
        }
    }else if(o->state_flags==2u){
        o->timer--;
        if(o->timer==0u){o->state_flags=1u;mechanism_sound(g,50u);}
    }else if(o->state_flags==1u){
        const int16_t old_hi=o->origin_x,old_lo=o->x;
        int16_t hi=mechanism_remove_horizontal(g,(int16_t)(old_hi-16),
            o->origin_y,old_hi);
        if(hi==old_hi)blocked++;
        o->origin_x=hi;
        int16_t lo=mechanism_remove_horizontal(g,(int16_t)(old_lo+16),
            o->y,old_lo);
        if(lo==old_lo)blocked++;
        o->x=lo;
        if(blocked>=2u){
            o->state_flags=0u;o->origin_x=(int16_t)(o->origin_x-16);
            o->x=(int16_t)(o->x+16);o->timer=50u;
        }
    }else if(o->state_flags==0u){
        o->timer--;
        if(o->timer==0u){
            o->state_flags=3u;
            if(mechanism_visible(g,o))mechanism_sound(g,49u);
        }
    }
}

static void update_vertical_mechanism(FloodGame *g,FloodObject *o){
    unsigned blocked=0;
    if(o->state_flags==3u){
        const int16_t old_hi=o->origin_y,old_lo=o->y;
        int16_t hi=mechanism_place_vertical(g,o,o->origin_x,
            (int16_t)(old_hi+16),old_hi,&blocked);
        if(hi==old_hi)blocked++;
        o->origin_y=hi;
        int16_t lo=mechanism_place_vertical(g,o,o->x,
            (int16_t)(old_lo-16),old_lo,&blocked);
        if(lo==old_lo)blocked++;
        o->y=lo;
        if(blocked>=2u){
            o->state_flags=2u;o->timer=50u;
            o->origin_y=(int16_t)(o->origin_y+16);o->y=(int16_t)(o->y-16);
            if(blocked>=13u)o->state_flags=1u;
        }
    }else if(o->state_flags==2u){
        o->timer--;
        if(o->timer==0u){o->state_flags=1u;mechanism_sound(g,50u);}
    }else if(o->state_flags==1u){
        const int16_t old_hi=o->origin_y,old_lo=o->y;
        int16_t hi=mechanism_remove_vertical(g,o->origin_x,
            (int16_t)(old_hi-16),old_hi);
        if(hi==old_hi)blocked++;
        o->origin_y=hi;
        int16_t lo=mechanism_remove_vertical(g,o->x,
            (int16_t)(old_lo+16),old_lo);
        if(lo==old_lo)blocked++;
        o->y=lo;
        if(blocked>=2u){
            o->state_flags=0u;o->origin_y=(int16_t)(o->origin_y-16);
            o->y=(int16_t)(o->y+16);o->timer=50u;
        }
    }else if(o->state_flags==0u){
        o->timer--;
        if(o->timer==0u){
            o->state_flags=3u;
            if(mechanism_visible(g,o))mechanism_sound(g,49u);
        }
    }
}

static bool water_cell_nonzero(const FloodGame *g,int16_t x,int16_t y){
    uint16_t index;
    return !map_index_for_point(x,y,&index)||g->world.water[index]!=0u;
}

static void action_sound(FloodGame *g,uint8_t sound){
    request_sound(g,sound);
}

static void action_emit(FloodGame *g,uint16_t sprite,int16_t x,int16_t y){
    if(g->action_render_count>=FLOOD_ACTION_RENDER_MAX)return;
    g->action_renders[g->action_render_count++]=(FloodActionRender){x,y,sprite};
}

/* $F9E6's action callers convert the returned 20-byte object index into a
   record, award points, advance its state word, and clear animation byte +9. */
static int action_find_object(FloodGame *g,int16_t x,int16_t y,int width,int height){
    for(unsigned n=0;n<FLOOD_OBJECT_MAX;n++){
        FloodObject *o=&g->objects[n];
        if(!o->active||o->state==0u)continue;
        /* $F9E6's state table admits only these damageable families. */
        switch(o->state){
        case 1u:case 5u:case 8u:case 12u:case 14u:case 23u:case 24u:case 25u:
            break;
        default:continue;
        }
        /* $F9E6 uses inclusive comparisons at both sprite bounds.  Every
           admitted host family has the original 32x32 collision extent. */
        if(x>o->x+32||x+width<o->x||y>o->y+32||y+height<o->y)continue;
        return (int)n;
    }
    return -1;
}

static int action_hit_object_common(FloodGame *g,int16_t x,int16_t y,
    int width,int height,uint32_t points,bool hit_opponent){
    if(hit_opponent&&g->multiplayer_opponent&&
       g->multiplayer_opponent->death_mode==0u&&
       g->multiplayer_opponent->invulnerable_timer==0u&&
       rectangles_overlap(x,y,width,height,g->multiplayer_opponent->x+8,
           g->multiplayer_opponent->y+8,16,16))
        g->multiplayer_opponent->life_force-=16;
    const int n=action_find_object(g,x,y,width,height);
    if(n<0)return -1;
    FloodObject *o=&g->objects[n];g->score+=points;o->state++;o->anim=0u;
    return n;
}
static int action_hit_object(FloodGame *g,int16_t x,int16_t y,int width,int height,
    uint32_t points){
    return action_hit_object_common(g,x,y,width,height,points,true);
}
static int action_hit_object_only(FloodGame *g,int16_t x,int16_t y,
    int width,int height,uint32_t points){
    return action_hit_object_common(g,x,y,width,height,points,false);
}

static int action_sign(int16_t from,int16_t to){return from<to?1:from>to?-1:0;}

static void action_grenade_init(FloodGame *g,FloodAction *a){
    const FloodPlayer *p=&g->player;
    a->x=(int16_t)(p->x+8);a->y=p->y;
    a->dx=(int16_t)(p->dx+(p->hbank?4:-4));a->dy=-12;
    a->state=2u;a->timer=32u;a->anim=0u;a->aux=0x4eu;a->target=0u;
}

static void action_grenade_explode(FloodGame *g,FloodAction *a){
    int16_t x=(int16_t)(a->x&~15),y=(int16_t)(a->y-16);
    a->x=(int16_t)(x+32);a->y=y;a->dx=(int16_t)(x-32);a->dy=y;
    a->state=3u;a->timer=7u;a->anim=0u;a->aux=0u;action_sound(g,6u);
}

static void action_grenade_move(FloodGame *g,FloodAction *a){
    int16_t dx=a->dx,dy=(int16_t)(a->dy+2);
    if(dy>16)dy=16;
    if(dy<-12){dy=-12;a->target=1u;}
    FloodCollisionResult q=flood_query_collision(g,(uint16_t)(a->x+4),
        (uint16_t)(a->y+4),dx,dy,8,8);
    if(q.x_flags&FLOOD_ATTR_SOLID){dx=(int16_t)-dx;action_sound(g,17u);}
    if(q.y_flags&FLOOD_ATTR_SOLID){
        action_sound(g,17u);
        if(dy<0)dy=(int16_t)-dy;
        else{
            dy=(int16_t)((uint16_t)dy>>1);
            if(dx>0&&dy<=dx)dx--;
            else if(dx<0){
                int16_t magnitude=(int16_t)-dx;
                if(dy<=magnitude)magnitude--;
                dx=(int16_t)-magnitude;
            }
            dy=(int16_t)-dy;a->target=1u;
        }
    }
    if(--a->timer==0u){action_grenade_explode(g,a);return;}
    a->x=(int16_t)(a->x+dx);a->y=(int16_t)(a->y+dy);a->dx=dx;a->dy=dy;
    a->anim=(uint16_t)((a->anim+object_frame_step(g))&7u);
    action_emit(g,(uint16_t)(0x0cu+a->anim),a->x,a->y);
}

static void action_grenade_blast(FloodGame *g,FloodAction *a){
    int16_t right_x=a->x,left_x=a->dx;
    FloodPlayer *other=g->multiplayer_opponent;
    for(int phase=3;phase>=0;phase--){
        if(rectangles_overlap(right_x,a->y,28,28,
                g->player.x+8,g->player.y+8,16,16))g->player.life_force-=10;
        if(rectangles_overlap(left_x,a->dy,32,32,
                g->player.x+8,g->player.y+8,16,16))g->player.life_force-=10;
        if(other&&other->death_mode==0u){
            if(rectangles_overlap(right_x,a->y,28,28,
                    other->x+8,other->y+8,16,16))other->life_force-=10;
            if(rectangles_overlap(left_x,a->dy,32,32,
                    other->x+8,other->y+8,16,16))other->life_force-=10;
        }
        const uint16_t right_sprite=(uint16_t)(0x98u+a->anim+(unsigned)phase);
        const uint16_t left_sprite=(uint16_t)(0x98u+a->aux+(unsigned)phase);
        /* The original tests right_x for both draws; preserve that quirk. */
        if(right_x>=0&&right_sprite<=0x9bu)action_emit(g,right_sprite,right_x,a->y);
        if(right_x>=0&&left_sprite<=0x9bu)action_emit(g,left_sprite,left_x,a->dy);
        left_x=(int16_t)(left_x+16);right_x=(int16_t)(right_x-16);
    }
    action_hit_object_only(g,a->x,a->y,80,32,10u);
    action_hit_object_only(g,(int16_t)(a->dx-48),a->dy,80,32,10u);

    FloodCollisionResult q=flood_query_collision(g,(uint16_t)(a->dx+32),
        (uint16_t)(a->dy+8),16,0,32,1);
    if(q.x_flags&FLOOD_ATTR_SOLID)a->aux=(uint16_t)(a->aux+object_frame_step(g));
    else a->dx=(int16_t)(a->dx+16);
    q=flood_query_collision(g,(uint16_t)(a->x-16),(uint16_t)(a->y+8),-16,0,32,1);
    if(q.x_flags&FLOOD_ATTR_SOLID)a->anim=(uint16_t)(a->anim+object_frame_step(g));
    else a->x=(int16_t)(a->x-16);
    if(a->timer>0u)a->timer--;
    if(a->timer==0u)a->state=0u;
}

static void action_boomerang_init(FloodGame *g,FloodAction *a){
    const FloodPlayer *p=&g->player;
    a->state=5u;a->x=(int16_t)(((uint16_t)p->x>>4)<<4);a->x+=16;
    a->y=(int16_t)((((uint16_t)(p->y+8))>>4)<<4);
    a->dx=(int16_t)((p->hbank?2:-2)*8);a->dy=0;a->timer=20u;
}

static void action_boomerang_scan(FloodGame *g,FloodAction *a){
    if(a->timer)a->timer--;
    if(a->timer==0u)a->state=0u;
    FloodCollisionResult q=flood_query_collision(g,(uint16_t)a->x,(uint16_t)a->y,
        a->dx,a->dy,16,16);
    if(!(q.x_flags&FLOOD_ATTR_SOLID))a->x=(int16_t)(a->x+a->dx);
    else a->state=0u;

    a->target=UINT16_MAX;a->timer=20u;a->state=6u;
    int displacement=0;
    do{
        displacement+=16;
        q=flood_query_collision(g,(uint16_t)a->x,(uint16_t)a->y,
            0,(int16_t)displacement,16,16);
    }while(!(q.y_flags&FLOOD_ATTR_SOLID));
    const int16_t bottom=(int16_t)(a->y+displacement);
    displacement=0;
    do{
        displacement-=16;
        q=flood_query_collision(g,(uint16_t)a->x,(uint16_t)a->y,
            0,(int16_t)displacement,16,16);
    }while(!(q.y_flags&FLOOD_ATTR_SOLID));
    const int16_t top=(int16_t)(a->y+displacement);
    const int target=action_find_object(g,a->x,top,16,bottom-top);
    if(target>0)a->target=(uint16_t)target;
}

static void action_boomerang_launch(FloodGame *g,FloodAction *a){
    const FloodPlayer *p=&g->player;
    a->x=p->x;a->y=(int16_t)(p->y+8);
    a->dx=(int16_t)(action_sign(a->x,g->action_target_x)*16);
    a->dy=(int16_t)(action_sign(a->y,g->action_target_y)*4);a->aux=(uint16_t)a->dx;
    a->state=7u;a->timer=20u;action_sound(g,24u);
}

static void action_boomerang_move(FloodGame *g,FloodAction *a){
    const int16_t target=(int16_t)a->target;
    if(target>0){
        if((unsigned)target>=FLOOD_OBJECT_MAX){a->state=0u;return;}
        FloodObject *o=&g->objects[(unsigned)target];
        g->action_target_x=o->x;g->action_target_y=o->y;
        if(!o->active||o->state==0u){a->state=0u;a->x=o->x;a->y=o->y;return;}
    }else{
        g->action_target_x=(int16_t)(g->player.x+((int)g->player.hbank-2)*128);
        g->action_target_y=g->player.y;
        if(a->timer)a->timer--;
        if(a->timer==0u){a->state=0u;return;}
    }
    int16_t dx=a->dx,dy=a->dy;
    if(dx==(int16_t)a->aux){
        a->aux=(uint16_t)(action_sign(a->x,g->action_target_x)*16);
        dy=(int16_t)(action_sign(a->y,g->action_target_y)*4);
    }else if(dx>(int16_t)a->aux)dx-=2;
    else dx+=2;
    const int16_t x=(int16_t)(a->x+dx),y=(int16_t)(a->y+dy);
    a->dx=dx;a->dy=dy;
    if(action_hit_object(g,x,y,16,16,10u)>=0){a->state=0u;a->x=x;a->y=y;return;}
    if(x<=0){a->state=0u;return;}
    a->anim=(uint16_t)((a->anim+1u)&7u);
    action_emit(g,(uint16_t)(0x20u+a->anim),x,y);a->x=x;a->y=y;
}

static void action_shuriken_init(FloodGame *g,FloodAction *a){
    const FloodPlayer *p=&g->player;a->x=(int16_t)(p->x+8);a->y=p->y;
    a->dx=(int16_t)(p->dx+(p->hbank?8:-8));a->dy=8;
    a->state=9u;a->timer=32u;a->anim=0u;a->target=0u;
}

static void action_shuriken_move(FloodGame *g,FloodAction *a){
    FloodCollisionResult q=flood_query_collision(g,(uint16_t)(a->x+8),
        (uint16_t)(a->y+8),a->dx,a->dy,1,1);
    if(q.x_flags&FLOOD_ATTR_SOLID){a->dx=(int16_t)-a->dx;action_sound(g,17u);}
    if(q.y_flags&FLOOD_ATTR_SOLID){a->dy=(int16_t)-a->dy;action_sound(g,17u);}
    if(a->timer)a->timer--;
    if(a->timer==0u){a->state=0u;a->anim=0u;return;}
    if(action_hit_object(g,(int16_t)(a->x+8),(int16_t)(a->y+8),1,1,10u)>=0)a->state=0u;
    a->x=(int16_t)(a->x+a->dx);a->y=(int16_t)(a->y+a->dy);
    action_emit(g,(uint16_t)(0x31u+object_frame_step(g)),a->x,a->y);
}

static void action_dynamite_init(FloodGame *g,FloodAction *a){
    if(!(g->player.contact_mask&0x10u)){a->state=0u;return;}
    a->x=(int16_t)(g->player.x+8);a->y=g->player.y;a->dx=0;a->dy=0;
    a->state=11u;a->timer=16u;a->anim=0u;action_sound(g,25u);
}

static void action_dynamite_fuse(FloodGame *g,FloodAction *a){
    a->anim=(uint16_t)((a->anim+object_frame_step(g))&1u);a->dy++;
    FloodCollisionResult q=flood_query_collision(g,(uint16_t)a->x,(uint16_t)a->y,
        0,a->dy,16,16);
    if(q.y_flags&FLOOD_ATTR_SOLID){
        a->dy=0;if(q.y_flags&0x10u)a->dx=-4;if(q.y_flags&0x20u)a->dx=4;
    }
    if(a->timer)a->timer--;
    if(a->timer==0u){a->state=12u;a->anim=0u;a->x-=8;a->y-=16;return;}
    a->x=(int16_t)(a->x+a->dx);a->y=(int16_t)(a->y+a->dy);
    action_emit(g,(uint16_t)(0x35u+a->anim),a->x,a->y);
}

static void action_dynamite_blast(FloodGame *g,FloodAction *a){
    if(a->anim==0u)action_sound(g,7u);
    uint16_t sprite=(uint16_t)(0x94u+a->anim);
    action_emit(g,sprite,(int16_t)(a->x-16),a->y);
    action_emit(g,sprite,a->x,a->y);action_emit(g,sprite,(int16_t)(a->x+16),a->y);
    action_hit_object_only(g,(int16_t)(a->x-16),a->y,64,32,20u);
    if(a->anim==0u&&rectangles_overlap(a->x-16,a->y,64,32,
            g->player.x+8,g->player.y+8,16,16))g->player.life_force-=40;
    if(a->anim==0u&&g->multiplayer_opponent&&
       g->multiplayer_opponent->death_mode==0u&&
       rectangles_overlap(a->x-16,a->y,64,32,
           g->multiplayer_opponent->x+8,g->multiplayer_opponent->y+8,16,16))
        g->multiplayer_opponent->life_force-=40;
    a->anim=(uint16_t)(a->anim+object_frame_step(g));
    if(a->anim==4u){a->state=0u;a->anim=0u;}
}

static void action_flame_beam_init(FloodGame *g,FloodAction *a){
    /* $15346 tests the contact word produced by this same gameplay update.
       The precise original jump+fire window allocates State 13 one update
       before Up starts the jump, so the pre-movement south contact is still
       visible here without a host-side remembered-support shortcut. */
    if(!(g->player.contact_mask&0x10u)){
        a->state=0u;g->weapon_main_offset=0u;g->weapon_tip_offset=0u;return;
    }
    a->state=(uint16_t)((flood_random16(g)&15u)==0u?15u:14u);
    a->target=g->player.hbank?1u:0u;
    a->x=(int16_t)(g->player.x+(a->target?48:-32));a->y=(int16_t)(g->player.y-3);
    a->anim=0u;a->timer=0u;g->weapon_pose_active=true;
    /* $D250/$D26E-$D296: failure substitutes the two-sprite chicken once
       the held-fire pose reaches phase three. */
    if(a->state==15u){
        g->weapon_main_offset=(uint8_t)(g->player.hbank?13u:16u);
        g->weapon_tip_offset=(uint8_t)(g->player.hbank?12u:10u);
    }else{
        g->weapon_main_offset=0u;g->weapon_tip_offset=0u;
    }
    action_sound(g,a->state==15u?28u:47u);
}

static void action_flame_beam(FloodGame *g,FloodAction *a,bool visible){
    /* Support is required only by State 13 when the flame starts.  Once the
       beam exists, held fire continues through a retracting bridge or fall. */
    if(g->player.death_mode||g->fire_ticks==0u){
        a->state=0u;g->weapon_pose_active=false;
        g->weapon_main_offset=0u;g->weapon_tip_offset=0u;return;
    }
    if(!visible||g->fire_ticks<=4u)return;
    a->target=g->player.hbank?1u:0u;
    a->x=(int16_t)(g->player.x+(a->target?48:-32));a->y=(int16_t)(g->player.y-3);
    int16_t step=a->target?16:-16,x=a->x;
    action_emit(g,(uint16_t)(0x37u+(g->player.hbank>>2)),x,a->y);x+=step;
    /* DBRA executes the clamped counter-minus-three value plus one times. */
    unsigned count=g->fire_ticks>15u?13u:(unsigned)g->fire_ticks-2u;
    for(unsigned n=0;n<count;n++,x=(int16_t)(x+step))
        action_emit(g,(uint16_t)(0x39u+3u*object_frame_step(g)),x,a->y);
    action_emit(g,(uint16_t)(0x3au+(g->player.hbank>>2)),x,a->y);
    int16_t left=a->x<x?a->x:x;int width=(a->x<x?x-a->x:a->x-x)+1;
    action_hit_object(g,left,(int16_t)(a->y+5),width,16,10u);
}

static bool radial_flame_active(const FloodGame *g){
    for(unsigned n=0;n<FLOOD_ACTION_MAX;n++)if(g->actions[n].state==17u)return true;
    return false;
}

static void action_radial_flame_init(FloodGame *g,FloodAction *a){
    if(radial_flame_active(g))return;
    a->x=(int16_t)((g->player.x+8)&~15);a->y=(int16_t)(g->player.y&~15);
    a->dx=(int16_t)(g->player.dx+(g->player.hbank?16:-16));a->state=17u;
    a->aux=20u;a->timer=10u;a->anim=0u;a->target=0u;
}

static void action_radial_flame(FloodGame *g,FloodAction *a){
    static const int8_t directions[8][2]={
        {-1,0},{-1,-1},{0,-1},{1,-1},{1,0},{1,1},{0,1},{-1,1}
    };
    for(unsigned n=0;n<8u;n++){
        int16_t x=(int16_t)(a->x+directions[n][0]*16*(int)a->target);
        int16_t y=(int16_t)(a->y+directions[n][1]*16*(int)a->target);
        action_emit(g,(uint16_t)(0x18u+n),x,y);action_hit_object(g,x,y,16,16,10u);
    }
    a->target++;if(a->timer)a->timer--;if(a->timer==0u)a->state=0u;
}

/* Complete 18-entry jump table at $CB40, iterated over eight records from
   $7D0F6.  A state change is not redispatched until the next game tick. */
void flood_update_actions(FloodGame *g){
    g->action_render_count=0u;
    for(unsigned n=0;n<FLOOD_ACTION_MAX;n++){
        FloodAction *a=&g->actions[n];const uint16_t state=a->state;
        switch(state){
        case 0u:
            if(g->fire_ticks==1u){
                a->state=g->selected_weapon_state;
                g->fire_ticks++;
            }
            break;
        case 1u:action_grenade_init(g,a);break;
        case 2u:action_grenade_move(g,a);break;
        case 3u:action_grenade_blast(g,a);break;
        case 4u:action_boomerang_init(g,a);break;
        case 5u:action_boomerang_scan(g,a);break;
        case 6u:action_boomerang_launch(g,a);break;
        case 7u:action_boomerang_move(g,a);break;
        case 8u:action_shuriken_init(g,a);break;
        case 9u:action_shuriken_move(g,a);break;
        case 10u:action_dynamite_init(g,a);break;
        case 11u:action_dynamite_fuse(g,a);break;
        case 12u:action_dynamite_blast(g,a);break;
        case 13u:action_flame_beam_init(g,a);break;
        case 14u:action_flame_beam(g,a,true);break;
        case 15u:action_flame_beam(g,a,false);break;
        case 16u:action_radial_flame_init(g,a);break;
        case 17u:action_radial_flame(g,a);break;
        default:a->state=0u;break;
        }
    }
}

/* $1306A/$13230: dormant horizontal counterpart of State 18. */
static void update_state11_growth(FloodGame *g,FloodObject *o){
    if(o->state_flags==3u){
        uint16_t index;
        if(!mechanism_cell((int16_t)(o->x-16),o->y,&index)||
            g->world.terrain[index]!=0u)return;
        if(state12_hits_player(g,(int16_t)(o->x-8),(int16_t)(o->y-8),32,24))return;
        o->state_flags=1u;return;
    }
    if(o->state_flags!=1u)return;
    const int16_t saved_life=g->player.life_force;
    unsigned blocked=0;
    int16_t hi=(int16_t)(o->origin_x+16);
    if(water_cell_nonzero(g,hi,o->origin_y)){o->state=0u;return;}
    hi=mechanism_place_horizontal(g,o,hi,o->origin_y,o->origin_x,&blocked);
    o->origin_x=hi;
    int16_t lo=(int16_t)(o->x-16);
    if(water_cell_nonzero(g,lo,o->y)){o->state=0u;return;}
    lo=mechanism_place_horizontal(g,o,lo,o->y,o->x,&blocked);o->x=lo;
    g->player.life_force=saved_life;
}

static void update_state18_growth(FloodGame *g,FloodObject *o){
    if(o->state_flags==3u){
        uint16_t index;
        if(!mechanism_cell(o->x,(int16_t)(o->y-16),&index)||
            g->world.terrain[index]!=0u)return;
        if(state12_hits_player(g,(int16_t)(o->x-8),(int16_t)(o->y-8),32,24))return;
        o->state_flags=1u;mechanism_sound(g,26u);return;
    }
    if(o->state_flags!=1u)return;
    const int16_t saved_life=g->player.life_force;
    unsigned blocked=0;
    int16_t hi=(int16_t)(o->origin_y+16);
    if(water_cell_nonzero(g,o->origin_x,hi)){o->state=0u;return;}
    hi=mechanism_place_vertical(g,o,o->origin_x,hi,o->origin_y,&blocked);
    o->origin_y=hi;
    int16_t lo=(int16_t)(o->y-16);
    if(water_cell_nonzero(g,o->x,lo)){o->state=0u;return;}
    lo=mechanism_place_vertical(g,o,o->x,lo,o->y,&blocked);o->y=lo;
    g->player.life_force=saved_life;
}

/* State 11 is unconditional in the dispatcher.  Only States 16-18 are
   suppressed while $E1EA is nonzero. */
void flood_update_mechanisms_11_18(FloodGame *g){
    for(unsigned n=0;n<FLOOD_OBJECT_MAX;n++){
        FloodObject *o=&g->objects[n];if(!o->active)continue;
        if(o->state==11u)update_state11_growth(g,o);
        else if(!g->world.mechanisms_paused){
            if(o->state==16u)update_horizontal_mechanism(g,o);
            else if(o->state==17u)update_vertical_mechanism(g,o);
            else if(o->state==18u)update_state18_growth(g,o);
        }
    }
}

static bool quiffy_item_cell(const FloodGame *g,uint16_t *index){
    const int16_t x=(int16_t)(g->player.x+16),y=(int16_t)(g->player.y+16);
    if(x<0||y<0||x>=FLOOD_MAP_W*16||y>=FLOOD_MAP_H*16)return false;
    *index=flood_tile_index_at(x,y);return true;
}

/* $FC76-$FD26 uses a cache separate from the ordinary item dispatcher.  $E7
   and $E8 persist in the map and therefore reactivate only after Quiffy has
   entered a different cell.  $DC hands off to the auxiliary State-1 path. */
static void update_special_items(FloodGame *g){
    FloodPlayer *p=&g->player;
    if(p->death_mode)return;
    uint16_t index;if(!quiffy_item_cell(g,&index))return;
    if(index==g->last_special_tile)return;
    g->last_special_tile=index;
    const uint8_t tile=g->world.terrain[index];
    if(tile==0xdcu){
        FloodQuiffyEffect *e=&g->quiffy_effect;
        e->state=1u;e->dy=0;e->y=0;e->aux=0;
        g->world.terrain[index]=0u;p->y=(int16_t)(p->y-16);
        g->special_entry_offset=true;return;
    }
    if(tile==0xe7u){
        p->parachute_timer=1000u;p->balloon_timer=0u;action_sound(g,38u);
    }else if(tile==0xe8u){
        p->balloon_timer=1000u;p->parachute_timer=0u;action_sound(g,37u);
    }
}

static void set_quiffy_contact_word(FloodPlayer *p,uint16_t contacts){
    p->contact_mask=(uint8_t)contacts;
    p->contact_aux=(uint16_t)(contacts&0xff00u);
    p->contact_count=(uint8_t)popcount8(p->contact_mask);
}

/* $10382-$10469: 58 signed X/Y steps consumed by auxiliary State 2. */
static const int16_t quiffy_effect_path[58][2]={
    {0,-16},{0,-16},{0,-16},{0,-16},{16,-16},{16,0},{16,16},{16,16},
    {0,16},{0,16},{0,16},{0,16},{-16,16},{-16,16},{-16,16},{-16,0},
    {-16,0},{-16,0},{-16,-16},{0,-16},{0,-16},{0,-16},{0,-16},{0,-16},
    {16,-16},{0,-16},{16,-16},{16,-16},{-16,-16},{-16,0},{-16,0},{-16,16},
    {-16,16},{-16,16},{0,16},{0,16},{0,16},{16,16},{16,0},{16,0},
    {16,0},{16,0},{16,0},{16,0},{16,0},{16,0},{16,0},{16,0},
    {16,0},{16,-16},{0,-16},{0,-16},{0,-16},{-16,-16},{-16,-16},{-16,0},
    {-16,0},{-16,0}
};

/* Returns 0 for ordinary player processing, 1 when it prepared the contact
   word for State 1, and 2 when State 2 replaces Quiffy's normal update. */
static unsigned update_quiffy_effect(FloodGame *g,FloodInput in,
    uint16_t *prepared_contacts,bool *state1_geometry){
    FloodQuiffyEffect *e=&g->quiffy_effect;FloodPlayer *p=&g->player;
    *state1_geometry=false;
    g->quiffy_effect_pose_active=false;g->quiffy_effect_render_active=false;
    if(e->state==0u)return 0u;
    if(e->state==2u){
        p->raw_y=0;set_quiffy_contact_word(p,0u);
        const unsigned step=(unsigned)e->phase/4u;
        if(step<58u){e->x=(int16_t)(e->x+quiffy_effect_path[step][0]);
            e->y=(int16_t)(e->y+quiffy_effect_path[step][1]);}
        e->phase=(int16_t)(e->phase+4);
        g->quiffy_effect_render_x=e->x;g->quiffy_effect_render_y=e->y;
        g->quiffy_effect_sprite=0xb4u;g->quiffy_effect_render_active=true;
        e->dy=(int16_t)(e->dy-1);
        if(e->dy==0)e->state=0u;
        return 2u;
    }

    if(p->death_mode==1u){
        uint16_t index;
        if(map_index_for_point((int16_t)(p->x+16),(int16_t)(p->y+24),&index))
            g->last_special_tile=index;
        e->state=0u;g->special_entry_offset=false;return 0u;
    }

    /* $101EA/$101F2 set $E1EC=32 and $E1F2=0 for State 1.  That geometry
       remains live through ordinary movement; only the fatal branch at
       $10240-$10248 restores 16/+8 during this auxiliary visit. */
    uint16_t contacts=(uint16_t)(build_quiffy_contacts_geometry(g,0,32)&0x110u);
    *state1_geometry=true;
    set_quiffy_contact_word(p,contacts);*prepared_contacts=contacts;
    if(contacts&0x100u){
        e->state=2u;e->y=p->y;e->x=p->x;e->phase=0;e->dy=58;
        set_quiffy_contact_word(p,0u);*prepared_contacts=0u;
        /* $10240-$10248 explicitly restore 16/+8 only on this fatal path. */
        *state1_geometry=false;
        g->special_entry_offset=false;return 1u;
    }

    if(g->fire_ticks){
        g->fire_ticks=2u;uint16_t index;
        if(map_index_for_point((int16_t)(p->x+16),(int16_t)(p->y+24),&index)&&
                g->world.terrain[index]==0u){
            set_quiffy_contact_word(p,0u);*prepared_contacts=0u;
            g->last_special_tile=index;e->state=0u;
            g->world.terrain[index]=0xdcu;g->special_entry_offset=false;
            return 1u;
        }
    }

    g->quiffy_effect_sprite=(uint16_t)(0xaeu+p->hbank+(p->anim_phase&3u));
    g->quiffy_effect_pose_active=true;
    if(contacts&0x10u){
        int16_t velocity=e->dy;
        if(velocity!=0)action_sound(g,33u);
        if(in.y<0){
            velocity=(int16_t)(velocity-4);if(velocity<-24)velocity=-24;
            e->dy=velocity;e->y=velocity;p->dy=velocity;e->aux=0;
            contacts=0u;
        }else if(velocity!=0){
            velocity=(int16_t)(velocity+4);if(velocity>=0)velocity=0;
            e->dy=velocity;p->dy=velocity;contacts&=0xff00u;
        }
        set_quiffy_contact_word(p,contacts);*prepared_contacts=contacts;
    }
    if(p->dy>=14)e->dy=-20;
    if(e->y<-16){p->dy=-16;e->y=(int16_t)(e->y+1);}
    return 1u;
}

static uint8_t weapon_tile(uint16_t state){
    switch(state){
    case 1u:return 0xddu;case 4u:return 0xdeu;case 8u:return 0xebu;
    case 10u:return 0xecu;case 13u:return 0xedu;case 16u:return 0xeau;
    default:return 0u;
    }
}

static void select_weapon(FloodGame *g,uint16_t index,uint16_t state){
    /* Multiplayer keeps weapon stations in the shared cavern so each actor
       can collect the same weapon independently. Single-player swaps as on
       the original machine. */
    if(!g->multiplayer_opponent)
        g->world.terrain[index]=weapon_tile(g->selected_weapon_state);
    g->selected_weapon_state=state;action_sound(g,16u);
}

/* $10168 searches from map offset $31FF downward, so duplicate partner tiles
   deliberately choose the last map occurrence. */
static void begin_transport(FloodGame *g,uint8_t tile){
    const uint8_t partner=(uint8_t)(tile<=0xa5u?tile+10u:tile-10u);
    for(int index=FLOOD_MAP_W*FLOOD_MAP_H-1;index>=0;index--){
        if(g->world.terrain[index]!=partner)continue;
        const int h=(int)g->player.hbank-2;
        g->transport_x=(int16_t)((index%FLOOD_MAP_W)*16+h*8-4);
        g->transport_y=(int16_t)((index/FLOOD_MAP_W)*16-
            (g->special_entry_offset?24:8));
        break;
    }
    g->transport_timer=10u;action_sound(g,51u);
}

static void update_transport(FloodGame *g){
    g->transport_render_prejump=false;
    if(!g->transport_timer){g->transport_effect_stage=0u;return;}
    g->transport_effect_stage=(uint8_t)(g->transport_timer>5u?
        11u-g->transport_timer:g->transport_timer);
    if(g->transport_timer==6u){
        g->transport_render_x=g->player.x;g->transport_render_y=g->player.y;
        g->transport_render_prejump=true;
        g->player.y=g->transport_y;g->player.x=g->transport_x;
    }
    g->transport_timer--;
}

/* $FDA0-$100C4: complete ordinary map-item chain. */
static void update_map_items(FloodGame *g){
    FloodPlayer *p=&g->player;if(p->death_mode)return;
    uint16_t index;if(!quiffy_item_cell(g,&index))return;
    if(index==g->last_pickup_tile)return;
    g->last_pickup_tile=index;
    const uint8_t tile=g->world.terrain[index];
    uint16_t weapon=0u;
    switch(tile){
    case 0xddu:weapon=1u;break;case 0xdeu:weapon=4u;break;
    case 0xebu:weapon=8u;break;case 0xecu:weapon=10u;break;
    case 0xedu:weapon=13u;break;case 0xeau:weapon=16u;break;
    default:break;
    }
    if(weapon){select_weapon(g,index,weapon);return;}
    if(tile==0xdbu){g->world.terrain[index]=0u;return;}
    if(tile==0xdfu||tile==0xe0u){
        g->world.mechanisms_paused=tile==0xdfu;
        action_sound(g,16u);g->world.terrain[index]=0u;return;
    }
    if(tile==0xeeu){action_sound(g,54u);g->zap_message_pending=true;return;}
    if(tile==0xe1u){g->lives++;action_sound(g,58u);g->world.terrain[index]=0u;return;}
    if(tile==0xe2u){
        g->score+=5u;action_sound(g,16u);p->invulnerable_timer=50u;
        g->world.terrain[index]=0u;return;
    }
    if(tile==0xe3u){
        action_sound(g,16u);p->orange_timer=50u;g->world.terrain[index]=0u;return;
    }
    if(tile==0x94u||(tile>=0x24u&&tile<=0x27u)){
        g->world.remaining_food--;g->score+=2u;action_sound(g,16u);
        g->world.terrain[index]=0u;return;
    }
    if(tile==0xe4u){
        g->world.terrain[index]=0;g->world.water_speed=6u;action_sound(g,10u);
        return;
    }
    if(tile==0xe5u){g->world.terrain[index]=0;g->world.water_pause=100u;return;}
    if(tile==0xffu){action_sound(g,26u);flood_activate_trigger(g,index,0);return;}
    if(tile==0x03u){
        g->world.terrain[index]=0;action_sound(g,63u);
        flood_activate_trigger(g,index,1);return;
    }
    if(g->world.remaining_food==0&&tile==0x88u){
        g->world.remaining_food=-1;g->level_complete=true;action_sound(g,60u);
    }
    if(tile>=0x9cu&&tile<=0xafu)begin_transport(g,tile);
}

/* State 19 mine trigger ($FD28-$FD9C), the shared State 2/13/20 explosion
   handler ($1091A-$109A4), and State 21 Heart ($FB42-$FC64).

   State 20 is the mine's runtime explosion state.  States 2 and 13 dispatch
   to the very same handler, although neither is instantiated by the shipped
   static maps or active trigger payloads. */
static void update_mine_family_object(FloodGame *g,FloodObject *o,uint8_t state){
        if(state==19u){
            if(rectangles_overlap(o->x+4,o->y+12,8,4,g->player.x+8,g->player.y+8,16,16)){
                if(o->x>=0&&o->y>=0&&o->x<FLOOD_MAP_W*16&&o->y<FLOOD_MAP_H*16)
                    g->world.terrain[flood_tile_index_at(o->x,o->y)]=0;
                o->state=20;o->anim=0;o->x=(int16_t)(o->x-8);o->y=(int16_t)(o->y-16);o->sprite=0;
                if(g->player.life_force>0)g->player.forced_up_timer=3;
            }
        }else if(state==2u||state==13u||state==20u){
            if(o->anim==0u)request_sound(g,8u);
            o->anim=(uint8_t)(o->anim+object_frame_step(g));
            if(o->anim==4u){
                o->state=21u;o->x=(int16_t)(o->x+4);o->dy=-12;
                o->aux=0;o->anim=40u;o->sprite=0;
                return;
            }
            o->sprite=(uint8_t)(0x94u+o->anim);
            o->render_x=o->x;o->render_y=o->y;
            if(o->anim==0u&&rectangles_overlap(o->x,o->y,32,32,
                    g->player.x+8,g->player.y+8,16,16))
                g->player.life_force=(int16_t)(g->player.life_force-40);
        }else if(state==21u){
            int16_t dx=0,dy=o->dy;
            dy=(int16_t)(dy+(object_word8_nonzero(o)?1:-1));
            if(dy>8)dy=8;
            if(dy<-12){dy=-12;object_set_word8(o,1u);}
            FloodCollisionResult q=flood_query_collision(g,(uint16_t)(o->x+4),(uint16_t)(o->y+4),0,dy,8,8);
            if(q.x_flags&FLOOD_ATTR_SOLID)dx=(int16_t)-dx;
            if(q.y_flags&FLOOD_ATTR_SOLID){
                if(dy>=0){dy=(int16_t)-((uint16_t)dy>>1);object_set_word8(o,1u);}
                else dy=(int16_t)-dy;
            }
            o->x=(int16_t)(o->x+dx);o->y=(int16_t)(o->y+dy);
            o->dx=dx;o->dy=dy;o->sprite=0x30u;
            o->render_x=(int16_t)(o->x+4);o->render_y=(int16_t)(o->y+4);
            if(rectangles_overlap(o->x+4,o->y+4,8,8,
                    g->player.x+8,g->player.y+8,16,16)){
                int life=g->player.life_force+64;
                g->player.life_force=(int16_t)(life>511?511:life);
                g->score+=10;o->state=0u;
            }
            o->anim=(uint8_t)(o->anim-1u);
            if(o->anim==0u)o->state=0u;
        }
}

void flood_update_states_2_13_19_21(FloodGame *g){
    for(unsigned n=0;n<FLOOD_OBJECT_MAX;n++){
        FloodObject *o=&g->objects[n];if(!o->active)continue;
        if(o->state==2u||o->state==13u||o->state==19u||
           o->state==20u||o->state==21u)
            update_mine_family_object(g,o,o->state);
    }
}

/* Main object dispatcher $CC60-$CC82: every record at or below y=$063F is
   dispatched; y >= $0640 first clears its state word. */
void flood_prepare_object_dispatch(FloodGame *g){
    for(unsigned n=0;n<FLOOD_OBJECT_MAX;n++){
        FloodObject *o=&g->objects[n];if(!o->active)continue;
        if(o->y>=1600)o->state=0u;
    }
}

/* State 24 entry at $CD94.  It only changes the state word to 12 and then
   exits, so the State-12 handler must not run until the following tick. */
void flood_finish_object_dispatch(FloodGame *g){
    for(unsigned n=0;n<FLOOD_OBJECT_MAX;n++){
        FloodObject *o=&g->objects[n];if(o->active&&o->state==24u)o->state=12u;
    }
}

/* Runtime state 22, exact control flow from $1363A-$13780.  Bit zero of
   state_flags is record byte +8 (impact/reset mode); anim is byte +9. */
static void update_bolt_object(FloodGame *g,FloodObject *o){
        if(o->state_flags&1u){
            /* $1372C-$13734: the impact/reset animation starts the shared
               explosion effect on its phase-zero visit.  The following six
               frames are silent; after phase seven the bolt returns to its
               launch cell. */
            if(o->anim==0u)request_sound(g,7u);
            o->anim++;
            if(o->anim==7u){
                o->state_flags&=(uint8_t)~1u;o->anim=0;
                o->x=o->origin_x;o->y=o->origin_y;o->sprite=o->sprite_base;
            }else o->sprite=(uint8_t)(0x98u+(o->anim>>1));
            return;
        }
        FloodCollisionResult q=flood_query_collision(g,(uint16_t)o->x,
            (uint16_t)(o->y+4),8,0,32,8);
        if(q.x_flags&FLOOD_ATTR_SOLID){o->state_flags|=1u;return;}
        o->sprite=(uint8_t)(o->sprite_base+2u);
        if(rectangles_overlap(o->x,o->y+4,32,8,
                g->player.x+8,g->player.y+8,16,16)){
            g->player.life_force=-1;o->state_flags|=1u;
        }
        o->x=(int16_t)(o->x+8);
}

/* $CC56-$CDA0 is one forward pass over the 128 records.  Cross-state
   grouping is observably different when an earlier record changes a later
   record, so the active frame path dispatches each slot exactly once. */
static void update_object_dispatch_exact(FloodGame *g){
    const uint8_t step=object_frame_step(g);
    for(unsigned n=0;n<FLOOD_OBJECT_MAX;n++){
        FloodObject *o=&g->objects[n];if(!o->active)continue;
        if(o->y>=1600)o->state=0u;
        const uint8_t state=o->state;
        switch(state){
        case 1u:update_doctor_dusty(g,o,step);break;
        case 2u:case 13u:case 19u:case 20u:case 21u:
            update_mine_family_object(g,o,state);break;
        case 5u:update_state5_object(g,o);break;
        case 6u:case 9u:case 15u:update_shared_burst(g,o);break;
        case 8u:update_state8_object(g,o,step);break;
        case 10u:update_doctor_dust(g,o);break;
        case 11u:update_state11_growth(g,o);break;
        case 12u:
            if(o->sprite_base==0x38u)update_state12_teddy(g,o,step);
            else if(o->sprite_base==0x6eu)update_state12_vong(g,o,step);
            else update_state12_walker(g,o,step);
            break;
        case 14u:update_state14_object(g,o,step);break;
        case 16u:if(!g->world.mechanisms_paused)update_horizontal_mechanism(g,o);break;
        case 17u:if(!g->world.mechanisms_paused)update_vertical_mechanism(g,o);break;
        case 18u:if(!g->world.mechanisms_paused)update_state18_growth(g,o);break;
        case 22u:update_bolt_object(g,o);break;
        case 24u:o->state=12u;break;
        default:break;
        }
    }
}

/* $D830-$D840 and $D87E-$D894 both enter death mode 1.  Exhausted Life
   Force adds the 20-update Matilda history hold; fatal terrain enters the
   same mode directly and does not perform that write. */
static void begin_quiffy_death(FloodGame *g,bool hold_matilda){
    FloodPlayer *p=&g->player;
    if(p->death_mode)return;
    p->death_mode=1u;
    p->parachute_timer=0;
    p->balloon_timer=0;
    if(hold_matilda)flood_matilda_add_death_delay(g);
}

/* $D348-$D362 runs after R's complete cavern setup/banner call returns.  A
   non-positive remaining count writes -1 to Life Force and then reaches the
   ordinary $D830 fatal-Life-Force test later in the same gameplay update. */
void flood_game_restart_finish(FloodGame *g){
    if(g->lives>0)g->lives--;
    if(g->lives<=0){
        g->lives=0;g->player.life_force=-1;
        begin_quiffy_death(g,true);
    }
}

/* Renderer path $CDF0-$CE7E.  Mode 2 advances by the global alternating
   1,0 step, clamps visible phases above 7 to the last cross frame, and only
   restores Quiffy after the counter reaches 16. */
static void advance_quiffy_death_mode2(FloodGame *g){
    FloodPlayer *p=&g->player;
    if(p->death_mode!=2u)return;
    if(p->death_phase==8u)request_sound(g,0x38u);
    p->death_phase=(uint8_t)(p->death_phase+object_frame_step(g));
    if(p->death_phase!=16u)return;
    p->death_mode=0u;
    p->death_phase=0u;
    p->anim_phase=0u;
    p->life_force=511;
    p->air=63;
    p->invulnerable_timer=100u;
    if(g->lives>0)g->lives--;
    if(g->lives<=0){g->lives=0;g->running=false;}
}

/* $D186-$D1E0 consumes the overlap/damage latch after Quiffy and Matilda
   render state has been selected.  The bubble is visible for every latched
   frame; the voice fires only on the zero-to-one edge. */
static void update_damage_feedback(FloodGame *g,int16_t life_before_dispatch){
    static const uint8_t ouch_sound[16]={
        1u,1u,1u,2u,2u,2u,3u,3u,3u,3u,4u,4u,4u,4u,5u,5u
    };
    const bool contact=g->player.death_mode!=2u&&
        g->player.life_force<life_before_dispatch;
    g->ouch_visible=contact;
    if(contact&&!g->damage_contact_previous)
        request_sound(g,ouch_sound[(flood_random16(g)&0x1eu)>>1]);
    g->damage_contact_previous=contact;

    /* $D1E8-$D220 starts the paired water-entry components once on the
       dry-to-wet edge. */
    const bool water=flood_water_fill_at(g,g->player.x+8,g->player.y+8)!=0u;
    if(water&&!g->water_contact_previous){request_sound(g,13u);request_sound(g,15u);}
    g->water_contact_previous=water;
}

static void update_player(FloodGame *g,FloodInput in,uint8_t old_contacts,
    unsigned effect_mode,uint16_t prepared_contacts,bool state1_geometry){
    FloodPlayer *p=&g->player;
    g->player_blink_active=p->invulnerable_timer!=0u;
    /* Escape exhausts the lives counter and enters the ordinary death path
       immediately.  The original leaves the visible Life Force gauge at its
       current value throughout that animation; keep a private snapshot.
       Cocktail protection cannot cancel an already-entered death mode, so
       retain its timer and the alternating-buffer blink during the sequence. */
    if(in.restart){
        g->lives=0;
        if(!g->escape_death_active)g->escape_life_force=p->life_force;
        g->escape_death_active=true;begin_quiffy_death(g,true);return;
    }
    if(p->death_mode==2u)return;
    /* $D07C-$D0BA: Cocktail/protection decrements first, then restores both
       gauges on every active update. */
    if(p->invulnerable_timer&&!g->escape_death_active){
        if(p->invulnerable_timer==100u&&g->lives>0)request_sound(g,52u);
        p->invulnerable_timer--;p->life_force=511;p->air=63;
    }
    if(p->death_mode==0u&&p->life_force<=0)begin_quiffy_death(g,true);
    if(effect_mode==2u)return;
    uint16_t contacts=effect_mode==1u?prepared_contacts:
        flood_build_quiffy_contacts(g);
    /* Preserve the live direction produced by $F22C before the attachment
       cache is allowed to replay an older corner direction.  In the
       one-cell conveyor passages used by the original maps, the ceiling and
       floor each contribute a one-unit belt correction. */
    const int8_t contact_raw_x=p->raw_x;
    /* $D8D8: a mine primes three updates of forced upward travel.  Keep only
       south support and the fatal-terrain flag while the impulse is active. */
    if(p->forced_up_timer){
        p->forced_up_timer--;
        p->raw_y=-1;
        contacts=(uint16_t)((contacts|0x10u)&0x110u);
        p->contact_mask=(uint8_t)contacts;
        p->contact_count=(uint8_t)popcount8(p->contact_mask);
        p->contact_aux=(uint16_t)(contacts&0x100u);
    }
    if((contacts&0x100u)&&!p->invulnerable_timer&&p->death_mode==0u){
        begin_quiffy_death(g,false);
        return;
    }
    if(p->contact_mask){
        if(p->parachute_timer||p->balloon_timer)request_sound(g,20u);
        p->parachute_timer=0;p->balloon_timer=0;
    }
    if(!old_contacts && p->contact_mask){
        p->y=(int16_t)(p->y+2);p->y=(int16_t)(p->y&~3);
    }
    if(p->attachment_mode)flood_maintain_quiffy_attachment(p);
    if(p->contact_count<=2)flood_resolve_quiffy_contacts(p,p->contact_mask);
    /* Passage alignment may suppress the vertical motion component below,
       but the original Down request still selects the crawl pose.  Preserve
       the post-attachment direction separately from the collision vector. */
    const int8_t pose_raw_y=p->raw_y;

    /* At a one-tile-high passage mouth, the wall attachment cache can still
       contain the preceding climb direction even though the live horizontal
       route has become clear.  Prefer that explicit horizontal command when
       at least two contacts still describe the passage rim.  Requiring a
       clear cardinal probe and multiple contacts leaves the lone-diagonal
       convex-corner carry unchanged. */
    const bool cached_wall_climb=p->attachment_mode==2u&&p->surface_y!=0&&
        in.x==p->surface_x;
    /* A one-cell shaft can keep five or six rim contacts active for longer
       than the original ten-update corner cache.  Once that cache expires,
       the raw vertical command still proves that Quiffy is climbing; allow
       the same strictly geometry-gated passage handoff in that state. */
    const bool dense_shaft_climb=p->attachment_mode==0u&&
        p->contact_count>2u&&in.y!=0;
    /* Ordinary horizontal approach needs the same geometry handoff.  A
       falling or recently grounded body can meet the passage lip four or
       eight pixels away from its exact 16-pixel centre.  The side contact
       then suppresses free-flight gravity, so requiring an active climb
       leaves it permanently blocked.  Keep this source attachment-free and
       direction-neutral; the corridor probes below remain authoritative. */
    const bool walking_passage_entry=p->attachment_mode==0u&&in.y==0;
    if(p->contact_count>=2u&&in.x!=0&&
            (cached_wall_climb||dense_shaft_climb||walking_passage_entry)){
        const int16_t step=(int16_t)(in.x*4);
        const int16_t core_x=(int16_t)(p->x+8),core_y=(int16_t)(p->y+8);
        /* A 16-pixel opening has only one exactly aligned update at four
           pixels per step.  Round to the nearest cell boundary while the
           wall cache still describes an active climb, then verify that the
           short vertical correction and the complete horizontal route are
           both clear.  This restores the original's forgiving capture at a
           tight passage without extending the convex-corner cache. */
        const int16_t passage_y=(int16_t)(((core_y+8)/16)*16);
        const int16_t align_step=(int16_t)(passage_y-core_y);
        const int16_t lead_x=(int16_t)(core_x+step+(step>0?15:0));
        FloodCollisionResult alignment=flood_query_collision(g,
            (uint16_t)core_x,(uint16_t)core_y,0,align_step,16,16);
        FloodCollisionResult passage=flood_query_collision(g,
            (uint16_t)core_x,(uint16_t)passage_y,step,0,16,16);
        const bool bounded_above=(flood_tile_attr_at(g,lead_x,
            (int16_t)(passage_y-1))&FLOOD_ATTR_SOLID)!=0u;
        const bool bounded_below=(flood_tile_attr_at(g,lead_x,
            (int16_t)(passage_y+16))&FLOOD_ATTR_SOLID)!=0u;
        if(!(alignment.y_flags&FLOOD_ATTR_SOLID)&&
                !(passage.x_flags&FLOOD_ATTR_SOLID)&&
                bounded_above&&bounded_below){
            p->y=(int16_t)(passage_y-8);
            p->attachment_mode=0u;
            p->attachment_ticks=0u;
            p->surface_y=0;
            /* The corridor handoff changes alignment and attachment state,
               but the original does not repoll or replace $17E78 here.
               Retain both conveyor corrections instead of reducing the
               result back to the unmodified keyboard/gamepad direction. */
            p->raw_x=contact_raw_x;
            p->raw_y=0;
        }
    }

    /* $DBE8-$DC28 handles the falling-death state before the ordinary
       movement selector.  South support raises the sprite eight pixels and
       starts the cross sequence immediately.  With no south support it
       clears the complete contact word, even when Quiffy is still touching
       (or attached to) a wall or ceiling; the no-contact branch below then
       applies gravity automatically instead of accepting surface movement. */
    bool falling_death=false;
    if(p->death_mode==1u){
        if(p->contact_mask&0x10u){
            p->y=(int16_t)(p->y-8);
            p->anim_phase=0u;
            p->death_phase=0u;
            p->death_mode=2u;
            request_sound(g,0x38u);
            return;
        }
        p->contact_mask=0u;
        p->contact_count=0u;
        p->contact_aux=0u;
        /* attachment_mode is a host-side cache derived from that contact
           word.  It must not survive the original's complete-word clear. */
        p->attachment_mode=0u;
        p->attachment_ticks=0u;
        p->surface_x=0;
        p->surface_y=0;
        falling_death=true;
    }

    if(p->contact_mask){
        int8_t mx=p->attachment_mode?p->surface_x:p->raw_x;
        int8_t my=p->attachment_mode?p->surface_y:p->raw_y;
        p->dx=(int16_t)(mx*4);
        p->dy=(int16_t)(my*4);
    }else{
        p->dx=(int16_t)(in.x*4);
        /* Death mode 1 reaches this branch specifically to force the dry
           gravity path.  Applying ordinary water buoyancy here can pin a
           dead Quiffy to a wall or ceiling until down is pressed. */
        uint8_t f0=falling_death?0u:flood_water_fill_at(g,p->x+8,p->y+8);
        uint8_t f1=falling_death?0u:flood_water_fill_at(g,p->x+8,p->y+7);
        p->dy=(int16_t)(p->dy+((((p->y+8)&15)+f0>=16)?-1:1));
        p->dy=(int16_t)(p->dy+((((p->y+7)&15)+f1>=16)?-1:1));
        if(p->dy>0)p->dy--;else if(p->dy<0)p->dy++;
    }
    /* $DE34-$DE4A: negative X selects bank 0; positive X selects bank 4. */
    if(p->dx<0)p->hbank=0; else if(p->dx>0)p->hbank=4;
    if(p->dy>16)p->dy=16;
    if(p->dy<-16)p->dy=-16;
    const uint8_t fill=flood_water_fill_at(g,p->x+8,p->y+8);
    if(!falling_death&&fill&&p->contact_mask==0u&&p->raw_y>0)p->dy=2;
    /* $DEB4-$DEF2: the centre-depth test drains air only on buffer 1.
       The subtract happens before the signed test, so air zero also damages
       Life Force on that same update.  Recovery is an exact +2. */
    if(fill>=8u){
        if(g->render_buffer_index){
            p->air--;
            if(p->air<0)p->life_force-=4;
        }
    }else if(p->air<63)p->air=(int16_t)(p->air+2);
    if(p->parachute_timer){int cap=in.y<0?2:in.y>0?8:4;if(p->dy>cap)p->dy=(int16_t)cap;p->parachute_timer--;}
    if(p->balloon_timer){p->dy=-4;p->balloon_timer--;}

    /* $DF20: on ordinary south support, a direct vertical command is scaled
       once more unless a side contact opposes the horizontal command. */
    if((p->contact_mask&0x10u)&&!p->attachment_mode){
        bool blocked=((p->contact_mask&0x04u)&&p->raw_x>0)||
                     ((p->contact_mask&0x40u)&&p->raw_x<0);
        if(!blocked)p->dy=(int16_t)(p->dy*4);
    }
    if(p->dy<0)p->vbank=0; else if(p->dy>0)p->vbank=4;
    FloodPoseSelection pose=flood_select_quiffy_pose(
        (uint16_t)(p->contact_mask|p->contact_aux),p->raw_contact_mask,
        p->contact_count,pose_raw_y,p->anim_phase,p->hbank,p->vbank,
        p->pose_code,flood_water_fill_at(g,p->x+8,p->y+8)!=0u);
    p->pose_code=pose.code;p->pose_active=pose.active;
    p->hbank=pose.hbank;p->vbank=pose.vbank;

    /* State 1 leaves $E1EC/$E1F2 at 32/0 through this complete movement
       resolution.  Every other path uses Quiffy's centred 16/+8 body. */
    const int16_t body_y=(int16_t)(p->y+(state1_geometry?0:8));
    const uint16_t body_height=(uint16_t)(state1_geometry?32:16);
    FloodCollisionResult q=flood_query_collision(g,(uint16_t)(p->x+8),
        (uint16_t)body_y,p->dx,p->dy,16,body_height);
    if((q.x_flags&FLOOD_ATTR_SOLID)&&!(q.x_flags&FLOOD_ATTR_SLOPE))p->dx=0;
    if(!(q.y_flags&FLOOD_ATTR_SLOPE)){
        while((q.y_flags&FLOOD_ATTR_SOLID)&&p->dy){
            p->dy=(int16_t)(p->dy+(p->dy<0?1:-1));
            q=flood_query_collision(g,(uint16_t)(p->x+8),(uint16_t)body_y,
                0,p->dy,16,body_height);
        }
        if((q.corner_flags&FLOOD_ATTR_SOLID)&&p->dx&&p->dy)p->dx=0;
    }
    /* $DEA4-$DEAC: a completely blocked update cancels the attachment
       cache.  This prevents a stale captured direction from being replayed
       indefinitely after Quiffy has come to rest against collision. */
    if(p->dx==0&&p->dy==0)p->attachment_mode=0u;
    p->x=(int16_t)(p->x+p->dx);p->y=(int16_t)(p->y+p->dy);
    if(p->dx||p->dy)
        p->anim_phase=(uint8_t)((p->anim_phase+object_frame_step(g))&3u);
}

bool flood_player_should_draw(const FloodGame *g){
    const bool protected_now=g->player_blink_active||g->player.invulnerable_timer!=0u;
    return !protected_now||g->render_buffer_index==0u;
}

static void finish_dispatch_after_items(FloodGame *g){
    flood_update_actions(g);update_object_dispatch_exact(g);
}

static void finish_gameplay_frame(FloodGame *g,int16_t life_before_dispatch){
    advance_quiffy_death_mode2(g);update_matilda(g);
    /* Damage, drowning, and actor dispatch can still touch Life Force while
       a mode-1 body falls.  Escape death freezes only the displayed gauge;
       the ordinary full reset from mode-2 completion remains visible once
       the final frame hands off to the screen transition. */
    if(g->escape_death_active){
        if(g->player.death_mode)g->player.life_force=g->escape_life_force;
        else g->escape_death_active=false;
    }
    update_damage_feedback(g,life_before_dispatch);flood_update_hud(g);
    /* `$ED66` has already consumed the visible secondary map when `$AF3C`
       advances it.  Preserve that pre-step state for the host presentation. */
    memcpy(g->world.render_water,g->world.water,sizeof(g->world.render_water));
    flood_update_water(g);update_transport(g);
    /* $CDA4-$CDC4 clears the skip latch, then reasserts it while the Orange
       Can timer was nonzero. */
    g->dispatch_suppressed=false;
    if(g->player.orange_timer){
        g->player.orange_timer--;g->dispatch_suppressed=true;
    }
    g->ticks++;
}

/* $D7E4-$D7F4: the original tests $E1AC immediately after polling input and
   zeroes both joystick axes whenever the active flame pose has set it.  The
   flag is independent of fire count, contact geometry, and airborne state. */
static FloodInput filter_flamethrower_movement(const FloodGame *g,FloodInput in){
    if(g->weapon_pose_active){in.x=0;in.y=0;}
    return in;
}

void flood_game_tick(FloodGame *g,FloodInput in){
    g->sound_pending=false;
    g->sound_queue_count=0u;
    if(in.quit){g->running=false;return;}
    if(g->zap_message_pending)return;
    g->render_tile_phase=(uint8_t)(g->ticks&3u);
    g->render_buffer_index=(uint8_t)(g->ticks&1u);
    if(in.fire){if(g->fire_ticks<255u)g->fire_ticks++;}else g->fire_ticks=0u;
    if(g->player.death_mode)g->fire_ticks=2u;
    in=filter_flamethrower_movement(g,in);
    g->player.raw_x=in.x;g->player.raw_y=in.y;
    const uint8_t old_contacts=g->player.contact_mask;
    const uint16_t effect_state=g->quiffy_effect.state;
    uint16_t prepared_contacts=0u;bool state1_geometry=false;
    /* The active outer loop enters $D7CE before snapshotting the camera and
       composing terrain.  Its $D29A auxiliary-record visit (including the
       independent $FC6C item path) precedes ordinary Quiffy physics. */
    const unsigned effect_mode=update_quiffy_effect(g,in,&prepared_contacts,
        &state1_geometry);
    if(effect_state==0u)update_special_items(g);
    update_player(g,in,old_contacts,effect_mode,prepared_contacts,state1_geometry);
    const int16_t life_before_dispatch=g->player.life_force;
    flood_update_camera(g);
    /* $EC4E composes terrain before $CAD2 can remove pickups, fire
       triggers, or mutate mechanism cells. */
    memcpy(g->world.render_terrain,g->world.terrain,sizeof(g->world.render_terrain));
    g->world.render_terrain_valid=true;

    /* After terrain has been composed, $CAD2 visits the ordinary map item,
       eight action records, and 128 object records in that order.  All of
       them consequently observe Quiffy's post-movement coordinates. */
    const bool skip_dispatch=g->dispatch_suppressed||effect_mode==2u;
    g->render_dispatch_suppressed=skip_dispatch;
    if(skip_dispatch)g->action_render_count=0u;
    if(!skip_dispatch){
        update_map_items(g);
        if(g->zap_message_pending)return;
        finish_dispatch_after_items(g);
    }
    /* $CDF0-$D002 advances death mode 2 before Aunt Matilda consumes and
       appends her history sample; the player sprite is selected afterward. */
    finish_gameplay_frame(g,life_before_dispatch);
}

/* $AD94-$B202: the level banner is displayed for counter values 50..0.
   Only its first frame calls $D7CE; all 51 frames still exchange buffers and
   advance the two global render phases. */
void flood_game_level_banner_tick(FloodGame *g,FloodInput in,bool first_frame){
    g->sound_pending=false;g->sound_queue_count=0u;
    if(in.quit){g->running=false;return;}
    g->render_tile_phase=(uint8_t)(g->ticks&3u);
    g->render_buffer_index=(uint8_t)(g->ticks&1u);
    if(first_frame){
        if(in.fire){if(g->fire_ticks<255u)g->fire_ticks++;}else g->fire_ticks=0u;
        if(g->player.death_mode)g->fire_ticks=2u;
        in=filter_flamethrower_movement(g,in);
        g->player.raw_x=in.x;g->player.raw_y=in.y;
        const uint8_t old_contacts=g->player.contact_mask;
        const uint16_t effect_state=g->quiffy_effect.state;
        uint16_t prepared_contacts=0u;bool state1_geometry=false;
        const unsigned effect_mode=update_quiffy_effect(g,in,&prepared_contacts,
            &state1_geometry);
        if(effect_state==0u)update_special_items(g);
        update_player(g,in,old_contacts,effect_mode,prepared_contacts,
            state1_geometry);
        flood_update_camera(g);
    }
    memcpy(g->world.render_terrain,g->world.terrain,sizeof(g->world.render_terrain));
    g->world.render_terrain_valid=true;flood_update_hud(g);g->ticks++;
}

/* $FF00 follows the blocking $99AC call, so the award becomes visible only
   after the message returns. */
void flood_complete_zap_message(FloodGame *g){
    if(!g->zap_message_pending)return;
    g->zap_message_pending=false;g->score+=20u;
    finish_dispatch_after_items(g);finish_gameplay_frame(g,g->player.life_force);
}

/* The optional cooperative mode lives in its own translation unit fragment so
   it can reuse the exact private player and item routines above. */
#include "multiplayer_game.inc"
