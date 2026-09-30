/* Included only by game.c, after the single-player implementation. The
   original flood_game_tick is untouched and still advances the shared map,
   water, objects, and global clock exactly once. */
#include "flood_multiplayer.h"

/* Per-actor fields that the original interpreter keeps in FloodGame. Keep
   this list in one place for switching actors and rendering either camera. */
#define MP_ACTOR_FIELDS(X) \
    X(player) X(matilda) X(actions) X(quiffy_effect) X(action_renders) X(hud_mask) \
    X(camera_x) X(camera_y) X(lives) X(score) X(last_pickup_tile) X(last_special_tile) \
    X(action_render_count) X(selected_weapon_state) X(fire_ticks) \
    X(weapon_main_offset) X(weapon_tip_offset) X(transport_timer) \
    X(transport_effect_stage) X(transport_x) X(transport_y) \
    X(transport_render_x) X(transport_render_y) X(action_target_x) \
    X(action_target_y) X(quiffy_effect_render_x) X(quiffy_effect_render_y) \
    X(quiffy_effect_sprite) X(weapon_pose_active) X(dispatch_suppressed) \
    X(render_dispatch_suppressed) X(quiffy_effect_pose_active) \
    X(quiffy_effect_render_active) X(player_blink_active) \
    X(special_entry_offset) X(transport_render_prejump) X(ouch_visible) \
    X(damage_contact_previous) X(water_contact_previous) \
    X(slope_correction_previous) X(escape_death_active) X(escape_life_force)

static void mp_swap_actors(FloodGame *a,FloodGame *b){
#define MP_SWAP(f) do{unsigned char tmp[sizeof(a->f)];memcpy(tmp,&a->f,sizeof(tmp)); \
    memcpy(&a->f,&b->f,sizeof(tmp));memcpy(&b->f,tmp,sizeof(tmp));}while(0);
    MP_ACTOR_FIELDS(MP_SWAP)
#undef MP_SWAP
}

/* Quiffy 2 and Matilda 2 use the same brown ramp. Only the second actor's
   masked pixels pass through this table; original game art remains untouched. */
uint8_t flood_multiplayer_actor_colour(uint8_t original){
    static const uint8_t brown[16]={
        0u,4u,4u,5u,4u,5u,6u,7u,
        4u,5u,6u,7u,4u,5u,6u,7u
    };
    return brown[original&15u];
}

void flood_multiplayer_second_view(const FloodMultiplayer *m,FloodGame *out){
    *out=*m->game;
#define MP_COPY(f) memcpy(&out->f,&m->second->f,sizeof(out->f));
    MP_ACTOR_FIELDS(MP_COPY)
#undef MP_COPY
    out->multiplayer_opponent=&m->game->player;
}

void flood_multiplayer_reset_second(FloodMultiplayer *m){
    FloodGame *g=m->game,*s=m->second;
    const uint32_t earned=s->score;
    const int16_t lives=s->lives;
    memset(s,0,sizeof(*s));
    s->player=g->player;s->matilda.read_pos=1u;
    s->score=earned;
    s->lives=lives;s->last_pickup_tile=UINT16_MAX;
    s->last_special_tile=UINT16_MAX;
    /* A short horizontal offset makes both actors visible. Stay on the same
       spawn when nearby terrain blocks either candidate. */
    const int16_t start=g->player.x;
    for(unsigned n=0;n<2u;n++){
        const int step=n==0u?8:-8;
        FloodCollisionResult q=flood_query_collision(g,(uint16_t)(start+8),
            (uint16_t)(g->player.y+8),(int16_t)step,0,16,16);
        if(!(q.x_flags&FLOOD_ATTR_SOLID)){
            s->player.x=(int16_t)(start+step);break;
        }
    }
    mp_swap_actors(g,s);flood_update_camera(g);flood_update_hud(g);mp_swap_actors(g,s);
    g->multiplayer_opponent=s->lives>0?&s->player:NULL;
}

void flood_multiplayer_begin(FloodMultiplayer *m){
    m->terminal_animation=false;
    m->second->score=0u;
    m->second->lives=3;
    flood_multiplayer_reset_second(m);
}

void flood_multiplayer_restart_finish(FloodMultiplayer *m,
    int16_t first_lives,int16_t second_lives){
    FloodGame *g=m->game,*s=m->second;
    if(first_lives>0)flood_game_restart_finish(g);
    else{g->lives=0;g->player.death_mode=0u;}
    flood_update_hud(g);
    mp_swap_actors(g,s);
    if(second_lives>0)flood_game_restart_finish(g);
    else{g->lives=0;g->player.death_mode=0u;}
    flood_update_hud(g);
    mp_swap_actors(g,s);
    g->multiplayer_opponent=s->lives>0?&s->player:NULL;
    m->terminal_animation=g->lives<=0&&s->lives<=0&&
        (g->player.death_mode!=0u||s->player.death_mode!=0u);
}

/* The original 51-frame entry sequence updates movement and protection on
   its first frame only. Run that first update for each actor but advance the
   shared banner clock only once. */
void flood_multiplayer_banner_tick(FloodMultiplayer *m,FloodInput first,
    FloodInput second,bool first_frame){
    FloodGame *g=m->game,*s=m->second;
    const uint32_t previous=g->ticks;
    flood_game_level_banner_tick(g,first,first_frame);
    const uint32_t next=g->ticks;
    g->ticks=previous;
    mp_swap_actors(g,s);
    flood_game_level_banner_tick(g,second,first_frame);
    mp_swap_actors(g,s);
    g->ticks=next;
    g->multiplayer_opponent=s->lives>0?&s->player:NULL;
}

/* Object movement is authoritative in the first dispatch. Resolve contact
   against the second body afterward without advancing an object twice. */
static void mp_second_object_contacts(FloodGame *g){
    FloodPlayer *p=&g->player;
    if(p->death_mode||p->life_force<=0)return;
    for(unsigned n=0;n<FLOOD_OBJECT_MAX;n++){
        const FloodObject *o=&g->objects[n];
        if(!o->active||!o->state)continue;
        int damage=0,width=32,height=32;
        switch(o->state){
        case 1u:case 8u:case 12u:case 14u:damage=16;break;
        case 5u:case 6u:case 9u:case 15u:damage=8;break;
        case 2u:case 13u:case 20u:damage=40;break;
        case 22u:damage=512;height=8;break;
        default:break;
        }
        if(damage&&rectangles_overlap(o->x,o->y,width,height,
            p->x+8,p->y+8,16,16))p->life_force=(int16_t)(p->life_force-damage);
    }
}

/* The original Matilda update already tests its owner. The other player's
   body is tested after both ghosts and both players have moved this frame. */
static bool mp_ghost_hits_other(const FloodMatilda *ghost,FloodPlayer *other){
    if(!ghost->visible||!rectangles_overlap(ghost->x,ghost->y,16,16,
        other->x,other->y,16,16))return false;
    other->life_force=(int16_t)(other->life_force-10);
    return true;
}

/* The native feedback pass runs inside each actor's dispatcher. A weapon
   owned by the other actor can hit outside that pass, so combine those hits
   with cross-Matilda contact before closing the frame's damage latch. */
static void mp_cross_damage_feedback(FloodGame *g,bool cross_hit,
    bool contact_before){
    if(!cross_hit||g->ouch_visible)return;
    g->damage_contact_previous=contact_before;
    /* The hit may precede this actor's protection update. Use the observed
       hit event to drive the same bubble/voice even if that update restored
       Life Force before the native feedback pass. */
    const int16_t before=g->player.life_force==INT16_MAX?INT16_MAX:
        (int16_t)(g->player.life_force+1);
    update_damage_feedback(g,before);
}

void flood_multiplayer_tick(FloodMultiplayer *m,FloodInput first,FloodInput second){
    FloodGame *g=m->game,*s=m->second;
    if(g->lives<=0&&g->player.death_mode==0u){
        if(s->lives<=0&&s->player.death_mode==0u){g->running=false;return;}
        /* The surviving actor becomes authoritative for the shared world.
           Switch back immediately so external player identities stay fixed. */
        mp_swap_actors(g,s);
        flood_multiplayer_tick(m,second,first);
        mp_swap_actors(g,s);
        g->multiplayer_opponent=s->lives>0?&s->player:NULL;
        return;
    }
    g->multiplayer_opponent=s->lives>0?&s->player:NULL;
    /* The first player runs the authoritative world dispatcher. */
    const bool first_contact_before=g->damage_contact_previous;
    const bool second_contact_before=s->damage_contact_previous;
    const int16_t second_life_before_first=s->player.life_force;
    const uint32_t frame_tick=g->ticks;
    flood_game_tick(g,first);
    if(g->zap_message_pending)return;
    const int16_t second_life_after_first=s->player.life_force;
    const bool hit_second_weapon=second_life_after_first<second_life_before_first;
    const int16_t first_life_before_second=g->player.life_force;
    const uint32_t next_tick=g->ticks;
    g->ticks=frame_tick;
    mp_swap_actors(g,s);
    g->multiplayer_opponent=s->lives>0?&s->player:NULL;
    if(g->lives>0||g->player.death_mode){
        if(second.fire){if(g->fire_ticks<255u)g->fire_ticks++;}
        else g->fire_ticks=0u;
        if(g->player.death_mode)g->fire_ticks=2u;
        second=filter_flamethrower_movement(g,second);
        g->player.raw_x=second.x;g->player.raw_y=second.y;
        const uint8_t old_contacts=g->player.contact_mask;
        const uint16_t effect_state=g->quiffy_effect.state;
        uint16_t prepared_contacts=0u;bool state1_geometry=false;
        const unsigned effect_mode=update_quiffy_effect(g,second,&prepared_contacts,
            &state1_geometry);
        if(effect_state==0u)update_special_items(g);
        const bool protection_refresh=g->player.invulnerable_timer!=0u&&
            !g->escape_death_active;
        update_player(g,second,old_contacts,effect_mode,prepared_contacts,state1_geometry);
        /* The authoritative actor's action ran before this actor's
           protection refresh. Apply its loss after that refresh, matching
           the order used when the protected actor owns the weapon. */
        if(hit_second_weapon&&protection_refresh&&
            g->player.life_force>second_life_after_first){
            const int loss=(int)second_life_before_first-second_life_after_first;
            const int corrected=(int)g->player.life_force-loss;
            g->player.life_force=(int16_t)(corrected<INT16_MIN?INT16_MIN:corrected);
        }
        const int16_t life_before=g->player.life_force;
        mp_second_object_contacts(g);
        flood_update_camera(g);
        if(!g->dispatch_suppressed&&effect_mode!=2u){
            update_map_items(g);
            if(!g->zap_message_pending)flood_update_actions(g);
        }else g->action_render_count=0u;
        advance_quiffy_death_mode2(g);
        update_matilda(g);
        update_damage_feedback(g,life_before);
        flood_update_hud(g);
        update_transport(g);
        g->dispatch_suppressed=false;
        if(g->player.orange_timer){g->player.orange_timer--;g->dispatch_suppressed=true;}
    }
    mp_swap_actors(g,s);
    g->multiplayer_opponent=s->lives>0?&s->player:NULL;
    g->ticks=next_tick;
    const bool hit_first_weapon=g->player.life_force<first_life_before_second;
    bool hit_second_ghost=false,hit_first_ghost=false;
    if(g->lives>0&&s->lives>0){
        hit_second_ghost=mp_ghost_hits_other(&g->matilda,&s->player);
        hit_first_ghost=mp_ghost_hits_other(&s->matilda,&g->player);
    }
    mp_cross_damage_feedback(g,hit_first_weapon||hit_first_ghost,
        first_contact_before);
    mp_swap_actors(g,s);
    mp_cross_damage_feedback(g,hit_second_weapon||hit_second_ghost,
        second_contact_before);
    flood_update_hud(g);
    mp_swap_actors(g,s);
    if(g->lives<=0)g->matilda.visible=false;
    if(s->lives<=0)s->matilda.visible=false;
    flood_update_hud(g);
    /* One player can continue after the other has exhausted their lives. */
    if(!first.quit)g->running=g->lives>0||s->lives>0||
        (m->terminal_animation&&
            (g->player.death_mode!=0u||s->player.death_mode!=0u));
}
#undef MP_ACTOR_FIELDS
