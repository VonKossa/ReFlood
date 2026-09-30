#include "flood_game.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_swept_collision(void){
    FloodGame g; memset(&g,0,sizeof(g));g.matilda.read_pos=1u;
    g.world.attr[7]=FLOOD_ATTR_SOLID;
    /* 16x16 actor at (16,16), moving +2 x: right edge reaches tile x=2. */
    g.world.terrain[1*FLOOD_MAP_W+2]=7;
    FloodCollisionResult q=flood_query_collision(&g,16,16,2,0,16,16);
    assert((q.x_flags&FLOOD_ATTR_SOLID)!=0);
    assert(q.y_flags==0);
}

static void test_contact_ring_and_resolver(void){
    FloodGame g; memset(&g,0,sizeof(g));g.matilda.read_pos=1u; g.world.attr[7]=FLOOD_ATTR_SOLID;
    g.player.x=8; g.player.y=8; /* collision core begins at (16,16) */
    g.world.terrain[2*FLOOD_MAP_W+0]=7;
    g.world.terrain[2*FLOOD_MAP_W+1]=7;
    g.world.terrain[2*FLOOD_MAP_W+2]=7;
    uint16_t m=flood_build_quiffy_contacts(&g);
    assert((m&0x38u)==0x38u); assert(g.player.contact_count==3);
    g.player.raw_y=1; g.player.raw_x=1;
    flood_resolve_quiffy_contacts(&g.player,(uint8_t)m);
    assert(g.player.attachment_mode==1 && g.player.surface_y==1);
    memset(g.world.terrain,0,sizeof(g.world.terrain));
    g.world.terrain[0*FLOOD_MAP_W+2]=7;
    g.world.terrain[1*FLOOD_MAP_W+2]=7;
    g.world.terrain[2*FLOOD_MAP_W+2]=7;
    m=flood_build_quiffy_contacts(&g);
    assert((m&0x0eu)==0x0eu);
}

static void test_all_cardinal_attachments(void){
    FloodPlayer p; memset(&p,0,sizeof(p));
    p.raw_y=1; flood_resolve_quiffy_contacts(&p,0x38u);
    assert(p.attachment_mode==1&&p.surface_y==1);
    memset(&p,0,sizeof(p));p.raw_y=-1;flood_resolve_quiffy_contacts(&p,0x83u);
    assert(p.attachment_mode==1&&p.surface_y==-1);
    memset(&p,0,sizeof(p));p.raw_x=1;flood_resolve_quiffy_contacts(&p,0x0eu);
    assert(p.attachment_mode==2&&p.surface_x==1);
    memset(&p,0,sizeof(p));p.raw_x=-1;flood_resolve_quiffy_contacts(&p,0xe0u);
    assert(p.attachment_mode==2&&p.surface_x==-1);
}

static void test_exposed_surface_end_requires_toward_input(void){
    static const struct {
        uint8_t contacts,mode;
        int8_t toward_x,toward_y,away_x,away_y;
    } cases[]={
        {0x81u,1u, 1,-1,-1,-1}, /* north, exposed east */
        {0x03u,1u,-1,-1, 1,-1}, /* north, exposed west */
        {0x30u,1u, 1, 1,-1, 1}, /* south, exposed east */
        {0x18u,1u,-1, 1, 1, 1}, /* south, exposed west */
        {0x06u,2u, 1, 1, 1,-1}, /* east, exposed south */
        {0x0cu,2u, 1,-1, 1, 1}, /* east, exposed north */
        {0xc0u,2u,-1, 1,-1,-1}, /* west, exposed south */
        {0x60u,2u,-1,-1,-1, 1}, /* west, exposed north */
    };
    for(unsigned n=0u;n<sizeof(cases)/sizeof(cases[0]);n++){
        FloodPlayer p;memset(&p,0,sizeof(p));
        p.raw_x=cases[n].toward_x;p.raw_y=cases[n].toward_y;
        flood_resolve_quiffy_contacts(&p,cases[n].contacts);
        assert(p.attachment_mode==cases[n].mode);
        assert(p.surface_x==cases[n].toward_x);
        assert(p.surface_y==cases[n].toward_y);

        memset(&p,0,sizeof(p));
        p.raw_x=cases[n].away_x;p.raw_y=cases[n].away_y;
        flood_resolve_quiffy_contacts(&p,cases[n].contacts);
        assert(p.attachment_mode==0u&&p.surface_x==0&&p.surface_y==0);
    }
}

static void test_attachment_maintenance(void){
    FloodPlayer p; memset(&p,0,sizeof(p));
    p.attachment_mode=1;p.surface_x=1;p.surface_y=1;p.contact_count=1;
    flood_maintain_quiffy_attachment(&p);
    assert(p.surface_x==0&&p.attachment_mode==1&&p.attachment_ticks==1);
    p.contact_count=2;flood_maintain_quiffy_attachment(&p);
    assert(p.attachment_mode==0&&p.attachment_ticks==0);
    memset(&p,0,sizeof(p));p.attachment_mode=2;p.surface_y=1;p.y=17;p.contact_count=1;
    flood_maintain_quiffy_attachment(&p);
    assert(p.y==18&&p.surface_y==0&&p.attachment_mode==2);
    p.surface_y=1;p.contact_count=1;p.attachment_ticks=9;
    flood_maintain_quiffy_attachment(&p);assert(p.attachment_mode==0);
}

static void test_attachment_replays_captured_direction(void){
    FloodPlayer p;memset(&p,0,sizeof(p));
    p.attachment_mode=1u;p.attachment_ticks=3u;p.contact_count=1u;
    p.surface_x=1;p.surface_y=-1;p.raw_x=0;p.raw_y=0;
    flood_maintain_quiffy_attachment(&p);
    assert(p.surface_x==0&&p.surface_y==-1&&p.attachment_ticks==4u);

    p.raw_x=-1;p.raw_y=0;p.contact_count=3u;
    flood_maintain_quiffy_attachment(&p);
    assert(p.attachment_mode==0u&&p.attachment_ticks==0u);
    assert(p.raw_x==0&&p.raw_y==-1);

    memset(&p,0,sizeof(p));p.attachment_mode=2u;p.contact_count=2u;
    p.surface_x=1;p.surface_y=-1;p.raw_y=1;
    flood_maintain_quiffy_attachment(&p);
    assert(p.surface_x==1&&p.surface_y==-1);
    assert(p.raw_x==1&&p.raw_y==-1);
}

static void test_tight_passage_wall_handoff(void){
    FloodGame g;memset(&g,0,sizeof(g));g.running=true;g.lives=3;
    g.matilda.read_pos=1u;g.player.x=40;g.player.y=24;
    g.player.life_force=511;g.player.air=63;g.player.pose_code=0x55u;
    g.player.attachment_mode=2u;g.player.attachment_ticks=4u;
    g.player.surface_x=1;g.player.surface_y=-1;
    g.world.attr[7]=FLOOD_ATTR_SOLID;
    /* The open cell at (4,2) is bounded by solid cells above and below. */
    g.world.terrain[1u*FLOOD_MAP_W+4u]=7u;
    g.world.terrain[3u*FLOOD_MAP_W+4u]=7u;
    FloodInput into={0};into.x=1;into.y=-1;
    flood_game_tick(&g,into);
    assert(g.player.raw_contact_mask==0x0au);
    assert(g.player.x==44&&g.player.y==24);
    assert(g.player.dx==4&&g.player.dy==0);
    assert(g.player.attachment_mode==0u);

    /* Mirror the handoff for a passage entered from its right side. */
    memset(&g,0,sizeof(g));g.running=true;g.lives=3;g.matilda.read_pos=1u;
    g.player.x=72;g.player.y=24;g.player.life_force=511;g.player.air=63;
    g.player.pose_code=0x55u;g.player.attachment_mode=2u;
    g.player.attachment_ticks=4u;g.player.surface_x=-1;g.player.surface_y=-1;
    g.world.attr[7]=FLOOD_ATTR_SOLID;
    g.world.terrain[1u*FLOOD_MAP_W+4u]=7u;
    g.world.terrain[3u*FLOOD_MAP_W+4u]=7u;
    into.x=-1;
    flood_game_tick(&g,into);
    assert(g.player.raw_contact_mask==0xa0u);
    assert(g.player.x==68&&g.player.y==24);
    assert(g.player.dx==-4&&g.player.dy==0);
    assert(g.player.attachment_mode==0u);

    /* The same opening must accept an ordinary horizontal approach when the
       body meets its upper lip eight pixels above the corridor centre. */
    memset(&g,0,sizeof(g));g.running=true;g.lives=3;g.matilda.read_pos=1u;
    g.player.x=40;g.player.y=16;g.player.life_force=511;g.player.air=63;
    g.player.pose_code=0x55u;g.world.attr[7]=FLOOD_ATTR_SOLID;
    g.world.terrain[1u*FLOOD_MAP_W+4u]=7u;
    g.world.terrain[3u*FLOOD_MAP_W+4u]=7u;
    FloodInput right={0};right.x=1;
    flood_game_tick(&g,right);
    assert(g.player.raw_contact_mask==0x06u);
    assert(g.player.x==44&&g.player.y==24);
    assert(g.player.dx==4&&g.player.dy==0);

    memset(&g,0,sizeof(g));g.running=true;g.lives=3;g.matilda.read_pos=1u;
    g.player.x=72;g.player.y=16;g.player.life_force=511;g.player.air=63;
    g.player.pose_code=0x55u;g.world.attr[7]=FLOOD_ATTR_SOLID;
    g.world.terrain[1u*FLOOD_MAP_W+4u]=7u;
    g.world.terrain[3u*FLOOD_MAP_W+4u]=7u;
    FloodInput left={0};left.x=-1;
    flood_game_tick(&g,left);
    assert(g.player.raw_contact_mask==0xc0u);
    assert(g.player.x==68&&g.player.y==24);
    assert(g.player.dx==-4&&g.player.dy==0);
}

static void test_shipped_tight_passages(void){
    char error[256];FloodInput into={0};into.x=1;into.y=-1;
    FloodGame level8;flood_game_init(&level8);
    assert(flood_game_load_level(&level8,FLOOD_DATA_DIR,8u,error,sizeof(error)));
    memset(level8.objects,0,sizeof(level8.objects));
    memset(level8.actions,0,sizeof(level8.actions));
    /* Level 8's long lower corridor begins at tile (3,11).  Approach while
       falling with Right alone: the body first meets the lip eight pixels
       above the exact corridor centre.  It must align and crawl through
       without requiring Down or a pre-existing wall attachment. */
    level8.player.x=16;level8.player.y=156;
    level8.player.dx=0;level8.player.dy=0;
    level8.player.invulnerable_timer=0u;
    level8.player.life_force=511;level8.player.air=63;
    FloodInput right={0};right.x=1;
    for(unsigned n=0u;n<4u;n++)flood_game_tick(&level8,right);
    assert(level8.player.x==32&&level8.player.y==168);
    assert(level8.player.dx==4&&level8.player.dy==0);
    for(unsigned n=0u;n<12u;n++)flood_game_tick(&level8,right);
    assert(level8.player.x==80&&level8.player.y==168);

    /* Down selects the crawl pose.  Adding Right must retain that pose while
       the passage handoff suppresses only vertical collision movement. */
    assert(flood_game_load_level(&level8,FLOOD_DATA_DIR,8u,error,sizeof(error)));
    memset(level8.objects,0,sizeof(level8.objects));
    memset(level8.actions,0,sizeof(level8.actions));
    level8.player.x=48;level8.player.y=168;
    level8.player.invulnerable_timer=0u;
    level8.player.life_force=511;level8.player.air=63;
    FloodInput down={0};down.y=1;
    flood_game_tick(&level8,down);
    assert(level8.player.pose_code==0x4cu);
    FloodInput crawl_right={0};crawl_right.x=1;crawl_right.y=1;
    flood_game_tick(&level8,crawl_right);
    assert(level8.player.x==52&&level8.player.y==168);
    assert(level8.player.dx==4&&level8.player.dy==0);
    assert(level8.player.raw_y==0&&level8.player.pose_code==0x50u);
    assert(flood_player_sprite_id(&level8)==0xa0u);

    level8.player.x=80;level8.player.y=168;
    level8.player.anim_phase=0u;level8.player.hbank=0u;
    FloodInput crawl_left={0};crawl_left.x=-1;crawl_left.y=1;
    flood_game_tick(&level8,crawl_left);
    assert(level8.player.x==76&&level8.player.y==168);
    assert(level8.player.dx==-4&&level8.player.dy==0);
    assert(level8.player.raw_y==0&&level8.player.pose_code==0x4cu);
    assert(flood_player_sprite_id(&level8)==0x9cu);

    FloodGame level17;flood_game_init(&level17);
    assert(flood_game_load_level(&level17,FLOOD_DATA_DIR,17u,error,sizeof(error)));
    /* The extra-life room's real entrance is the one-cell gap at (51,32),
       approached from its east side while climbing the wall.  Milestone 141
       accidentally began inside the room at column 49 and never tested it. */
    level17.player.x=52*16-8;
    level17.player.y=35*16-8;
    level17.player.attachment_mode=2u;level17.player.attachment_ticks=4u;
    level17.player.surface_x=-1;level17.player.surface_y=-1;
    into.x=0;
    for(unsigned n=0u;n<13u;n++)flood_game_tick(&level17,into);
    assert(level17.player.x==52*16-8&&level17.player.y==32*16-12);
    /* Turn left one update after exact alignment.  The passage assist must
       capture the opening instead of carrying Quiffy farther up the wall. */
    into.x=-1;
    flood_game_tick(&level17,into);
    assert(level17.player.x==52*16-12&&level17.player.y==32*16-8);
    assert(level17.player.dx==-4&&level17.player.dy==0);
    assert(level17.player.attachment_mode==0u);
    for(unsigned n=0u;n<16u&&level17.lives==3;n++)flood_game_tick(&level17,into);
    assert(level17.lives==4);
    assert(level17.world.terrain[32u*FLOOD_MAP_W+49u]==0u);

    assert(flood_game_load_level(&level17,FLOOD_DATA_DIR,17u,error,sizeof(error)));
    level17.player.x=45*16-8;  /* second reported narrow opening */
    level17.player.y=39*16-8;
    level17.player.attachment_mode=2u;level17.player.attachment_ticks=4u;
    level17.player.surface_x=1;level17.player.surface_y=-1;
    into.x=1;
    flood_game_tick(&level17,into);
    assert(level17.player.raw_contact_mask==0xcau);
    assert(level17.player.x==45*16-4&&level17.player.y==39*16-8);

    /* Level 28's marked opening is the gap at (2,22), entered from the
       one-cell shaft at column 1.  Both walls produce five/six contacts, so
       the ten-update wall cache expires before Quiffy reaches the opening. */
    FloodGame level28;flood_game_init(&level28);
    assert(flood_game_load_level(&level28,FLOOD_DATA_DIR,28u,error,sizeof(error)));
    level28.player.x=1*16-8;level28.player.y=27*16-8;
    level28.player.attachment_mode=2u;level28.player.attachment_ticks=1u;
    level28.player.surface_x=1;level28.player.surface_y=-1;
    into.x=1;into.y=-1;
    for(unsigned n=0u;n<20u;n++)flood_game_tick(&level28,into);
    assert(level28.player.attachment_mode==0u);
    assert(level28.player.x==1*16-4&&level28.player.y==22*16-8);
    assert(level28.player.dx==4&&level28.player.dy==0);
    for(unsigned n=0u;n<8u;n++)flood_game_tick(&level28,into);
    assert(level28.player.x>2*16&&level28.player.y==22*16-8);

    FloodGame level27;flood_game_init(&level27);
    assert(flood_game_load_level(&level27,FLOOD_DATA_DIR,27u,error,sizeof(error)));
    level27.player.x=35*16+8;  /* collision core occupies column 36 */
    level27.player.y=41*16-8;  /* collision core occupies the button row */
    level27.player.attachment_mode=2u;level27.player.attachment_ticks=4u;
    level27.player.surface_x=1;level27.player.surface_y=-1;
    for(unsigned n=0u;n<4u;n++)flood_game_tick(&level27,into);
    assert(level27.player.x==36*16+8&&level27.player.y==41*16-8);
    assert(level27.last_pickup_tile==41u*FLOOD_MAP_W+37u);
    assert(level27.world.terrain[level27.last_pickup_tile]==0xffu);
}

static void test_wall_to_top_full_jump(void){
    FloodGame g;memset(&g,0,sizeof(g));g.running=true;g.lives=3;
    g.matilda.read_pos=1u;g.player.x=40;g.player.y=80;
    g.player.life_force=511;g.player.air=63;g.player.pose_code=0x55u;
    g.world.attr[7]=FLOOD_ATTR_SOLID;
    for(unsigned y=4u;y<=6u;y++)for(unsigned x=4u;x<=6u;x++)
        g.world.terrain[y*FLOOD_MAP_W+x]=7u;
    FloodInput up={0};up.y=-1;
    for(unsigned n=0;n<11u;n++)flood_game_tick(&g,up);
    assert(g.player.x==44&&g.player.y==40&&g.player.attachment_mode==2u);
    flood_game_tick(&g,up);
    assert(g.player.x==44&&g.player.y==24&&g.player.dy==-16);
    assert(g.player.attachment_mode==0u&&(g.player.contact_mask&0x10u));

    memset(&g,0,sizeof(g));g.running=true;g.lives=3;g.matilda.read_pos=1u;
    g.player.x=104;g.player.y=80;g.player.life_force=511;g.player.air=63;
    g.player.pose_code=0x55u;g.world.attr[7]=FLOOD_ATTR_SOLID;
    for(unsigned y=4u;y<=6u;y++)for(unsigned x=4u;x<=6u;x++)
        g.world.terrain[y*FLOOD_MAP_W+x]=7u;
    for(unsigned n=0;n<11u;n++)flood_game_tick(&g,up);
    assert(g.player.x==100&&g.player.y==40&&g.player.attachment_mode==2u);
    flood_game_tick(&g,up);
    assert(g.player.x==100&&g.player.y==24&&g.player.dy==-16);
    assert(g.player.attachment_mode==0u&&(g.player.contact_mask&0x10u));
}

static FloodGame single_surface_tile_game(int16_t x,int16_t y){
    FloodGame g;memset(&g,0,sizeof(g));g.running=true;g.lives=3;
    g.matilda.read_pos=1u;g.player.x=x;g.player.y=y;
    g.player.life_force=511;g.player.air=63;g.player.pose_code=0x55u;
    g.player.attachment_mode=1u;g.world.attr[7]=FLOOD_ATTR_SOLID;
    g.world.terrain[4u*FLOOD_MAP_W+4u]=7u;
    return g;
}

static void test_cardinal_surface_replays_attachment(void){
    FloodInput right={0};right.x=1;
    FloodGame ceiling=single_surface_tile_game(56,60);
    ceiling.player.surface_y=-1;
    flood_game_tick(&ceiling,right);
    assert(ceiling.player.contact_mask==0x01u&&ceiling.player.contact_count==1u);
    assert(ceiling.player.raw_x==0&&ceiling.player.raw_y==-1);
    assert(ceiling.player.dx==0&&ceiling.player.x==56);
    assert(ceiling.player.dy==0&&ceiling.player.attachment_mode==0u);

    FloodGame floor=single_surface_tile_game(56,40);
    floor.player.surface_y=1;
    flood_game_tick(&floor,right);
    assert(floor.player.contact_mask==0x10u&&floor.player.contact_count==1u);
    assert(floor.player.raw_x==0&&floor.player.raw_y==1);
    assert(floor.player.dx==0&&floor.player.x==56);
    assert(floor.player.dy==0&&floor.player.attachment_mode==0u);

    /* A lone diagonal remains the actual corner-carry case. */
    FloodGame corner=single_surface_tile_game(40,72);
    corner.player.surface_y=-1;
    flood_game_tick(&corner,right);
    assert(corner.player.contact_mask==0x02u&&corner.player.contact_count==1u);
    assert(corner.player.dx==0&&corner.player.x==40);
}

static void test_attachment_cache_ignores_new_perpendicular_input(void){
    for(int direction=-1;direction<=1;direction+=2){
        FloodPlayer horizontal;memset(&horizontal,0,sizeof(horizontal));
        horizontal.attachment_mode=1u;horizontal.attachment_ticks=6u;
        horizontal.contact_count=1u;horizontal.contact_mask=0x01u;
        horizontal.surface_y=-1;horizontal.raw_y=(int8_t)direction;
        flood_maintain_quiffy_attachment(&horizontal);
        assert(horizontal.attachment_mode==1u&&horizontal.attachment_ticks==7u);
        assert(horizontal.raw_x==0&&horizontal.raw_y==-1);

        FloodPlayer vertical;memset(&vertical,0,sizeof(vertical));
        vertical.attachment_mode=2u;vertical.attachment_ticks=6u;
        vertical.contact_count=1u;vertical.contact_mask=0x04u;
        vertical.surface_x=1;vertical.raw_x=(int8_t)direction;
        flood_maintain_quiffy_attachment(&vertical);
        assert(vertical.attachment_mode==2u&&vertical.attachment_ticks==7u);
        assert(vertical.raw_x==1&&vertical.raw_y==0);
    }
}

static void test_original_cache_advances_corner_handoff(void){
    FloodGame g;memset(&g,0,sizeof(g));g.running=true;g.lives=3;
    g.matilda.read_pos=1u;g.player.x=40;g.player.y=40;
    g.player.life_force=511;g.player.air=63;g.player.pose_code=0x55u;
    g.world.attr[7]=FLOOD_ATTR_SOLID;
    for(unsigned y=4u;y<=6u;y++)for(unsigned x=4u;x<=6u;x++)
        g.world.terrain[y*FLOOD_MAP_W+x]=7u;

    FloodInput up={0};up.y=-1;
    for(unsigned n=0u;n<5u;n++)flood_game_tick(&g,up);
    FloodInput up_right={0};up_right.x=1;up_right.y=-1;
    for(unsigned n=0u;n<6u;n++)flood_game_tick(&g,up_right);

    assert(g.player.x==60&&g.player.y==24);
    assert(g.player.dy==-16&&g.player.attachment_mode==0u);
}

static void test_level13_original_corner_trace(void){
    static const struct {
        int16_t x,y,dx,dy;
        uint8_t contact,count,mode;
        int8_t surface_x,surface_y;
        uint8_t cache_ticks;
    } expected[]={
        {56,736,0,-4,0x0eu,3u,0u,0, 1,0u},
        {56,732,0,-4,0x0eu,3u,0u,0, 1,0u},
        {56,728,0,-4,0x0eu,3u,0u,0, 1,0u},
        {56,724,0,-4,0x0cu,2u,2u,1,-1,0u},
        {56,720,0,-4,0x0cu,2u,2u,1,-1,1u},
        {56,716,0,-4,0x0cu,2u,2u,1,-1,2u},
        {56,712,0,-4,0x0cu,2u,2u,1,-1,3u},
        {60,712,4, 0,0x08u,1u,2u,1, 0,4u},
        {64,696,4,-16,0x18u,2u,0u,1, 0,0u},
    };
    FloodGame g;char error[160]={0};flood_game_init(&g);
    assert(flood_game_load_level(&g,FLOOD_DATA_DIR,13u,error,sizeof(error)));
    memset(g.objects,0,sizeof(g.objects));memset(g.actions,0,sizeof(g.actions));
    g.player.x=56;g.player.y=800;g.player.dx=0;g.player.dy=0;
    g.player.invulnerable_timer=0u;g.player.life_force=511;g.player.air=63;
    FloodInput input={0};input.y=-1;
    for(unsigned tick=1u;tick<=24u;tick++){
        if(tick==19u)input.x=1;
        flood_game_tick(&g,input);
        if(tick>=16u){
            const unsigned n=tick-16u;
            assert(g.player.x==expected[n].x&&g.player.y==expected[n].y);
            assert(g.player.dx==expected[n].dx&&g.player.dy==expected[n].dy);
            assert(g.player.contact_mask==expected[n].contact);
            assert(g.player.contact_count==expected[n].count);
            assert(g.player.attachment_mode==expected[n].mode);
            assert(g.player.surface_x==expected[n].surface_x);
            assert(g.player.surface_y==expected[n].surface_y);
            assert(g.player.attachment_ticks==expected[n].cache_ticks);
        }
    }
}

static void test_level4_ceiling_corner_reversal(void){
    FloodGame g;char error[160]={0};flood_game_init(&g);
    assert(flood_game_load_level(&g,FLOOD_DATA_DIR,4u,error,sizeof(error)));
    memset(g.objects,0,sizeof(g.objects));memset(g.actions,0,sizeof(g.actions));
    assert(g.player.x==688&&g.player.y==448);
    g.player.invulnerable_timer=0u;g.player.life_force=511;g.player.air=63;

    FloodInput up_left={0};up_left.x=-1;up_left.y=-1;
    for(unsigned tick=0u;tick<15u;tick++)flood_game_tick(&g,up_left);
    assert(g.player.x==628&&g.player.y==440);
    assert(g.player.dx==-4&&g.player.dy==0);
    assert(g.player.contact_mask==0x81u&&g.player.contact_count==2u);
    assert(g.player.attachment_mode==0u);

    FloodInput right={0};right.x=1;
    flood_game_tick(&g,right);
    assert(g.player.x==632&&g.player.y==440);
    assert(g.player.dx==4&&g.player.dy==0);
    assert(g.player.attachment_mode==0u);
}

static void test_diagonal_corner_handoff_is_retained(void){
    static const struct {
        uint8_t mode,contact;
        int8_t normal_x,normal_y,raw_x,raw_y;
    } cases[]={
        {1u,0x02u, 0,-1, 1,-1}, /* ceiling -> east wall */
        {1u,0x80u, 0,-1,-1,-1}, /* ceiling -> west wall */
        {1u,0x08u, 0, 1, 1, 1}, /* floor -> east wall */
        {1u,0x20u, 0, 1,-1, 1}, /* floor -> west wall */
        {2u,0x02u, 1, 0, 1,-1}, /* east wall -> ceiling */
        {2u,0x08u, 1, 0, 1, 1}, /* east wall -> floor */
        {2u,0x80u,-1, 0,-1,-1}, /* west wall -> ceiling */
        {2u,0x20u,-1, 0,-1, 1}, /* west wall -> floor */
    };
    for(unsigned n=0u;n<sizeof(cases)/sizeof(cases[0]);n++){
        FloodPlayer p;memset(&p,0,sizeof(p));
        p.attachment_mode=cases[n].mode;p.contact_count=1u;
        p.contact_mask=cases[n].contact;p.surface_x=cases[n].normal_x;
        p.surface_y=cases[n].normal_y;p.raw_x=cases[n].raw_x;
        p.raw_y=cases[n].raw_y;
        flood_maintain_quiffy_attachment(&p);
        assert(p.attachment_mode==cases[n].mode&&p.attachment_ticks==1u);
        if(p.attachment_mode==1u)
            assert(p.surface_x==0&&p.surface_y==cases[n].normal_y);
        else
            assert(p.surface_x==cases[n].normal_x&&p.surface_y==0);
    }

    /* With both requested directions held, the lone NE diagonal keeps the
       ceiling normal and produces a full four-pixel corner-carry step. */
    FloodInput up_right={0};up_right.x=1;up_right.y=-1;
    FloodGame corner=single_surface_tile_game(40,72);
    corner.player.surface_y=-1;
    flood_game_tick(&corner,up_right);
    assert(corner.player.contact_mask==0x02u&&corner.player.contact_count==1u);
    assert(corner.player.attachment_mode==1u);
    assert(corner.player.dx==0&&corner.player.dy==-4);
    assert(corner.player.x==40&&corner.player.y==68);
}

static FloodGame supported_flamethrower_game(void){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;
    g.running=true;g.lives=3;g.player.x=8;g.player.y=136;
    g.player.life_force=511;g.player.air=63;g.player.pose_code=0x55u;
    g.selected_weapon_state=13u;g.world.attr[7]=FLOOD_ATTR_SOLID;
    g.world.terrain[10u*FLOOD_MAP_W+0u]=7u;
    g.world.terrain[10u*FLOOD_MAP_W+1u]=7u;
    g.world.terrain[10u*FLOOD_MAP_W+2u]=7u;
    return g;
}

static void test_flamethrower_holds_quiffy_input(void){
    static const int8_t directions[4][2]={{-1,0},{1,0},{0,-1},{0,1}};
    bool saw_flame=false,saw_malfunction=false;
    for(unsigned n=0;n<4u;n++){
        FloodGame g=supported_flamethrower_game();
        FloodInput fire={0};fire.fire=true;flood_game_tick(&g,fire);
        assert(g.actions[0].state==13u&&g.fire_ticks==2u&&!g.weapon_pose_active);
        g.rng_state=n==0u?1u:0u;
        flood_game_tick(&g,fire);
        assert(g.weapon_pose_active);
        assert(g.actions[0].state==14u||g.actions[0].state==15u);
        FloodInput held={0};held.x=directions[n][0];held.y=directions[n][1];held.fire=true;
        flood_game_tick(&g,held);
        assert(g.player.raw_x==0&&g.player.raw_y==0);
        assert(g.player.dx==0&&g.player.dy==0&&g.player.x==8&&g.player.y==136);
        assert(g.actions[0].state==14u||g.actions[0].state==15u);
        saw_flame|=g.actions[0].state==14u;
        saw_malfunction|=g.actions[0].state==15u;
    }
    assert(saw_flame&&saw_malfunction);

    FloodGame g=supported_flamethrower_game();
    FloodInput held={0};held.fire=true;
    flood_game_tick(&g,held);flood_game_tick(&g,held);
    FloodInput released={0};released.x=1;
    flood_game_tick(&g,released);
    assert(g.fire_ticks==0u&&g.player.raw_x==0&&g.player.x==8);
    assert(g.actions[0].state==0u&&!g.weapon_pose_active);
    flood_game_tick(&g,released);
    assert(g.player.raw_x==1&&g.player.x==12);

    g=supported_flamethrower_game();g.selected_weapon_state=1u;held.x=1;
    flood_game_tick(&g,held);
    assert(g.player.raw_x==1&&g.player.x>8);

    /* Exact original timing: the fire edge allocates State 13 first.  Up on
       the following update starts the jump before State 13 sets the movement
       lock from that update's still-supported contact word. */
    g=supported_flamethrower_game();
    FloodInput fire={0};fire.fire=true;flood_game_tick(&g,fire);
    FloodInput jump={0};jump.y=-1;jump.fire=true;
    flood_game_tick(&g,jump);
    assert(g.player.raw_x==0&&g.player.raw_y==-1);
    assert(g.player.x==8&&g.player.y==120);
    assert(g.player.dx==0&&g.player.dy==-16&&g.weapon_pose_active);
    assert(g.actions[0].state==14u||g.actions[0].state==15u);

    FloodInput steer={0};steer.x=1;steer.y=-1;steer.fire=true;
    flood_game_tick(&g,steer);
    assert(g.player.raw_x==0&&g.player.raw_y==0);
    assert(g.player.x==8&&g.player.y==107);
    assert(g.player.dx==0&&g.player.dy==-13);
    assert(g.actions[0].state==14u||g.actions[0].state==15u);

    for(unsigned n=0u;n<8u;n++)flood_game_tick(&g,steer);
    assert(g.player.x==8);
    assert(g.player.dy>0&&g.actions[0].state!=0u);

    /* The original $E1AC lock is geometry-independent.  Landing does not
       enable movement while the flame remains active. */
    for(unsigned n=0u;n<32u&&(!(g.player.contact_mask&0x10u)||g.player.dy!=0);n++)
        flood_game_tick(&g,steer);
    assert((g.player.contact_mask&0x10u)&&g.player.dy==0);
    assert(g.player.x==8&&g.player.y==136);
    steer.y=0;flood_game_tick(&g,steer);
    assert(g.player.raw_x==0&&g.player.x==8&&g.weapon_pose_active);

    /* Release is observed by the action after movement, exactly one update
       after $D7E4 has applied the still-active lock. */
    steer.fire=false;flood_game_tick(&g,steer);
    assert(g.player.x==8&&g.fire_ticks==0u&&!g.weapon_pose_active);
    flood_game_tick(&g,steer);
    assert(g.player.raw_x==1&&g.player.x==12);

    /* Ceiling contact zeroes the remaining vertical motion; the active flag
       continues to suppress joystick input while fire is held. */
    g=supported_flamethrower_game();
    for(unsigned x=0u;x<3u;x++)g.world.terrain[7u*FLOOD_MAP_W+x]=7u;
    flood_game_tick(&g,fire);flood_game_tick(&g,jump);
    assert(g.weapon_pose_active&&g.player.y==120&&g.player.dy==-16);
    steer.fire=true;steer.y=-1;
    flood_game_tick(&g,steer);
    assert(g.player.y==120&&g.player.dy==0&&(g.player.contact_mask&0x01u));
    const int16_t ceiling_x=g.player.x;
    flood_game_tick(&g,steer);
    assert(g.player.raw_x==0&&g.player.raw_y==0);
    assert(g.player.x==ceiling_x&&g.player.y==120&&g.player.dy==0);

    /* Up+fire on the allocation edge is too early: State 13 is not dispatched
       until the next update and then finds that south support is gone. */
    g=supported_flamethrower_game();jump.fire=true;
    flood_game_tick(&g,jump);
    assert(g.actions[0].state==13u&&!g.weapon_pose_active&&g.player.y==120);
    flood_game_tick(&g,fire);
    assert(g.actions[0].state==0u&&!g.weapon_pose_active);

    /* An established flame survives removal of its supporting bridge and
       follows Quiffy while gravity pulls him down. */
    g=supported_flamethrower_game();held.x=0;held.y=0;held.fire=true;
    flood_game_tick(&g,held);flood_game_tick(&g,held);
    const uint16_t flame_state=g.actions[0].state;
    memset(g.world.terrain,0,sizeof(g.world.terrain));
    const int16_t supported_y=g.player.y;
    for(unsigned n=0;n<5u;n++)flood_game_tick(&g,held);
    assert(g.actions[0].state==flame_state&&g.fire_ticks>2u&&g.player.y>supported_y);

    g=supported_flamethrower_game();
    assert((flood_build_quiffy_contacts(&g)&0x38u)==0x38u);
    g.fire_ticks=2u;g.weapon_pose_active=true;held.x=1;
    flood_game_level_banner_tick(&g,held,true);
    assert(g.player.raw_x==0&&g.player.raw_y==0&&g.player.x==8&&g.player.y==136);
}

static void test_binary_gravity_step(void){
    FloodGame g; memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.player.x=64;g.player.y=64;
    g.player.life_force=511;g.player.air=63;
    FloodInput in={0};flood_game_tick(&g,in);
    assert(g.player.dy==1&&g.player.y==65);
}

static void test_exact_quiffy_animation_cadence(void){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;
    g.player.x=64;g.player.y=64;g.player.life_force=511;g.player.air=63;
    FloodInput right={0};right.x=1;
    flood_game_tick(&g,right);assert(g.ticks==1u&&g.player.anim_phase==1u);
    flood_game_tick(&g,right);assert(g.ticks==2u&&g.player.anim_phase==1u);
    flood_game_tick(&g,right);assert(g.ticks==3u&&g.player.anim_phase==2u);
    flood_game_tick(&g,right);assert(g.ticks==4u&&g.player.anim_phase==2u);
}

static FloodGame matilda_test_game(void){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;
    g.player.x=400;g.player.y=400;g.player.life_force=511;g.player.air=63;
    g.dispatch_suppressed=true;
    return g;
}

static void test_exact_matilda_history_chase(void){
    FloodInput in={0};FloodGame g=matilda_test_game();

    /* $E0A8/$E0D8 initializes writer 0, reader 1.  $CEE0 advances the
       writer before consuming the reader; $CFA2 adds the first catch-up
       skip as the reader reaches record 32. */
    flood_game_tick(&g,in);
    assert(g.matilda.write_pos==1u&&g.matilda.read_pos==2u);
    assert(!g.matilda.visible&&g.matilda.history[1].x==g.player.x);
    const FloodHistorySample first=g.matilda.history[1];
    for(unsigned i=1;i<30u;i++)flood_game_tick(&g,in);
    assert(g.matilda.write_pos==30u&&g.matilda.read_pos==31u);
    flood_game_tick(&g,in);
    assert(g.matilda.write_pos==31u&&g.matilda.read_pos==33u);
    for(unsigned i=31u;i<248u;i++){
        flood_game_tick(&g,in);assert(!g.matilda.visible);
    }
    flood_game_tick(&g,in);
    assert(g.ticks==249u&&g.matilda.visible);
    assert(g.matilda.x==first.x&&g.matilda.y==first.y);

    /* A death hold ends as soon as the pre-advanced writer catches the
       reader.  This preserves the long history lap instead of collapsing
       Matilda to a 20-record trail. */
    g=matilda_test_game();g.matilda.read_pos=5u;
    flood_matilda_add_death_delay(&g);
    for(unsigned i=0;i<4u;i++)flood_game_tick(&g,in);
    assert(g.matilda.write_pos==4u&&g.matilda.read_pos==5u&&g.matilda.hold==16u);
    flood_game_tick(&g,in);
    assert(g.matilda.write_pos==5u&&g.matilda.read_pos==6u&&g.matilda.hold==0u);

    /* The 32-record catch-up test remains active during a hold, but the
       original eight-record safety comparison suppresses the extra skip. */
    g=matilda_test_game();g.matilda.read_pos=32u;g.matilda.hold=20u;
    flood_game_tick(&g,in);
    assert(g.matilda.write_pos==1u&&g.matilda.read_pos==33u&&g.matilda.hold==19u);
    g=matilda_test_game();g.matilda.write_pos=39u;g.matilda.read_pos=32u;
    g.matilda.hold=1u;flood_game_tick(&g,in);
    assert(g.matilda.write_pos==40u&&g.matilda.read_pos==32u&&g.matilda.hold==0u);
}

static void test_slope_micro_corrections(void){
    FloodGame g; memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.player.x=8;g.player.y=8;
    g.world.attr[7]=(uint8_t)(FLOOD_ATTR_SOLID|0x10u);
    /* NW y-edge samples tile (1,0); SE y-edge samples tile (1,2). */
    g.world.terrain[0*FLOOD_MAP_W+1]=7;g.player.raw_x=0;
    flood_build_quiffy_contacts(&g);assert(g.player.raw_x==1);
    assert(g.sound_queue_count==1u&&g.sound_queue[0]==22u&&g.slope_correction_previous);
    g.sound_queue_count=0u;g.player.raw_x=0;flood_build_quiffy_contacts(&g);
    assert(g.player.raw_x==1&&g.sound_queue_count==0u);
    memset(g.world.terrain,0,sizeof(g.world.terrain));
    flood_build_quiffy_contacts(&g);assert(!g.slope_correction_previous);
    g.world.terrain[2*FLOOD_MAP_W+1]=7;g.player.raw_x=0;
    flood_build_quiffy_contacts(&g);assert(g.player.raw_x==-1);
    assert(g.sound_queue_count==1u&&g.sound_queue[0]==22u);
    g.sound_queue_count=0u;
    g.world.attr[7]=(uint8_t)(FLOOD_ATTR_SOLID|0x20u);g.player.raw_x=0;
    flood_build_quiffy_contacts(&g);assert(g.player.raw_x==1&&g.sound_queue_count==0u);
}

static void test_fatal_terrain_contact(void){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;
    g.player.x=32;g.player.y=32;g.player.life_force=511;g.player.air=63;
    g.running=true;g.lives=3;
    g.world.attr[7]=FLOOD_ATTR_FATAL;
    /* The NW sweep samples this cell and promotes attribute bit 6 to
       contact-mask bit 8, exactly as $F270-$F27C does. */
    g.world.terrain[2*FLOOD_MAP_W+2]=7;
    assert((flood_build_quiffy_contacts(&g)&0x100u)!=0u);
    FloodInput in={0};flood_game_tick(&g,in);
    assert(g.lives==3&&g.player.life_force==511);
    assert(g.player.death_mode==1u&&g.matilda.hold==0u);

    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;
    g.player.x=32;g.player.y=32;g.player.life_force=511;g.player.air=63;
    g.player.invulnerable_timer=2;g.running=true;g.lives=3;
    g.world.attr[7]=FLOOD_ATTR_FATAL;
    g.world.terrain[2*FLOOD_MAP_W+2]=7;
    flood_game_tick(&g,in);
    assert(g.lives==3&&g.player.life_force==511&&g.player.invulnerable_timer==1);
    assert(g.player.death_mode==0u);
    assert(g.player_blink_active&&flood_player_should_draw(&g));
    flood_game_tick(&g,in);
    assert(g.player.invulnerable_timer==0u&&g.player_blink_active);
    assert(!flood_player_should_draw(&g));
    flood_game_tick(&g,in);
    assert(!g.player_blink_active&&flood_player_should_draw(&g));
}

static void test_level23_sparkling_fungi_identity(void){
    char error[256];FloodGame g;flood_game_init(&g);
    assert(flood_game_load_level(&g,FLOOD_DATA_DIR,23u,error,sizeof(error)));
    const uint16_t fungi=9u*FLOOD_MAP_W+1u;
    assert(g.world.terrain[fungi]==0x3du);
    assert(g.world.attr[0x3du]&FLOOD_ATTR_FATAL);
    assert(g.player.x==4*16&&g.player.y==5*16);
}

static void test_exact_quiffy_death_lifecycle(void){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;
    g.running=true;g.lives=3;g.player.x=32;g.player.y=32;
    /* Model an established history lap rather than the single reset frame,
       where $CEFE intentionally cancels a newly written hold. */
    g.matilda.read_pos=100u;
    g.player.life_force=0;g.player.air=0;g.player.pose_code=0x55u;
    g.world.attr[7]=FLOOD_ATTR_SOLID;
    g.world.terrain[3*FLOOD_MAP_W+2]=7;
    g.world.terrain[3*FLOOD_MAP_W+3]=7;
    FloodInput in={0};

    /* $D830 accepts zero as exhausted, and $DBE8 lands mode 1 immediately
       on south support before the renderer advances mode 2 by one. */
    flood_game_tick(&g,in);
    assert(g.player.death_mode==2u&&g.player.death_phase==1u);
    assert(g.player.y==24&&g.player.life_force==0&&g.lives==3);
    assert(g.matilda.hold==19u&&g.sound_pending&&g.last_sound_id==0x38u);

    for(uint8_t phase=0;phase<16u;phase++){
        g.player.death_mode=2u;g.player.death_phase=phase;
        assert(flood_player_sprite_id(&g)==(uint16_t)(0x78u+(phase>7u?7u:phase)));
    }
    g.player.death_phase=1u;

    unsigned updates=1u,sound_frames=1u;
    while(g.player.death_mode&&updates<40u){
        flood_game_tick(&g,in);updates++;
        if(g.sound_pending&&g.last_sound_id==0x38u)sound_frames++;
    }
    assert(updates==31u&&sound_frames==3u);
    assert(g.player.death_mode==0u&&g.player.death_phase==0u);
    assert(g.player.life_force==511&&g.player.air==63);
    assert(g.lives==2&&g.player.invulnerable_timer==100u&&g.running);
    g.render_buffer_index=1u;assert(!flood_player_should_draw(&g));
    g.render_buffer_index=0u;assert(flood_player_should_draw(&g));

    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=1;
    g.player.death_mode=2u;g.player.death_phase=15u;
    flood_game_tick(&g,in);
    assert(!g.running&&g.lives==0&&g.player.death_mode==0u);
}

static void test_escape_final_life_abort(void){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=100u;
    g.running=true;g.lives=3;g.player.x=32;g.player.y=32;
    g.player.life_force=213;g.player.air=63;
    g.world.attr[7]=FLOOD_ATTR_SOLID;
    g.world.terrain[3*FLOOD_MAP_W+2]=7;
    g.world.terrain[3*FLOOD_MAP_W+3]=7;
    for(unsigned glyph=0;glyph<FLOOD_HUD_GLYPH_COUNT;glyph++)
        memset(&g.hud_glyphs[glyph*FLOOD_HUD_H],(int)glyph,FLOOD_HUD_H);
    flood_update_hud(&g);uint8_t held_health_meter[FLOOD_HUD_H*4u];
    for(unsigned y=0;y<FLOOD_HUD_H;y++)
        memcpy(held_health_meter+y*4u,&g.hud_mask[y*FLOOD_HUD_ROW_BYTES+36u],4u);

    FloodInput abort={0};abort.restart=true;flood_game_tick(&g,abort);
    assert(g.running&&g.lives==0&&g.player.life_force==213);
    assert(g.player.death_mode==1u&&g.escape_death_active);
    assert(!memcmp(g.hud_digits+7u,"00",2u));
    for(unsigned y=0;y<FLOOD_HUD_H;y++)
        assert(!memcmp(held_health_meter+y*4u,
            &g.hud_mask[y*FLOOD_HUD_ROW_BYTES+36u],4u));

    FloodInput released={0};flood_game_tick(&g,released);
    assert(g.player.death_mode==2u&&g.player.death_phase==0u&&
        g.player.life_force==213);
    for(unsigned n=0;n<40u&&g.running;n++){
        flood_game_tick(&g,released);
        if(g.running){
            assert(g.player.life_force==213&&g.escape_death_active);
            for(unsigned y=0;y<FLOOD_HUD_H;y++)
                assert(!memcmp(held_health_meter+y*4u,
                    &g.hud_mask[y*FLOOD_HUD_ROW_BYTES+36u],4u));
        }
    }
    assert(!g.running&&g.lives==0&&g.player.death_mode==0u);
    assert(g.player.life_force==511&&!g.escape_death_active);
    bool full_meter_differs=false;
    for(unsigned y=0;y<FLOOD_HUD_H;y++)if(memcmp(held_health_meter+y*4u,
        &g.hud_mask[y*FLOOD_HUD_ROW_BYTES+36u],4u))full_meter_differs=true;
    assert(full_meter_differs);

    /* Escape is an explicit abort and cannot be cancelled by an active
       Cocktail/immortality timer.  Its visual alternating-buffer blink does
       continue throughout the ordinary fall/cross death sequence. */
    memset(&g,0,sizeof(g));g.matilda.read_pos=100u;
    g.running=true;g.lives=3;g.player.x=32;g.player.y=32;
    g.player.life_force=287;g.player.air=63;g.player.invulnerable_timer=50u;
    flood_game_tick(&g,abort);
    for(unsigned n=0;n<12u;n++){
        flood_game_tick(&g,released);
        assert(g.player.death_mode==1u&&g.player.invulnerable_timer==50u);
        assert(g.player_blink_active);
        assert(flood_player_should_draw(&g)==(g.render_buffer_index==0u));
    }

    memset(&g,0,sizeof(g));g.matilda.read_pos=100u;
    g.running=true;g.lives=3;g.player.x=32;g.player.y=32;
    g.player.life_force=287;g.player.air=63;g.player.invulnerable_timer=50u;
    g.world.attr[7]=FLOOD_ATTR_SOLID;
    g.world.terrain[3*FLOOD_MAP_W+2]=7;
    g.world.terrain[3*FLOOD_MAP_W+3]=7;
    flood_game_tick(&g,abort);
    assert(g.running&&g.lives==0&&g.player.life_force==287);
    assert(g.player.invulnerable_timer==50u&&g.player_blink_active);
    assert(flood_player_should_draw(&g));
    assert(g.player.death_mode==1u);
    flood_game_tick(&g,released);
    assert(g.player.death_mode==2u&&g.player.life_force==287);
    assert(g.player.invulnerable_timer==50u&&g.player_blink_active);
    for(unsigned n=0;n<40u&&g.running;n++){
        flood_game_tick(&g,released);
        if(g.running){
            assert(g.player.life_force==287&&g.player.invulnerable_timer==50u);
            assert(g.player_blink_active);
            assert(flood_player_should_draw(&g)==(g.render_buffer_index==0u));
        }
    }
    assert(!g.running&&g.lives==0&&g.player.death_mode==0u);
    assert(g.player.life_force==511&&!g.escape_death_active);
}

static void test_airborne_death_forces_gravity(void){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;
    g.running=true;g.lives=3;g.player.x=8;g.player.y=8;
    g.player.life_force=0;g.player.air=63;
    g.world.attr[7]=FLOOD_ATTR_SOLID;
    /* A full-height wall produces NE/E/SE contact throughout the descent.
       Holding into it normally selects the east attachment, but $DBE8 must
       keep forcing gravity until the floor starts the cross sequence. */
    for(unsigned y=0;y<=8u;y++)g.world.terrain[y*FLOOD_MAP_W+2]=7;
    for(unsigned x=0;x<=2u;x++)g.world.terrain[8u*FLOOD_MAP_W+x]=7;
    FloodInput in={0};in.x=1;in.y=-1;

    flood_game_tick(&g,in);
    assert(g.player.death_mode==1u&&g.player.contact_mask==0u);
    assert(g.player.contact_count==0u&&g.player.contact_aux==0u);
    assert(g.player.dy==1&&g.player.y==9);

    unsigned updates=1u;
    while(g.player.death_mode==1u&&updates<100u){
        const int16_t y=g.player.y;flood_game_tick(&g,in);updates++;
        if(g.player.death_mode==1u)assert(g.player.y>=y);
    }
    assert(updates<100u&&g.player.death_mode==2u);
    assert(flood_player_sprite_id(&g)>=0x78u&&flood_player_sprite_id(&g)<=0x7fu);

    /* Repeat from a ceiling with held upward input and no side wall. */
    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;
    g.player.x=40;g.player.y=8;g.player.life_force=0;g.player.air=63;
    g.world.attr[7]=FLOOD_ATTR_SOLID;
    for(unsigned x=2;x<=5u;x++){
        g.world.terrain[x]=7;
        g.world.terrain[8u*FLOOD_MAP_W+x]=7;
    }
    in.x=0;in.y=-1;updates=0u;
    while(g.player.death_mode!=2u&&updates<100u){
        flood_game_tick(&g,in);updates++;
    }
    assert(updates<100u&&g.player.death_mode==2u);
    assert(flood_player_sprite_id(&g)>=0x78u&&flood_player_sprite_id(&g)<=0x7fu);

    /* Deep water must not route death mode 1 back through buoyancy.  The old
       implementation stalled here unless the player held down. */
    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;
    g.player.x=8;g.player.y=8;g.player.life_force=0;g.player.air=63;
    g.world.attr[7]=FLOOD_ATTR_SOLID;
    memset(g.world.water,40,sizeof(g.world.water));
    for(unsigned y=0;y<=8u;y++)g.world.terrain[y*FLOOD_MAP_W+2]=7;
    for(unsigned x=0;x<=2u;x++)g.world.terrain[8u*FLOOD_MAP_W+x]=7;
    in.x=1;in.y=-1;updates=0u;
    while(g.player.death_mode!=2u&&updates<100u){
        flood_game_tick(&g,in);updates++;
    }
    assert(updates<100u&&g.player.death_mode==2u);
}

static void test_action_record_dispatch_and_pickups(void){
    assert(sizeof(FloodAction)==18u);
    static const uint8_t tiles[6]={0xddu,0xdeu,0xebu,0xecu,0xedu,0xeau};
    static const uint16_t states[6]={1u,4u,8u,10u,13u,16u};
    for(unsigned n=0;n<6u;n++){
        FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;
        g.player.x=0;g.player.y=0;g.player.life_force=511;g.player.air=63;
        g.player.pose_code=0x55u;g.last_pickup_tile=UINT16_MAX;
        const uint16_t cell=flood_tile_index_at(16,16);g.world.terrain[cell]=tiles[n];
        FloodInput in={0};flood_game_tick(&g,in);
        assert(g.selected_weapon_state==states[n]&&g.world.terrain[cell]==0u);
    }

    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;
    g.player.x=0;g.player.y=0;g.player.life_force=511;g.player.air=63;
    g.player.pose_code=0x55u;g.last_pickup_tile=UINT16_MAX;
    uint16_t cell=flood_tile_index_at(16,16);g.world.terrain[cell]=0xddu;
    FloodInput in={0};flood_game_tick(&g,in);
    g.last_pickup_tile=UINT16_MAX;g.world.terrain[cell]=0xdeu;
    flood_game_tick(&g,in);
    assert(g.selected_weapon_state==4u&&g.world.terrain[cell]==0xddu);

    memset(&g.actions,0,sizeof(g.actions));g.selected_weapon_state=1u;
    in.fire=true;flood_game_tick(&g,in);
    assert(g.fire_ticks==2u&&g.actions[0].state==1u);
    for(unsigned n=1;n<FLOOD_ACTION_MAX;n++)assert(g.actions[n].state==0u);
    flood_game_tick(&g,in);
    assert(g.fire_ticks==3u&&g.actions[0].state==2u&&g.actions[0].timer==32u);
    in.fire=false;flood_game_tick(&g,in);assert(g.fire_ticks==0u);
    in.fire=true;flood_game_tick(&g,in);
    assert(g.actions[1].state==1u&&g.fire_ticks==2u);
}

static void test_exact_hud_builder(FloodGame *g){
    g->score=12345u;g->world.remaining_food=7;g->lives=3;
    g->player.life_force=511;g->player.air=63;
    flood_update_hud(g);
    assert(memcmp(g->hud_digits,"123450703",9u)==0&&g->hud_digits[9]==0u);
    for(unsigned y=0;y<FLOOD_HUD_H;y++){
        assert(g->hud_mask[y*FLOOD_HUD_ROW_BYTES+11u]==g->hud_glyphs[0x1du*8u+y]);
        assert(g->hud_mask[y*FLOOD_HUD_ROW_BYTES+23u]==g->hud_glyphs[0x1cu*8u+y]);
        assert(g->hud_mask[y*FLOOD_HUD_ROW_BYTES+34u]==g->hud_glyphs[0x1fu*8u+y]);
        assert(g->hud_mask[y*FLOOD_HUD_ROW_BYTES+3u]==g->hud_glyphs[0x6eu*8u+y]);
        assert(g->hud_mask[y*FLOOD_HUD_ROW_BYTES+2u]==g->hud_glyphs[0x6fu*8u+y]);
        assert(g->hud_mask[y*FLOOD_HUD_ROW_BYTES+39u]==g->hud_glyphs[0x6eu*8u+y]);
        assert(g->hud_mask[y*FLOOD_HUD_ROW_BYTES+38u]==g->hud_glyphs[0x6fu*8u+y]);
    }
    g->world.remaining_food=-1;g->lives=-1;g->player.life_force=-1;g->player.air=-1;
    flood_update_hud(g);
    assert(memcmp(g->hud_digits,"123450000",9u)==0);
    for(unsigned y=0;y<FLOOD_HUD_H;y++){
        for(unsigned x=0;x<4u;x++)
            assert(g->hud_mask[y*FLOOD_HUD_ROW_BYTES+x]==g->hud_glyphs[0x67u*8u+y]);
        for(unsigned x=36u;x<40u;x++)
            assert(g->hud_mask[y*FLOOD_HUD_ROW_BYTES+x]==g->hud_glyphs[0x67u*8u+y]);
    }
}

static void test_exact_hud_scroll_placement(void){
    assert(FLOOD_HUD_BACKING_Y==16);
    assert(FLOOD_HUD_VISIBLE_Y==0);
    for(unsigned phase=0;phase<16u;phase++){
        const FloodHudPlacement p=flood_hud_placement((uint16_t)(0x120u+phase));
        assert(p.camera_phase==phase);
        assert(p.bplcon1_shift==((16u-phase)&15u));
        assert(p.bplcon1==(uint8_t)(p.bplcon1_shift*0x11u));
        assert(p.compositor_base_bias==(phase?-704:-702));
        assert(p.frame_byte_offset==(phase?0u:2u));
        assert(p.backing_x==(phase?phase:16u));
        assert(p.fetch_x==FLOOD_HUD_FETCH_X);
        assert(p.visible_x==FLOOD_HUD_VISIBLE_X);
        assert((unsigned)p.fetch_x+FLOOD_HUD_W==336u);
        assert((unsigned)p.visible_x+FLOOD_HUD_W==320u);
    }
}

static void test_exact_camera_follow(void){
    FloodGame g;flood_game_init(&g);
    g.world.bounds_x=0x6c0;g.world.bounds_y=0x570;
    g.player.x=152;g.player.y=92;flood_update_camera(&g);
    assert(g.camera_x==0&&g.camera_y==0);
    g.player.x=153;g.player.y=93;flood_update_camera(&g);
    assert(g.camera_x==1&&g.camera_y==1);
    g.player.x=1000;g.player.y=700;flood_update_camera(&g);
    assert(g.camera_x==848&&g.camera_y==608);
    g.player.x=2040;g.player.y=1590;flood_update_camera(&g);
    assert(g.camera_x==0x6c0&&g.camera_y==0x570);
    g.world.bounds_x=1040;g.world.bounds_y=432;
    flood_update_camera(&g);
    assert(g.camera_x==1040&&g.camera_y==432);

    char err[160];FloodGame level;
    flood_game_init(&level);
    assert(flood_game_load_level(&level,FLOOD_DATA_DIR,5,err,sizeof(err)));
    assert(level.world.bounds_x==1040&&level.world.bounds_y==432);
    flood_game_init(&level);
    assert(flood_game_load_level(&level,FLOOD_DATA_DIR,8,err,sizeof(err)));
    assert(level.world.bounds_x==0&&level.world.bounds_y==0);
}

static void test_exact_tile_overfetch(void){
    FloodGame g;flood_game_init(&g);
    g.render_tile_phase=2u;g.render_buffer_index=1u;
    assert(flood_render_tile_id(&g,0x1du)==0x1fu);
    assert(flood_render_tile_id(&g,0x20u)==0x1eu);
    assert(flood_render_tile_id(&g,0x47u)==0x48u);
    assert(flood_render_tile_id(&g,0x55u)==0x55u);

    memset(g.world.terrain,0,sizeof(g.world.terrain));
    for(unsigned tile=7u;tile<=10u;tile++)
        memset(&g.world.tile_pixels[tile*FLOOD_TILE_PIXELS],(int)tile,FLOOD_TILE_PIXELS);
    g.world.terrain[1u*FLOOD_MAP_W+1u]=7u;
    g.world.terrain[1u*FLOOD_MAP_W+21u]=8u;
    g.world.terrain[14u*FLOOD_MAP_W+1u]=9u;
    g.world.terrain[14u*FLOOD_MAP_W+21u]=10u;
    g.camera_x=17;g.camera_y=31;
    uint8_t window[FLOOD_VIEW_PIXELS];
    flood_build_tile_window(&g,window);
    assert(window[0]==7u);
    assert(window[FLOOD_VIEW_W-1u]==8u);
    assert(window[(FLOOD_VIEW_H-1u)*FLOOD_VIEW_W]==9u);
    assert(window[FLOOD_VIEW_PIXELS-1u]==10u);

    /* At the full-map maxima, every visible sample remains inside the map;
       only the deliberately overfetched columns/rows require padding. */
    g.camera_x=0x6c0;g.camera_y=0x570;
    memset(g.world.terrain,7,sizeof(g.world.terrain));
    flood_build_tile_window(&g,window);
    for(unsigned n=0;n<FLOOD_VIEW_PIXELS;n++)assert(window[n]==7u);
}

static void test_level1_conveyor_corridors(void){
    FloodGame g;char err[160];

    /* Level 1 rows 23/25 form a right-moving one-cell passage.  The
       original $F4B2/$F4DE pair contributes +2 before the four-pixel
       movement scale: opposite, neutral, and matching input yield 4/8/12. */
    for(int input_x=-1;input_x<=1;input_x++){
        flood_game_init(&g);
        assert(flood_game_load_level(&g,FLOOD_DATA_DIR,1u,err,sizeof(err)));
        g.running=true;g.player.life_force=511;g.player.air=63;
        g.player.x=800;g.player.y=376;
        FloodInput in={0};in.x=(int8_t)input_x;
        flood_game_tick(&g,in);
        assert(g.player.contact_mask==0xbbu&&g.player.contact_count==6u);
        assert(g.player.raw_x==input_x+2);
        assert(g.player.dx==(input_x+2)*4);
        assert(g.player.x==800+(input_x+2)*4);
    }

    /* Rows 25/27 are the mirrored left-moving passage. */
    for(int input_x=-1;input_x<=1;input_x++){
        flood_game_init(&g);
        assert(flood_game_load_level(&g,FLOOD_DATA_DIR,1u,err,sizeof(err)));
        g.running=true;g.player.life_force=511;g.player.air=63;
        g.player.x=800;g.player.y=408;
        FloodInput in={0};in.x=(int8_t)input_x;
        flood_game_tick(&g,in);
        assert(g.player.contact_mask==0xbbu&&g.player.contact_count==6u);
        assert(g.player.raw_x==input_x-2);
        assert(g.player.dx==(input_x-2)*4);
        assert(g.player.x==800+(input_x-2)*4);
    }
}

static void test_exact_overlay_compositor(void){
    FloodGame g;flood_game_init(&g);
    uint8_t window[FLOOD_VIEW_PIXELS];
    memset(window,3,sizeof(window));

    /* A disabled layer and state zero are transparent. */
    flood_composite_overlay_window(&g,window);
    for(unsigned n=0;n<FLOOD_VIEW_PIXELS;n++)assert(window[n]==3u);
    g.world.water_active=true;
    flood_composite_overlay_window(&g,window);
    for(unsigned n=0;n<FLOOD_VIEW_PIXELS;n++)assert(window[n]==3u);

    /* Camera (17,31) starts in map cell (1,1), with the first record clipped
       one pixel on the left and fifteen pixels at the top. */
    g.camera_x=17;g.camera_y=31;g.render_buffer_index=0u;
    g.world.render_water[1u*FLOOD_MAP_W+1u]=4u;
    memset(&g.world.overlay_pixels[4u*FLOOD_OVERLAY_TILE_PIXELS],1,
        FLOOD_OVERLAY_TILE_PIXELS);
    flood_composite_overlay_window(&g,window);
    for(unsigned x=0;x<15u;x++)assert(window[x]==7u);
    assert(window[15u]==3u);
    assert(window[FLOOD_VIEW_W]==3u);

    /* Bank 1 selects the second $0A80 block; value 2 sets colour bit 3.
       Out-of-range secondary states are skipped just like a guarded host
       lookup (the shipped updater emits only 0..40). */
    memset(window,1,sizeof(window));
    memset(g.world.render_water,0,sizeof(g.world.render_water));
    g.camera_x=0;g.camera_y=0;g.render_buffer_index=1u;
    g.world.render_water[0]=4u;
    memset(&g.world.overlay_pixels[
        (FLOOD_OVERLAY_TILE_COUNT+4u)*FLOOD_OVERLAY_TILE_PIXELS],2,
        FLOOD_OVERLAY_TILE_PIXELS);
    flood_composite_overlay_window(&g,window);
    assert(window[0]==9u&&window[15u]==9u&&window[16u]==1u);
    g.world.render_water[0]=FLOOD_OVERLAY_TILE_COUNT;
    memset(window,1,sizeof(window));
    flood_composite_overlay_window(&g,window);
    assert(window[0]==1u);
}

static void test_exact_backing_crop_fades_and_transport_filter(void){
    FloodGame g;flood_game_init(&g);char err[160];
    assert(flood_game_load_level(&g,FLOOD_DATA_DIR,1u,err,sizeof(err)));
    g.camera_x=13;g.camera_y=7;
    uint8_t *window=malloc(FLOOD_VIEW_PIXELS),*backing=malloc(FLOOD_BACKING_PIXELS);
    assert(window&&backing);
    flood_build_tile_window(&g,window);flood_build_tile_backing(&g,backing);
    flood_composite_overlay_window(&g,window);flood_composite_overlay_backing(&g,backing);
    for(unsigned y=0;y<FLOOD_VIEW_H;y++)
        assert(!memcmp(window+y*FLOOD_VIEW_W,
            backing+y*FLOOD_BACKING_W+FLOOD_BACKING_X,FLOOD_VIEW_W));

    memset(backing,0,FLOOD_BACKING_PIXELS);
    backing[5u*FLOOD_BACKING_W+16u]=5u;
    backing[4u*FLOOD_BACKING_W+16u]=1u;
    backing[4u*FLOOD_BACKING_W+18u]=12u;
    backing[0u*FLOOD_BACKING_W+16u]=8u;
    backing[0]=9u;
    assert(flood_transport_effect_pixel(backing,0u,5u,0u)==5u);
    assert(flood_transport_effect_pixel(backing,1u,5u,1u)==1u);
    assert(flood_transport_effect_pixel(backing,3u,5u,1u)==12u);
    assert(flood_transport_effect_pixel(backing,3u,5u,2u)==1u);
    assert(flood_transport_effect_pixel(backing,7u,5u,3u)==8u);
    assert(flood_transport_effect_pixel(backing,15u,7u,4u)==8u);
    assert(flood_transport_effect_pixel(backing,0u,7u,5u)==9u);
    assert(flood_transport_effect_pixel(backing,16u,7u,5u)==0u);
    free(backing);free(window);

    assert(flood_fade_colour(0xf84u,0u)==0xf84u);
    assert(flood_fade_colour(0xf84u,1u)==0xe73u);
    assert(flood_fade_colour(0xf84u,4u)==0xb40u);
    assert(flood_fade_colour(0xf84u,8u)==0x700u);
    assert(flood_fade_colour(0xf84u,15u)==0x000u);
    assert(flood_fade_colour(0x123u,1u)==0x012u);
}

static FloodGame water_game(unsigned x,unsigned y,uint8_t state){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;
    g.running=true;g.world.water_active=true;g.world.water_speed=1u;
    g.world.water_scan_row=(uint16_t)y;g.world.water_age=1u;
    g.world.water[y*FLOOD_MAP_W+x]=state;
    return g;
}

static void water_block(FloodGame *g,unsigned index){
    g->world.attr[1]=FLOOD_ATTR_WATER_BLOCK;g->world.terrain[index]=1u;
}

static void test_exact_water_falling_and_spread(void){
    const unsigned x=8u,y=8u,index=y*FLOOD_MAP_W+x;
    FloodGame g=water_game(x,y,1u);
    assert(flood_water_step(&g));
    assert(g.world.water[index]==1u&&g.world.water[index+FLOOD_MAP_W]==5u);
    assert(g.world.water_scan_row==y+1u&&g.world.water_age==2u);

    g=water_game(x,y,2u);water_block(&g,index+FLOOD_MAP_W);
    assert(flood_water_step(&g)&&g.world.water[index]==8u);
    g=water_game(x,y,5u);
    assert(flood_water_step(&g)&&g.world.water[index+FLOOD_MAP_W]==5u);
    g=water_game(x,y,6u);water_block(&g,index+FLOOD_MAP_W);
    assert(flood_water_step(&g)&&g.world.water[index]==10u);

    /* State 12 tries left first.  A free diagonal below it selects falling
       edge 1; a blocked diagonal selects level-fill state 12. */
    g=water_game(x,y,12u);
    assert(flood_water_step(&g)&&g.world.water[index-1u]==1u);
    g=water_game(x,y,12u);water_block(&g,index+FLOOD_MAP_W-1u);
    assert(flood_water_step(&g)&&g.world.water[index-1u]==12u);

    /* Once left is occupied, right is considered. */
    g=water_game(x,y,12u);g.world.water[index-1u]=40u;
    assert(flood_water_step(&g)&&g.world.water[index+1u]==2u);
    g=water_game(x,y,12u);g.world.water[index-1u]=40u;
    water_block(&g,index+FLOOD_MAP_W+1u);
    assert(flood_water_step(&g)&&g.world.water[index+1u]==12u);

    g=water_game(x,y,12u);g.world.water[index-1u]=40u;
    g.world.water[index+1u]=40u;
    assert(flood_water_step(&g)&&g.world.water[index]==16u);
}

static void test_exact_water_fill_and_rise(void){
    const unsigned x=8u,y=8u,index=y*FLOOD_MAP_W+x;
    static const uint8_t before[]={8u,16u,20u,24u,28u,32u};
    for(unsigned n=0;n<sizeof(before);n++){
        FloodGame g=water_game(x,y,before[n]);
        assert(flood_water_step(&g));
        assert(g.world.water[index]==(uint8_t)(before[n]+4u));
    }

    FloodGame g=water_game(x,y,36u);
    assert(flood_water_step(&g));
    assert(g.world.water[index]==40u&&g.world.water[index-FLOOD_MAP_W]==4u);
    g=water_game(x,y,38u);g.world.water[index-FLOOD_MAP_W]=12u;
    assert(flood_water_step(&g));
    assert(g.world.water[index]==40u&&g.world.water[index-FLOOD_MAP_W]==16u);
    g=water_game(x,y,36u);water_block(&g,index-FLOOD_MAP_W);
    assert(flood_water_step(&g));
    assert(g.world.water[index]==40u&&g.world.water[index-FLOOD_MAP_W]==0u);
}

static void test_exact_water_scheduler_and_rebuild(void){
    const unsigned x=8u,y=8u,index=y*FLOOD_MAP_W+x;
    FloodGame g=water_game(x,y,4u);
    g.world.water_pause=2u;g.world.water_speed=6u;g.render_buffer_index=1u;
    flood_update_water(&g);
    assert(g.world.water_pause==1u&&g.world.water[index]==4u);
    assert(g.world.water_speed==5u&&g.world.water_age==1u);
    g.world.water_pause=0u;g.world.water_speed=1u;
    flood_update_water(&g);
    assert(g.world.water[index]==8u&&g.world.water_age==2u);

    g=water_game(x,y,4u);g.world.water[index-1u]=40u;
    g.world.water[index+1u]=40u;g.world.water_speed=2u;
    g.render_buffer_index=1u;flood_update_water(&g);
    assert(g.world.water[index]==20u&&g.world.water_age==5u);
    assert(g.world.water_speed==1u); /* four steps, then odd-buffer decay */

    g=water_game(x,y,40u);g.world.water_source_x=(int16_t)(x*16u);
    g.world.water_source_y=(int16_t)(y*16u);g.world.water_age=2u;
    g.world.water[3]=17u;
    flood_rebuild_water(&g);
    assert(g.world.water_age==2u&&g.world.water_scan_row==y);
    assert(g.world.water[index]==12u&&g.world.water[3]==0u);

    /* $F5E8 tests source X, so the original cannot replay an x=0 source. */
    g=water_game(0u,y,40u);g.world.water_source_x=0;
    g.world.water_source_y=(int16_t)(y*16u);g.world.water_age=7u;
    flood_rebuild_water(&g);
    assert(g.world.water_age==7u&&g.world.water[y*FLOOD_MAP_W]==0u);
}

static void test_level1_water_snapshot(void){
    FloodGame g;flood_game_init(&g);char err[160];
    assert(flood_game_load_level(&g,FLOOD_DATA_DIR,1,err,sizeof(err)));
    const unsigned source=3u*FLOOD_MAP_W+39u;
    assert(g.world.water_active&&g.world.water[source]==4u);
    assert(g.world.water_source_x==624&&g.world.water_source_y==48);
    assert(g.world.water_scan_row==99u&&g.world.water_age==1u);
    for(unsigned n=0;n<1000u;n++)assert(flood_water_step(&g));
    unsigned counts[41]={0};
    for(unsigned n=0;n<FLOOD_MAP_W*FLOOD_MAP_H;n++)
        if(g.world.water[n]<=40u)counts[g.world.water[n]]++;
    assert(g.world.water_scan_row==14u&&g.world.water_age==1001u);
    assert(counts[1]==6u&&counts[5]==7u&&counts[12]==9u&&counts[13]==5u);
    assert(counts[16]==3u&&counts[17]==1u&&counts[20]==18u&&counts[40]==100u);
}

static void test_water_frame_order_and_quiffy_coupling(void){
    static const uint8_t exact_fill[42]={
        0,0,0,0,1,0,0,0,1,1,1,0,2,2,2,0,4,4,4,0,6,6,6,0,
        8,8,8,0,10,10,10,0,12,12,12,0,14,14,14,0,16,0
    };
    FloodGame lut_game;memset(&lut_game,0,sizeof(lut_game));lut_game.matilda.read_pos=1u;
    for(unsigned state=0;state<42u;state++){
        lut_game.world.water[0]=(uint8_t)state;
        assert(flood_water_fill_at(&lut_game,0,0)==exact_fill[state]);
    }
    const unsigned index=8u*FLOOD_MAP_W+8u;
    FloodGame g=water_game(8u,8u,4u);
    memcpy(g.world.render_water,g.world.water,sizeof(g.world.render_water));
    g.lives=3;g.last_pickup_tile=UINT16_MAX;
    g.player.x=400;g.player.y=400;g.player.life_force=511;g.player.air=63;
    g.player.pose_code=0x55u;
    FloodInput in={0};flood_game_tick(&g,in);
    assert(g.world.render_water[index]==4u&&g.world.water[index]==8u);
    flood_game_tick(&g,in);
    assert(g.world.render_water[index]==8u&&g.world.water[index]==12u);

    /* Gameplay samples the live map, not the pre-step render snapshot. */
    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;
    g.last_pickup_tile=UINT16_MAX;g.player.x=64;g.player.y=64;
    g.player.life_force=100;g.player.air=-1;g.player.pose_code=0x55u;
    const unsigned player_cell=4u*FLOOD_MAP_W+4u;
    g.world.water[player_cell]=38u;g.ticks=1u;
    flood_game_tick(&g,in);
    assert(g.player.dy==-1&&g.player.air==-2&&g.player.life_force==96);

    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;
    g.last_pickup_tile=UINT16_MAX;g.player.x=64;g.player.y=64;
    g.player.life_force=100;g.player.air=62;g.player.pose_code=0x55u;
    g.world.render_water[player_cell]=38u;
    flood_game_tick(&g,in);
    assert(g.player.air==64&&g.player.life_force==100);

    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;g.ticks=1u;
    g.last_pickup_tile=UINT16_MAX;g.player.x=64;g.player.y=64;
    g.player.life_force=100;g.player.air=0;g.player.pose_code=0x55u;
    g.world.water[player_cell]=38u;
    flood_game_tick(&g,in);
    assert(g.player.air==-1&&g.player.life_force==96);

    /* At the 60 ms gameplay cadence, only the five odd-buffer phases in ten
       updates drain air and remove four Life Force units: 20 points/600 ms.
       From full 511 Life Force this is 15.36 seconds to a negative value once
       oxygen has reached zero, matching $DEB4-$DEF2. */
    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;
    g.last_pickup_tile=UINT16_MAX;g.player.x=64;g.player.y=64;
    g.player.life_force=100;g.player.air=0;g.player.pose_code=0x55u;
    memset(g.world.water,40,sizeof(g.world.water));
    for(unsigned n=0;n<10u;n++)flood_game_tick(&g,in);
    assert(g.ticks==10u&&g.player.air==-5&&g.player.life_force==80);

    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;
    g.last_pickup_tile=UINT16_MAX;g.player.x=64;g.player.y=64;
    g.player.life_force=100;g.player.air=63;g.player.pose_code=0x55u;
    g.world.water[player_cell]=38u;in.y=1;
    flood_game_tick(&g,in);
    assert(g.player.dy==2);in.y=0;

    /* Stable full water is state 40, the two-byte tail omitted by the old
       host table. It must consume air on the active alternating phase. */
    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;g.ticks=1u;
    g.last_pickup_tile=UINT16_MAX;g.player.x=64;g.player.y=64;
    g.player.life_force=100;g.player.air=63;g.player.pose_code=0x55u;
    g.world.water[player_cell]=40u;
    flood_game_tick(&g,in);
    assert(g.player.air==62&&g.player.life_force==100);
}

static void test_multiple_and_zero_x_sources(void){
    FloodGame g;char err[160];flood_game_init(&g);
    assert(flood_game_load_level(&g,FLOOD_DATA_DIR,30,err,sizeof(err)));
    const unsigned a=97u*FLOOD_MAP_W+45u,b=a+1u,c=a+2u;
    assert(g.world.water[a]==4u&&g.world.water[b]==4u&&g.world.water[c]==4u);
    assert(g.world.render_water[a]==4u&&g.world.render_water[b]==4u&&
        g.world.render_water[c]==4u);
    assert(g.world.water_source_x==720&&g.world.water_source_y==1552);
    flood_rebuild_water(&g);
    assert(g.world.water[a]==8u&&g.world.water[b]==0u&&g.world.water[c]==0u);

    flood_game_init(&g);
    assert(flood_game_load_level(&g,FLOOD_DATA_DIR,36,err,sizeof(err)));
    const unsigned edge=FLOOD_MAP_W;
    assert(g.world.water[edge]==4u&&g.world.water_source_x==0);
    flood_rebuild_water(&g);
    for(unsigned n=0;n<FLOOD_MAP_W*FLOOD_MAP_H;n++)assert(g.world.water[n]==0u);
}

static void test_action_family_lifecycles(void){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;
    g.player.x=128;g.player.y=128;g.player.life_force=511;g.player.air=63;
    g.player.hbank=4;g.player.contact_mask=0x10u;g.player.pose_code=0x55u;

    FloodAction *a=&g.actions[0];a->state=1u;flood_update_actions(&g);
    assert(a->state==2u&&a->x==136&&a->y==128&&a->dy==-12&&a->timer==32u);
    for(unsigned n=0;n<32u;n++){g.ticks=n;flood_update_actions(&g);}
    assert(a->state==3u&&a->timer==7u&&g.last_sound_id==6u);
    for(unsigned n=0;n<7u;n++){g.ticks=32u+n;flood_update_actions(&g);}
    assert(a->state==0u);

    memset(g.actions,0,sizeof(g.actions));a=&g.actions[0];a->state=4u;
    flood_update_actions(&g);assert(a->state==5u&&a->timer==20u);
    flood_update_actions(&g);assert(a->state==6u&&a->timer==20u);
    flood_update_actions(&g);assert(a->state==7u&&a->dx==-16&&a->dy==-4);
    assert(g.last_sound_id==24u);
    flood_update_actions(&g);assert(a->state==7u&&g.action_render_count==1u);
    assert(g.action_renders[0].sprite_id>=0x20u&&g.action_renders[0].sprite_id<=0x27u);

    memset(g.actions,0,sizeof(g.actions));a=&g.actions[0];a->state=8u;
    flood_update_actions(&g);assert(a->state==9u&&a->dx==8&&a->dy==8&&a->timer==32u);
    flood_update_actions(&g);assert(a->state==9u&&g.action_render_count==1u);
    assert(g.action_renders[0].sprite_id==0x31u||g.action_renders[0].sprite_id==0x32u);

    memset(g.actions,0,sizeof(g.actions));a=&g.actions[0];a->state=10u;
    flood_update_actions(&g);assert(a->state==11u&&a->timer==16u&&g.last_sound_id==25u);
    for(unsigned n=0;n<16u;n++){g.ticks=n;flood_update_actions(&g);}
    assert(a->state==12u&&a->anim==0u);
    for(unsigned n=0;n<9u&&a->state;n++){g.ticks=n;flood_update_actions(&g);}
    assert(a->state==0u);

    memset(g.actions,0,sizeof(g.actions));a=&g.actions[0];a->state=13u;
    g.fire_ticks=5u;g.rng_state=0u;flood_update_actions(&g);
    assert(a->state==14u&&g.weapon_pose_active&&g.last_sound_id==47u);
    assert(g.weapon_main_offset==0u&&g.weapon_tip_offset==0u);
    assert(flood_player_sprite_id(&g)==0xcfu&&flood_weapon_tip_sprite_id(&g)==0xd1u);
    flood_update_actions(&g);assert(a->state==14u&&g.action_render_count==5u);
    g.fire_ticks=15u;flood_update_actions(&g);assert(g.action_render_count==15u);
    g.fire_ticks=255u;flood_update_actions(&g);assert(g.action_render_count==15u);
    g.fire_ticks=0u;flood_update_actions(&g);
    assert(a->state==0u&&!g.weapon_pose_active&&g.action_render_count==0u);

    memset(g.actions,0,sizeof(g.actions));a=&g.actions[0];a->state=13u;
    g.player.hbank=0u;g.fire_ticks=5u;g.rng_state=1u;flood_update_actions(&g);
    assert(a->state==15u&&g.weapon_pose_active&&g.last_sound_id==28u);
    assert(g.weapon_main_offset==16u&&g.weapon_tip_offset==10u);
    assert(flood_player_sprite_id(&g)==0xdbu&&flood_weapon_tip_sprite_id(&g)==0xdau);
    a->state=13u;g.player.hbank=4u;g.fire_ticks=5u;g.rng_state=1u;
    flood_update_actions(&g);
    assert(a->state==15u&&g.weapon_main_offset==13u&&g.weapon_tip_offset==12u);
    assert(flood_player_sprite_id(&g)==0xdcu&&flood_weapon_tip_sprite_id(&g)==0xddu);

    memset(g.actions,0,sizeof(g.actions));a=&g.actions[0];a->state=16u;
    flood_update_actions(&g);assert(a->state==17u&&a->timer==10u);
    g.actions[1].state=16u;flood_update_actions(&g);
    assert(g.actions[1].state==16u&&g.action_render_count==8u);
    for(unsigned n=1;n<10u;n++)flood_update_actions(&g);
    assert(a->state==0u);
    flood_update_actions(&g);assert(g.actions[1].state==17u);
}

static void test_exact_grenade_blast_lobes(void){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;g.ticks=0u;
    g.player.x=120;g.player.y=100;g.player.life_force=511;g.player.air=63;
    FloodAction *a=&g.actions[0];
    a->state=3u;a->x=160;a->y=100;a->dx=96;a->dy=100;a->timer=7u;
    g.objects[0].active=true;g.objects[0].state=12u;g.objects[0].x=180;g.objects[0].y=100;
    g.objects[1].active=true;g.objects[1].state=12u;g.objects[1].x=80;g.objects[1].y=100;
    flood_update_actions(&g);
    assert(g.action_render_count==8u);
    static const int16_t xs[8]={160,96,144,112,128,128,112,144};
    static const uint16_t sprites[8]={0x9bu,0x9bu,0x9au,0x9au,0x99u,0x99u,0x98u,0x98u};
    for(unsigned n=0;n<8u;n++){
        assert(g.action_renders[n].x==xs[n]);
        assert(g.action_renders[n].y==100);
        assert(g.action_renders[n].sprite_id==sprites[n]);
    }
    assert(g.player.life_force==471);
    assert(g.objects[0].state==13u&&g.objects[1].state==13u&&g.score==20u);
    assert(a->x==144&&a->dx==112&&a->anim==0u&&a->aux==0u&&a->timer==6u);

    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.world.attr[1]=FLOOD_ATTR_SOLID;
    memset(g.world.terrain,1,sizeof(g.world.terrain));
    a=&g.actions[0];a->state=2u;a->x=100;a->y=100;
    a->dx=4;a->dy=4;a->timer=5u;
    flood_update_actions(&g);
    assert(a->state==2u&&a->x==97&&a->y==97);
    assert(a->dx==-3&&a->dy==-3&&a->target==1u&&a->timer==4u);
    assert(g.last_sound_id==17u&&g.action_render_count==1u);
}

static void test_exact_boomerang_corridor_and_homing(void){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;
    g.player.x=128;g.player.y=128;g.player.hbank=4;
    FloodAction *a=&g.actions[0];a->state=5u;a->x=144;a->y=128;a->dx=16;a->timer=20u;
    g.objects[1].active=true;g.objects[1].state=12u;g.objects[1].x=160;g.objects[1].y=300;
    flood_update_actions(&g);
    assert(a->state==6u&&a->x==160&&a->target==1u&&a->timer==20u);
    flood_update_actions(&g);
    assert(a->state==7u&&a->x==128&&a->y==136&&a->dx==-16&&a->dy==-4);
    assert((int16_t)a->aux==-16&&g.last_sound_id==24u);
    flood_update_actions(&g);
    assert(a->state==7u&&a->x==112&&a->y==140&&a->dx==-16&&a->dy==4);
    assert((int16_t)a->aux==16&&g.action_target_x==160&&g.action_target_y==300);
    flood_update_actions(&g);
    assert(a->x==98&&a->y==144&&a->dx==-14&&a->dy==4);

    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.player.x=128;g.player.y=128;g.player.hbank=4;
    a=&g.actions[0];a->state=5u;a->x=144;a->y=128;a->dx=16;a->timer=20u;
    g.objects[0].active=true;g.objects[0].state=12u;g.objects[0].x=160;g.objects[0].y=300;
    flood_update_actions(&g);
    assert(a->state==6u&&a->target==UINT16_MAX);
}

static void test_carried_items_and_mine_impulse(void){
    FloodGame g; memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.player.x=8;g.player.y=8;
    g.player.life_force=511;g.player.air=63;g.player.parachute_timer=1000;g.player.balloon_timer=1000;
    g.world.attr[7]=FLOOD_ATTR_SOLID;
    g.world.terrain[2*FLOOD_MAP_W+0]=7;g.world.terrain[2*FLOOD_MAP_W+1]=7;
    g.world.terrain[2*FLOOD_MAP_W+2]=7;
    FloodInput in={0};flood_game_tick(&g,in);
    assert(g.player.parachute_timer==0&&g.player.balloon_timer==0);
    memset(g.world.terrain,0,sizeof(g.world.terrain));g.player.forced_up_timer=3;
    flood_game_tick(&g,in);
    assert(g.player.forced_up_timer==2&&g.player.dy<0&&g.player.vbank==0);
}

static void test_verified_frame_banks(void){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.weapon_pose_active=true;
    for(uint8_t phase=0;phase<4;phase++){
        g.fire_ticks=(uint16_t)(phase+1u);
        g.player.hbank=0;assert(flood_player_sprite_id(&g)==(uint16_t)(0xC8u+phase));
        g.player.hbank=4;assert(flood_player_sprite_id(&g)==(uint16_t)(0xCCu+phase));
    }
    g.weapon_pose_active=false;g.fire_ticks=0u;g.player.pose_code=0u;
    assert(flood_player_sprite_id(&g)==0x50u);
    g.player.pose_code=0x5du;assert(flood_player_sprite_id(&g)==0xadu);
}

static void test_all_single_corner_poses(void){
    static const uint8_t contacts[4]={0x02u,0x08u,0x20u,0x80u};
    /* rows NE,SE,SW,NW; columns right/up, right/down, left/up, left/down */
    static const uint8_t expected[4][4]={
        {0x5bu,0x55u,0x5bu,0x57u},
        {0x55u,0x56u,0x5au,0x5au},
        {0x59u,0x5du,0x55u,0x5du},
        {0x5cu,0x5cu,0x58u,0x55u}
    };
    static const uint8_t h[4]={0,0,4,4},v[4]={0,4,0,4};
    for(unsigned corner=0;corner<4;corner++)for(unsigned facing=0;facing<4;facing++)
        assert(flood_select_quiffy_corner_pose(contacts[corner],h[facing],v[facing])==expected[corner][facing]);
    assert(flood_select_quiffy_corner_pose(0,0,0)==0x55u);
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.player.pose_code=0x56u;g.player.pose_active=true;
    assert(flood_player_sprite_id(&g)==0xa6u);
    g.player.pose_code=0x5du;assert(flood_player_sprite_id(&g)==0xadu);
}

static void test_complete_pose_selector(void){
    FloodPoseSelection p;
    p=flood_select_quiffy_pose(0x01u,0x01u,1,0,2,4,0,0x33u,false);
    assert(p.active&&p.code==0x0eu&&p.vbank==4u);
    p=flood_select_quiffy_pose(0x51u,0x51u,3,0,2,4,4,0x33u,false);
    assert(p.code==0x06u&&p.vbank==0u); /* south overwrites north; S beats W */
    p=flood_select_quiffy_pose(0x10u,0x10u,1,1,2,4,4,0x33u,false);
    assert(p.code==0x52u&&p.vbank==0u);
    p=flood_select_quiffy_pose(0x40u,0x40u,1,0,3,0,4,0x33u,false);
    assert(p.code==0x17u);
    p=flood_select_quiffy_pose(0x04u,0x04u,1,0,3,0,4,0x33u,false);
    assert(p.code==0x1fu);
    p=flood_select_quiffy_pose(0x204u,0x04u,1,0,1,0,4,0x33u,false);
    assert(p.code==0x19u&&p.vbank==0u); /* auxiliary bit 9 forces vertical bank 0 */
    p=flood_select_quiffy_pose(0,0,0,0,3,4,0,0x12u,true);
    assert(p.active&&p.code==0x27u);
    p=flood_select_quiffy_pose(0,0,0,0,3,4,0,0x12u,false);
    assert(!p.active&&p.code==0x12u);
    p=flood_select_quiffy_pose(0x02u,0x02u,1,0,0,0,4,0x31u,false);
    assert(p.active&&p.code==0x31u); /* unrecognized NE orientation retains state */
}

static void test_map_pickups(void){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.last_pickup_tile=UINT16_MAX;
    g.player.x=64;g.player.y=64;g.player.life_force=511;g.player.air=63;g.player.pose_code=0x55u;
    uint16_t i=flood_tile_index_at(80,80);g.world.terrain[i]=0xe7u;
    FloodInput in={0};flood_game_tick(&g,in);
    assert(g.player.parachute_timer==999u&&g.player.balloon_timer==0u&&g.world.terrain[i]==0xe7u);
    flood_game_tick(&g,in);assert(g.player.parachute_timer==998u);
    g.player.x=80;i=flood_tile_index_at(96,80);g.world.terrain[i]=0xe8u;
    flood_game_tick(&g,in);assert(g.player.balloon_timer==999u&&g.player.parachute_timer==0u);

    g.player.balloon_timer=0u;g.player.dy=0;
    g.player.x=96;g.player.y=64;i=flood_tile_index_at(112,80);g.world.terrain[i]=0xe4u;
    flood_game_tick(&g,in);
    assert(g.world.terrain[i]==0u&&g.world.water_speed==6u);
    assert(g.sound_pending&&g.last_sound_id==10u);
    g.player.x=112;g.player.y=64;i=flood_tile_index_at(128,80);g.world.terrain[i]=0xe5u;
    flood_game_tick(&g,in);
    assert(g.world.terrain[i]==0u&&g.world.water_pause==100u);
}

static void test_instruction_frame_order(void){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;
    g.last_pickup_tile=UINT16_MAX;g.last_special_tile=UINT16_MAX;
    g.player.x=47;g.player.y=48;g.player.life_force=511;g.player.air=63;
    g.player.pose_code=0x55u;
    const uint16_t entered=4u*FLOOD_MAP_W+4u;
    g.world.terrain[entered]=0x24u;g.world.remaining_food=1;
    FloodInput right={0};right.x=1;
    flood_game_tick(&g,right);
    /* $D7CE movement crosses the cell boundary before $FDA0 samples it. */
    assert(g.player.x==51&&g.world.terrain[entered]==0u&&g.score==2u);
    /* Terrain was already blitted, so the collected tile survives in this
       frame's immutable render snapshot and disappears on the next one. */
    assert(g.world.render_terrain_valid&&g.world.render_terrain[entered]==0x24u);
    flood_game_tick(&g,right);assert(g.world.render_terrain[entered]==0u);

    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;
    g.last_pickup_tile=UINT16_MAX;g.last_special_tile=UINT16_MAX;
    g.player.x=48;g.player.y=48;g.player.life_force=511;g.player.air=63;
    g.player.pose_code=0x55u;g.world.terrain[4u*FLOOD_MAP_W+4u]=0xdcu;
    FloodInput none={0};flood_game_tick(&g,none);
    /* The independent $FC6C path is inside $D7CE and therefore precedes
       terrain composition: a consumed $DC is absent immediately. */
    assert(g.quiffy_effect.state==1u&&g.world.render_terrain[4u*FLOOD_MAP_W+4u]==0u);
}

static void test_exact_level_banner_timing(void){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;
    g.last_pickup_tile=UINT16_MAX;g.last_special_tile=UINT16_MAX;
    g.player.x=47;g.player.y=48;g.player.life_force=511;g.player.air=63;
    g.player.pose_code=0x55u;g.world.terrain[4u*FLOOD_MAP_W+4u]=0x24u;
    g.world.remaining_food=1;
    g.objects[0].active=true;g.objects[0].state=5u;g.objects[0].x=200;g.objects[0].y=200;
    FloodInput right={0};right.x=1;
    for(unsigned frame=0;frame<FLOOD_LEVEL_BANNER_FRAMES;frame++)
        flood_game_level_banner_tick(&g,right,frame==0u);
    assert(g.ticks==51u&&g.player.x==51);
    assert(g.world.terrain[4u*FLOOD_MAP_W+4u]==0x24u&&g.score==0u);
    assert(g.objects[0].x==200&&g.objects[0].y==200);
    flood_game_tick(&g,right);
    assert(g.world.terrain[4u*FLOOD_MAP_W+4u]==0u&&g.score==2u);
}

static void test_exact_ouch_feedback(void){
    FloodGame g;flood_game_init(&g);g.rng_state=0u;
    g.objects[0].active=true;g.objects[0].state=5u;
    g.objects[0].x=g.player.x;g.objects[0].y=g.player.y;
    FloodInput none={0};flood_game_tick(&g,none);
    assert(g.player.life_force==503&&g.ouch_visible&&g.damage_contact_previous);
    assert(g.sound_queue_count==1u&&g.sound_queue[0]>=1u&&g.sound_queue[0]<=5u);
    flood_game_tick(&g,none);
    assert(g.player.life_force==495&&g.ouch_visible&&g.damage_contact_previous);
    assert(g.sound_queue_count==0u); /* continuous contact keeps the edge latch set */
    g.objects[0].x=400;g.objects[0].y=400;flood_game_tick(&g,none);
    assert(!g.ouch_visible&&!g.damage_contact_previous);
}

static void test_exact_incidental_sound_edges(void){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;
    g.player.x=64;g.player.y=64;g.player.life_force=511;g.player.air=63;
    g.player.invulnerable_timer=100u;
    FloodInput none={0};flood_game_tick(&g,none);
    assert(g.sound_queue_count==1u&&g.sound_queue[0]==52u);

    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;
    g.player.x=64;g.player.y=64;g.player.life_force=511;g.player.air=63;
    const uint16_t cell=flood_tile_index_at(g.player.x+8,g.player.y+8);
    g.world.water[cell]=22u;flood_game_tick(&g,none);
    assert(g.water_contact_previous&&g.sound_queue_count==2u);
    assert(g.sound_queue[0]==13u&&g.sound_queue[1]==15u);
    flood_game_tick(&g,none);assert(g.sound_queue_count==0u);
    g.world.water[cell]=0u;flood_game_tick(&g,none);
    assert(!g.water_contact_previous&&g.sound_queue_count==0u);
    g.world.water[flood_tile_index_at(g.player.x+8,g.player.y+8)]=22u;
    flood_game_tick(&g,none);
    assert(g.sound_queue_count==2u&&g.sound_queue[0]==13u&&g.sound_queue[1]==15u);
}

static FloodGame item_game(uint8_t tile){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;
    g.last_pickup_tile=UINT16_MAX;g.last_special_tile=UINT16_MAX;
    g.player.x=64;g.player.y=64;g.player.life_force=100;g.player.air=20;
    g.player.pose_code=0x55u;
    g.world.terrain[flood_tile_index_at(80,80)]=tile;
    return g;
}

static void test_complete_item_dispatcher(void){
    assert(sizeof(FloodQuiffyEffect)==18u);
    FloodInput in={0};uint16_t cell=flood_tile_index_at(80,80);
    static const uint8_t food[5]={0x94u,0x24u,0x25u,0x26u,0x27u};
    for(unsigned n=0;n<5u;n++){
        FloodGame g=item_game(food[n]);g.world.remaining_food=3;g.score=9u;
        flood_game_tick(&g,in);
        assert(g.world.terrain[cell]==0u&&g.world.remaining_food==2);
        assert(g.score==11u&&g.sound_pending&&g.last_sound_id==16u);
    }

    FloodGame g=item_game(0xe1u);flood_game_tick(&g,in);
    assert(g.lives==4&&g.world.terrain[cell]==0u&&g.last_sound_id==58u);

    g=item_game(0xe2u);g.score=7u;flood_game_tick(&g,in);
    assert(g.score==12u&&g.player.invulnerable_timer==50u);
    assert(g.player.life_force==100&&g.player.air==22&&g.last_sound_id==16u);
    flood_game_tick(&g,in);
    assert(g.player.invulnerable_timer==49u&&g.player.life_force==511&&g.player.air==63);

    g=item_game(0xdfu);flood_game_tick(&g,in);
    assert(g.world.mechanisms_paused&&g.world.terrain[cell]==0u&&g.last_sound_id==16u);
    g=item_game(0xe0u);g.world.mechanisms_paused=true;flood_game_tick(&g,in);
    assert(!g.world.mechanisms_paused&&g.world.terrain[cell]==0u);

    g=item_game(0xdbu);flood_game_tick(&g,in);
    assert(g.world.terrain[cell]==0u&&!g.sound_pending);

    g=item_game(0xeeu);g.score=30u;
    g.objects[0].active=true;g.objects[0].state=5u;
    g.objects[0].x=400;g.objects[0].y=400;g.objects[0].state_flags=1u;
    flood_game_tick(&g,in);
    assert(g.score==30u&&g.zap_message_pending&&g.world.terrain[cell]==0xeeu);
    assert(g.objects[0].y==400);
    assert(g.last_sound_id==54u);const uint32_t zap_tick=g.ticks;
    g.sound_pending=false;flood_game_tick(&g,in);
    assert(g.ticks==zap_tick&&g.score==30u&&!g.sound_pending);
    flood_complete_zap_message(&g);
    assert(g.score==50u&&!g.zap_message_pending&&g.ticks==zap_tick+1u);
    assert(g.objects[0].y==401);
    flood_game_tick(&g,in);
    assert(g.score==50u&&!g.sound_pending); /* same-cell cache */

    g=item_game(0x88u);g.world.remaining_food=0;flood_game_tick(&g,in);
    assert(g.level_complete&&g.world.remaining_food==-1);
    assert(g.world.terrain[cell]==0x88u&&g.last_sound_id==60u);
    g=item_game(0x88u);g.world.remaining_food=1;flood_game_tick(&g,in);
    assert(!g.level_complete&&g.world.remaining_food==1);
}

static void test_exact_level_completion_handoff(void){
    char err[160];FloodGame g;flood_game_init(&g);
    assert(flood_game_load_level(&g,FLOOD_DATA_DIR,1u,err,sizeof(err)));
    assert(flood_game_advance_level(&g,FLOOD_DATA_DIR,err,sizeof(err)));
    assert(g.world.level_number==1u&&!g.level_complete&&!g.game_complete&&g.running);

    g.score=1234u;g.lives=7;g.ticks=99u;g.rng_state=0x4567u;
    g.player.life_force=17;g.player.air=3;g.player.parachute_timer=44u;
    g.player.orange_timer=22u;g.selected_weapon_state=13u;g.fire_ticks=9u;
    g.actions[0].state=7u;g.objects[127].active=true;g.objects[127].state=21u;
    g.matilda.visible=true;g.matilda.history[4].x=88;
    g.quiffy_effect.state=2u;g.special_entry_offset=true;
    g.transport_timer=6u;g.world.mechanisms_paused=true;
    g.level_complete=true;g.world.remaining_food=-1;
    assert(flood_game_advance_level(&g,FLOOD_DATA_DIR,err,sizeof(err)));
    assert(g.world.level_number==2u&&!g.level_complete&&!g.game_complete&&g.running);
    assert(g.score==1234u&&g.lives==7&&g.ticks==99u);
    assert(g.player.life_force==511&&g.player.air==63);
    assert(g.player.invulnerable_timer==100u&&g.player.parachute_timer==0u);
    assert(g.player.orange_timer==0u&&g.selected_weapon_state==0u&&g.fire_ticks==0u);
    assert(g.actions[0].state==0u&&!g.objects[127].active);
    assert(!g.matilda.visible&&g.matilda.history[4].x==0);
    assert(g.quiffy_effect.state==0u&&!g.special_entry_offset&&g.transport_timer==0u);
    assert(!g.world.mechanisms_paused&&g.last_pickup_tile==UINT16_MAX);

    /* Force each remaining sentinel to audit the complete 1..42 loader chain. */
    while(g.world.level_number<42u){
        const unsigned previous=g.world.level_number;
        g.level_complete=true;g.world.remaining_food=-1;
        assert(flood_game_advance_level(&g,FLOOD_DATA_DIR,err,sizeof(err)));
        assert(g.world.level_number==previous+1u&&g.running&&!g.game_complete);
    }
    const uint32_t score=g.score,ticks=g.ticks;const int16_t lives=g.lives;
    g.level_complete=true;g.world.remaining_food=-1;
    assert(flood_game_advance_level(&g,FLOOD_DATA_DIR,err,sizeof(err)));
    assert(g.world.level_number==42u&&g.level_complete&&g.game_complete&&!g.running);
    assert(g.score==score&&g.ticks==ticks&&g.lives==lives);
}

static void test_restart_level_control(void){
    char err[160];FloodGame g;flood_game_init(&g);
    assert(flood_game_load_level(&g,FLOOD_DATA_DIR,1u,err,sizeof(err)));
    const int16_t start_x=g.player.x,start_y=g.player.y;
    const uint8_t terrain=g.world.terrain[1000];
    const bool object0_active=g.objects[0].active;
    g.score=12345u;g.lives=4;g.ticks=321u;
    g.player.x+=80;g.player.y+=64;g.player.life_force=19;g.player.air=2;
    g.world.terrain[1000]^=0xffu;g.objects[0].active=true;
    g.actions[0].state=7u;g.selected_weapon_state=13u;g.fire_ticks=9u;
    g.world.mechanisms_paused=true;g.level_complete=true;
    assert(flood_game_restart_begin(&g,FLOOD_DATA_DIR,err,sizeof(err)));
    assert(g.running&&g.lives==4&&g.score==12345u&&g.ticks==321u);
    assert(g.world.level_number==1u&&!g.level_complete&&!g.game_complete);
    assert(g.player.x==start_x&&g.player.y==start_y);
    assert(g.player.life_force==511&&g.player.air==63);
    assert(g.world.terrain[1000]==terrain&&g.objects[0].active==object0_active);
    assert(g.actions[0].state==0u&&g.selected_weapon_state==0u&&g.fire_ticks==0u);
    assert(!g.world.mechanisms_paused);
    flood_game_restart_finish(&g);
    assert(g.running&&g.lives==3&&g.player.life_force==511);

    g.lives=1;g.running=true;g.player.x=777;
    assert(flood_game_restart_begin(&g,FLOOD_DATA_DIR,err,sizeof(err)));
    assert(g.lives==1&&g.running&&g.player.x==start_x);
    flood_game_restart_finish(&g);
    assert(g.lives==0&&g.running&&g.player.life_force==-1);
    assert(g.player.death_mode==1u);
}

static void test_special_items_and_orange_skip(void){
    FloodInput in={0};uint16_t cell=flood_tile_index_at(80,80);
    FloodGame g=item_game(0xe7u);flood_game_tick(&g,in);
    assert(g.player.parachute_timer==999u&&g.world.terrain[cell]==0xe7u);
    assert(g.last_sound_id==38u);
    g.player.x=80;flood_game_tick(&g,in);g.player.x=64;flood_game_tick(&g,in);
    assert(g.last_sound_id==38u); /* leave and re-enter persistent pickup */

    g=item_game(0xe8u);flood_game_tick(&g,in);
    assert(g.player.balloon_timer==999u&&g.player.parachute_timer==0u);
    assert(g.world.terrain[cell]==0xe8u&&g.last_sound_id==37u);

    g=item_game(0xdcu);flood_game_tick(&g,in);
    assert(g.world.terrain[cell]==0u&&g.quiffy_effect.state==1u);
    assert(g.quiffy_effect.y==0&&g.quiffy_effect.dy==0&&g.quiffy_effect.aux==0);
    assert(g.special_entry_offset&&g.player.y<64);

    g=item_game(0xe3u);FloodObject *o=&g.objects[0];
    o->active=true;o->state=5u;o->x=300;o->y=300;o->dx=4;o->dy=0;
    flood_game_tick(&g,in);
    assert(g.player.orange_timer==49u&&g.dispatch_suppressed);
    const int16_t ox=o->x,oy=o->y;
    flood_game_tick(&g,in);assert(o->x==ox&&o->y==oy);
    g.player.orange_timer=1u;g.dispatch_suppressed=true;
    flood_game_tick(&g,in);assert(g.player.orange_timer==0u&&g.dispatch_suppressed);
    flood_game_tick(&g,in);assert(!g.dispatch_suppressed&&o->x==ox&&o->y==oy);
    flood_game_tick(&g,in);assert(o->x!=ox||o->y!=oy);
}

static void test_paired_transport_tiles(void){
    FloodInput in={0};FloodGame g=item_game(0x9cu);
    const unsigned earlier=10u*FLOOD_MAP_W+20u;
    const unsigned later=20u*FLOOD_MAP_W+30u;
    g.world.terrain[earlier]=0xa6u;g.world.terrain[later]=0xa6u;
    g.player.hbank=0u;flood_game_tick(&g,in);
    assert(g.transport_timer==9u&&g.transport_effect_stage==1u&&g.transport_x==30*16-20);
    assert(g.transport_y==20*16-8&&g.last_sound_id==51u);
    for(unsigned stage=2u;stage<=5u;stage++){
        flood_game_tick(&g,in);assert(g.transport_effect_stage==stage);
    }
    assert(g.transport_timer==5u&&g.player.x==30*16-20&&g.player.y==20*16-8);
    assert(g.transport_render_prejump&&g.transport_render_x==64&&g.transport_render_y==79);
    static const uint8_t return_stages[5]={5u,4u,3u,2u,1u};
    for(unsigned n=0;n<5u;n++){
        flood_game_tick(&g,in);assert(g.transport_effect_stage==return_stages[n]);
        assert(!g.transport_render_prejump);
    }
    assert(g.transport_timer==0u);flood_game_tick(&g,in);
    assert(g.transport_effect_stage==0u);

    g=item_game(0xa6u);g.world.terrain[earlier]=0x9cu;
    g.player.hbank=4u;g.special_entry_offset=true;flood_game_tick(&g,in);
    assert(g.transport_x==20*16+12&&g.transport_y==10*16-24);
}

static void test_dc_state1_bounce_and_restore(void){
    FloodInput in={0};FloodGame g=item_game(0u);
    g.player.x=8;g.player.y=8;g.player.hbank=0u;g.player.anim_phase=0u;
    g.world.attr[7]=FLOOD_ATTR_SOLID;
    g.world.terrain[2*FLOOD_MAP_W+0]=7u;
    g.world.terrain[2*FLOOD_MAP_W+1]=7u;
    g.world.terrain[2*FLOOD_MAP_W+2]=7u;
    g.quiffy_effect.state=1u;g.quiffy_effect.dy=-8;g.quiffy_effect.y=-8;
    in.y=-1;flood_game_tick(&g,in);
    assert(g.quiffy_effect.state==1u&&g.quiffy_effect.dy==-12);
    assert(g.quiffy_effect.y==-12&&g.quiffy_effect_pose_active);
    assert(g.quiffy_effect_sprite==0xaeu&&g.last_sound_id==33u);

    g=item_game(0u);g.quiffy_effect.state=1u;g.special_entry_offset=true;
    in.y=0;in.fire=true;flood_game_tick(&g,in);
    const uint16_t restore=flood_tile_index_at(80,88);
    assert(g.quiffy_effect.state==0u&&g.world.terrain[restore]==0xdcu);
    assert(g.last_special_tile==restore&&!g.special_entry_offset);
    assert(g.fire_ticks==2u&&!g.quiffy_effect_pose_active);

    g=item_game(0u);g.quiffy_effect.state=1u;g.special_entry_offset=true;
    g.player.death_mode=1u;in.fire=false;flood_game_tick(&g,in);
    assert(g.quiffy_effect.state==0u&&!g.special_entry_offset);
}

static void test_space_hopper_full_height_support(void){
    FloodInput in={0};FloodGame g=item_game(0u);
    g.player.x=40;g.player.y=32;g.world.attr[7]=FLOOD_ATTR_SOLID;
    for(unsigned x=2u;x<=4u;x++)g.world.terrain[4u*FLOOD_MAP_W+x]=7u;
    g.quiffy_effect.state=1u;
    flood_game_tick(&g,in);
    assert(g.quiffy_effect.state==1u&&g.quiffy_effect_pose_active);
    assert(g.player.contact_mask==0x10u);
    assert(g.player.x==40&&g.player.y==32&&g.player.dy==0);

    char error[256];flood_game_init(&g);
    assert(flood_game_load_level(&g,FLOOD_DATA_DIR,27u,error,sizeof(error)));
    g.player.x=23*16-16;g.player.y=11*16-24;
    g.player.invulnerable_timer=0u;g.last_special_tile=UINT16_MAX;
    for(unsigned n=0u;n<20u;n++)flood_game_tick(&g,in);
    assert(g.quiffy_effect.state==1u&&g.quiffy_effect_pose_active);
    assert(g.player.x==23*16-16&&g.player.y==11*16-32);
    assert(g.player.contact_mask==0x10u&&g.player.dy==0);

    /* Down is accepted as input but the State-1 16x32 movement body—not
       ordinary Quiffy's 16x16 body—must prevent motion through the floor. */
    in.y=1;flood_game_tick(&g,in);
    assert(g.player.y==11*16-32&&g.player.dy==0);

    /* A short bounce must return to exactly the same support line.  The
       sprite now shares that physical Y directly; no render offset hides a
       second resting position above or below the floor. */
    in.y=-1;
    for(unsigned n=0u;n<4u;n++)flood_game_tick(&g,in);
    assert(g.player.y<11*16-32);
    in.y=0;
    for(unsigned n=0u;n<40u;n++)flood_game_tick(&g,in);
    assert(g.quiffy_effect.state==1u&&g.quiffy_effect_pose_active);
    assert(g.player.y==11*16-32&&g.player.dy==0);
    assert(g.player.contact_mask==0x10u);
}

static void test_dc_state1_fatal_transition(void){
    FloodInput in={0};FloodGame g=item_game(0u);
    g.player.x=32;g.player.y=32;g.world.attr[7]=FLOOD_ATTR_FATAL;
    g.world.terrain[2*FLOOD_MAP_W+2]=7u;
    g.quiffy_effect.state=1u;g.special_entry_offset=true;
    flood_game_tick(&g,in);
    assert(g.quiffy_effect.state==2u&&g.quiffy_effect.x==32);
    assert(g.quiffy_effect.phase==0&&g.quiffy_effect.dy==58);
    assert(!g.special_entry_offset&&!g.quiffy_effect_render_active);
    assert(g.player.death_mode==0u); /* State 1 consumed contact bit 8. */
}

static void test_dc_state2_exact_path(void){
    static const int16_t expected[58][2]={
        {0,-16},{0,-16},{0,-16},{0,-16},{16,-16},{16,0},{16,16},{16,16},
        {0,16},{0,16},{0,16},{0,16},{-16,16},{-16,16},{-16,16},{-16,0},
        {-16,0},{-16,0},{-16,-16},{0,-16},{0,-16},{0,-16},{0,-16},{0,-16},
        {16,-16},{0,-16},{16,-16},{16,-16},{-16,-16},{-16,0},{-16,0},{-16,16},
        {-16,16},{-16,16},{0,16},{0,16},{0,16},{16,16},{16,0},{16,0},
        {16,0},{16,0},{16,0},{16,0},{16,0},{16,0},{16,0},{16,0},
        {16,0},{16,-16},{0,-16},{0,-16},{0,-16},{-16,-16},{-16,-16},{-16,0},
        {-16,0},{-16,0}
    };
    FloodInput in={0};FloodGame g=item_game(0u);
    g.quiffy_effect.state=2u;g.quiffy_effect.x=100;g.quiffy_effect.y=100;
    g.quiffy_effect.phase=0;g.quiffy_effect.dy=58;
    FloodObject *o=&g.objects[0];o->active=true;o->state=5u;
    o->x=300;o->y=300;o->dx=4;o->dy=0;
    int16_t x=100,y=100;
    for(unsigned n=0;n<58u;n++){
        flood_game_tick(&g,in);x=(int16_t)(x+expected[n][0]);y=(int16_t)(y+expected[n][1]);
        assert(g.quiffy_effect.x==x&&g.quiffy_effect.y==y);
        assert(g.quiffy_effect.phase==(int16_t)((n+1u)*4u));
        assert(g.quiffy_effect.dy==(int16_t)(57u-n));
        assert(g.quiffy_effect_render_active&&g.quiffy_effect_sprite==0xb4u);
        assert(g.quiffy_effect_render_x==x&&g.quiffy_effect_render_y==y);
        assert(g.render_dispatch_suppressed&&o->x==300&&o->y==300);
    }
    assert(g.quiffy_effect.state==0u&&g.quiffy_effect.phase==232);
    assert(g.quiffy_effect.x==132&&g.quiffy_effect.y==4);
    assert(g.quiffy_effect_render_active); /* final draw precedes clear */
    flood_game_tick(&g,in);
    assert(!g.quiffy_effect_render_active&&!g.render_dispatch_suppressed);
}

static void test_mine_to_heart_path(void){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.last_pickup_tile=UINT16_MAX;
    g.player.x=64;g.player.y=64;g.player.life_force=511;g.player.air=63;g.player.pose_code=0x55u;
    FloodObject *o=&g.objects[0];o->active=true;o->state=19;o->x=72;o->y=68;
    g.world.terrain[flood_tile_index_at(o->x,o->y)]=0xe6u;
    flood_update_states_2_13_19_21(&g);
    assert(o->state==20u&&o->x==64&&o->y==52&&o->anim==0u&&o->sprite==0u);
    assert(g.player.forced_up_timer==3u);
    assert(g.world.terrain[flood_tile_index_at(72,68)]==0u);

    /* $17E86=0 holds frame $94: sound 8 and damage both repeat while the
       stored phase remains zero. */
    g.ticks=1;flood_update_states_2_13_19_21(&g);
    assert(o->state==20u&&o->anim==0u&&o->sprite==0x94u);
    assert(g.sound_pending&&g.last_sound_id==8u&&g.player.life_force==471);
    g.sound_pending=false;flood_update_states_2_13_19_21(&g);
    assert(o->anim==0u&&g.sound_pending&&g.player.life_force==431);

    /* The alternating 1,0 global step makes every nonzero frame last twice. */
    g.ticks=0;flood_update_states_2_13_19_21(&g);
    assert(o->anim==1u&&o->sprite==0x95u&&g.player.life_force==431);
    g.ticks=1;flood_update_states_2_13_19_21(&g);assert(o->anim==1u&&o->sprite==0x95u);
    g.ticks=0;flood_update_states_2_13_19_21(&g);assert(o->anim==2u&&o->sprite==0x96u);
    g.ticks=1;flood_update_states_2_13_19_21(&g);assert(o->anim==2u&&o->sprite==0x96u);
    g.ticks=0;flood_update_states_2_13_19_21(&g);assert(o->anim==3u&&o->sprite==0x97u);
    g.ticks=1;flood_update_states_2_13_19_21(&g);assert(o->anim==3u&&o->sprite==0x97u);
    o->aux=99u;g.ticks=0;flood_update_states_2_13_19_21(&g);
    assert(o->state==21u&&o->x==68&&o->y==52&&o->dy==-12);
    assert(o->aux==0u&&o->anim==40u&&o->sprite==0u);

    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.player.x=64;g.player.y=64;g.player.life_force=0;
    o=&g.objects[0];o->active=true;o->state=19u;o->x=72;o->y=68;
    flood_update_states_2_13_19_21(&g);
    assert(o->state==20u&&g.player.forced_up_timer==0u);
}

static void test_shared_states_2_13_explosion(void){
    for(uint8_t state=2u;state<=13u;state=(uint8_t)(state+11u)){
        FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.player.x=400;g.player.y=400;g.ticks=0;
        FloodObject *o=&g.objects[0];o->active=true;o->state=state;o->x=32;o->y=48;
        o->dx=7;o->dy=9;o->aux=123u;o->anim=3u;
        flood_update_states_2_13_19_21(&g);
        assert(o->state==21u&&o->x==36&&o->y==48&&o->dx==7&&o->dy==-12);
        assert(o->aux==0u&&o->anim==40u&&o->sprite==0u);
    }
}

static void test_exact_heart_motion_collision_and_collection(void){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.player.x=400;g.player.y=400;
    FloodObject *o=&g.objects[0];o->active=true;o->state=21u;o->x=64;o->y=64;
    o->dy=-12;o->anim=40u;o->state_flags=0u;
    flood_update_states_2_13_19_21(&g);
    assert(o->state==21u&&o->x==64&&o->y==53&&o->dx==0&&o->dy==-11);
    assert(o->state_flags==0u&&o->anim==39u&&o->sprite==0x30u);
    assert(o->render_x==68&&o->render_y==57);
    flood_update_states_2_13_19_21(&g);
    assert(o->y==43&&o->dy==-10&&o->anim==38u);

    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.player.x=400;g.player.y=400;
    o=&g.objects[0];o->active=true;o->state=21u;o->x=64;o->y=64;
    o->dy=7;o->anim=40u;o->state_flags=1u;
    g.world.attr[1]=FLOOD_ATTR_SOLID;g.world.terrain[5u*FLOOD_MAP_W+4u]=1u;
    flood_update_states_2_13_19_21(&g);
    assert(o->y==60&&o->dy==-4&&o->state_flags==0u);
    assert(o->state==0u&&o->anim==0u);

    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.player.x=60;g.player.y=61;g.player.life_force=480;
    o=&g.objects[0];o->active=true;o->state=21u;o->x=64;o->y=64;
    o->dy=0;o->anim=40u;o->state_flags=1u;
    flood_update_states_2_13_19_21(&g);
    assert(o->state==0u&&o->active&&o->anim==39u);
    assert(g.score==10u&&g.player.life_force==511);
    assert(o->sprite==0x30u&&o->render_x==68&&o->render_y==69);

    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.player.x=400;g.player.y=400;
    o=&g.objects[0];o->active=true;o->state=21u;o->x=64;o->y=64;
    o->anim=1u;o->state_flags=1u;
    flood_update_states_2_13_19_21(&g);
    assert(o->state==0u&&o->active&&o->anim==0u);
}

static void test_state5_beady_ball_free_flight(void){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.player.x=400;g.player.y=400;g.player.life_force=511;
    FloodObject *o=&g.objects[0];o->active=true;o->state=5u;o->x=64;o->y=64;
    o->dx=4;o->dy=16;
    flood_update_state5(&g);
    assert(o->x==68&&o->y==79&&o->dx==4&&o->dy==15);
    assert(o->state_flags==0u&&o->anim==0u);
    assert(o->sprite==0xaeu&&o->render_x==68&&o->render_y==79);

    o->x=64;o->y=64;o->dx=4;o->dy=-12;o->state_flags=0u;o->anim=0u;
    flood_update_state5(&g);
    assert(o->x==68&&o->y==52&&o->dy==-12);
    /* move.w #1,+8 is big-endian: byte +8=0, byte +9=1. */
    assert(o->state_flags==0u&&o->anim==1u);
    flood_update_state5(&g);
    assert(o->x==72&&o->y==41&&o->dy==-11);
}

static void test_state5_beady_ball_collisions_and_damage(void){
    FloodGame g;FloodObject *o;

    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.player.x=400;g.player.y=400;
    o=&g.objects[0];o->active=true;o->state=5u;o->x=64;o->y=64;
    o->dx=4;o->dy=0;o->anim=1u;
    g.world.attr[1]=FLOOD_ATTR_SOLID;
    g.world.terrain[4u*FLOOD_MAP_W+6u]=1u;
    flood_update_state5(&g);
    assert(o->x==60&&o->y==65&&o->dx==-4&&o->dy==1);

    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.player.x=400;g.player.y=400;
    o=&g.objects[0];o->active=true;o->state=5u;o->x=64;o->y=64;
    o->dy=7;o->anim=1u;g.world.attr[1]=FLOOD_ATTR_SOLID;
    g.world.terrain[6u*FLOOD_MAP_W+4u]=1u;
    flood_update_state5(&g);
    assert(o->y==60&&o->dy==-4&&o->state_flags==0u&&o->anim==1u);

    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.player.x=400;g.player.y=400;
    o=&g.objects[0];o->active=true;o->state=5u;o->x=64;o->y=64;
    o->dy=1;o->anim=1u;g.world.attr[1]=FLOOD_ATTR_SOLID;
    g.world.terrain[6u*FLOOD_MAP_W+4u]=1u;
    flood_update_state5(&g);
    assert(o->y==52&&o->dy==-12&&o->anim==1u);

    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.player.x=400;g.player.y=400;
    o=&g.objects[0];o->active=true;o->state=5u;o->x=64;o->y=64;
    o->dy=-4;g.world.attr[1]=FLOOD_ATTR_SOLID;
    g.world.terrain[3u*FLOOD_MAP_W+4u]=1u;
    flood_update_state5(&g);
    assert(o->y==69&&o->dy==5&&o->anim==0u);

    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.player.x=60;g.player.y=61;g.player.life_force=100;
    o=&g.objects[0];o->active=true;o->state=5u;o->x=64;o->y=64;o->anim=1u;
    flood_update_state5(&g);
    assert(o->x==64&&o->y==65&&g.player.life_force==92);
}

static FloodGame bolt_game(void){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;g.last_pickup_tile=UINT16_MAX;
    g.player.x=400;g.player.y=400;g.player.life_force=511;g.player.air=63;g.player.pose_code=0x55u;
    FloodObject *o=&g.objects[0];o->active=true;o->state=22u;o->x=64;o->y=64;
    o->origin_x=64;o->origin_y=64;o->sprite_base=0x14u;o->sprite=0x14u;
    return g;
}

static void test_bolt_terrain_impact_and_reset(void){
    FloodGame g=bolt_game();FloodObject *o=&g.objects[0];
    g.world.attr[7]=FLOOD_ATTR_SOLID;
    g.world.terrain[4*FLOOD_MAP_W+6]=7; /* swept right edge at x=103, y=68 */
    FloodInput in={0};flood_game_tick(&g,in);
    assert((o->state_flags&1u)&&o->x==64&&o->anim==0&&
        g.sound_queue_count==0u);
    static const uint8_t frames[6]={0x98u,0x99u,0x99u,0x9au,0x9au,0x9bu};
    for(unsigned n=0;n<6;n++){
        flood_game_tick(&g,in);
        assert(o->anim==n+1u&&o->sprite==frames[n]);
        assert(g.sound_queue_count==(n==0u?1u:0u));
        if(n==0u)assert(g.sound_queue[0]==7u);
    }
    flood_game_tick(&g,in);
    assert(!(o->state_flags&1u)&&o->anim==0&&o->x==o->origin_x&&o->y==o->origin_y);
}

static void test_bolt_player_hit_final_advance(void){
    FloodGame g=bolt_game();FloodObject *o=&g.objects[0];
    g.player.x=64;g.player.y=60;
    FloodInput in={0};flood_game_tick(&g,in);
    assert(g.lives==3&&g.player.life_force==-1&&g.player.death_mode==0u);
    assert((o->state_flags&1u)&&o->x==72);
    flood_game_tick(&g,in);
    assert(g.lives==3&&g.player.death_mode==1u);
}

static void test_marker20_initialization(void){
    FloodGame g;flood_game_init(&g);char err[160];
    assert(flood_game_load_level(&g,FLOOD_DATA_DIR,9,err,sizeof(err)));
    unsigned bolts=0;
    for(unsigned n=0;n<FLOOD_OBJECT_MAX;n++)if(g.objects[n].active&&g.objects[n].state==22u){
        const FloodObject *o=&g.objects[n];bolts++;
        assert(o->x==o->origin_x&&o->y==o->origin_y&&o->sprite_base==0x14u);
    }
    assert(bolts>=2u);
}

static unsigned count_state(const FloodGame *g,uint8_t state){
    unsigned count=0;for(unsigned n=0;n<FLOOD_OBJECT_MAX;n++)
        count+=g->objects[n].active&&g->objects[n].state==state;
    return count;
}

static void test_trigger_payload_swap_and_marker22(void){
    FloodGame g;flood_game_init(&g);char err[160];
    assert(flood_game_load_level(&g,FLOOD_DATA_DIR,1,err,sizeof(err)));
    assert(g.world.trigger_count==2u&&g.world.trigger_data[20]==22u);
    const uint16_t target=3755u;const uint8_t old=g.world.terrain[target];
    assert(flood_activate_trigger(&g,280u,0)==1u);
    assert(g.world.water[3u*FLOOD_MAP_W+39u]==8u&&g.world.water_age==1u);
    assert(g.world.terrain[target]==0u&&g.world.trigger_data[20]==old);
    assert(g.player.x==(int16_t)((target%FLOOD_MAP_W)*16u));
    assert(g.player.y==(int16_t)((target/FLOOD_MAP_W)*16u));
    assert(flood_activate_trigger(&g,280u,0)==1u);
    assert(g.world.terrain[target]==old&&g.world.trigger_data[20]==0u);
}

static void test_trigger_creates_six_bolts(void){
    FloodGame g;flood_game_init(&g);char err[160];
    assert(flood_game_load_level(&g,FLOOD_DATA_DIR,5,err,sizeof(err)));
    const unsigned before=count_state(&g,22u);
    assert(g.world.trigger_count==3u);
    assert(flood_activate_trigger(&g,1677u,0)==7u); /* two matching records: 1 + 6 cells */
    assert(count_state(&g,22u)==before+6u);
    for(unsigned row=0;row<6;row++){
        const uint16_t index=(uint16_t)(514u+row*FLOOD_MAP_W);
        assert(g.world.terrain[index]==0u);
    }
}

static void test_level5_rocket_switch_and_impact_sound(void){
    FloodGame g;flood_game_init(&g);char err[160];
    assert(flood_game_load_level(&g,FLOOD_DATA_DIR,5,err,sizeof(err)));
    g.player.x=13*16-8;g.player.y=13*16-8;
    g.player.contact_mask=0x10u;g.player.surface_y=1;
    FloodInput in={0};flood_game_tick(&g,in);
    assert(count_state(&g,22u)==6u);
    assert(g.sound_queue_count==2u&&g.sound_queue[0]==52u&&
        g.sound_queue[1]==26u);

    /* Six simultaneously impacting wall rockets each enter $13718 and
       request the original phase-zero explosion sound. */
    for(unsigned n=0;n<FLOOD_OBJECT_MAX;n++)if(g.objects[n].active&&
        g.objects[n].state==22u){
        g.objects[n].state_flags|=1u;g.objects[n].anim=0u;
    }
    flood_game_tick(&g,in);
    assert(g.sound_queue_count==6u);
    for(unsigned n=0;n<g.sound_queue_count;n++)assert(g.sound_queue[n]==7u);
}

static void test_complete_marker_jump_table(void){
    static const struct {uint8_t marker,state,subtype,dx,dy,flags,map;} cases[]={
        {1,1,0xe2,2,4,0,0},{2,12,0x66,2,0,4,0},{5,5,0,4,16,0,0},
        {8,8,0,0,0,0,0},{11,11,0x4e,0,0,3,0xcb},{12,12,0x30,2,0,4,0},
        {13,13,0,0,0,0,0},{14,14,0,4,16,0,0},{16,16,0x4e,0,0,0,0xc9},
        {17,17,0x4e,0,0,0,0xcd},{18,18,0x4e,0,0,3,0xcb},
        {19,19,0,0,0,0,0xe6},{20,22,0x14,0,0,0,0},
        {24,12,0x6e,4,0,4,0},{25,12,0x38,4,0,0,0}
    };
    for(unsigned i=0;i<sizeof(cases)/sizeof(cases[0]);i++){
        FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;const uint16_t at=400u;
        g.world.terrain[at]=cases[i].marker;
        assert(flood_initialize_marker(&g,at,cases[i].marker));
        const FloodObject *o=&g.objects[0];assert(o->active&&o->state==cases[i].state);
        assert(o->sprite_base==cases[i].subtype&&o->dx==cases[i].dx&&o->dy==cases[i].dy);
        assert(o->state_flags==cases[i].flags&&g.world.terrain[at]==cases[i].map);
        if(cases[i].marker==1u){
            assert(o->aux==1u&&g.objects[1].active&&g.objects[1].state==0u);
            assert(g.objects[1].x==o->x&&g.objects[1].y==o->y&&g.objects[1].timer==10u);
        }
        if(cases[i].marker==11u||cases[i].marker==18u)assert(o->timer==0u);
        if(cases[i].marker==16u||cases[i].marker==17u)assert(o->timer>=1u&&o->timer<=100u);
        if(cases[i].marker==16u||cases[i].marker==17u)assert(o->timer==40u);
    }
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;
    memset(&g.world.tile_pixels[3u*FLOOD_TILE_PIXELS],7,FLOOD_TILE_PIXELS);
    assert(flood_initialize_marker(&g,300u,3u));
    for(unsigned n=0;n<FLOOD_TILE_PIXELS;n++)assert(g.world.tile_pixels[3u*FLOOD_TILE_PIXELS+n]==0u);
    assert(flood_initialize_marker(&g,301u,21u));
    assert(g.world.water_active&&g.world.water[301]==4u);
    assert(flood_initialize_marker(&g,302u,22u));
    assert(g.player.x==(int16_t)((302u%FLOOD_MAP_W)*16u)&&g.world.terrain[302]==0u);
    g.world.terrain[398]=0x10u;g.world.terrain[399]=0x77u;g.world.terrain[400]=23u;
    assert(flood_initialize_marker(&g,400u,23u));
    assert(g.world.mechanism_enabled&&g.world.bounds_tile==0x77u&&g.world.terrain[400]==0u);
    for(uint8_t marker=4;marker<=15;marker++)if(marker==4||marker==6||marker==7||marker==9||marker==10||marker==15){
        g.world.terrain[500]=marker;assert(flood_initialize_marker(&g,500,marker));assert(g.world.terrain[500]==0);
    }
}

static void test_all_level_marker_population(void){
    unsigned totals[26]={0};char err[160];
    for(unsigned level=1;level<=42;level++){
        FloodGame g;flood_game_init(&g);assert(flood_game_load_level(&g,FLOOD_DATA_DIR,level,err,sizeof(err)));
        for(unsigned n=0;n<FLOOD_OBJECT_MAX;n++)if(g.objects[n].active){
            uint8_t state=g.objects[n].state;
            if(state<26u)totals[state]++;
        }
    }
    assert(totals[0]==6u&&totals[1]==6u&&totals[3]==0u&&totals[4]==0u);
    assert(totals[5]==0u&&totals[7]==0u&&totals[8]==80u&&totals[12]==375u&&totals[14]==88u);
    assert(totals[16]==89u&&totals[17]==62u&&totals[18]==8u&&totals[19]==109u&&totals[22]==20u);
    assert(totals[23]==0u&&totals[24]==0u);
}

static void test_dispatch_noops_bounds_and_state24_bridge(void){
    static const uint8_t noops[6]={0u,3u,4u,7u,23u,25u};
    for(unsigned n=0;n<6;n++){
        FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;
        g.last_pickup_tile=UINT16_MAX;g.player.x=400;g.player.y=400;
        g.player.life_force=511;g.player.air=63;
        FloodObject *o=&g.objects[0];o->active=true;o->state=noops[n];
        o->x=64;o->y=64;o->dx=3;o->dy=-5;o->aux=77u;o->anim=6u;
        o->sprite=0x55u;o->render_x=61;o->render_y=62;
        FloodInput in={0};flood_game_tick(&g,in);
        assert(o->state==noops[n]&&o->x==64&&o->y==64);
        assert(o->dx==3&&o->dy==-5&&o->aux==77u&&o->anim==6u);
        assert(o->sprite==0x55u&&o->render_x==61&&o->render_y==62);
    }

    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;
    g.last_pickup_tile=UINT16_MAX;g.player.x=400;g.player.y=400;
    g.player.life_force=511;g.player.air=63;
    FloodObject *o=&g.objects[0];o->active=true;o->state=24u;
    o->x=32;o->y=32;o->dx=2;o->sprite_base=0x30u;o->state_flags=4u;
    FloodInput in={0};flood_game_tick(&g,in);
    assert(o->state==12u&&o->x==32&&o->y==32&&o->anim==0u&&o->sprite==0u);
    flood_game_tick(&g,in);
    assert(o->state==12u&&o->sprite!=0u);

    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;o=&g.objects[0];o->active=true;o->state=5u;
    o->x=64;o->y=1600;o->dx=4;o->dy=16;
    flood_prepare_object_dispatch(&g);assert(o->state==0u);
    flood_update_state5(&g);
    assert(o->x==64&&o->y==1600&&o->dx==4&&o->dy==16);

    o->state=5u;o->y=1599;flood_prepare_object_dispatch(&g);
    assert(o->state==5u);
}

static void test_state5_shipped_reachability(void){
    char err[160];
    for(unsigned level=1;level<=42;level++){
        FloodGame g;flood_game_init(&g);
        assert(flood_game_load_level(&g,FLOOD_DATA_DIR,level,err,sizeof(err)));
        assert(count_state(&g,5u)==0u);
        for(unsigned record=0;record<g.world.trigger_count;record++){
            const uint8_t *r=&g.world.trigger_data[record*10u];
            const unsigned width=(unsigned)((r[4]<<8)|r[5]);
            const unsigned height=(unsigned)((r[6]<<8)|r[7]);
            const unsigned payload=(unsigned)((r[8]<<8)|r[9]);
            for(unsigned i=0;i<width*height;i++)
                if(payload+i<FLOOD_TRIGGER_DATA_SIZE)
                    assert(g.world.trigger_data[payload+i]!=5u);
        }
    }
}

static FloodGame state12_game(uint8_t subtype){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.player.x=400;g.player.y=400;
    g.player.life_force=511;g.player.air=63;
    FloodObject *o=&g.objects[0];o->active=true;o->state=12;o->sprite_base=subtype;
    o->x=32;o->y=32;o->dx=(subtype==0x30u||subtype==0x66u)?2:4;
    o->state_flags=subtype==0x38u?0:4;
    return g;
}

static void test_state12_lumpy_and_snail(void){
    for(unsigned kind=0;kind<2;kind++){
        FloodGame g=state12_game(kind?0x66u:0x30u);FloodObject *o=&g.objects[0];
        g.world.attr[7]=FLOOD_ATTR_SOLID;
        g.world.terrain[4*FLOOD_MAP_W+2]=7;
        g.world.terrain[4*FLOOD_MAP_W+3]=7;
        g.world.terrain[4*FLOOD_MAP_W+4]=7;
        flood_update_state12(&g);
        assert(o->x==34&&o->y==32&&o->dx==2&&o->dy==0&&o->anim==1);
        assert(o->render_x==32&&o->render_y==34);
        assert(o->sprite==(uint8_t)(0x50u+o->sprite_base+5u));
        g.world.terrain[2*FLOOD_MAP_W+4]=7;
        flood_update_state12(&g);
        assert(o->dx==-2&&o->state_flags==0&&o->x==32);
    }
    FloodGame g=state12_game(0x30u);g.player.x=32;g.player.y=32;
    flood_update_state12(&g);assert(g.player.life_force==495);
}

static void test_state12_teddy(void){
    FloodGame g=state12_game(0x38u);FloodObject *o=&g.objects[0];
    g.world.remaining_food=1;
    g.world.terrain[3*FLOOD_MAP_W+3]=0x25u;
    flood_update_state12(&g);
    assert(g.world.terrain[3*FLOOD_MAP_W+3]==0&&g.world.remaining_food==0);
    assert(o->x==36&&o->y==34&&o->render_x==36&&o->render_y==34);
    assert(o->dx==4&&o->dy==2&&o->anim==1&&o->sprite==0x8du);
    g=state12_game(0x38u);o=&g.objects[0];g.world.attr[7]=FLOOD_ATTR_SOLID;
    g.world.terrain[3*FLOOD_MAP_W+3]=7;
    flood_update_state12(&g);
    assert(o->state_flags==1&&o->dy==-16&&o->dx==4&&o->x==32&&o->y==32);
    g=state12_game(0x38u);g.player.x=28;g.player.y=32;
    flood_update_state12(&g);assert(g.player.life_force==-1);

    /* Teddy writes -1 during object dispatch. On the following update the
       ordinary exhausted-Life-Force branch begins death and applies the same
       20-update Matilda history hold as drowning or accumulated damage. The
       already delayed ghost sample remains visible behind Quiffy. */
    g=state12_game(0x38u);g.lives=3;g.player.x=28;g.player.y=32;
    g.matilda.read_pos=2u;
    g.matilda.history[2]=(FloodHistorySample){12,32,0u};
    flood_update_state12(&g);assert(g.player.life_force==-1&&g.player.death_mode==0u);
    FloodInput in={0};flood_game_tick(&g,in);
    assert(g.player.death_mode==1u&&g.matilda.hold==19u);
    assert(g.matilda.visible&&g.matilda.x==12&&g.matilda.y==32&&g.matilda.sprite_id==0xd2u);
}

static void test_state12_vong(void){
    FloodGame g=state12_game(0x6eu);FloodObject *o=&g.objects[0];
    g.world.attr[7]=FLOOD_ATTR_SOLID;
    g.world.terrain[4*FLOOD_MAP_W+2]=7;g.world.terrain[4*FLOOD_MAP_W+3]=7;
    g.world.remaining_food=1;g.rng_state=11;
    flood_update_state12(&g);
    assert(g.world.terrain[3*FLOOD_MAP_W+3]==0x25u&&g.world.remaining_food==2);
    assert(o->x==36&&o->y==32&&o->render_x==32&&o->render_y==32);
    assert(o->dx==4&&o->dy==0&&o->sprite==0xc3u);
    g=state12_game(0x6eu);o=&g.objects[0];g.world.attr[7]=FLOOD_ATTR_SOLID;
    g.world.terrain[2*FLOOD_MAP_W+3]=7;
    flood_update_state12(&g);
    assert(o->dx==-4&&o->state_flags==0&&o->x==36); /* local velocity advances once */
    g=state12_game(0x6eu);g.player.x=28;g.player.y=32;
    flood_update_state12(&g);assert(g.player.life_force==-1);
}

static void test_level_food_counter(void){
    FloodGame g;flood_game_init(&g);char err[160];
    assert(flood_game_load_level(&g,FLOOD_DATA_DIR,1,err,sizeof(err)));
    assert(g.world.remaining_food==7);
}

static FloodGame state14_game(void){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.player.x=400;g.player.y=400;
    g.player.life_force=511;g.player.air=63;
    FloodObject *o=&g.objects[0];o->active=true;o->state=14u;
    o->x=32;o->y=32;o->dx=4;o->dy=16;
    return g;
}

static void test_state14_plonkin(void){
    FloodGame g=state14_game();FloodObject *o=&g.objects[0];
    flood_update_state14(&g);
    assert(o->x==36&&o->y==47&&o->dx==4&&o->dy==15);
    assert(o->anim==1u&&o->sprite==0xdfu&&o->render_x==30&&o->render_y==33);

    g=state14_game();o=&g.objects[0];o->dy=-12;
    flood_update_state14(&g);
    assert(o->dy==-12&&o->state_flags==1u&&o->x==36&&o->y==20);

    g=state14_game();o=&g.objects[0];o->state_flags=1u;o->dy=4;
    g.world.attr[7]=FLOOD_ATTR_SOLID;g.world.terrain[3*FLOOD_MAP_W+2]=7;
    flood_update_state14(&g);
    assert(o->x==36&&o->y==20&&o->dy==-12&&o->state_flags==1u);

    g=state14_game();o=&g.objects[0];o->state_flags=1u;o->dy=9;
    g.world.attr[7]=FLOOD_ATTR_SOLID;g.world.terrain[3*FLOOD_MAP_W+2]=7;
    flood_update_state14(&g);
    assert(o->x==36&&o->y==27&&o->dy==-5);

    g=state14_game();o=&g.objects[0];o->state_flags=1u;o->dy=0;
    g.world.attr[7]=FLOOD_ATTR_SOLID;g.world.terrain[2*FLOOD_MAP_W+3]=7;
    flood_update_state14(&g);
    assert(o->x==28&&o->y==33&&o->dx==-4&&o->dy==1);

    g=state14_game();o=&g.objects[0];g.world.attr[7]=FLOOD_ATTR_SOLID;
    g.world.terrain[2*FLOOD_MAP_W+2]=7;
    flood_update_state14(&g);
    assert(o->x==32&&o->y==32&&o->dx==4&&o->dy==16);
    assert(o->render_x==26&&o->render_y==18);

    g=state14_game();g.player.x=28;g.player.y=39;
    flood_update_state14(&g);assert(g.player.life_force==503);

    g=state14_game();o=&g.objects[0];g.ticks=1;
    flood_update_state14(&g);assert(o->anim==0u&&o->sprite==0xdeu);
}

static FloodGame state1_game(void){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.player.x=400;g.player.y=400;
    g.player.life_force=511;g.player.air=63;
    FloodObject *o=&g.objects[0];o->active=true;o->state=1u;o->aux=1u;
    o->x=32;o->y=32;o->dx=2;o->dy=4;o->sprite_base=0xe2u;
    g.objects[1].active=true;g.objects[1].state=0u;g.objects[1].timer=10u;
    return g;
}

static void test_state1_doctor_and_companion(void){
    FloodGame g=state1_game();FloodObject *o=&g.objects[0],*dust=&g.objects[1];
    g.world.attr[7]=FLOOD_ATTR_SOLID;
    g.world.terrain[4*FLOOD_MAP_W+2]=7;g.world.terrain[4*FLOOD_MAP_W+3]=7;
    g.world.terrain[4*FLOOD_MAP_W+4]=7;
    flood_update_state1_chain(&g);
    assert(o->x==34&&o->y==32&&o->dx==2&&o->dy==0);
    assert(o->anim==1u&&o->sprite==0xe3u&&o->render_x==32&&o->render_y==32);
    /* The following slot is spawned and then updated later in the same pass. */
    assert(dust->state==10u&&dust->x==46&&dust->y==33&&dust->dy==1);
    assert(dust->anim==19u&&dust->sprite==0x2bu);
    assert(dust->render_x==50&&dust->render_y==37);
    flood_update_state1_chain(&g);flood_update_state1_chain(&g);
    assert(dust->state==10u&&dust->anim==17u&&dust->dy==3&&dust->y==38);
    assert(dust->render_x==50&&dust->render_y==42);

    g=state1_game();o=&g.objects[0];flood_update_state1_chain(&g);
    assert(o->dx==-2&&o->state_flags==0u&&o->x==30);

    g=state1_game();g.player.x=26;g.player.y=33;
    flood_update_state1_chain(&g);assert(g.player.life_force==495);

    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;dust=&g.objects[0];dust->active=true;dust->state=10u;
    dust->x=32;dust->y=32;dust->anim=1u;g.world.attr[7]=FLOOD_ATTR_SOLID;
    g.world.terrain[2*FLOOD_MAP_W+2]=7;
    flood_update_state1_chain(&g);
    assert(dust->state==10u&&dust->anim==0u&&dust->dy==0&&dust->y==32);
    assert(dust->sprite==0x28u&&dust->render_x==36&&dust->render_y==36);
    flood_update_state1_chain(&g);
    assert(dust->state==9u&&dust->y==20&&dust->anim==0u&&dust->sprite==0u);

    g.player.x=24;g.player.y=12;g.player.life_force=511;
    flood_update_states_6_9_15(&g);
    assert(dust->state==9u&&dust->sprite==0x98u&&dust->anim==1u);
    assert(g.sound_pending&&g.last_sound_id==7u&&g.player.life_force==471);
    for(unsigned n=0;n<6;n++)flood_update_states_6_9_15(&g);
    assert(dust->state==0u&&dust->sprite==0u&&dust->anim==0u);

    memset(&g,0,sizeof(g));g.matilda.read_pos=1u;dust=&g.objects[0];dust->active=true;dust->state=10u;
    dust->x=32;dust->y=32;dust->anim=20u;dust->dy=-8;
    flood_update_state1_chain(&g);
    assert(dust->dy==-7&&dust->state_flags==0u&&dust->y==25);
    dust->state_flags=1u;dust->dy=8;dust->anim=20u;
    flood_update_state1_chain(&g);assert(dust->dy==8);
}

static void test_shared_states_6_9_15_burst(void){
    static const uint8_t states[3]={6u,9u,15u};
    static const uint8_t frames[6]={0x98u,0x99u,0x99u,0x9au,0x9au,0x9bu};
    for(unsigned kind=0;kind<3;kind++){
        FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.player.x=24;g.player.y=24;
        g.player.life_force=511;
        FloodObject *o=&g.objects[0];o->active=true;o->state=states[kind];
        o->x=32;o->y=32;o->aux=77u;
        for(unsigned n=0;n<6;n++){
            g.sound_pending=false;flood_update_states_6_9_15(&g);
            assert(o->state==states[kind]&&o->anim==n+1u&&o->sprite==frames[n]);
            assert(o->render_x==32&&o->render_y==32);
            assert(g.sound_pending==(n==0u));
            if(n==0u)assert(g.last_sound_id==7u&&g.player.life_force==471);
            else assert(g.player.life_force==471);
        }
        flood_update_states_6_9_15(&g);
        assert(o->state==0u&&o->aux==0u&&o->anim==0u&&o->sprite==0u);
    }

    /* The grouped host dispatch preserves the original single visit: a
       State-10 record that becomes State 9 does not burst until next tick. */
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.lives=3;
    g.last_pickup_tile=UINT16_MAX;g.player.x=400;g.player.y=400;
    g.player.life_force=511;g.player.air=63;
    FloodObject *o=&g.objects[0];o->active=true;o->state=10u;o->anim=0u;
    FloodInput in={0};flood_game_tick(&g,in);
    assert(o->state==9u&&o->anim==0u&&!g.sound_pending);
    flood_game_tick(&g,in);
    assert(o->state==9u&&o->anim==1u&&g.sound_pending&&g.last_sound_id==7u);
}

static void test_states_6_15_shipped_reachability(void){
    char err[160];
    for(unsigned level=1;level<=42;level++){
        FloodGame g;flood_game_init(&g);
        assert(flood_game_load_level(&g,FLOOD_DATA_DIR,level,err,sizeof(err)));
        assert(count_state(&g,6u)==0u&&count_state(&g,15u)==0u);
        for(unsigned record=0;record<g.world.trigger_count;record++){
            const uint8_t *r=&g.world.trigger_data[record*10u];
            const unsigned width=(unsigned)((r[4]<<8)|r[5]);
            const unsigned height=(unsigned)((r[6]<<8)|r[7]);
            const unsigned payload=(unsigned)((r[8]<<8)|r[9]);
            for(unsigned i=0;i<width*height;i++)if(payload+i<FLOOD_TRIGGER_DATA_SIZE){
                assert(g.world.trigger_data[payload+i]!=6u);
                assert(g.world.trigger_data[payload+i]!=15u);
            }
        }
    }
}

static FloodGame state8_game(void){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.player.x=100;g.player.y=100;
    g.player.life_force=511;g.player.air=63;g.rng_state=1u;
    FloodObject *o=&g.objects[0];o->active=true;o->state=8u;o->x=32;o->y=32;
    return g;
}

static void test_state8_vacuous_gombo(void){
    static const int16_t player_xy[8][2]={
        {32,0},{100,0},{100,32},{100,100},{32,100},{0,100},{0,32},{0,0}
    };
    static const int16_t velocity[8][2]={
        {0,-4},{4,-4},{4,0},{4,4},{0,4},{-4,4},{-4,0},{-4,-4}
    };
    for(unsigned direction=0;direction<8;direction++){
        FloodGame q=state8_game();FloodObject *v=&q.objects[0];
        q.player.x=player_xy[direction][0];q.player.y=player_xy[direction][1];
        flood_update_state8(&q);
        assert(v->state_flags==direction&&v->dx==velocity[direction][0]&&v->dy==velocity[direction][1]);
    }

    FloodGame g=state8_game();FloodObject *o=&g.objects[0];
    flood_update_state8(&g);
    assert(o->x==32&&o->y==32&&o->dx==4&&o->dy==4);
    assert(o->aux==0u&&o->state_flags==3u);
    assert(g.rng_state==0x4980u);
    assert(o->anim==1u&&o->sprite==0x91u&&o->render_x==32&&o->render_y==32);
    flood_update_state8(&g);
    assert(o->x==36&&o->y==36&&o->dx==4&&o->dy==4&&o->sprite==0x92u);

    g=state8_game();o=&g.objects[0];g.rng_state=0u;
    flood_update_state8(&g);
    assert(o->aux==1u&&o->state_flags==6u&&o->dx==-4&&o->dy==0);
    assert(g.rng_state==0xb11eu);

    g=state8_game();o=&g.objects[0];o->dx=4;o->dy=4;
    g.world.attr[7]=FLOOD_ATTR_SOLID;g.world.terrain[2*FLOOD_MAP_W+3]=7;
    flood_update_state8(&g);
    assert(o->x==32&&o->y==36); /* X is canceled only for this update. */
    assert(o->dx==4&&o->dy==4);

    g=state8_game();o=&g.objects[0];o->dx=4;o->dy=4;
    g.player.x=0;g.player.y=0;g.world.attr[7]=FLOOD_ATTR_SOLID;
    g.world.terrain[2*FLOOD_MAP_W+3]=7;g.world.terrain[4*FLOOD_MAP_W+2]=7;
    flood_update_state8(&g);
    assert(o->x==32&&o->y==32&&o->dx==-4&&o->dy==-4);
    assert(o->aux==0u&&o->state_flags==7u);

    g=state8_game();g.player.x=28;g.player.y=24;
    flood_update_state8(&g);assert(g.player.life_force==503);

    g=state8_game();o=&g.objects[0];g.player.x=32;g.player.y=32;g.ticks=1;
    flood_update_state8(&g);
    assert(o->state_flags==0u&&o->dx==0&&o->dy==-4&&o->anim==0u&&o->sprite==0x90u);
}

static FloodGame mechanism_game(uint8_t state,uint8_t phase){
    FloodGame g;memset(&g,0,sizeof(g));g.matilda.read_pos=1u;g.running=true;g.player.x=400;g.player.y=400;
    g.player.life_force=511;g.player.air=63;
    FloodObject *o=&g.objects[0];o->active=true;o->state=state;o->state_flags=phase;
    o->x=64;o->y=64;o->origin_x=64;o->origin_y=64;o->timer=50u;
    return g;
}

static void test_state16_horizontal_mechanism(void){
    FloodGame g=mechanism_game(16u,3u);FloodObject *o=&g.objects[0];
    g.world.terrain[4*FLOOD_MAP_W+4]=0xc9u;
    flood_update_mechanisms_11_18(&g);
    assert(o->x==48&&o->origin_x==80&&o->state_flags==3u);
    assert(g.world.terrain[4*FLOOD_MAP_W+3]==0xc8u);
    assert(g.world.terrain[4*FLOOD_MAP_W+5]==0xc8u);

    g.world.terrain[4*FLOOD_MAP_W+2]=7u;g.world.terrain[4*FLOOD_MAP_W+6]=7u;
    flood_update_mechanisms_11_18(&g);
    assert(o->x==32&&o->origin_x==96&&o->state_flags==2u&&o->timer==50u);
    o->timer=1u;flood_update_mechanisms_11_18(&g);
    assert(o->state_flags==1u&&o->timer==0u&&g.sound_pending&&g.last_sound_id==50u);

    g.sound_pending=false;flood_update_mechanisms_11_18(&g);
    assert(o->x==48&&o->origin_x==80&&o->state_flags==1u);
    assert(g.world.terrain[4*FLOOD_MAP_W+3]==0u);
    assert(g.world.terrain[4*FLOOD_MAP_W+5]==0u);
    flood_update_mechanisms_11_18(&g);
    assert(o->x==64&&o->origin_x==64&&o->state_flags==0u&&o->timer==50u);

    g=mechanism_game(16u,0u);o=&g.objects[0];o->timer=1u;
    g.player.x=64;g.player.y=64;
    flood_update_mechanisms_11_18(&g);
    assert(o->state_flags==3u&&o->timer==0u&&g.sound_pending&&g.last_sound_id==49u);

    g=mechanism_game(16u,3u);o=&g.objects[0];g.world.mechanisms_paused=true;
    flood_update_mechanisms_11_18(&g);
    assert(o->x==64&&o->origin_x==64&&o->state_flags==3u);
}

static void test_state17_vertical_mechanism(void){
    FloodGame g=mechanism_game(17u,3u);FloodObject *o=&g.objects[0];
    g.world.terrain[4*FLOOD_MAP_W+4]=0xcdu;
    flood_update_mechanisms_11_18(&g);
    assert(o->y==48&&o->origin_y==80&&o->state_flags==3u);
    assert(g.world.terrain[3*FLOOD_MAP_W+4]==0xcau);
    assert(g.world.terrain[5*FLOOD_MAP_W+4]==0xcau);

    g.world.terrain[2*FLOOD_MAP_W+4]=7u;g.world.terrain[6*FLOOD_MAP_W+4]=7u;
    flood_update_mechanisms_11_18(&g);
    assert(o->y==32&&o->origin_y==96&&o->state_flags==2u&&o->timer==50u);
    o->state_flags=1u;
    flood_update_mechanisms_11_18(&g);
    assert(o->y==48&&o->origin_y==80&&g.world.terrain[3*FLOOD_MAP_W+4]==0u);
    assert(g.world.terrain[5*FLOOD_MAP_W+4]==0u);
    flood_update_mechanisms_11_18(&g);
    assert(o->y==64&&o->origin_y==64&&o->state_flags==0u&&o->timer==50u);

    g=mechanism_game(17u,3u);o=&g.objects[0];g.player.x=56;g.player.y=72;
    g.world.terrain[4*FLOOD_MAP_W+4]=0xcdu;
    flood_update_mechanisms_11_18(&g);
    assert(g.player.life_force==495&&o->state_flags==1u);
}

static void test_state18_water_limited_growth(void){
    FloodGame g=mechanism_game(18u,3u);FloodObject *o=&g.objects[0];
    g.world.terrain[4*FLOOD_MAP_W+4]=0xcbu;
    flood_update_mechanisms_11_18(&g);
    assert(o->state_flags==1u&&g.sound_pending&&g.last_sound_id==26u);
    flood_update_mechanisms_11_18(&g);
    assert(o->y==48&&o->origin_y==80&&o->state==18u);
    assert(g.world.terrain[3*FLOOD_MAP_W+4]==0xccu);
    assert(g.world.terrain[5*FLOOD_MAP_W+4]==0xccu);
    g.world.water[6*FLOOD_MAP_W+4]=4u;
    flood_update_mechanisms_11_18(&g);
    assert(o->state==0u&&o->origin_y==80);

    g=mechanism_game(18u,3u);o=&g.objects[0];
    g.world.terrain[3*FLOOD_MAP_W+4]=7u;
    flood_update_mechanisms_11_18(&g);
    assert(o->state_flags==3u&&!g.sound_pending);

    g=mechanism_game(18u,3u);o=&g.objects[0];g.player.x=48;g.player.y=48;
    flood_update_mechanisms_11_18(&g);
    assert(o->state_flags==3u&&!g.sound_pending);

    g=mechanism_game(18u,1u);o=&g.objects[0];g.player.x=56;g.player.y=72;
    flood_update_mechanisms_11_18(&g);
    assert(o->state==18u&&g.player.life_force==511);
}

static void test_state11_dormant_horizontal_growth(void){
    FloodGame g=mechanism_game(11u,3u);FloodObject *o=&g.objects[0];
    g.world.terrain[4*FLOOD_MAP_W+4]=0xcbu;
    flood_update_mechanisms_11_18(&g);
    assert(o->state_flags==1u&&!g.sound_pending);
    flood_update_mechanisms_11_18(&g);
    assert(o->state==11u&&o->x==48&&o->origin_x==80);
    assert(g.world.terrain[4*FLOOD_MAP_W+3]==0xccu);
    assert(g.world.terrain[4*FLOOD_MAP_W+5]==0xccu);
    g.world.water[4*FLOOD_MAP_W+6]=4u;
    flood_update_mechanisms_11_18(&g);
    assert(o->state==0u&&o->origin_x==80);

    g=mechanism_game(11u,3u);o=&g.objects[0];
    g.world.terrain[4*FLOOD_MAP_W+3]=7u;
    flood_update_mechanisms_11_18(&g);
    assert(o->state_flags==3u);

    g=mechanism_game(11u,3u);o=&g.objects[0];g.player.x=48;g.player.y=48;
    flood_update_mechanisms_11_18(&g);
    assert(o->state_flags==3u);

    g=mechanism_game(11u,1u);o=&g.objects[0];g.player.x=72;g.player.y=56;
    g.world.terrain[4*FLOOD_MAP_W+4]=0xcbu;
    flood_update_mechanisms_11_18(&g);
    assert(o->state==11u&&g.player.life_force==511);

    g=mechanism_game(11u,3u);o=&g.objects[0];g.world.mechanisms_paused=true;
    flood_update_mechanisms_11_18(&g);
    assert(o->state_flags==1u); /* State 11 is not gated by $E1EA. */
}

static uint32_t fnv1a(const uint8_t *bytes,size_t count){
    uint32_t hash=2166136261u;
    for(size_t n=0;n<count;n++)hash=(hash^bytes[n])*16777619u;
    return hash;
}

static uint32_t intro_pixel_hash(const FloodIntro *intro){
    uint32_t hash=2166136261u;
    for(unsigned y=FLOOD_INTRO_CONTENT_Y;y<FLOOD_INTRO_CONTENT_Y+FLOOD_INTRO_CONTENT_H;y++)
        for(unsigned x=FLOOD_INTRO_CONTENT_X;x<FLOOD_INTRO_CONTENT_X+FLOOD_INTRO_CONTENT_W;x++)
        hash=(hash^flood_intro_pixel(intro,x,y))*16777619u;
    return hash;
}

static void test_exact_bullfrog_intro(void){
    FloodIntro intro;char err[160];
    assert(flood_intro_load(&intro,FLOOD_DATA_DIR,err,sizeof(err)));
    assert(intro.loaded&&!intro.finished&&!intro.skipped&&intro.frame==0u&&intro.event_pos==0u);
    assert(FLOOD_INTRO_W==320u&&FLOOD_INTRO_H==240u);
    assert(FLOOD_INTRO_CONTENT_X==92u);
    assert(flood_intro_pixel(&intro,0u,0u)==0u&&flood_intro_pixel(&intro,319u,239u)==0u);
    assert(intro.palette[0]==0x000u&&intro.palette[1]==0xfd9u&&intro.palette[31]==0xfb4u);
    assert(intro_pixel_hash(&intro)==0x59c7998au);
    flood_intro_tick(&intro,false);
    assert(intro.frame==1u&&intro.event_pos==32u&&intro_pixel_hash(&intro)==0x59c7998au);
    while(intro.frame<240u)flood_intro_tick(&intro,false);
    assert(intro.event_pos==8u*16u&&intro_pixel_hash(&intro)==0x9c5fdc4bu);
    unsigned min_x=FLOOD_INTRO_W,max_x=0u;
    for(unsigned y=0;y<FLOOD_INTRO_H;y++)for(unsigned x=0;x<FLOOD_INTRO_W;x++)
        if(flood_intro_pixel(&intro,x,y)){if(x<min_x)min_x=x;if(x>max_x)max_x=x;}
    assert(min_x==92u&&max_x==226u&&FLOOD_INTRO_W-1u-max_x==min_x+1u);
    while(intro.frame<320u)flood_intro_tick(&intro,false);
    assert(intro.event_pos==115u*16u&&intro_pixel_hash(&intro)==0x02cd7dffu);
    while(intro.frame<512u)flood_intro_tick(&intro,false);
    assert(intro.event_pos==177u*16u&&intro_pixel_hash(&intro)==0x61ffa37bu);
    while(!intro.finished)flood_intro_tick(&intro,false);
    assert(intro.frame==1216u&&intro.event_pos==204u*16u&&!intro.skipped);
    assert(intro_pixel_hash(&intro)==0x66e84ecfu);
    assert(flood_intro_load(&intro,FLOOD_DATA_DIR,err,sizeof(err)));
    flood_intro_tick(&intro,true);
    assert(intro.frame==1u&&intro.finished&&intro.skipped&&intro.event_pos==0u);
}

static void test_exact_title_screen(void){
    FloodTitle title;char err[160];uint32_t hash=2166136261u;
    assert(FLOOD_TITLE_W==320u&&FLOOD_TITLE_H==208u&&FLOOD_TITLE_BACKING_H==240u);
    assert(FLOOD_TITLE_PLANE_BYTES==FLOOD_TITLE_ROW_BYTES*FLOOD_TITLE_BACKING_H);
    assert(flood_title_load(&title,FLOOD_DATA_DIR,err,sizeof(err))&&title.loaded);
    assert(title.palette[0]==0x000u&&title.palette[1]==0x510u&&title.palette[31]==0xa90u);
    for(unsigned y=0;y<FLOOD_TITLE_H;y++)for(unsigned x=0;x<FLOOD_TITLE_W;x++)
        hash=(hash^flood_title_pixel(&title,x,y))*16777619u;
    assert(hash==0x47f78be6u);
    assert(flood_title_pixel(&title,0u,0u)==16u&&flood_title_pixel(&title,319u,208u)==0u);
}

static uint32_t selector_pixel_hash(const FloodLevelSelect *select){
    uint32_t hash=2166136261u;
    for(unsigned y=0;y<FLOOD_LEVEL_SELECT_H;y++)for(unsigned x=0;x<FLOOD_LEVEL_SELECT_W;x++)
        hash=(hash^flood_level_select_pixel(select,x,y))*16777619u;
    return hash;
}

static void selector_key(FloodLevelSelect *select,uint8_t key){
    flood_level_select_tick(select,key,false);flood_level_select_tick(select,0u,false);
}

static void test_exact_level_selector(void){
    FloodLevelSelect select;char err[160];
    assert(FLOOD_LEVEL_SELECT_W==320u&&FLOOD_LEVEL_SELECT_H==208u&&
        FLOOD_LEVEL_SELECT_BACKING_H==240u);
    assert(FLOOD_LEVEL_SELECT_PLANE_BYTES==FLOOD_LEVEL_SELECT_ROW_BYTES*
        FLOOD_LEVEL_SELECT_BACKING_H);
    assert(flood_level_select_load(&select,FLOOD_DATA_DIR,1u,err,sizeof(err)));
    assert(select.loaded&&!select.finished&&select.selected_level==1u&&select.cursor==0u);
    assert(select.palette[0]==0x000u&&select.palette[7]==0xeeeu&&select.palette[15]==0xc6cu);
    assert(!memcmp(select.passwords[0],"FROG",4u));
    assert(!memcmp(select.passwords[11],"VINE",4u));
    assert(!memcmp(select.passwords[41],"MEEK",4u));
    assert(!memcmp(select.passwords[47],"????",4u));
    assert(selector_pixel_hash(&select)==0xd2ad2c71u);
    selector_key(&select,'V');selector_key(&select,'I');
    assert(select.cursor==2u&&!memcmp(select.panel+76u,"VI??",4u));
    assert(selector_pixel_hash(&select)==0x978d7f7fu);
    selector_key(&select,'N');selector_key(&select,'E');selector_key(&select,0x0du);
    assert(select.selected_level==12u&&select.result_level==12u&&select.cursor==0u);
    assert(select.status_timer==128u&&!memcmp(select.saved_password,"VINE",5u));
    assert(select.panel[57]=='1'&&select.panel[58]=='2');
    assert(selector_pixel_hash(&select)==0xd27d3082u);
    flood_level_select_tick(&select,0u,true);assert(select.finished);

    assert(flood_level_select_load(&select,FLOOD_DATA_DIR,42u,err,sizeof(err)));
    selector_key(&select,'N');selector_key(&select,'O');selector_key(&select,'P');selector_key(&select,'E');
    selector_key(&select,0x0du);
    assert(select.selected_level==1u&&select.result_level==0u&&select.status_timer==128u);
    assert(!memcmp(select.saved_password,"????",5u)&&selector_pixel_hash(&select)==0x927b8f35u);
}

static uint32_t zap_pixel_hash(const FloodZapMessage *message){
    uint32_t hash=2166136261u;
    for(unsigned y=0;y<FLOOD_ZAP_H;y++)for(unsigned x=0;x<FLOOD_ZAP_W;x++)
        hash=(hash^flood_zap_message_pixel(message,x,y))*16777619u;
    return hash;
}

static void test_exact_zap_message(void){
    FloodZapMessage message;char err[160];
    assert(flood_zap_message_load(&message,FLOOD_DATA_DIR,1u,err,sizeof(err)));
    assert(message.loaded&&!message.finished&&message.level==1u);
    assert(!memcmp(message.panel,"%$$$$$$$$$$$$$$$$$$%",20u));
    assert(!memcmp(message.panel+42u,"# ZAP TO LEVEL 01  #",20u));
    assert(!memcmp(message.panel+63u,"#  PASSWORD  FROG  #",20u));
    assert(message.panel[57]=='0'&&message.panel[58]=='1');
    assert(!memcmp(message.panel+76u,"FROG",4u));
    assert(message.palette[0]==0x000u&&message.palette[1]==0x323u&&message.palette[15]==0xcb9u);
    assert(zap_pixel_hash(&message)==0xb457a196u);
    assert(flood_zap_message_pixel(&message,FLOOD_ZAP_W,0u)==0u);
    flood_zap_message_tick(&message,false);assert(!message.finished);
    flood_zap_message_tick(&message,true);assert(message.finished);

    assert(flood_zap_message_load(&message,FLOOD_DATA_DIR,12u,err,sizeof(err)));
    assert(message.panel[57]=='1'&&message.panel[58]=='2');
    assert(!memcmp(message.panel+76u,"VINE",4u));
}

static uint32_t protection_pixel_hash(const FloodProtection *protection){
    uint32_t hash=2166136261u;
    for(unsigned y=0;y<FLOOD_PROTECTION_H;y++)for(unsigned x=0;x<FLOOD_PROTECTION_W;x++)
        hash=(hash^flood_protection_pixel(protection,x,y))*16777619u;
    return hash;
}

static uint32_t high_score_pixel_hash(const FloodHighScores *scores){
    uint32_t hash=2166136261u;
    for(unsigned y=0;y<FLOOD_HIGHSCORE_H;y++)for(unsigned x=0;x<FLOOD_HIGHSCORE_W;x++)
        hash=(hash^flood_high_scores_pixel(scores,x,y))*16777619u;
    return hash;
}

static void high_score_cycles(FloodHighScores *scores,int direction,unsigned count){
    for(unsigned n=0;n<count*12u;n++)
        flood_high_scores_tick(scores,0,(int8_t)direction,false);
    flood_high_scores_tick(scores,0,0,false);
}

static void high_score_confirm(FloodHighScores *scores){
    flood_high_scores_tick(scores,0,0,true);
    flood_high_scores_tick(scores,0,0,false);
}

static void test_exact_high_score_board_and_editor(void){
    FloodHighScores scores;char err[160];
    assert(FLOOD_HIGHSCORE_W==320u&&FLOOD_HIGHSCORE_H==208u&&
        FLOOD_HIGHSCORE_BACKING_H==240u);
    assert(FLOOD_HIGHSCORE_PLANE_BYTES==FLOOD_HIGHSCORE_ROW_BYTES*
        FLOOD_HIGHSCORE_BACKING_H);
    assert(flood_high_scores_load(&scores,FLOOD_DATA_DIR,50u,err,sizeof(err)));
    assert(scores.loaded&&!scores.editing&&scores.entry_index==-1&&!scores.finished);
    assert(scores.scores[0]==2000u&&scores.scores[1]==1800u&&scores.scores[2]==900u&&
        scores.scores[3]==400u&&scores.scores[4]==100u);
    assert(!memcmp(scores.names[0],"SEANY======",11u));
    assert(!memcmp(scores.names[4],"PETER======",11u));
    assert(scores.palette[0]==0x000u&&scores.palette[7]==0xeeeu&&scores.palette[15]==0xc6cu);
    assert(flood_high_scores_pixel(&scores,5u,21u)==11u&&scores.palette[11]==0x600u);
    assert(high_score_pixel_hash(&scores)==0x4fe9bb96u);
    flood_high_scores_tick(&scores,0,0,true);assert(!scores.finished);
    flood_high_scores_tick(&scores,0,0,false);assert(scores.finished);

    assert(flood_high_scores_load(&scores,FLOOD_DATA_DIR,1234u,err,sizeof(err)));
    assert(scores.editing&&scores.entry_index==2&&scores.cursor==0u);
    assert(scores.scores[0]==2000u&&scores.scores[1]==1800u&&scores.scores[2]==1234u&&
        scores.scores[3]==900u&&scores.scores[4]==400u);
    assert(!memcmp(scores.names[2],"===========",11u));
    assert(!memcmp(scores.names[3],"KEVIN======",11u));
    assert(!memcmp(scores.names[4],"SIMON======",11u));
    assert(high_score_pixel_hash(&scores)==0x26f51da1u);

    for(unsigned n=0;n<6u;n++)flood_high_scores_tick(&scores,0,1,false);
    assert(scores.glyph_offset==3240u&&scores.target_offset==3360u&&scores.glyph_step==20);
    assert(high_score_pixel_hash(&scores)==0x751e39dbu);

    assert(flood_high_scores_load(&scores,FLOOD_DATA_DIR,1234u,err,sizeof(err)));
    flood_high_scores_tick(&scores,0,0,false);
    flood_high_scores_tick(&scores,1,0,false);
    assert(scores.cursor==1u&&!memcmp(scores.names[2],"============",11u));
    flood_high_scores_tick(&scores,0,0,false);
    flood_high_scores_tick(&scores,-1,0,false);
    assert(scores.cursor==0u);

    assert(flood_high_scores_load(&scores,FLOOD_DATA_DIR,1234u,err,sizeof(err)));
    high_score_cycles(&scores,1,6u);high_score_confirm(&scores);  /* = -> C */
    high_score_cycles(&scores,1,12u);high_score_confirm(&scores); /* C -> O */
    high_score_cycles(&scores,-1,11u);high_score_confirm(&scores);/* O -> D */
    high_score_cycles(&scores,-1,10u);high_score_confirm(&scores);/* D -> : */
    assert(!scores.editing&&scores.cursor==3u);
    assert(!memcmp(scores.names[2],"COD========",11u));
    assert(high_score_pixel_hash(&scores)==0x808c402eu);
    flood_high_scores_tick(&scores,0,0,true);
    flood_high_scores_tick(&scores,0,0,false);
    assert(scores.finished);
}

static void test_level_selector_over_score_background(void){
    FloodHighScores scores;FloodLevelSelect select;char err[160];
    assert(flood_high_scores_load(&scores,FLOOD_DATA_DIR,50u,err,sizeof(err)));
    assert(flood_level_select_load(&select,FLOOD_DATA_DIR,1u,err,sizeof(err)));

    unsigned background_x=0u,background_y=0u,panel_x=0u,panel_y=0u;
    bool found_background=false,found_panel=false;
    for(unsigned y=0;y<FLOOD_LEVEL_SELECT_H;y++)for(unsigned x=0;x<FLOOD_LEVEL_SELECT_W;x++){
        const uint8_t score=flood_high_scores_pixel(&scores,x,y);
        const uint8_t clean=flood_level_select_pixel(&select,x,y);
        if(score==clean)continue;
        if(y<72u||y>=120u){
            if(!found_background){background_x=x;background_y=y;found_background=true;}
        }else if(x>=80u&&x<240u&&!found_panel){
            panel_x=x;panel_y=y;found_panel=true;
        }
    }
    assert(found_background&&found_panel);
    const uint8_t score_background=flood_high_scores_pixel(
        &scores,background_x,background_y);
    const uint8_t clean_panel=flood_level_select_pixel(&select,panel_x,panel_y);
    flood_level_select_set_background(&select,scores.planes);
    assert(flood_level_select_pixel(&select,background_x,background_y)==score_background);
    assert(flood_level_select_pixel(&select,panel_x,panel_y)==clean_panel);
}

static void protection_key(FloodProtection *protection,uint8_t key){
    flood_protection_tick(protection,0,key,false);
    flood_protection_tick(protection,0,0u,false);
}

static void protection_choose_english(FloodProtection *protection){
    flood_protection_tick(protection,0,0u,true);
    assert(protection->phase==1u&&protection->selection_pending);
    flood_protection_tick(protection,0,0u,false);
    assert(protection->phase==2u&&!protection->selection_pending);
}

static void test_exact_copy_protection(void){
    FloodProtection protection;char err[160];
    assert(flood_protection_load(&protection,FLOOD_DATA_DIR,0u,err,sizeof(err)));
    assert(protection.loaded&&!protection.finished&&protection.phase==1u);
    assert(protection.palette[0]==0x000u&&protection.palette[1]==0x323u&&
        protection.palette[15]==0xcb9u);
    assert(!memcmp(protection.challenges,"How many Balloons are there?",28u));
    assert(protection.challenges[60]==0u&&protection.challenges[61]=='6');
    assert(!memcmp(protection.challenges+19u*74u,"How many stripes on the Snail?",30u));
    assert(protection_pixel_hash(&protection)==0xd9f71d7fu);

    protection_choose_english(&protection);
    assert(protection.rng_state==0x24dfu&&protection.challenge==15u);
    assert(protection_pixel_hash(&protection)==0x453992ceu);
    protection_key(&protection,'2');protection_key(&protection,0x0du);
    assert(protection.phase==3u&&!protection.finished&&protection.failures==0u);
    assert(protection_pixel_hash(&protection)==0x2c2894e6u);
    flood_protection_tick(&protection,0,0u,true);
    flood_protection_tick(&protection,0,0u,false);
    assert(protection.finished&&protection.passed&&!protection.exhausted);

    assert(flood_protection_load(&protection,FLOOD_DATA_DIR,0u,err,sizeof(err)));
    protection_choose_english(&protection);
    protection_key(&protection,'9');protection_key(&protection,0x0du);
    assert(protection.phase==4u&&protection.failures==1u);
    assert(protection_pixel_hash(&protection)==0x9727a3e2u);
    flood_protection_tick(&protection,0,0u,true);
    flood_protection_tick(&protection,0,0u,false);
    assert(protection.phase==2u&&protection.cursor==0u&&!memcmp(protection.answer,"             ",13u));
    protection_key(&protection,'9');protection_key(&protection,0x0du);
    flood_protection_tick(&protection,0,0u,true);
    flood_protection_tick(&protection,0,0u,false);
    assert(protection.finished&&!protection.passed&&protection.exhausted&&protection.failures==2u);

    assert(flood_protection_load(&protection,FLOOD_DATA_DIR,0u,err,sizeof(err)));
    flood_protection_tick(&protection,-1,0u,false);
    assert(protection.language==3u); /* Signed left wraps English to Italian. */
    assert(protection.phase==1u&&!protection.selection_pending);
    flood_protection_tick(&protection,-1,0u,false);
    assert(protection.language==3u); /* A held direction does not auto-repeat. */
    flood_protection_tick(&protection,0,0u,false);
    flood_protection_tick(&protection,1,0u,false);
    assert(protection.language==0u);

    assert(flood_protection_load(&protection,FLOOD_DATA_DIR,0u,err,sizeof(err)));
    flood_protection_seed(&protection,0x1234u);
    protection_choose_english(&protection);
    const uint8_t first_challenge=protection.challenge;
    assert(flood_protection_load(&protection,FLOOD_DATA_DIR,0u,err,sizeof(err)));
    flood_protection_seed(&protection,0x5678u);
    protection_choose_english(&protection);
    assert(protection.challenge!=first_challenge);
}

static void test_exact_ending_sequence(void){
    FloodEnding ending;char err[160];
    assert(FLOOD_ENDING_W==320u&&FLOOD_ENDING_H==208u&&FLOOD_ENDING_BACKING_H==240u);
    assert(FLOOD_ENDING_PLANE_BYTES==FLOOD_ENDING_ROW_BYTES*FLOOD_ENDING_BACKING_H);
    assert(flood_ending_load(&ending,FLOOD_DATA_DIR,err,sizeof(err)));
    assert(ending.loaded&&!ending.finished&&ending.event_index==0u&&ending.hold==1u);
    assert(ending.palette[0]==0x000u&&ending.palette[7]==0xb96u&&ending.palette[15]==0x950u);
    assert(fnv1a(ending.screen,FLOOD_ENDING_SCREEN_BYTES)==0xea25b5d0u);
    assert(flood_ending_pixel(&ending,0,0)==0u);
    /* The first off-window row contains the old visible artefact, but the
       original 208-line DIW clips it. */
    {const unsigned x=133u,y=208u;const size_t byte=(size_t)y*FLOOD_ENDING_ROW_BYTES+x/8u;
     const uint8_t bit=(uint8_t)(0x80u>>(x&7u));uint8_t backing=0u;
     for(unsigned plane=0;plane<4u;plane++)if(ending.screen[plane*FLOOD_ENDING_PLANE_BYTES+byte]&bit)
         backing|=(uint8_t)(1u<<plane);
     assert(backing==12u&&flood_ending_pixel(&ending,x,y)==0u);}
    flood_ending_tick(&ending);assert(ending.frames==1u&&ending.event_index==0u);
    while(ending.event_index<39u)flood_ending_tick(&ending);
    assert(fnv1a(ending.screen,FLOOD_ENDING_SCREEN_BYTES)==0x66a53719u);
    while(ending.event_index<47u)flood_ending_tick(&ending);
    assert(fnv1a(ending.screen,FLOOD_ENDING_SCREEN_BYTES)==0x7daa6d4cu);
    while(!ending.finished)flood_ending_tick(&ending);
    assert(ending.frames==334u&&ending.event_index==FLOOD_ENDING_EVENT_COUNT&&ending.hold==0u);
    assert(fnv1a(ending.screen,FLOOD_ENDING_SCREEN_BYTES)==0x93ba1d50u);
}

static void test_exact_audio_dispatch_and_replay(void){
    FloodAudio audio;char err[160];
    assert(flood_audio_load(&audio,FLOOD_DATA_DIR,44100u,err,sizeof(err)));
    assert(audio.song_base==2u&&audio.track_table==18u&&audio.speed==3u);
    flood_audio_trigger(&audio,7u);
    assert(!audio.channel[0].active&&audio.channel[1].active&&!audio.channel[2].active&&!audio.channel[3].active);
    assert(!audio.channel[1].loop_track);
    int16_t *pcm=malloc(44100u*2u*sizeof(*pcm));assert(pcm);
    flood_audio_mix(&audio,pcm,413u);
    assert(audio.channel[1].period==0u);
    flood_audio_mix(&audio,pcm+826u,1u);
    assert(audio.channel[1].period==0x02cfu&&audio.channel[1].sample_id==2u);
    assert(audio.channel[1].dma_state==1u&&!audio.channel[1].sample_playing&&pcm[826u]==0&&pcm[827u]==0);
    flood_audio_mix(&audio,pcm+828u,44100u-414u);
    assert(fnv1a((const uint8_t *)pcm,44100u*2u*sizeof(*pcm))==0x7e21203au);
    free(pcm);
    assert(flood_audio_load(&audio,FLOOD_DATA_DIR,44100u,err,sizeof(err)));
    flood_audio_trigger(&audio,60u);
    for(unsigned n=0;n<4u;n++)assert(audio.channel[n].active);
    assert(flood_audio_is_playing(&audio));
    int16_t finish_pcm[1024u*2u];unsigned finish_frames=0u;
    while(flood_audio_is_playing(&audio)&&finish_frames<44100u*30u){
        flood_audio_mix(&audio,finish_pcm,1024u);finish_frames+=1024u;
    }
    assert(!flood_audio_is_playing(&audio));
    assert(finish_frames>0u&&finish_frames<44100u*30u);
    /* Water entry requests effect 15 after the finite 13/14 components.
       Its original $00A4 control word carries bit $20, so channel 2 must
       wrap its current effect track instead of becoming inactive at $FF. */
    assert(flood_audio_load(&audio,FLOOD_DATA_DIR,44100u,err,sizeof(err)));
    flood_audio_trigger(&audio,15u);
    assert(audio.channel[2].active&&audio.channel[2].loop_track);
    assert(audio.channel[2].track_start==audio.channel[2].track_pos);
    {bool wrapped=false;size_t previous=audio.channel[2].track_pos;
     for(unsigned tick=0u;tick<20000u&&!wrapped;tick++){
         flood_audio_tick(&audio);
         if(audio.channel[2].track_pos<previous)wrapped=true;
         previous=audio.channel[2].track_pos;
     }
     assert(wrapped&&audio.channel[2].active);}
    /* Sound 24 is one original two-note track.  Macro C7 must ramp the first
       note to full volume before C8 starts and fades the higher second note. */
    assert(flood_audio_load(&audio,FLOOD_DATA_DIR,44100u,err,sizeof(err)));
    flood_audio_trigger(&audio,24u);
    for(unsigned frames=0u;frames<26460u;){
        const unsigned block=26460u-frames<1024u?26460u-frames:1024u;
        flood_audio_mix(&audio,finish_pcm,block);frames+=block;
    }
    assert(audio.channel[1].active&&audio.channel[1].track_pos==0x247u);
    assert(audio.channel[1].period==559u&&audio.channel[1].volume==63u);
    for(unsigned frames=26460u;frames<83790u;){
        const unsigned block=83790u-frames<1024u?83790u-frames:1024u;
        flood_audio_mix(&audio,finish_pcm,block);frames+=block;
    }
    assert(audio.channel[1].active&&audio.channel[1].track_pos==0x249u);
    assert(audio.channel[1].period==373u&&audio.channel[1].volume==59u);
    FloodGame game;memset(&game,0,sizeof(game));game.matilda.read_pos=1u;game.player.x=400;game.player.y=400;
    game.objects[0].active=true;game.objects[0].state=6u;
    game.objects[1].active=true;game.objects[1].state=9u;
    flood_update_states_6_9_15(&game);
    assert(game.sound_queue_count==2u&&game.sound_queue[0]==7u&&game.sound_queue[1]==7u);
}

static void test_exact_music_sequence_and_replay(void){
    FloodAudio audio;char err[160];
    assert(flood_music_load(&audio,FLOOD_DATA_DIR,44100u,err,sizeof(err)));
    assert(audio.music_mode&&audio.song_base==2u&&audio.track_table==18u&&audio.speed==3u);
    assert(audio.volume_macros==0x20f5u&&audio.pitch_macros==0x2295u);
    assert(audio.channel[0].sequence_start==0x138u&&audio.channel[0].track_pos==0x20bu);
    assert(audio.channel[1].sequence_start==0x165u&&audio.channel[1].track_pos==0x784u);
    assert(audio.channel[2].sequence_start==0x192u&&audio.channel[2].track_pos==0xcedu);
    assert(audio.channel[3].sequence_start==0x1c4u&&audio.channel[3].track_pos==0x199du);
    int16_t *pcm=malloc(44100u*5u*2u*sizeof(*pcm));assert(pcm);
    flood_audio_mix(&audio,pcm,413u);
    flood_audio_mix(&audio,pcm+826u,1u);
    assert(audio.channel[0].period==0x023au&&audio.channel[0].sample_id==5u&&audio.channel[0].delay==48u);
    assert(audio.channel[1].period==0x01fcu&&audio.channel[1].sample_id==5u&&audio.channel[1].delay==16u);
    assert(audio.channel[2].period==0x01abu&&audio.channel[2].sample_id==5u&&audio.channel[2].delay==32u);
    assert(audio.channel[3].period==0x02f9u&&audio.channel[3].sample_id==5u&&audio.channel[3].delay==64u);
    for(unsigned n=0;n<FLOOD_AUDIO_CHANNELS;n++)
        assert(audio.channel[n].dma_state==1u&&!audio.channel[n].sample_playing);
    flood_audio_mix(&audio,pcm+828u,44100u*5u-414u);
    assert(fnv1a((const uint8_t *)pcm,44100u*5u*2u*sizeof(*pcm))==0x30179021u);
    for(unsigned n=0;n<FLOOD_AUDIO_CHANNELS;n++)assert(audio.channel[n].active&&audio.channel[n].loop_sequence);
    free(pcm);
    assert(flood_music_load(&audio,FLOOD_DATA_DIR,44100u,err,sizeof(err)));
    bool wrapped[FLOOD_AUDIO_CHANNELS]={false,false,false,false};
    unsigned wrap_tick[FLOOD_AUDIO_CHANNELS]={0,0,0,0};
    size_t previous[FLOOD_AUDIO_CHANNELS];
    for(unsigned n=0;n<FLOOD_AUDIO_CHANNELS;n++)previous[n]=audio.channel[n].sequence_pos;
    bool heard_pitch_macro=false;
    for(unsigned tick=0;tick<25000u;tick++){
        flood_audio_tick(&audio);
        if(tick==3197u){
            /* Roughly 30 seconds into presentation playback, all four Paula
               voices must still be audible.  The missing per-note envelope
               restart previously left channels 0, 1, and 3 at volume zero. */
            assert(audio.channel[0].volume==43u&&audio.channel[1].volume==24u&&
                audio.channel[2].volume==60u&&audio.channel[3].volume==60u);
            assert(audio.channel[0].sample_id==0u&&audio.channel[1].sample_id==2u&&
                audio.channel[2].sample_id==0u&&audio.channel[3].sample_id==2u);
        }
        for(unsigned n=0;n<FLOOD_AUDIO_CHANNELS;n++){
            if(audio.channel[n].sequence_pos<previous[n]){wrapped[n]=true;if(!wrap_tick[n])wrap_tick[n]=tick+1u;}
            previous[n]=audio.channel[n].sequence_pos;
            const FloodAudioEnvelope *e=&audio.channel[n].pitch_env;
            int32_t expected=(int32_t)audio.channel[n].base_period+(int16_t)e->value-(int16_t)e->base;
            if(expected<0)expected=0;
            assert(audio.channel[n].period==(uint16_t)expected);
            if(e->value||e->base)heard_pitch_macro=true;
        }
    }
    assert(heard_pitch_macro);
    for(unsigned n=0;n<FLOOD_AUDIO_CHANNELS;n++){
        assert(wrapped[n]&&wrap_tick[n]==24960u&&audio.channel[n].active);
    }
}

int main(void){
    FloodGame g; flood_game_init(&g); char err[160];
    assert(flood_game_load_level(&g,FLOOD_DATA_DIR,1,err,sizeof(err)));
    char custom_header[1024];
    assert(snprintf(custom_header,sizeof(custom_header),"%s/levels/level_01_header.bin",
        FLOOD_DATA_DIR)<(int)sizeof(custom_header));
    FloodGame selected;flood_game_init(&selected);
    assert(flood_game_load_custom_level(&selected,FLOOD_DATA_DIR,custom_header,err,sizeof(err)));
    assert(selected.world.level_number==1u&&selected.world.block_bank==g.world.block_bank);
    assert(selected.player.x==g.player.x&&selected.player.y==g.player.y);
    assert(memcmp(selected.world.terrain,g.world.terrain,sizeof(g.world.terrain))==0);
    assert(!flood_game_load_custom_level(&selected,FLOOD_DATA_DIR,
        "not_a_level.bin",err,sizeof(err)));
    assert(g.player.x==96 && g.player.y==80);
    assert(g.world.block_bank==1);
    assert(g.matilda.write_pos==0u&&g.matilda.read_pos==1u);
    FloodGame level5;flood_game_init(&level5);
    assert(flood_game_load_level(&level5,FLOOD_DATA_DIR,5,err,sizeof(err)));
    assert(level5.player.x==4*16&&level5.player.y==7*16);
    assert(level5.world.bounds_x==1040&&level5.world.bounds_y==432);
    flood_update_camera(&level5);
    assert(level5.camera_x==0&&level5.camera_y==20);
    assert(g.world.bounds_x==0x6c0&&g.world.bounds_y==0x570);
    const FloodSprite *q=flood_sprite(&g,0xD2);
    assert(q && q->width==32 && q->height==32);
    unsigned opaque=0; for(unsigned i=0;i<FLOOD_MAX_SPRITE_PIXELS;i++) opaque+=q->mask[i]!=0;
    assert(opaque>40);
    assert(flood_player_sprite_id(&g)==0x50);
    assert(flood_tile_index_at(16,16)==129);
    /* Plane 0 is color bit 0: catches the rejected permutation-07 decoder. */
    assert(flood_tile_pixel(&g,7,5,0)==1);
    assert(flood_tile_pixel(&g,7,6,0)==2);
    assert(flood_tile_pixel(&g,7,7,0)==3);
    assert(flood_tile_pixel(&g,7,8,0)==15);
    test_exact_hud_builder(&g);
    test_exact_hud_scroll_placement();
    test_exact_camera_follow();
    test_exact_tile_overfetch();
    test_level1_conveyor_corridors();
    test_exact_overlay_compositor();
    test_exact_backing_crop_fades_and_transport_filter();
    test_exact_water_falling_and_spread();
    test_exact_water_fill_and_rise();
    test_exact_water_scheduler_and_rebuild();
    test_level1_water_snapshot();
    test_water_frame_order_and_quiffy_coupling();
    test_multiple_and_zero_x_sources();
    g.score=0u;g.lives=3;g.player.life_force=511;g.player.air=63;
    flood_update_hud(&g);
    test_swept_collision();
    test_contact_ring_and_resolver();
    test_all_cardinal_attachments();
    test_exposed_surface_end_requires_toward_input();
    test_attachment_maintenance();
    test_attachment_replays_captured_direction();
    test_tight_passage_wall_handoff();
    test_shipped_tight_passages();
    test_wall_to_top_full_jump();
    test_cardinal_surface_replays_attachment();
    test_attachment_cache_ignores_new_perpendicular_input();
    test_original_cache_advances_corner_handoff();
    test_level13_original_corner_trace();
    test_level4_ceiling_corner_reversal();
    test_diagonal_corner_handoff_is_retained();
    test_flamethrower_holds_quiffy_input();
    test_binary_gravity_step();
    test_exact_quiffy_animation_cadence();
    test_exact_matilda_history_chase();
    test_slope_micro_corrections();
    test_fatal_terrain_contact();
    test_level23_sparkling_fungi_identity();
    test_exact_quiffy_death_lifecycle();
    test_escape_final_life_abort();
    test_airborne_death_forces_gravity();
    test_action_record_dispatch_and_pickups();
    test_action_family_lifecycles();
    test_exact_grenade_blast_lobes();
    test_exact_boomerang_corridor_and_homing();
    test_carried_items_and_mine_impulse();
    test_verified_frame_banks();
    test_all_single_corner_poses();
    test_complete_pose_selector();
    test_map_pickups();
    test_instruction_frame_order();
    test_exact_level_banner_timing();
    test_exact_ouch_feedback();
    test_exact_incidental_sound_edges();
    test_complete_item_dispatcher();
    test_exact_level_completion_handoff();
    test_restart_level_control();
    test_special_items_and_orange_skip();
    test_paired_transport_tiles();
    test_dc_state1_bounce_and_restore();
    test_space_hopper_full_height_support();
    test_dc_state1_fatal_transition();
    test_dc_state2_exact_path();
    test_mine_to_heart_path();
    test_shared_states_2_13_explosion();
    test_exact_heart_motion_collision_and_collection();
    test_state5_beady_ball_free_flight();
    test_state5_beady_ball_collisions_and_damage();
    test_bolt_terrain_impact_and_reset();
    test_bolt_player_hit_final_advance();
    test_marker20_initialization();
    test_trigger_payload_swap_and_marker22();
    test_trigger_creates_six_bolts();
    test_level5_rocket_switch_and_impact_sound();
    test_complete_marker_jump_table();
    test_all_level_marker_population();
    test_dispatch_noops_bounds_and_state24_bridge();
    test_state5_shipped_reachability();
    test_state12_lumpy_and_snail();
    test_state12_teddy();
    test_state12_vong();
    test_level_food_counter();
    test_state14_plonkin();
    test_state1_doctor_and_companion();
    test_shared_states_6_9_15_burst();
    test_states_6_15_shipped_reachability();
    test_state8_vacuous_gombo();
    test_state16_horizontal_mechanism();
    test_state17_vertical_mechanism();
    test_state18_water_limited_growth();
    test_state11_dormant_horizontal_growth();
    test_exact_ending_sequence();
    test_exact_bullfrog_intro();
    test_exact_title_screen();
    test_exact_level_selector();
    test_exact_zap_message();
    test_exact_copy_protection();
    test_exact_high_score_board_and_editor();
    test_level_selector_over_score_background();
    test_exact_audio_dispatch_and_replay();
    test_exact_music_sequence_and_replay();
    FloodInput in={0}; in.x=1; flood_game_tick(&g,in);
    assert(flood_player_sprite_id(&g)==(uint16_t)(0x50u+g.player.pose_code));
    puts("core ok");
    return 0;
}
