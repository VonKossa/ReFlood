#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define main flood_sdl_host_entry
#include "../src/main.c"
#undef main

static SDL_Event queued[8];
static unsigned queued_count;
static unsigned queued_pos;
static Uint8 keys[512];
static int controller_joystick_count;
static bool controller_recognized=true;
static bool controller_attached;
static unsigned controller_open_calls,controller_close_calls;
static int controller_last_open_index=-1;
static Sint16 controller_axes[2];
static Uint8 controller_buttons[7];
static SDL_JoystickID controller_instance=17;
static unsigned fullscreen_calls;
static Uint32 fullscreen_flags;
static unsigned window_size_calls;
static unsigned window_position_calls;
static unsigned render_clear_calls;
static unsigned render_present_calls;
static unsigned texture_width,texture_height,texture_scale_calls;
static SDL_ScaleMode texture_scale_mode;
static int texture_update_pitch,logical_width,logical_height;
static Uint32 ticks;
static Uint32 scheduled_fire_tick=UINT32_MAX;
static Uint32 scheduled_fire_release_tick=UINT32_MAX;
static Uint32 scheduled_quit_tick=UINT32_MAX;
static Uint32 scheduled_escape_tick=UINT32_MAX;
static Uint32 scheduled_second_fire_tick=UINT32_MAX;
static Uint32 scheduled_second_fire_release_tick=UINT32_MAX;
static bool score_autofire;
static Uint32 audio_unlock_ticks[64];
static unsigned audio_unlock_count;
static FloodAudio *observed_audio;
static Uint32 first_channel_one_active_tick=UINT32_MAX;

static void reset_input_state(void){
    memset(queued,0,sizeof(queued));
    memset(keys,0,sizeof(keys));
    queued_count=0;queued_pos=0;
    scheduled_fire_tick=UINT32_MAX;
    scheduled_fire_release_tick=UINT32_MAX;
    scheduled_escape_tick=UINT32_MAX;
    scheduled_second_fire_tick=UINT32_MAX;
    scheduled_second_fire_release_tick=UINT32_MAX;
}

static void reset_controller_state(void){
    memset(controller_axes,0,sizeof(controller_axes));
    memset(controller_buttons,0,sizeof(controller_buttons));
    controller_joystick_count=0;controller_recognized=true;
    controller_attached=false;controller_open_calls=0;controller_close_calls=0;
    controller_last_open_index=-1;
    game_controller=NULL;
}

static void queue_key(SDL_Keycode key,Uint8 repeat){
    assert(queued_count<sizeof(queued)/sizeof(queued[0]));
    queued[queued_count].type=SDL_KEYDOWN;
    queued[queued_count].key.keysym.sym=key;
    queued[queued_count].key.repeat=repeat;
    queued_count++;
}
static void queue_controller_event(Uint32 type,int which){
    assert(queued_count<sizeof(queued)/sizeof(queued[0]));
    queued[queued_count].type=type;queued[queued_count].cdevice.which=which;
    queued_count++;
}
static bool file_exists(const char *path){
    FILE *file=fopen(path,"rb");if(!file)return false;fclose(file);return true;
}

int main(void){
    SDL_Window *window=(SDL_Window *)(uintptr_t)1u;
    FloodInput in;

    ReFloodSettings settings=settings_defaults();bool parsed=false;
    ReFloodScaler parsed_scaler=REFLOOD_SCALER_COUNT;
    ReFloodGameSpeed parsed_speed=REFLOOD_GAME_SPEED_COUNT;
    ReFloodInputMode legacy=REFLOOD_INPUT_COUNT;
    unsigned choice=0u;
    assert(!settings.fullscreen&&settings.scaler==REFLOOD_SCALER_NEAREST&&
        settings.game_speed==REFLOOD_GAME_SPEED_NORMAL&&
        settings.multiplayer_input[0]==0u&&settings.multiplayer_input[1]==1u);
    assert(settings_parse_fullscreen("fullscreen=1\n",&parsed)&&parsed);
    assert(settings_parse_fullscreen(" fullscreen = windowed \r\n",&parsed)&&!parsed);
    assert(!settings_parse_fullscreen("fullscreen=1garbage\n",&parsed));
    assert(settings_parse_scaler("scaler=scale2x\n",&parsed_scaler)&&
        parsed_scaler==REFLOOD_SCALER_SCALE2X);
    assert(settings_parse_game_speed("game_speed=turbo\n",&parsed_speed)&&
        parsed_speed==REFLOOD_GAME_SPEED_TURBO);
    assert(settings_parse_input("input=gamepad\n",&legacy)&&
        legacy==REFLOOD_INPUT_GAMEPAD);
    assert(settings_parse_player("player1=wasd\n","player1",&choice)&&choice==1u);
    assert(settings_parse_player("player2=gamepad2\n","player2",&choice)&&choice==3u);
    assert(!settings_parse_player("player1=invalid\n","player1",&choice));
    char settings_err[160]={0};
    remove(REFLOOD_CONFIG_PATH);remove(REFLOOD_CONFIG_TEMP_PATH);
    assert(settings_tick(&settings,0,1,false)==1u&&settings.fullscreen);
    settings.selected=3u;settings.horizontal_ready=true;
    assert(settings_tick(&settings,0,1,false)==1u&&settings.multiplayer_input[0]==1u);
    settings.selected=4u;settings.confirm_ready=true;
    assert(settings_tick(&settings,0,0,true)==1u&&settings.multiplayer_input[1]==2u);
    settings.selected=5u;settings.confirm_ready=true;
    assert(settings_tick(&settings,0,0,true)==2u);
    settings.selected=6u;settings.confirm_ready=true;
    assert(settings_tick(&settings,0,0,true)==8u);
    settings.custom_map=true;
    strcpy(settings.custom_header,"custom/levels/my_map_header.bin");
    settings.selected=7u;settings.confirm_ready=true;
    assert(settings_tick(&settings,0,0,true)==4u&&!settings.multiplayer&&
        settings.custom_map&&!strcmp(settings.custom_header,"custom/levels/my_map_header.bin"));
    settings.selected=8u;settings.confirm_ready=true;
    assert(settings_tick(&settings,0,0,true)==4u&&settings.multiplayer&&settings.custom_map);
    settings.selected=0u;settings.vertical_ready=true;
    assert(settings_tick(&settings,-1,0,false)==0u&&settings.selected==8u);
    char level_directory[1024],selected_header[EDITOR_BROWSER_PATH]={0};
    char browser_message[48]={0};EditorBrowser browser;
    assert(snprintf(level_directory,sizeof(level_directory),"%s/levels",FLOOD_DATA_DIR)<
        (int)sizeof(level_directory));
    assert(editor_browser_open(&browser,level_directory,settings_err,sizeof(settings_err)));
    bool found_level=false;
    for(unsigned n=0;n<browser.count;n++)if(!strcmp(browser.entries[n].name,"level_01_header.bin")){
        browser.selected=n;found_level=true;break;
    }
    assert(found_level);
    assert(custom_browser_activate(&browser,selected_header,sizeof(selected_header),
        browser_message,sizeof(browser_message))==CUSTOM_BROWSER_SELECTED);
    assert(strstr(selected_header,"level_01_header.bin")&&browser_message[0]=='\0');
    uint8_t browser_font[SETTINGS_FONT_BYTES];
    assert(settings_load_font(browser_font,settings_err,sizeof(settings_err)));
    uint32_t *browser_frame=malloc(VIEW_W*VIEW_H*sizeof(*browser_frame));assert(browser_frame);
    reset_input_state();queue_key(SDLK_DELETE,0u);
    assert(run_custom_browser(window,(SDL_Renderer *)(uintptr_t)1u,
        (SDL_Texture *)(uintptr_t)1u,browser_frame,browser_font,selected_header,
        sizeof(selected_header))==CUSTOM_BROWSER_CLEARED);
    free(browser_frame);
    settings.scaler=REFLOOD_SCALER_SCALE2X;
    settings.game_speed=REFLOOD_GAME_SPEED_TURBO;
    assert(settings_save_configuration(&settings,settings_err,sizeof(settings_err)));
    settings=settings_defaults();settings_load_configuration(&settings);
    assert(settings.fullscreen&&settings.scaler==REFLOOD_SCALER_SCALE2X&&
        settings.game_speed==REFLOOD_GAME_SPEED_TURBO&&
        settings.multiplayer_input[0]==1u&&settings.multiplayer_input[1]==2u);
    remove(REFLOOD_CONFIG_PATH);remove(REFLOOD_CONFIG_TEMP_PATH);
    assert(settings_make_config_directory());
    char sequence_first[EDITOR_BROWSER_PATH],sequence_second[EDITOR_BROWSER_PATH];
    char sequence_next[EDITOR_BROWSER_PATH];
    assert(snprintf(sequence_first,sizeof(sequence_first),
        "%s/level_01_header.bin",REFLOOD_CONFIG_DIR)<(int)sizeof(sequence_first));
    assert(snprintf(sequence_second,sizeof(sequence_second),
        "%s/level_02_header.bin",REFLOOD_CONFIG_DIR)<(int)sizeof(sequence_second));
    remove(sequence_second);
    assert(!reflood_custom_next_header(sequence_first,sequence_next,sizeof(sequence_next)));
    FILE *sequence_file=fopen(sequence_second,"wb");assert(sequence_file);
    assert(fputc(0,sequence_file)!=EOF&&fclose(sequence_file)==0);
    assert(reflood_custom_next_header(sequence_first,sequence_next,sizeof(sequence_next)));
    assert(!strcmp(sequence_next,sequence_second));
    assert(!reflood_custom_next_header(sequence_second,sequence_next,sizeof(sequence_next)));
    assert(!reflood_custom_next_header("level_99_header.bin",sequence_next,
        sizeof(sequence_next)));
    assert(!reflood_custom_next_header("my_cave_header.bin",sequence_next,
        sizeof(sequence_next)));
    assert(!reflood_custom_next_header(sequence_first,sequence_next,8u));
    remove(sequence_second);
    FILE *old_configuration=fopen(REFLOOD_CONFIG_PATH,"wb");assert(old_configuration);
    assert(fputs("fullscreen=1\nscaler=linear\ninput=gamepad\n",old_configuration)>=0);
    assert(fclose(old_configuration)==0);
    settings=settings_defaults();settings_load_configuration(&settings);
    assert(settings.fullscreen&&settings.scaler==REFLOOD_SCALER_LINEAR&&
        settings.multiplayer_input[0]==2u&&settings.multiplayer_input[1]==1u);
    remove(REFLOOD_CONFIG_PATH);remove(REFLOOD_CONFIG_TEMP_PATH);

    FloodHighScores persisted_scores,loaded_scores;
    remove(REFLOOD_HISCORE_PATH);remove(REFLOOD_HISCORE_TEMP_PATH);
    assert(flood_high_scores_load(&persisted_scores,FLOOD_DATA_DIR,0u,
        settings_err,sizeof(settings_err)));
    persisted_scores.scores[0]=9999u;
    memcpy(persisted_scores.names[0],"TEST=======",12u);
    assert(high_scores_save_configuration(&persisted_scores,settings_err,sizeof(settings_err)));
    assert(file_exists(REFLOOD_HISCORE_PATH));
    assert(flood_high_scores_load(&loaded_scores,FLOOD_DATA_DIR,0u,
        settings_err,sizeof(settings_err)));
    assert(high_scores_load_configuration(&loaded_scores,settings_err,sizeof(settings_err)));
    assert(loaded_scores.scores[0]==9999u);
    assert(!memcmp(loaded_scores.names[0],"TEST=======",12u));
    flood_high_scores_enter(&loaded_scores,5000u);
    assert(loaded_scores.entry_index==1&&loaded_scores.scores[1]==5000u);
    assert(!memcmp(loaded_scores.names[1],"===========",12u));
    FILE *invalid_scores=fopen(REFLOOD_HISCORE_PATH,"wb");assert(invalid_scores);
    assert(fputs("INVALID\n",invalid_scores)>=0);assert(fclose(invalid_scores)==0);
    const uint32_t retained_score=loaded_scores.scores[0];settings_err[0]='\0';
    assert(!high_scores_load_configuration(&loaded_scores,settings_err,sizeof(settings_err)));
    assert(settings_err[0]&&loaded_scores.scores[0]==retained_score);
    remove(REFLOOD_HISCORE_PATH);remove(REFLOOD_HISCORE_TEMP_PATH);

    assert(GAME_FRAME_MS==60u);
    assert(TURBO_GAME_FRAME_MS==30u);
    assert(game_frame_ms(REFLOOD_GAME_SPEED_NORMAL)==60u);
    assert(game_frame_ms(REFLOOD_GAME_SPEED_TURBO)==30u);
    assert(strcmp(REFLOOD_WINDOW_TITLE,"ReFlood")==0);
    assert(INTRO_FRAME_MS==20u);
    assert(INTRO_MUSIC_LEAD_MS==8354u);
    assert(POST_SCORE_SCREEN_FRAMES==200u);
    assert(POST_SCORE_SCREEN_FRAMES*PRESENTATION_FRAME_MS==4000u);
    assert(FLOOD_ENDING_W==320u&&FLOOD_ENDING_H==208u);
    assert(FLOOD_TITLE_W==320u&&FLOOD_TITLE_H==208u);
    assert(FLOOD_LEVEL_SELECT_W==320u&&FLOOD_LEVEL_SELECT_H==208u);
    assert(FLOOD_HIGHSCORE_W==320u&&FLOOD_HIGHSCORE_H==208u);
    assert(presentation_framebuffer_pixels()==(size_t)320u*240u);
    assert(presentation_framebuffer_pixels()>(size_t)FLOOD_TITLE_W*FLOOD_TITLE_H);
    assert(ending_opens_post_score_loop(true,false));
    assert(!ending_opens_post_score_loop(false,false));
    assert(!ending_opens_post_score_loop(true,true));

    FloodProtection fade_protection;memset(&fade_protection,0,sizeof(fade_protection));
    assert(!protection_exit_fades(&fade_protection,false));
    fade_protection.passed=true;
    assert(protection_exit_fades(&fade_protection,false));
    assert(!protection_exit_fades(&fade_protection,true));
    FloodGame fade_game;memset(&fade_game,0,sizeof(fade_game));fade_game.lives=0;
    assert(game_over_exit_fades(&fade_game,false));
    fade_game.game_complete=true;
    assert(!game_over_exit_fades(&fade_game,false));
    fade_game.game_complete=false;
    assert(!game_over_exit_fades(&fade_game,true));

    uint32_t fade_pixels[2]={0xffffffffu,0xff112233u};
    ticks=0u;render_present_calls=0u;
    fade_out((SDL_Renderer *)(uintptr_t)1u,(SDL_Texture *)(uintptr_t)1u,
        fade_pixels,2u,1u);
    assert(fade_pixels[0]==0xff000000u&&fade_pixels[1]==0xff000000u);
    assert(ticks==16u*20u&&render_present_calls==16u);

    /* The real level-start cue is the paired sound-52 dispatch produced by
       the level-entry protection timer.  It begins only at active play. */
    FloodGame banner_game;FloodAudio banner_audio,expected_start_audio;
    flood_game_init(&banner_game);
    assert(flood_game_load_level(&banner_game,FLOOD_DATA_DIR,1u,
        settings_err,sizeof(settings_err)));
    assert(banner_game.player.invulnerable_timer==100u);
    assert(flood_audio_load(&banner_audio,FLOOD_DATA_DIR,44100u,
        settings_err,sizeof(settings_err)));
    assert(flood_audio_load(&expected_start_audio,FLOOD_DATA_DIR,44100u,
        settings_err,sizeof(settings_err)));
    flood_audio_trigger(&expected_start_audio,52u);
    uint32_t *banner_fb=malloc((size_t)VIEW_W*VIEW_H*sizeof(*banner_fb));
    uint8_t *banner_world=malloc(FLOOD_BACKING_PIXELS);
    assert(banner_fb&&banner_world);
    /* The brown variant keeps Quiffy's source goggle pixels and colors every
       Matilda eye frame red, including the mirrored facing direction. */
    memset(banner_world,0,FLOOD_BACKING_PIXELS);
    composite_sprite_variant(banner_world,&banner_game,0x50u,40,40,0,0,true,false);
    const FloodSprite *quiffy=flood_sprite(&banner_game,0x50u);
    for(unsigned y=0;y<quiffy->height;y++)for(unsigned x=0;x<quiffy->width;x++){
        const unsigned at=y*FLOOD_MAX_SPRITE_W+x;
        if(!quiffy->mask[at])continue;
        const uint8_t original=quiffy->pixels[at];
        const uint8_t painted=banner_world[(40u+y)*FLOOD_BACKING_W+FLOOD_BACKING_X+40u+x];
        if(original==3u||original==15u)assert(painted==original);
        if(original==8u)assert(painted==4u);
    }
    for(unsigned sprite=0xd2u;sprite<=0xd9u;sprite++){
        memset(banner_world,0,FLOOD_BACKING_PIXELS);
        composite_sprite_variant(banner_world,&banner_game,sprite,40,40,0,0,true,true);
        const unsigned eye_x=sprite<=0xd5u?12u:17u;
        assert(banner_world[53u*FLOOD_BACKING_W+FLOOD_BACKING_X+40u+eye_x]==16u);
        assert(banner_world[53u*FLOOD_BACKING_W+FLOOD_BACKING_X+42u+eye_x]==16u);
    }
    FloodGame *other_actor=calloc(1,sizeof(*other_actor));
    uint32_t *hidden_frame=malloc(FLOOD_VIEW_PIXELS*sizeof(*hidden_frame));
    assert(other_actor&&hidden_frame);
    other_actor->player.x=160;other_actor->player.y=80;
    other_actor->player.invulnerable_timer=99u;
    other_actor->player_blink_active=true;
    banner_game.render_buffer_index=1u;
    SDL_Rect panel={0,0,VIEW_W,VIEW_H};
    render_world_panel((SDL_Renderer *)(uintptr_t)1u,
        (SDL_Texture *)(uintptr_t)1u,&banner_game,banner_fb,banner_world,
        other_actor,0u,&panel,NULL);
    memcpy(hidden_frame,banner_fb,FLOOD_VIEW_PIXELS*sizeof(*banner_fb));
    other_actor->lives=3;
    render_world_panel((SDL_Renderer *)(uintptr_t)1u,
        (SDL_Texture *)(uintptr_t)1u,&banner_game,banner_fb,banner_world,
        other_actor,0u,&panel,NULL);
    assert(!memcmp(hidden_frame,banner_fb,FLOOD_VIEW_PIXELS*sizeof(*banner_fb)));
    other_actor->player.balloon_timer=10u;
    render_world_panel((SDL_Renderer *)(uintptr_t)1u,
        (SDL_Texture *)(uintptr_t)1u,&banner_game,banner_fb,banner_world,
        other_actor,0u,&panel,NULL);
    assert(memcmp(hidden_frame,banner_fb,FLOOD_VIEW_PIXELS*sizeof(*banner_fb))!=0);
    other_actor->player.balloon_timer=0u;
    other_actor->player.parachute_timer=10u;
    render_world_panel((SDL_Renderer *)(uintptr_t)1u,
        (SDL_Texture *)(uintptr_t)1u,&banner_game,banner_fb,banner_world,
        other_actor,1u,&panel,NULL);
    assert(memcmp(hidden_frame,banner_fb,FLOOD_VIEW_PIXELS*sizeof(*banner_fb))!=0);
    other_actor->player.parachute_timer=0u;
    other_actor->ouch_visible=true;
    render_world_panel((SDL_Renderer *)(uintptr_t)1u,
        (SDL_Texture *)(uintptr_t)1u,&banner_game,banner_fb,banner_world,
        other_actor,0u,&panel,NULL);
    assert(memcmp(hidden_frame,banner_fb,FLOOD_VIEW_PIXELS*sizeof(*banner_fb))!=0);
    other_actor->ouch_visible=false;
    banner_game.render_buffer_index=0u;
    render_world_panel((SDL_Renderer *)(uintptr_t)1u,
        (SDL_Texture *)(uintptr_t)1u,&banner_game,banner_fb,banner_world,
        other_actor,0u,&panel,NULL);
    memcpy(hidden_frame,banner_fb,FLOOD_VIEW_PIXELS*sizeof(*banner_fb));
    other_actor->lives=0;
    render_world_panel((SDL_Renderer *)(uintptr_t)1u,
        (SDL_Texture *)(uintptr_t)1u,&banner_game,banner_fb,banner_world,
        other_actor,0u,&panel,NULL);
    assert(memcmp(hidden_frame,banner_fb,FLOOD_VIEW_PIXELS*sizeof(*banner_fb))!=0);
    other_actor->lives=3;
    other_actor->matilda.visible=true;
    other_actor->matilda.x=220;other_actor->matilda.y=80;
    other_actor->matilda.sprite_id=0xd2u;
    render_world_panel((SDL_Renderer *)(uintptr_t)1u,
        (SDL_Texture *)(uintptr_t)1u,&banner_game,banner_fb,banner_world,
        other_actor,0u,&panel,NULL);
    unsigned ghost_pixels=0u;
    for(unsigned y=80u;y<96u;y++)for(unsigned x=220u;x<236u;x++)
        if(banner_fb[y*VIEW_W+x]!=hidden_frame[y*VIEW_W+x])ghost_pixels++;
    assert(ghost_pixels>0u);
    other_actor->lives=0;
    render_world_panel((SDL_Renderer *)(uintptr_t)1u,
        (SDL_Texture *)(uintptr_t)1u,&banner_game,banner_fb,banner_world,
        other_actor,0u,&panel,NULL);
    for(unsigned y=80u;y<96u;y++)for(unsigned x=220u;x<236u;x++)
        assert(banner_fb[y*VIEW_W+x]==hidden_frame[y*VIEW_W+x]);
    other_actor->matilda.visible=false;other_actor->lives=3;
    other_actor->player.invulnerable_timer=0u;
    other_actor->player_blink_active=false;
    other_actor->player.death_mode=2u;other_actor->player.death_phase=3u;
    render_world_panel((SDL_Renderer *)(uintptr_t)1u,
        (SDL_Texture *)(uintptr_t)1u,&banner_game,banner_fb,banner_world,
        other_actor,0u,&panel,NULL);
    const FloodSprite *death_cross=flood_sprite(&banner_game,0x7bu);
    unsigned visible_cross_pixels=0u;
    for(unsigned y=0;y<death_cross->height;y++)for(unsigned x=0;x<death_cross->width;x++){
        const unsigned at=y*FLOOD_MAX_SPRITE_W+x;
        if(!death_cross->mask[at])continue;
        assert(banner_fb[(80u+y)*VIEW_W+160u+x]==palette[death_cross->pixels[at]&15u]);
        visible_cross_pixels++;
    }
    assert(visible_cross_pixels>20u);
    FloodGame *second_view=malloc(sizeof(*second_view));assert(second_view);
    *second_view=banner_game;second_view->player=other_actor->player;
    second_view->player.death_mode=0u;
    second_view->player.invulnerable_timer=99u;
    second_view->player_blink_active=true;
    second_view->render_buffer_index=1u;second_view->lives=3;
    render_world_panel((SDL_Renderer *)(uintptr_t)1u,
        (SDL_Texture *)(uintptr_t)1u,second_view,banner_fb,banner_world,
        NULL,1u,&panel,NULL);
    memcpy(hidden_frame,banner_fb,FLOOD_VIEW_PIXELS*sizeof(*banner_fb));
    second_view->player.x=1000;
    render_world_panel((SDL_Renderer *)(uintptr_t)1u,
        (SDL_Texture *)(uintptr_t)1u,second_view,banner_fb,banner_world,
        NULL,1u,&panel,NULL);
    assert(!memcmp(hidden_frame,banner_fb,FLOOD_VIEW_PIXELS*sizeof(*banner_fb)));
    second_view->player.x=160;second_view->render_buffer_index=0u;
    render_world_panel((SDL_Renderer *)(uintptr_t)1u,
        (SDL_Texture *)(uintptr_t)1u,second_view,banner_fb,banner_world,
        NULL,1u,&panel,NULL);
    memcpy(hidden_frame,banner_fb,FLOOD_VIEW_PIXELS*sizeof(*banner_fb));
    second_view->player.x=1000;
    render_world_panel((SDL_Renderer *)(uintptr_t)1u,
        (SDL_Texture *)(uintptr_t)1u,second_view,banner_fb,banner_world,
        NULL,1u,&panel,NULL);
    assert(memcmp(hidden_frame,banner_fb,FLOOD_VIEW_PIXELS*sizeof(*banner_fb))!=0);
    second_view->player=other_actor->player;
    second_view->player.invulnerable_timer=0u;
    second_view->player_blink_active=false;
    render_world_panel((SDL_Renderer *)(uintptr_t)1u,
        (SDL_Texture *)(uintptr_t)1u,second_view,banner_fb,banner_world,
        NULL,1u,&panel,NULL);
    for(unsigned y=0;y<death_cross->height;y++)for(unsigned x=0;x<death_cross->width;x++){
        const unsigned at=y*FLOOD_MAX_SPRITE_W+x;
        if(death_cross->mask[at])
            assert(banner_fb[(80u+y)*VIEW_W+160u+x]==palette[death_cross->pixels[at]&15u]);
    }
    free(second_view);free(hidden_frame);free(other_actor);
    uint8_t game_over_font[SETTINGS_FONT_BYTES];
    assert(settings_load_font(game_over_font,settings_err,sizeof(settings_err)));
    memset(banner_fb,0,FLOOD_VIEW_PIXELS*sizeof(*banner_fb));
    mp_draw_game_over(banner_fb,game_over_font,1,0u);
    for(unsigned n=0;n<FLOOD_VIEW_PIXELS;n++)assert(banner_fb[n]==0u);
    mp_draw_game_over(banner_fb,game_over_font,0,0u);
    unsigned game_over_pixels=0u;
    for(unsigned y=104u;y<112u;y++)for(unsigned x=124u;x<196u;x++)
        if(banner_fb[y*VIEW_W+x]==0xffffffffu)game_over_pixels++;
    assert(game_over_pixels>30u);
    memset(banner_fb,0,FLOOD_VIEW_PIXELS*sizeof(*banner_fb));
    mp_draw_game_over(banner_fb,game_over_font,0,2u);
    for(unsigned n=0;n<FLOOD_VIEW_PIXELS;n++)assert(banner_fb[n]==0u);
    ticks=0u;reset_input_state();audio_unlock_count=0u;
    observed_audio=&banner_audio;first_channel_one_active_tick=UINT32_MAX;
    assert(!play_level_banner(window,(SDL_Renderer *)(uintptr_t)1u,
        (SDL_Texture *)(uintptr_t)1u,&banner_game,&banner_audio,1u,
        banner_fb,banner_world));
    assert(ticks==FLOOD_LEVEL_BANNER_FRAMES*GAME_FRAME_MS);
    assert(audio_unlock_count==FLOOD_LEVEL_BANNER_FRAMES+1u);
    assert(audio_unlock_ticks[0]==0u&&audio_unlock_ticks[audio_unlock_count-1u]==ticks);
    assert(first_channel_one_active_tick==ticks);
    assert(banner_audio.channel[0].active&&banner_audio.channel[1].active);
    assert(banner_audio.channel[0].track_pos==expected_start_audio.channel[0].track_pos);
    assert(banner_audio.channel[1].track_pos==expected_start_audio.channel[1].track_pos);
    observed_audio=NULL;
    /* A second multiplayer banner must explicitly restart the Let's Go cue
       after the prior effect channels have become idle. */
    FloodMultiplayer banner_multi={.game=&banner_game,
        .second=calloc(1,sizeof(FloodGame))};
    FloodGame *banner_view=malloc(sizeof(FloodGame));
    uint32_t *banner_pairs[2]={malloc(FLOOD_VIEW_PIXELS*sizeof(uint32_t)),
        malloc(FLOOD_VIEW_PIXELS*sizeof(uint32_t))};
    assert(banner_multi.second&&banner_view&&banner_pairs[0]&&banner_pairs[1]);
    SDL_Texture *banner_textures[2]={
        (SDL_Texture *)(uintptr_t)1u,(SDL_Texture *)(uintptr_t)1u};
    SDL_Rect banner_plays[2]={{0,48,318,265},{321,48,318,265}};
    SDL_Rect banner_huds[2]={{0,37,318,11},{321,37,318,11}};
    SDL_Rect banner_source={40,8,240,200},banner_hud_source={0,0,320,8};
    SDL_Rect banner_divider={319,0,2,360};
    ReFloodSettings banner_settings=settings_defaults();
    assert(flood_game_load_level(banner_multi.game,FLOOD_DATA_DIR,1u,
        settings_err,sizeof(settings_err)));
    flood_multiplayer_begin(&banner_multi);
    assert(flood_audio_load(&banner_audio,FLOOD_DATA_DIR,44100u,
        settings_err,sizeof(settings_err)));
    ticks=0u;reset_input_state();
    assert(!mp_play_level_banner(window,(SDL_Renderer *)(uintptr_t)1u,
        &banner_settings,&banner_multi,banner_view,banner_textures,banner_fb,
        banner_pairs,banner_world,banner_plays,banner_huds,&banner_source,
        &banner_hud_source,&banner_divider,&banner_audio,1u));
    assert(ticks==FLOOD_LEVEL_BANNER_FRAMES*GAME_FRAME_MS+24u);
    assert(banner_audio.channel[0].active&&banner_audio.channel[1].active);
    banner_audio.channel[0].active=banner_audio.channel[1].active=false;
    banner_multi.game->world.remaining_food=-1;
    assert(flood_game_advance_level(banner_multi.game,FLOOD_DATA_DIR,
        settings_err,sizeof(settings_err)));
    flood_multiplayer_reset_second(&banner_multi);
    assert(!mp_play_level_banner(window,(SDL_Renderer *)(uintptr_t)1u,
        &banner_settings,&banner_multi,banner_view,banner_textures,banner_fb,
        banner_pairs,banner_world,banner_plays,banner_huds,&banner_source,
        &banner_hud_source,&banner_divider,&banner_audio,1u));
    assert(ticks==2u*(FLOOD_LEVEL_BANNER_FRAMES*GAME_FRAME_MS+24u));
    assert(banner_audio.channel[0].active&&banner_audio.channel[1].active);
    assert(banner_multi.game->player.invulnerable_timer==99u);
    assert(banner_multi.second->player.invulnerable_timer==99u);
    free(banner_pairs[0]);free(banner_pairs[1]);
    free(banner_view);free(banner_multi.second);
    free(banner_world);free(banner_fb);

    uint8_t settings_font[SETTINGS_FONT_BYTES];
    assert(settings_load_font(settings_font,settings_err,sizeof(settings_err)));
    uint32_t *settings_fb=malloc((size_t)VIEW_W*VIEW_H*sizeof(*settings_fb));
    assert(settings_fb);settings=settings_defaults();
    render_settings((SDL_Renderer *)(uintptr_t)1u,(SDL_Texture *)(uintptr_t)1u,
        settings_fb,settings_font,&settings);
    size_t white_pixels=0u;
    for(size_t n=0;n<(size_t)VIEW_W*VIEW_H;n++){
        assert(settings_fb[n]==0xff000000u||settings_fb[n]==0xffffffffu);
        if(settings_fb[n]==0xffffffffu)white_pixels++;
    }
    assert(white_pixels>0u);free(settings_fb);

    uint32_t *colon_fb=malloc((size_t)VIEW_W*VIEW_H*sizeof(*colon_fb));assert(colon_fb);
    for(size_t n=0;n<(size_t)VIEW_W*VIEW_H;n++)colon_fb[n]=0xff000000u;
    settings_draw_text(colon_fb,settings_font,0,0,":");white_pixels=0u;
    for(unsigned y=0;y<8u;y++)for(unsigned x=0;x<8u;x++)
        if(colon_fb[(size_t)y*VIEW_W+x]==0xffffffffu)white_pixels++;
    assert(white_pixels==8u);free(colon_fb);

    const uint32_t scale_source[9]={0u,1u,0u,1u,2u,0u,0u,0u,0u};
    const uint32_t scale_expected[36]={
        0u,0u,1u,1u,0u,0u, 0u,1u,1u,1u,0u,0u,
        1u,1u,1u,2u,0u,0u, 1u,1u,2u,0u,0u,0u,
        0u,0u,0u,0u,0u,0u, 0u,0u,0u,0u,0u,0u};
    uint32_t scale_dest[36];scale2x_frame(scale_source,3u,3u,scale_dest);
    assert(!memcmp(scale_dest,scale_expected,sizeof(scale_dest)));
    assert(scale_dest[14]==1u&&scale_dest[15]==2u&&
        scale_dest[20]==2u&&scale_dest[21]==0u);
    active_scaler=REFLOOD_SCALER_LINEAR;texture_scale_calls=0u;
    SDL_Texture *scale_texture=create_stage_texture((SDL_Renderer *)(uintptr_t)1u,320u,208u);
    assert(scale_texture&&texture_width==320u&&texture_height==208u);
    assert(texture_scale_calls==1u&&texture_scale_mode==SDL_ScaleModeLinear);
    active_scaler=REFLOOD_SCALER_SCALE2X;
    scale_texture=create_stage_texture((SDL_Renderer *)(uintptr_t)1u,320u,208u);
    assert(scale_texture&&texture_width==640u&&texture_height==416u);
    assert(texture_scale_mode==SDL_ScaleModeNearest);
    stage_size(window,(SDL_Renderer *)(uintptr_t)1u,320,208);
    assert(logical_width==640&&logical_height==416);
    assert(update_stage_texture(scale_texture,scale_source,3u,3u)==0);
    assert(texture_update_pitch==24);active_scaler=REFLOOD_SCALER_NEAREST;
    stage_size(window,(SDL_Renderer *)(uintptr_t)1u,320,208);
    assert(logical_width==320&&logical_height==208);

    PostScoreAttract attract=post_score_attract_begin();
    assert(attract.show_scores&&!attract.fire_released&&attract.screen_frames==0u);
    assert(!post_score_attract_fire(&attract,false));
    assert(post_score_attract_fire(&attract,true)&&attract.show_scores);
    attract=post_score_attract_begin();
    for(unsigned n=0;n<POST_SCORE_SCREEN_FRAMES;n++){
        assert(!post_score_attract_fire(&attract,false));
        post_score_attract_advance(&attract);
    }
    assert(!attract.show_scores&&attract.screen_frames==0u);
    assert(post_score_attract_fire(&attract,true)&&!attract.show_scores);
    for(unsigned n=0;n<POST_SCORE_SCREEN_FRAMES;n++)post_score_attract_advance(&attract);
    assert(attract.show_scores&&attract.screen_frames==0u);

    uint32_t *attract_fb=malloc(presentation_framebuffer_pixels()*sizeof(*attract_fb));
    uint8_t attract_background[4*FLOOD_LEVEL_SELECT_PLANE_BYTES];
    SDL_Texture *attract_texture=(SDL_Texture *)(uintptr_t)1u;
    char attract_err[160];bool attract_quit=false,selector_over_scores=false;
    assert(attract_fb);ticks=0u;reset_input_state();scheduled_fire_tick=20u;
    assert(run_post_score_loop(window,(SDL_Renderer *)(uintptr_t)1u,
        &attract_texture,attract_fb,attract_background,0u,attract_err,
        sizeof(attract_err),&attract_quit,&selector_over_scores));
    assert(!attract_quit&&selector_over_scores);
    ticks=0u;reset_input_state();scheduled_fire_tick=
        POST_SCORE_SCREEN_FRAMES*PRESENTATION_FRAME_MS;
    selector_over_scores=true;attract_texture=(SDL_Texture *)(uintptr_t)1u;
    assert(run_post_score_loop(window,(SDL_Renderer *)(uintptr_t)1u,
        &attract_texture,attract_fb,attract_background,0u,attract_err,
        sizeof(attract_err),&attract_quit,&selector_over_scores));
    assert(!attract_quit&&!selector_over_scores);free(attract_fb);
    fullscreen=false;fullscreen_flags=0u;fullscreen_calls=0u;
    window_size_calls=0u;window_position_calls=0u;
    render_clear_calls=0u;render_present_calls=0u;

    FloodGame reset_game;memset(&reset_game,0,sizeof(reset_game));
    reset_game.score=54321u;reset_game.lives=0;reset_game.running=false;
    unsigned reset_level=17u;reset_after_game_over(&reset_game,&reset_level);
    assert(reset_level==1u&&reset_game.score==0u&&reset_game.lives==3&&reset_game.running);
    assert(protection_required(false));
    assert(!protection_required(true));

    reset_controller_state();saved_player1_choice=0u;
    reset_input_state();keys[SDL_SCANCODE_RCTRL]=1u;
    in=input(window,(SDL_Renderer *)(uintptr_t)1u,false,false);assert(in.fire);

    reset_input_state();keys[SDL_SCANCODE_SPACE]=1u;
    in=input(window,(SDL_Renderer *)(uintptr_t)1u,false,false);assert(!in.fire);

    saved_player1_choice=1u;
    reset_input_state();keys[SDL_SCANCODE_D]=1u;keys[SDL_SCANCODE_LCTRL]=1u;
    in=input(window,(SDL_Renderer *)(uintptr_t)1u,false,false);
    assert(in.x==1&&in.fire);
    saved_player1_choice=0u;

    reset_input_state();keys[SDL_SCANCODE_ESCAPE]=1u;
    in=input(window,(SDL_Renderer *)(uintptr_t)1u,false,false);
    assert(in.restart&&!in.quit);

    reset_input_state();keys[SDL_SCANCODE_R]=1u;
    in=input(window,(SDL_Renderer *)(uintptr_t)1u,false,false);
    assert(!in.restart&&!in.restart_level);

    reset_input_state();queue_key(SDLK_p,0u);
    in=input(window,(SDL_Renderer *)(uintptr_t)1u,false,false);
    assert(in.pause&&!in.restart_level);
    reset_input_state();queue_key(SDLK_r,0u);
    in=input(window,(SDL_Renderer *)(uintptr_t)1u,false,false);
    assert(in.restart_level&&!in.pause);
    reset_input_state();queue_key(SDLK_p,1u);queue_key(SDLK_r,1u);
    in=input(window,(SDL_Renderer *)(uintptr_t)1u,false,false);
    assert(!in.pause&&!in.restart_level);

    ticks=0u;reset_input_state();scheduled_fire_tick=40u;
    scheduled_fire_release_tick=80u;
    assert(!pause_until_fire(window,(SDL_Renderer *)(uintptr_t)1u,false));
    assert(ticks==80u);
    ticks=0u;reset_input_state();scheduled_fire_tick=0u;
    scheduled_fire_release_tick=40u;
    assert(!pause_until_fire(window,(SDL_Renderer *)(uintptr_t)1u,true));
    assert(ticks==40u);
    ReFloodSettings pause_settings=settings_defaults();
    ticks=0u;reset_input_state();scheduled_second_fire_tick=40u;
    scheduled_second_fire_release_tick=80u;
    assert(!mp_pause_until_fire(window,(SDL_Renderer *)(uintptr_t)1u,
        &pause_settings,false));
    assert(ticks==80u);
    ticks=0u;reset_input_state();scheduled_second_fire_tick=0u;
    scheduled_second_fire_release_tick=40u;
    assert(!mp_pause_until_fire(window,(SDL_Renderer *)(uintptr_t)1u,
        &pause_settings,true));
    assert(ticks==40u);

    /* Gameplay obeys the saved device choice. The settings screen can still
       be operated by either device so a user cannot lock themselves out. */
    controller_joystick_count=1;find_game_controller();
    assert(game_controller&&controller_open_calls==1u&&controller_attached);
    controller_buttons[SDL_CONTROLLER_BUTTON_DPAD_RIGHT]=1u;
    controller_buttons[SDL_CONTROLLER_BUTTON_A]=1u;
    controller_buttons[SDL_CONTROLLER_BUTTON_BACK]=1u;
    reset_input_state();keys[SDL_SCANCODE_LEFT]=1u;
    in=input(window,(SDL_Renderer *)(uintptr_t)1u,false,false);
    assert(in.x==-1&&!in.fire&&!in.restart&&!in.pause&&!in.restart_level);

    saved_player1_choice=2u;
    reset_input_state();keys[SDL_SCANCODE_LEFT]=1u;
    keys[SDL_SCANCODE_RCTRL]=1u;keys[SDL_SCANCODE_ESCAPE]=1u;
    in=input(window,(SDL_Renderer *)(uintptr_t)1u,false,false);
    assert(in.x==1&&in.y==0&&in.fire&&in.restart);
    reset_input_state();queue_key(SDLK_p,0u);queue_key(SDLK_r,0u);
    in=input(window,(SDL_Renderer *)(uintptr_t)1u,false,false);
    assert(!in.pause&&!in.restart_level);
    memset(controller_buttons,0,sizeof(controller_buttons));
    controller_axes[SDL_CONTROLLER_AXIS_LEFTX]=-20000;
    controller_axes[SDL_CONTROLLER_AXIS_LEFTY]=20001;
    reset_input_state();
    in=input(window,(SDL_Renderer *)(uintptr_t)1u,false,false);
    assert(in.x==-1&&in.y==1&&!in.fire&&!in.restart);
    controller_axes[SDL_CONTROLLER_AXIS_LEFTX]=1000;
    controller_axes[SDL_CONTROLLER_AXIS_LEFTY]=-1000;
    in=input(window,(SDL_Renderer *)(uintptr_t)1u,false,false);
    assert(in.x==0&&in.y==0);

    close_game_controller();controller_joystick_count=2;
    saved_player1_choice=3u;find_game_controller();
    assert(controller_last_open_index==1);

    saved_player1_choice=0u;
    controller_buttons[SDL_CONTROLLER_BUTTON_DPAD_UP]=1u;
    controller_buttons[SDL_CONTROLLER_BUTTON_A]=1u;
    reset_input_state();
    in=input(window,(SDL_Renderer *)(uintptr_t)1u,false,true);
    assert(in.y==-1&&in.fire);
    memset(controller_buttons,0,sizeof(controller_buttons));
    reset_input_state();keys[SDL_SCANCODE_RIGHT]=1u;
    in=input(window,(SDL_Renderer *)(uintptr_t)1u,false,true);
    assert(in.x==1&&!in.fire);

    close_game_controller();reset_input_state();
    assert(!game_controller&&controller_close_calls==2u);
    queue_controller_event(SDL_CONTROLLERDEVICEADDED,0);
    input(window,(SDL_Renderer *)(uintptr_t)1u,false,false);
    assert(game_controller&&controller_open_calls==3u);
    controller_joystick_count=0;reset_input_state();
    queue_controller_event(SDL_CONTROLLERDEVICEREMOVED,controller_instance);
    input(window,(SDL_Renderer *)(uintptr_t)1u,false,false);
    assert(!game_controller&&controller_close_calls==3u);

    reset_input_state();queue_key(SDLK_f,0u);
    in=input(window,(SDL_Renderer *)(uintptr_t)1u,false,false);assert(!in.key);
    assert(fullscreen_calls==1u&&fullscreen_flags==SDL_WINDOW_FULLSCREEN_DESKTOP);
    assert(render_clear_calls==2u&&render_present_calls==2u);
    assert(!file_exists(REFLOOD_CONFIG_PATH));

    reset_input_state();queue_key(SDLK_f,1u);
    input(window,(SDL_Renderer *)(uintptr_t)1u,false,false);assert(fullscreen_calls==1u);

    reset_input_state();queue_key(SDLK_f,0u);
    input(window,(SDL_Renderer *)(uintptr_t)1u,true,false);assert(fullscreen_calls==1u);

    reset_input_state();queue_key(SDLK_f,0u);
    input(window,(SDL_Renderer *)(uintptr_t)1u,false,false);
    assert(fullscreen_calls==2u&&fullscreen_flags==0u);
    assert(window_size_calls==1u&&window_position_calls==1u);

    reset_input_state();queue_key(SDLK_f,0u);
    in=input(window,(SDL_Renderer *)(uintptr_t)1u,true,false);assert(in.key=='F'&&fullscreen_calls==2u);

    reset_input_state();queue_key(SDLK_f,0u);input(window,(SDL_Renderer *)(uintptr_t)1u,false,false);
    assert(fullscreen_calls==3u&&fullscreen_flags==SDL_WINDOW_FULLSCREEN_DESKTOP);
    stage_size(window,(SDL_Renderer *)(uintptr_t)1u,160,96);
    assert(fullscreen_calls==3u&&window_size_calls==1u);
    reset_input_state();queue_key(SDLK_f,0u);input(window,(SDL_Renderer *)(uintptr_t)1u,false,false);
    assert(fullscreen_calls==4u&&fullscreen_flags==0u);
    assert(window_size_calls==2u&&window_position_calls==2u);
    stage_size(window,(SDL_Renderer *)(uintptr_t)1u,320,208);
    assert(window_size_calls==3u);

    /* The actual SDL flag, not a stale cached boolean, chooses the direction. */
    fullscreen=false;fullscreen_flags=SDL_WINDOW_FULLSCREEN_DESKTOP;
    reset_input_state();queue_key(SDLK_f,0u);input(window,(SDL_Renderer *)(uintptr_t)1u,false,false);
    assert(fullscreen_calls==5u&&fullscreen_flags==0u&&!fullscreen);

    /* Switching display mode clears both renderer buffers; the title wait
       loop must immediately present its static image again. */
    FloodTitle title;char err[160];
    assert(flood_title_load(&title,FLOOD_DATA_DIR,err,sizeof(err)));
    uint32_t *title_fb=malloc(presentation_framebuffer_pixels()*sizeof(*title_fb));assert(title_fb);
    const unsigned title_present_before=render_present_calls;
    render_title((SDL_Renderer *)(uintptr_t)1u,(SDL_Texture *)(uintptr_t)1u,&title,title_fb);
    assert(render_present_calls==title_present_before+1u);free(title_fb);

    FloodIntro intro;
    assert(flood_intro_load(&intro,FLOOD_DATA_DIR,err,sizeof(err)));
    uint32_t *presentation_fb=malloc(presentation_framebuffer_pixels()*sizeof(*presentation_fb));
    assert(presentation_fb);
    render_intro((SDL_Renderer *)(uintptr_t)1u,(SDL_Texture *)(uintptr_t)1u,
        &intro,presentation_fb);
    free(presentation_fb);

    uint32_t *fb=malloc(VIEW_W*VIEW_H*sizeof(*fb));assert(fb);
    ticks=0u;reset_input_state();
    assert(!intro_music_lead(window,(SDL_Renderer *)(uintptr_t)1u,
        (SDL_Texture *)(uintptr_t)1u,fb));
    assert(ticks==INTRO_MUSIC_LEAD_MS);free(fb);

    /* Exercise the complete entry banner, then one gameplay render. */
    reset_input_state();ticks=0u;
    scheduled_quit_tick=FLOOD_LEVEL_BANNER_FRAMES*GAME_FRAME_MS+game_frame_ms(saved_game_speed);
    ReFloodSettings multiplayer_settings=settings_defaults();
    const unsigned before_multiplayer=render_present_calls;
    const unsigned before_center=window_position_calls;
    uint32_t player_scores[2];bool both_dead=false,completed=false;
    assert(mp_run_gameplay(window,(SDL_Renderer *)(uintptr_t)1u,
        &multiplayer_settings,1u,0u,player_scores,&both_dead,&completed,NULL)==0);
    assert(!both_dead&&!completed);
    assert(render_present_calls>=before_multiplayer+FLOOD_LEVEL_BANNER_FRAMES);
    assert(logical_width==640*(int)scaler_output_factor());
    assert(window_position_calls>before_center);
    scheduled_quit_tick=UINT32_MAX;

    /* The split-screen host also accepts a selected header, independent of
       the original campaign's selected level. */
    ticks=0u;reset_input_state();
    scheduled_quit_tick=FLOOD_LEVEL_BANNER_FRAMES*GAME_FRAME_MS+
        game_frame_ms(saved_game_speed);
    assert(snprintf(selected_header,sizeof(selected_header),
        "%s/levels/level_01_header.bin",FLOOD_DATA_DIR)<(int)sizeof(selected_header));
    assert(mp_run_gameplay(window,(SDL_Renderer *)(uintptr_t)1u,
        &multiplayer_settings,42u,0u,player_scores,&both_dead,&completed,
        selected_header)==0);
    assert(!both_dead&&!completed);
    scheduled_quit_tick=UINT32_MAX;

    /* Esc is an in-game death sequence, not an immediate host exit. */
    ticks=0u;reset_input_state();
    const Uint32 esc_at=FLOOD_LEVEL_BANNER_FRAMES*GAME_FRAME_MS+60u;
    scheduled_escape_tick=esc_at;
    assert(mp_run_gameplay(window,(SDL_Renderer *)(uintptr_t)1u,
        &multiplayer_settings,1u,0u,player_scores,&both_dead,&completed,NULL)==0);
    assert(both_dead&&!completed);
    assert(ticks>esc_at+GAME_FRAME_MS);
    scheduled_escape_tick=UINT32_MAX;

    /* The multiplayer front end begins on the original single-window stage
       and honors Quit during the same intro-music lead as single-player. */
    uint32_t *front_fb=malloc(presentation_framebuffer_pixels()*sizeof(*front_fb));
    assert(front_fb);
    SDL_Texture *front_texture=(SDL_Texture *)(uintptr_t)1u;
    uint8_t score_background[4*FLOOD_LEVEL_SELECT_PLANE_BYTES]={0};
    uint16_t front_rng=0u;int front_status=0;unsigned front_level=1u;
    ticks=0u;reset_input_state();scheduled_quit_tick=100u;
    assert(!mp_play_frontend(window,(SDL_Renderer *)(uintptr_t)1u,
        &front_texture,front_fb,&front_level,false,false,score_background,
        &front_rng,&multiplayer_settings,&front_status));
    assert(front_status==0&&ticks>=100u&&ticks<INTRO_MUSIC_LEAD_MS);
    assert(logical_width==VIEW_W*(int)scaler_output_factor());
    ticks=0u;reset_input_state();scheduled_quit_tick=40u;
    assert(!mp_play_frontend(window,(SDL_Renderer *)(uintptr_t)1u,
        &front_texture,front_fb,&front_level,true,false,score_background,
        &front_rng,&multiplayer_settings,&front_status));
    assert(front_status==0);
    assert(logical_width==FLOOD_LEVEL_SELECT_W*(int)scaler_output_factor());
    free(front_fb);

    /* The original 75-event ending uses a single full-width presentation
       texture, its 20 ms cadence, and its concluding 16-step fade. */
    FloodEnding reference_ending;
    assert(flood_ending_load(&reference_ending,FLOOD_DATA_DIR,err,sizeof(err)));
    unsigned ending_frames=0u;
    while(!reference_ending.finished&&ending_frames<20000u){
        flood_ending_tick(&reference_ending);ending_frames++;
    }
    assert(reference_ending.finished);
    uint32_t *ending_fb=malloc(presentation_framebuffer_pixels()*sizeof(*ending_fb));
    assert(ending_fb);
    ticks=0u;reset_input_state();scheduled_quit_tick=UINT32_MAX;
    bool ending_quit=false;int ending_status=0;
    assert(mp_play_ending(window,(SDL_Renderer *)(uintptr_t)1u,&front_texture,
        ending_fb,&multiplayer_settings,&ending_quit,&ending_status));
    assert(!ending_quit&&ending_status==0);
    assert(ticks==(ending_frames+16u)*PRESENTATION_FRAME_MS);
    assert(logical_width==FLOOD_ENDING_W*(int)scaler_output_factor());
    free(ending_fb);

    /* Both scores are inserted into the same original board in descending
       order; the second entry remains eligible after the first insertion. */
    FloodHighScores dual_scores;
    assert(flood_high_scores_load(&dual_scores,FLOOD_DATA_DIR,0u,err,sizeof(err)));
    flood_high_scores_enter(&dual_scores,999999u);
    assert(dual_scores.editing&&dual_scores.entry_index==0);
    dual_scores.editing=false;
    flood_high_scores_enter(&dual_scores,888888u);
    assert(dual_scores.editing&&dual_scores.entry_index==1);
    assert(dual_scores.scores[0]==999999u&&dual_scores.scores[1]==888888u);
    assert(multiplayer_settings.multiplayer_input[0]!=
        multiplayer_settings.multiplayer_input[1]);

    /* Drive each player's own fire key through the original eleven-character
       editor and check that the shared saved board contains both entries. */
    reset_input_state();ticks=0u;score_autofire=true;
    scheduled_quit_tick=1200u;
    uint32_t high_score_values[2]={999999u,888888u};bool score_quit=false;
    bool selector_scores=false;int score_status=0;
    uint32_t *score_fb=malloc(presentation_framebuffer_pixels()*sizeof(*score_fb));
    assert(score_fb);
    const bool return_to_selector=mp_run_post_scores(window,
        (SDL_Renderer *)(uintptr_t)1u,
        &front_texture,score_fb,score_background,high_score_values,
        &multiplayer_settings,&score_quit,&selector_scores,&score_status);
    assert((return_to_selector||score_quit)&&score_status==0);
    score_autofire=false;
    FloodHighScores saved_board;
    assert(flood_high_scores_load(&saved_board,FLOOD_DATA_DIR,0u,err,sizeof(err)));
    assert(high_scores_load_configuration(&saved_board,err,sizeof(err)));
    assert(saved_board.scores[0]==999999u&&saved_board.scores[1]==888888u);
    free(score_fb);

    remove(REFLOOD_CONFIG_PATH);remove(REFLOOD_CONFIG_TEMP_PATH);
    remove(REFLOOD_HISCORE_PATH);remove(REFLOOD_HISCORE_TEMP_PATH);
#ifdef _WIN32
    _rmdir(REFLOOD_CONFIG_DIR);
#else
    rmdir(REFLOOD_CONFIG_DIR);
#endif
    free(scaled_framebuffer);scaled_framebuffer=NULL;scaled_framebuffer_pixels=0u;

    return 0;
}

int SDL_Init(Uint32 flags){(void)flags;return 0;}
void SDL_Quit(void){}
int SDL_PollEvent(SDL_Event *event){
    if(scheduled_escape_tick!=UINT32_MAX&&ticks>=scheduled_escape_tick){
        scheduled_escape_tick=UINT32_MAX;memset(event,0,sizeof(*event));
        event->type=SDL_KEYDOWN;event->key.keysym.sym=SDLK_ESCAPE;return 1;
    }
    if(scheduled_quit_tick!=UINT32_MAX&&ticks>=scheduled_quit_tick){
        scheduled_quit_tick=UINT32_MAX;memset(event,0,sizeof(*event));
        event->type=SDL_QUIT;return 1;
    }
    if(queued_pos>=queued_count)return 0;
    *event=queued[queued_pos++];return 1;
}
const Uint8 *SDL_GetKeyboardState(int *count){
    if(count)*count=512;
    if(score_autofire){
        keys[SDL_SCANCODE_RCTRL]=(Uint8)((ticks/20u)%2u==0u);
        keys[SDL_SCANCODE_LCTRL]=keys[SDL_SCANCODE_RCTRL];
    }
    if(scheduled_fire_tick!=UINT32_MAX)
        keys[SDL_SCANCODE_RCTRL]=(Uint8)(ticks>=scheduled_fire_tick&&
            ticks<scheduled_fire_release_tick);
    if(scheduled_second_fire_tick!=UINT32_MAX)
        keys[SDL_SCANCODE_LCTRL]=(Uint8)(ticks>=scheduled_second_fire_tick&&
            ticks<scheduled_second_fire_release_tick);
    return keys;
}
int SDL_NumJoysticks(void){return controller_joystick_count;}
int SDL_IsGameController(int index){
    return controller_recognized&&index>=0&&index<controller_joystick_count;
}
SDL_GameController *SDL_GameControllerOpen(int index){
    if(!SDL_IsGameController(index))return NULL;
    controller_open_calls++;controller_attached=true;controller_last_open_index=index;
    return (SDL_GameController *)(uintptr_t)2u;
}
const char *SDL_GameControllerNameForIndex(int index){
    (void)index;return "Test Controller";
}
void SDL_GameControllerClose(SDL_GameController *controller){
    (void)controller;controller_close_calls++;controller_attached=false;
}
int SDL_GameControllerGetAttached(SDL_GameController *controller){
    (void)controller;return controller_attached;
}
Sint16 SDL_GameControllerGetAxis(SDL_GameController *controller,SDL_GameControllerAxis axis){
    (void)controller;
    return axis>=SDL_CONTROLLER_AXIS_LEFTX&&axis<=SDL_CONTROLLER_AXIS_LEFTY?
        controller_axes[axis]:0;
}
Uint8 SDL_GameControllerGetButton(SDL_GameController *controller,
    SDL_GameControllerButton button){
    (void)controller;
    return button>=SDL_CONTROLLER_BUTTON_A&&button<=SDL_CONTROLLER_BUTTON_DPAD_RIGHT?
        controller_buttons[button]:0u;
}
SDL_Joystick *SDL_GameControllerGetJoystick(SDL_GameController *controller){
    (void)controller;return (SDL_Joystick *)(uintptr_t)3u;
}
SDL_JoystickID SDL_JoystickInstanceID(SDL_Joystick *joystick){
    (void)joystick;return controller_instance;
}
SDL_Window *SDL_CreateWindow(const char *t,int x,int y,int w,int h,Uint32 f){
    (void)t;(void)x;(void)y;(void)w;(void)h;(void)f;return (SDL_Window *)(uintptr_t)1u;
}
int SDL_SetWindowFullscreen(SDL_Window *w,Uint32 flags){
    (void)w;fullscreen_calls++;fullscreen_flags=flags;return 0;
}
Uint32 SDL_GetWindowFlags(SDL_Window *w){(void)w;return fullscreen_flags;}
void SDL_SetWindowPosition(SDL_Window *w,int x,int y){
    (void)w;(void)x;(void)y;window_position_calls++;
}
SDL_Renderer *SDL_CreateRenderer(SDL_Window *w,int i,Uint32 f){
    (void)w;(void)i;(void)f;return (SDL_Renderer *)(uintptr_t)1u;
}
int SDL_RenderSetLogicalSize(SDL_Renderer *r,int w,int h){
    (void)r;logical_width=w;logical_height=h;return 0;
}
SDL_Texture *SDL_CreateTexture(SDL_Renderer *r,Uint32 f,int a,int w,int h){
    (void)r;(void)f;(void)a;texture_width=(unsigned)w;texture_height=(unsigned)h;
    return (SDL_Texture *)(uintptr_t)1u;
}
int SDL_SetTextureScaleMode(SDL_Texture *t,SDL_ScaleMode mode){
    (void)t;texture_scale_calls++;texture_scale_mode=mode;return 0;
}
void SDL_DestroyTexture(SDL_Texture *t){(void)t;}
void SDL_DestroyRenderer(SDL_Renderer *r){(void)r;}
void SDL_DestroyWindow(SDL_Window *w){(void)w;}
void SDL_SetWindowSize(SDL_Window *window,int w,int h){
    (void)window;(void)w;(void)h;window_size_calls++;
}
int SDL_UpdateTexture(SDL_Texture *t,const void *r,const void *p,int n){
    (void)t;(void)r;(void)p;texture_update_pitch=n;return 0;
}
int SDL_SetRenderDrawColor(SDL_Renderer *r,Uint8 red,Uint8 green,Uint8 blue,Uint8 alpha){
    (void)r;(void)red;(void)green;(void)blue;(void)alpha;return 0;
}
int SDL_RenderClear(SDL_Renderer *r){(void)r;render_clear_calls++;return 0;}
int SDL_RenderCopy(SDL_Renderer *r,SDL_Texture *t,const void *s,const void *d){
    (void)r;(void)t;(void)s;(void)d;return 0;
}
int SDL_RenderFillRect(SDL_Renderer *r,const SDL_Rect *rect){
    (void)r;(void)rect;return 0;
}
void SDL_RenderPresent(SDL_Renderer *r){(void)r;render_present_calls++;}
SDL_AudioDeviceID SDL_OpenAudioDevice(const char *d,int c,const SDL_AudioSpec *w,SDL_AudioSpec *o,int a){
    (void)d;(void)c;(void)w;(void)o;(void)a;return 0;
}
void SDL_PauseAudioDevice(SDL_AudioDeviceID d,int p){(void)d;(void)p;}
void SDL_LockAudioDevice(SDL_AudioDeviceID d){(void)d;}
void SDL_UnlockAudioDevice(SDL_AudioDeviceID d){
    (void)d;
    if(audio_unlock_count<sizeof(audio_unlock_ticks)/sizeof(audio_unlock_ticks[0]))
        audio_unlock_ticks[audio_unlock_count++]=ticks;
    if(observed_audio&&first_channel_one_active_tick==UINT32_MAX&&
        observed_audio->channel[1].active)
        first_channel_one_active_tick=ticks;
}
void SDL_CloseAudioDevice(SDL_AudioDeviceID d){(void)d;}
Uint32 SDL_GetTicks(void){return ticks;}
void SDL_Delay(Uint32 delay){ticks+=delay;}
