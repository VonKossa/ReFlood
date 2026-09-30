#include "flood_multiplayer.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef FLOOD_DATA_DIR
#define FLOOD_DATA_DIR "./data"
#endif

int main(void){
    FloodMultiplayer m={.game=calloc(1,sizeof(FloodGame)),
        .second=calloc(1,sizeof(FloodGame))};
    assert(m.game&&m.second);
    char err[160]={0};
    flood_game_init(m.game);
    assert(flood_game_load_level(m.game,FLOOD_DATA_DIR,1u,err,sizeof(err)));
    flood_multiplayer_begin(&m);
    assert(m.game->multiplayer_opponent==&m.second->player);
    assert(m.second->player.x!=m.game->player.x);
    assert(m.second->player.y==m.game->player.y);
    const uint32_t before_banner=m.game->ticks;
    assert(m.game->player.invulnerable_timer==100u);
    assert(m.second->player.invulnerable_timer==100u);
    for(unsigned frame=0;frame<FLOOD_LEVEL_BANNER_FRAMES;frame++)
        flood_multiplayer_banner_tick(&m,(FloodInput){0},(FloodInput){0},frame==0u);
    assert(m.game->ticks==before_banner+FLOOD_LEVEL_BANNER_FRAMES);
    assert(m.game->player.invulnerable_timer==99u);
    assert(m.second->player.invulnerable_timer==99u);
    assert(m.game->player_blink_active&&m.second->player_blink_active);
    const uint32_t initial_ticks=m.game->ticks;
    FloodInput first={0},second={0};second.x=1;
    flood_multiplayer_tick(&m,first,second);
    assert(m.game->ticks==initial_ticks+1u);
    assert(m.game->matilda.write_pos==1u);
    assert(m.second->matilda.write_pos==1u);
    assert(m.second->matilda.history[1].x==m.second->player.x);
    assert(m.game->matilda.history[1].x==m.game->player.x);
    FloodGame *view=malloc(sizeof(*view));assert(view);
    flood_multiplayer_second_view(&m,view);
    assert(view->player.x==m.second->player.x);
    assert(view->world.remaining_food==m.game->world.remaining_food);
    assert(view->ticks==m.game->ticks);
    assert(view->score==m.second->score);
    assert(flood_multiplayer_actor_colour(8u)==4u);
    assert(flood_multiplayer_actor_colour(14u)==6u);
    /* The same cavern counter falls for either pickup, but only its owner
       receives the points. The second score survives a level reset. */
    memset(m.game->world.terrain,0,sizeof(m.game->world.terrain));
    memset(m.game->world.attr,0,sizeof(m.game->world.attr));
    memset(m.game->objects,0,sizeof(m.game->objects));
    memset(m.game->world.water,0,sizeof(m.game->world.water));
    m.game->world.remaining_food=2;
    m.game->player.x=128;m.game->player.y=64;
    m.second->player.x=64;m.second->player.y=64;
    m.game->last_pickup_tile=m.second->last_pickup_tile=UINT16_MAX;
    m.game->world.terrain[5u*FLOOD_MAP_W+5u]=0x94u;
    const uint32_t first_score=m.game->score,second_score=m.second->score;
    second=(FloodInput){0};
    flood_multiplayer_tick(&m,first,second);
    assert(m.game->score==first_score);
    assert(m.second->score==second_score+2u);
    assert(m.game->world.remaining_food==1);
    flood_multiplayer_second_view(&m,view);
    assert(view->score==m.second->score);
    assert(view->world.remaining_food==m.game->world.remaining_food);
    m.game->world.terrain[5u*FLOOD_MAP_W+9u]=0x94u;
    m.game->last_pickup_tile=UINT16_MAX;
    flood_multiplayer_tick(&m,first,second);
    assert(m.game->score==first_score+2u);
    assert(m.second->score==second_score+2u);
    assert(m.game->world.remaining_food==0);
    const uint32_t retained_score=m.second->score;
    flood_multiplayer_reset_second(&m);
    assert(m.second->score==retained_score);
    /* Either actor may use the shared exit after the last item is gone. */
    memset(m.game->world.terrain,0,sizeof(m.game->world.terrain));
    memset(m.game->world.attr,0,sizeof(m.game->world.attr));
    memset(m.game->objects,0,sizeof(m.game->objects));
    memset(m.game->world.water,0,sizeof(m.game->world.water));
    m.game->world.remaining_food=0;
    m.game->player.x=128;m.game->player.y=128;
    m.second->player.x=64;m.second->player.y=64;
    m.second->last_pickup_tile=UINT16_MAX;
    m.game->world.terrain[5u*FLOOD_MAP_W+5u]=0x88u;
    second=(FloodInput){0};
    flood_multiplayer_tick(&m,first,second);
    assert(m.game->level_complete);
    assert(m.game->world.remaining_food==-1);
    assert(m.game->ticks==initial_ticks+4u);
    /* An action owned by either player can hit the other body. */
    m.game->level_complete=false;m.game->world.remaining_food=1;
    memset(m.game->world.terrain,0,sizeof(m.game->world.terrain));
    m.game->player.x=64;m.game->player.y=64;
    m.second->player.x=80;m.second->player.y=64;
    m.game->player.invulnerable_timer=0;
    m.second->player.invulnerable_timer=0;
    m.game->actions[0].state=17u;
    m.game->actions[0].x=64;m.game->actions[0].y=64;
    m.game->actions[0].target=1u;m.game->actions[0].timer=10u;
    flood_multiplayer_tick(&m,first,second);
    assert(m.second->player.life_force<511);
    memset(m.game->actions,0,sizeof(m.game->actions));
    m.game->player.x=80;m.game->player.y=64;m.game->player.life_force=511;
    m.second->player.x=64;m.second->player.y=64;
    m.second->actions[0].state=17u;
    m.second->actions[0].x=64;m.second->actions[0].y=64;
    m.second->actions[0].target=1u;m.second->actions[0].timer=10u;
    flood_multiplayer_tick(&m,first,second);
    assert(m.game->player.life_force<511);
    /* Each ghost can damage the other player as well as its owner. */
    memset(m.game->actions,0,sizeof(m.game->actions));
    memset(m.second->actions,0,sizeof(m.second->actions));
    memset(&m.game->matilda,0,sizeof(m.game->matilda));
    memset(&m.second->matilda,0,sizeof(m.second->matilda));
    m.game->matilda.read_pos=m.second->matilda.read_pos=1u;
    m.game->player.x=64;m.game->player.y=64;
    m.second->player.x=160;m.second->player.y=64;
    m.game->player.life_force=m.second->player.life_force=511;
    m.game->damage_contact_previous=m.second->damage_contact_previous=false;
    m.game->ouch_visible=m.second->ouch_visible=false;
    m.game->matilda.history[1].x=160;m.game->matilda.history[1].y=64;
    m.second->matilda.history[1].x=64;m.second->matilda.history[1].y=64;
    flood_multiplayer_tick(&m,first,second);
    assert(m.game->player.life_force==501);
    assert(m.second->player.life_force==501);
    assert(m.game->ouch_visible&&m.second->ouch_visible);
    unsigned hurt_voices=0u;
    for(unsigned n=0;n<m.game->sound_queue_count;n++)
        if(m.game->sound_queue[n]>=1u&&m.game->sound_queue[n]<=5u)hurt_voices++;
    assert(hurt_voices==2u);
    /* Sustained overlap costs health each frame but must not restart either
       hurt voice until contact has actually ended and begun again. */
    for(unsigned frame=0;frame<3u;frame++){
        m.game->matilda.history[m.game->matilda.read_pos]=
            (FloodHistorySample){m.second->player.x,m.second->player.y,0u};
        m.second->matilda.history[m.second->matilda.read_pos]=
            (FloodHistorySample){m.game->player.x,m.game->player.y,0u};
        const int16_t before_first=m.game->player.life_force;
        const int16_t before_second=m.second->player.life_force;
        flood_multiplayer_tick(&m,first,second);
        assert(m.game->player.life_force==before_first-10);
        assert(m.second->player.life_force==before_second-10);
        hurt_voices=0u;
        for(unsigned n=0;n<m.game->sound_queue_count;n++)
            if(m.game->sound_queue[n]>=1u&&m.game->sound_queue[n]<=5u)hurt_voices++;
        assert(hurt_voices==0u);
    }
    m.game->matilda.history[m.game->matilda.read_pos]=(FloodHistorySample){0};
    m.second->matilda.history[m.second->matilda.read_pos]=(FloodHistorySample){0};
    flood_multiplayer_tick(&m,first,second);
    assert(!m.game->ouch_visible&&!m.second->ouch_visible);
    m.game->matilda.history[m.game->matilda.read_pos]=
        (FloodHistorySample){m.second->player.x,m.second->player.y,0u};
    m.second->matilda.history[m.second->matilda.read_pos]=
        (FloodHistorySample){m.game->player.x,m.game->player.y,0u};
    flood_multiplayer_tick(&m,first,second);
    hurt_voices=0u;
    for(unsigned n=0;n<m.game->sound_queue_count;n++)
        if(m.game->sound_queue[n]>=1u&&m.game->sound_queue[n]<=5u)hurt_voices++;
    assert(hurt_voices==2u);
    /* Owner contact plus cross contact applies one hit from each ghost. */
    memset(&m.game->matilda,0,sizeof(m.game->matilda));
    memset(&m.second->matilda,0,sizeof(m.second->matilda));
    m.game->matilda.read_pos=m.second->matilda.read_pos=1u;
    m.game->player.x=64;m.game->player.y=64;
    m.second->player.x=160;m.second->player.y=64;
    m.game->player.life_force=m.second->player.life_force=511;
    m.game->matilda.history[1].x=64;m.game->matilda.history[1].y=64;
    m.second->matilda.history[1].x=64;m.second->matilda.history[1].y=64;
    flood_multiplayer_tick(&m,first,second);
    assert(m.game->player.life_force==491);
    assert(m.second->player.life_force==511);
    assert(m.game->ouch_visible);
    /* The shared world continues with Player 2 when Player 1 is out. */
    m.game->lives=0;
    const int16_t surviving_x=m.second->player.x;
    const uint32_t before_survivor=m.game->ticks;
    second.x=1;
    flood_multiplayer_tick(&m,first,second);
    assert(m.game->running);
    assert(m.game->ticks==before_survivor+1u);
    assert(m.second->player.x>=surviving_x);
    assert(m.game->lives==0);
    /* No shipped-level coordinates are assumed by the second spawn/tick. */
    for(unsigned level=1u;level<=42u;level++){
        flood_game_init(m.game);
        assert(flood_game_load_level(m.game,FLOOD_DATA_DIR,level,err,sizeof(err)));
        flood_multiplayer_begin(&m);
        const uint32_t start=m.game->ticks;
        flood_multiplayer_tick(&m,(FloodInput){0},(FloodInput){0});
        assert(m.game->ticks==start+1u);
        assert(m.game->world.level_number==level);
    }
    flood_game_init(m.game);
    assert(flood_game_load_level(m.game,FLOOD_DATA_DIR,1u,err,sizeof(err)));
    flood_multiplayer_begin(&m);
    memset(m.game->world.terrain,0,sizeof(m.game->world.terrain));
    memset(m.game->world.attr,0,sizeof(m.game->world.attr));
    memset(m.game->objects,0,sizeof(m.game->objects));
    memset(m.game->world.water,0,sizeof(m.game->world.water));
    m.game->player.x=128;m.game->player.y=64;
    m.second->player.x=32;m.second->player.y=64;
    first=(FloodInput){0};second=(FloodInput){0};
    /* Weapon stations remain available in the shared map so either player
       can take the same grenade, regardless of who arrives first. */
    m.game->world.terrain[5u*FLOOD_MAP_W+9u]=0xddu;
    m.game->selected_weapon_state=m.second->selected_weapon_state=0u;
    m.game->last_pickup_tile=m.second->last_pickup_tile=UINT16_MAX;
    flood_multiplayer_tick(&m,first,second);
    assert(m.game->selected_weapon_state==1u);
    assert(m.second->selected_weapon_state==0u);
    assert(m.game->world.terrain[5u*FLOOD_MAP_W+9u]==0xddu);
    m.game->player.x=32;
    m.second->player.x=128;
    m.second->player.y=64;
    flood_multiplayer_tick(&m,first,second);
    assert(m.second->selected_weapon_state==1u);
    assert(m.game->world.terrain[5u*FLOOD_MAP_W+9u]==0xddu);
    m.game->world.terrain[5u*FLOOD_MAP_W+9u]=0xdeu;
    m.game->last_pickup_tile=m.second->last_pickup_tile=UINT16_MAX;
    m.second->player.x=32;
    flood_multiplayer_tick(&m,first,second);
    m.game->player.x=128;
    m.game->player.y=64;
    flood_multiplayer_tick(&m,first,second);
    assert(m.game->selected_weapon_state==4u);
    m.second->player.x=128;m.second->player.y=64;m.game->player.x=32;
    flood_multiplayer_tick(&m,first,second);
    assert(m.second->selected_weapon_state==4u);
    assert(m.game->world.terrain[5u*FLOOD_MAP_W+9u]==0xdeu);
    /* Escape starts the original falling/cross sequence for both actors;
       the shared session ends only after both animations have completed. */
    flood_game_init(m.game);
    assert(flood_game_load_level(m.game,FLOOD_DATA_DIR,1u,err,sizeof(err)));
    flood_multiplayer_begin(&m);
    m.terminal_animation=true;
    first=(FloodInput){0};second=(FloodInput){0};
    first.restart=second.restart=true;
    flood_multiplayer_tick(&m,first,second);
    assert(m.game->lives==0&&m.second->lives==0);
    assert(m.game->player.death_mode&&m.second->player.death_mode);
    assert(m.game->running);
    first.restart=second.restart=false;
    unsigned escape_frames=0u;
    while(m.game->running&&escape_frames++<180u)
        flood_multiplayer_tick(&m,first,second);
    assert(escape_frames<180u);
    assert(!m.game->running);
    assert(m.game->player.death_mode==0u&&m.second->player.death_mode==0u);
    assert(!m.game->matilda.visible&&!m.second->matilda.visible);

    /* Restart first shows the full banner and only then charges one life to
       each survivor. The last-life actor completes their death sequence. */
    flood_game_init(m.game);
    assert(flood_game_load_level(m.game,FLOOD_DATA_DIR,1u,err,sizeof(err)));
    flood_multiplayer_begin(&m);
    m.game->lives=2;m.second->lives=1;
    m.game->score=44u;m.second->score=33u;
    assert(flood_game_restart_begin(m.game,FLOOD_DATA_DIR,err,sizeof(err)));
    flood_multiplayer_reset_second(&m);
    assert(m.game->lives==2&&m.second->lives==1);
    for(unsigned n=0;n<FLOOD_LEVEL_BANNER_FRAMES;n++)
        flood_multiplayer_banner_tick(&m,(FloodInput){0},(FloodInput){0},n==0u);
    flood_multiplayer_restart_finish(&m,2,1);
    assert(m.game->lives==1&&m.second->lives==0);
    assert(m.game->score==44u&&m.second->score==33u);
    assert(m.second->player.death_mode==1u);
    flood_multiplayer_tick(&m,(FloodInput){0},(FloodInput){0});
    assert(!m.second->matilda.visible&&m.game->running);
    unsigned restart_frames=0u;
    while(m.second->player.death_mode&&restart_frames++<180u)
        flood_multiplayer_tick(&m,(FloodInput){0},(FloodInput){0});
    assert(restart_frames<180u&&m.game->running);

    /* Explosion contact mirrors the original self-damage for the other
       actor, even during level-start protection. Verify both directions. */
    flood_game_init(m.game);
    assert(flood_game_load_level(m.game,FLOOD_DATA_DIR,2u,err,sizeof(err)));
    flood_multiplayer_begin(&m);
    memset(m.game->world.terrain,0,sizeof(m.game->world.terrain));
    memset(m.game->world.attr,0,sizeof(m.game->world.attr));
    memset(m.game->objects,0,sizeof(m.game->objects));
    m.game->player.x=200;m.game->player.y=64;
    m.second->player.x=96;m.second->player.y=64;
    m.second->player.life_force=511;
    m.second->player.invulnerable_timer=99u;
    memset(m.game->actions,0,sizeof(m.game->actions));
    m.game->actions[0].state=12u;m.game->actions[0].x=104;
    m.game->actions[0].y=64;m.game->actions[0].anim=0u;
    flood_update_actions(m.game);
    assert(m.second->player.life_force==471);
    assert(m.game->player.life_force==511);
    m.second->multiplayer_opponent=&m.game->player;
    m.game->player.x=96;m.game->player.life_force=511;
    m.game->player.invulnerable_timer=99u;
    m.second->player.x=200;
    memset(m.second->actions,0,sizeof(m.second->actions));
    m.second->actions[0].state=12u;m.second->actions[0].x=104;
    m.second->actions[0].y=64;m.second->actions[0].anim=0u;
    flood_update_actions(m.second);
    assert(m.game->player.life_force==471);

    m.game->player.life_force=511;
    memset(m.second->actions,0,sizeof(m.second->actions));
    m.second->actions[0].state=3u;m.second->actions[0].x=96;
    m.second->actions[0].y=64;m.second->actions[0].dx=0;
    m.second->actions[0].dy=0;m.second->actions[0].timer=7u;
    flood_update_actions(m.second);
    assert(m.game->player.life_force<511);

    /* Projectile and beam families still use the common PvP collision. */
    m.second->multiplayer_opponent=NULL;
    m.game->player.x=64;m.game->player.y=64;
    m.second->player.x=96;m.second->player.y=64;
    m.second->player.invulnerable_timer=0u;
    m.game->multiplayer_opponent=&m.second->player;
    for(unsigned weapon=0;weapon<3u;weapon++){
        m.second->player.life_force=511;
        memset(m.game->actions,0,sizeof(m.game->actions));
        FloodAction *a=&m.game->actions[0];
        if(weapon==0u){
            a->state=7u;a->x=80;a->y=72;a->dx=16;a->dy=0;
            a->aux=16u;a->timer=20u;
        }else if(weapon==1u){
            a->state=9u;a->x=96;a->y=64;a->timer=3u;
        }else{
            a->state=14u;m.game->fire_ticks=5u;
            m.game->player.hbank=2u;
        }
        flood_update_actions(m.game);
        assert(m.second->player.life_force<511);
    }

    /* Every weapon family must produce the same owner-style bubble and one
       hurt voice on either opponent, including level-2 dynamite. */
    for(unsigned owner=0;owner<2u;owner++)for(unsigned weapon=0;weapon<6u;weapon++){
        flood_game_init(m.game);
        assert(flood_game_load_level(m.game,FLOOD_DATA_DIR,2u,err,sizeof(err)));
        flood_multiplayer_begin(&m);
        memset(m.game->world.terrain,0,sizeof(m.game->world.terrain));
        memset(m.game->world.attr,0,sizeof(m.game->world.attr));
        memset(m.game->world.water,0,sizeof(m.game->world.water));
        memset(m.game->objects,0,sizeof(m.game->objects));
        memset(&m.game->matilda,0,sizeof(m.game->matilda));
        memset(&m.second->matilda,0,sizeof(m.second->matilda));
        m.game->matilda.read_pos=m.second->matilda.read_pos=1u;
        m.game->player.x=owner?96:64;m.game->player.y=64;
        m.second->player.x=owner?64:96;m.second->player.y=64;
        m.game->player.life_force=m.second->player.life_force=511;
        m.game->player.invulnerable_timer=m.second->player.invulnerable_timer=0u;
        FloodGame *attacker=owner?m.second:m.game;
        FloodGame *victim=owner?m.game:m.second;
        FloodAction *a=&attacker->actions[0];
        switch(weapon){
        case 0u:a->state=12u;a->x=104;a->y=64;break; /* dynamite */
        case 1u:a->state=3u;a->x=96;a->y=64;a->dx=0;a->dy=0;
            a->timer=7u;break; /* grenade */
        case 2u:a->state=7u;a->x=80;a->y=72;a->dx=16;a->dy=0;
            a->aux=16u;a->timer=20u;break; /* boomerang */
        case 3u:a->state=9u;a->x=96;a->y=66;a->timer=3u;break; /* shuriken */
        case 4u:a->state=14u;attacker->fire_ticks=4u;
            attacker->player.hbank=2u;break; /* flamethrower */
        default:a->state=17u;a->x=64;a->y=64;a->target=2u;
            a->timer=10u;break; /* radial flame */
        }
        if(weapon==0u)victim->player.invulnerable_timer=99u;
        first=(FloodInput){0};second=(FloodInput){0};
        if(weapon==4u){if(owner)second.fire=true;else first.fire=true;}
        flood_multiplayer_tick(&m,first,second);
        assert(victim->player.life_force<511);
        if(weapon==0u)assert(victim->player.life_force==471);
        assert(victim->ouch_visible);
        flood_multiplayer_second_view(&m,view);
        assert((owner?m.game->ouch_visible:view->ouch_visible));
        unsigned voices=0u;
        for(unsigned n=0;n<m.game->sound_queue_count;n++)
            if(m.game->sound_queue[n]>=1u&&m.game->sound_queue[n]<=5u)voices++;
        assert(voices==(attacker->ouch_visible?2u:1u));
        if(weapon==0u){
            /* Continuous dynamite overlap keeps its bubble but does not
               restart the voice until a clear frame has elapsed. */
            attacker->actions[0].state=12u;
            attacker->actions[0].x=104;attacker->actions[0].y=64;
            attacker->actions[0].anim=0u;
            m.game->player.y=m.second->player.y=64;
            flood_multiplayer_tick(&m,first,second);
            assert(victim->ouch_visible);
            voices=0u;
            for(unsigned n=0;n<m.game->sound_queue_count;n++)
                if(m.game->sound_queue[n]>=1u&&m.game->sound_queue[n]<=5u)voices++;
            assert(voices==0u);
            memset(attacker->actions,0,sizeof(attacker->actions));
            flood_multiplayer_tick(&m,first,second);
            assert(!victim->ouch_visible);
            attacker->actions[0].state=12u;
            attacker->actions[0].x=104;attacker->actions[0].y=64;
            attacker->actions[0].anim=0u;
            m.game->player.y=m.second->player.y=64;
            flood_multiplayer_tick(&m,first,second);
            voices=0u;
            for(unsigned n=0;n<m.game->sound_queue_count;n++)
                if(m.game->sound_queue[n]>=1u&&m.game->sound_queue[n]<=5u)voices++;
            assert(voices==1u);
        }
    }

    flood_game_init(m.game);
    assert(flood_game_load_level(m.game,FLOOD_DATA_DIR,42u,err,sizeof(err)));
    flood_multiplayer_begin(&m);
    m.game->score=321u;m.second->score=123u;
    m.game->world.remaining_food=-1;m.game->level_complete=true;
    assert(flood_game_advance_level(m.game,FLOOD_DATA_DIR,err,sizeof(err)));
    assert(m.game->game_complete&&!m.game->running);
    assert(m.game->score==321u&&m.second->score==123u);
    free(view);free(m.second);free(m.game);
    puts("multiplayer core ok");return 0;
}
