/* SDL host for the opt-in experimental mode. Included by main.c after its
   drawing/input helpers; the original presentation path stays unchanged. */
static SDL_GameController *mp_pads[2];

/* Overlay after the shared world is rendered. Only the exhausted player's
   camera receives the original eight-pixel Flood font in white. */
static void mp_draw_game_over(uint32_t *frame,const uint8_t font[SETTINGS_FONT_BYTES],
    int16_t lives,uint8_t death_mode){
    if(lives<=0&&!death_mode)settings_draw_centered(frame,font,104,"GAME OVER");
}

static void mp_close_pads(void){
    for(unsigned n=0;n<2u;n++){
        if(mp_pads[n])SDL_GameControllerClose(mp_pads[n]);
        mp_pads[n]=NULL;
    }
}
static void mp_find_pads(void){
    for(unsigned n=0;n<2u;n++)if(mp_pads[n]&&!SDL_GameControllerGetAttached(mp_pads[n])){
        SDL_GameControllerClose(mp_pads[n]);mp_pads[n]=NULL;
    }
    unsigned ordinal=0u;
    for(int device=0;device<SDL_NumJoysticks()&&ordinal<2u;device++)
        if(SDL_IsGameController(device)){
            if(!mp_pads[ordinal])mp_pads[ordinal]=SDL_GameControllerOpen(device);
            ordinal++;
        }
}
static FloodInput mp_device_input(unsigned choice,const Uint8 *keys){
    FloodInput result={0};
    if(choice==0u){
        result.x=(int8_t)((keys[SDL_SCANCODE_RIGHT]!=0)-(keys[SDL_SCANCODE_LEFT]!=0));
        result.y=(int8_t)((keys[SDL_SCANCODE_DOWN]!=0)-(keys[SDL_SCANCODE_UP]!=0));
        result.fire=keys[SDL_SCANCODE_RCTRL]!=0;
    }else if(choice==1u){
        result.x=(int8_t)((keys[SDL_SCANCODE_D]!=0)-(keys[SDL_SCANCODE_A]!=0));
        result.y=(int8_t)((keys[SDL_SCANCODE_S]!=0)-(keys[SDL_SCANCODE_W]!=0));
        result.fire=keys[SDL_SCANCODE_LCTRL]!=0;
    }else if(choice<4u&&mp_pads[choice-2u]){
        SDL_GameController *pad=mp_pads[choice-2u];
        const Sint16 x=SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_LEFTX);
        const Sint16 y=SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_LEFTY);
        result.x=(int8_t)((SDL_GameControllerGetButton(pad,SDL_CONTROLLER_BUTTON_DPAD_RIGHT)||x>16000)-
            (SDL_GameControllerGetButton(pad,SDL_CONTROLLER_BUTTON_DPAD_LEFT)||x<-16000));
        result.y=(int8_t)((SDL_GameControllerGetButton(pad,SDL_CONTROLLER_BUTTON_DPAD_DOWN)||y>16000)-
            (SDL_GameControllerGetButton(pad,SDL_CONTROLLER_BUTTON_DPAD_UP)||y<-16000));
        result.fire=SDL_GameControllerGetButton(pad,SDL_CONTROLLER_BUTTON_A)!=0;
    }
    return result;
}

static FloodInput mp_front_input(SDL_Window *window,SDL_Renderer *renderer,
    bool text_entry,const ReFloodSettings *settings){
    FloodInput i=input(window,renderer,text_entry,false);
    mp_find_pads();
    const FloodInput actor=mp_device_input(settings->multiplayer_input[0],
        SDL_GetKeyboardState(NULL));
    i.x=actor.x;i.y=actor.y;i.fire=actor.fire;
    return i;
}

static bool mp_pause_until_fire(SDL_Window *window,SDL_Renderer *renderer,
    const ReFloodSettings *settings,bool initial_fire){
    bool fire_seen=initial_fire;Uint32 next=SDL_GetTicks();
    for(;;){
        FloodInput modal=input(window,renderer,false,false);
        if(modal.quit)return true;
        mp_find_pads();const Uint8 *keys=SDL_GetKeyboardState(NULL);
        const bool fire=mp_device_input(settings->multiplayer_input[0],keys).fire||
            mp_device_input(settings->multiplayer_input[1],keys).fire;
        if(!fire_seen){if(fire)fire_seen=true;}
        else if(!fire)return false;
        next+=PRESENTATION_FRAME_MS;const Uint32 now=SDL_GetTicks();
        if((Sint32)(next-now)>0)SDL_Delay(next-now);else next=now;
    }
}

/* The same full-width presentation stages used by single-player run before
   the split stage is installed. After scores, only the level selector repeats. */
static bool mp_play_frontend(SDL_Window *window,SDL_Renderer *renderer,
    SDL_Texture **texture,uint32_t *fb,unsigned *level,bool direct_select,
    bool over_scores,const uint8_t *score_background,uint16_t *rng_state,
    const ReFloodSettings *settings,int *status){
    char err[160]={0};bool quit=false;
    SDL_DestroyTexture(*texture);
    *texture=create_stage_texture(renderer,VIEW_W,VIEW_H);
    stage_size(window,renderer,VIEW_W,VIEW_H);
    if(!*texture){*status=3;return false;}
    if(!direct_select){
        FloodAudio music;SDL_AudioDeviceID music_device=0;
        SDL_AudioSpec wanted={0},obtained={0};
        wanted.freq=44100;wanted.format=AUDIO_S16SYS;wanted.channels=2;
        wanted.samples=1024;wanted.callback=audio_callback;wanted.userdata=&music;
        if(flood_music_load(&music,FLOOD_DATA_DIR,(uint32_t)wanted.freq,err,sizeof(err))){
            music_device=SDL_OpenAudioDevice(NULL,0,&wanted,&obtained,0);
            if(music_device)SDL_PauseAudioDevice(music_device,0);
        }else fprintf(stderr,"audio: %s\n",err);
        /* Keep the original lead's timing and still let the chosen P1 device
           drive every subsequent single-window presentation screen. */
        quit=intro_music_lead(window,renderer,*texture,fb);
        FloodIntro intro;
        if(!flood_intro_load(&intro,FLOOD_DATA_DIR,err,sizeof(err)))
            fprintf(stderr,"intro: %s\n",err);
        else if(!quit){
            SDL_DestroyTexture(*texture);
            *texture=create_stage_texture(renderer,FLOOD_INTRO_W,FLOOD_INTRO_H);
            stage_size(window,renderer,FLOOD_INTRO_W,FLOOD_INTRO_H);
            Uint32 next=SDL_GetTicks();
            while(*texture&&!intro.finished&&!quit){
                FloodInput i=mp_front_input(window,renderer,false,settings);
                if(i.quit)quit=true;
                flood_intro_tick(&intro,i.fire);render_intro(renderer,*texture,&intro,fb);
                next+=INTRO_FRAME_MS;const Uint32 now=SDL_GetTicks();
                if((Sint32)(next-now)>0)SDL_Delay(next-now);
            }
        }
        if(!quit){
            FloodTitle title;
            if(!flood_title_load(&title,FLOOD_DATA_DIR,err,sizeof(err)))
                fprintf(stderr,"title: %s\n",err);
            else{
                SDL_DestroyTexture(*texture);
                *texture=create_stage_texture(renderer,FLOOD_TITLE_W,FLOOD_TITLE_H);
                stage_size(window,renderer,FLOOD_TITLE_W,FLOOD_TITLE_H);
                bool released=false,accepted=false;Uint32 next=SDL_GetTicks();
                while(*texture&&!accepted&&!quit){
                    FloodInput i=mp_front_input(window,renderer,false,settings);
                    if(i.quit)quit=true;
                    if(!i.fire)released=true;else if(released)accepted=true;
                    render_title(renderer,*texture,&title,fb);
                    next+=PRESENTATION_FRAME_MS;const Uint32 now=SDL_GetTicks();
                    if((Sint32)(next-now)>0)SDL_Delay(next-now);
                }
            }
        }
        if(music_device)SDL_CloseAudioDevice(music_device);
    }
    if(quit)return false;
    FloodLevelSelect select;
    if(!flood_level_select_load(&select,FLOOD_DATA_DIR,*level,err,sizeof(err))){
        fprintf(stderr,"level selector: %s\n",err);*status=2;return false;
    }
    if(over_scores)flood_level_select_set_background(&select,score_background);
    SDL_DestroyTexture(*texture);
    *texture=create_stage_texture(renderer,FLOOD_LEVEL_SELECT_W,FLOOD_LEVEL_SELECT_H);
    stage_size(window,renderer,FLOOD_LEVEL_SELECT_W,FLOOD_LEVEL_SELECT_H);
    if(!*texture){*status=3;return false;}
    render_level_select(renderer,*texture,&select,fb);
    bool released=false;Uint32 next=SDL_GetTicks();
    while(!select.finished&&!quit){
        FloodInput i=mp_front_input(window,renderer,true,settings);
        if(i.quit)quit=true;
        if(!i.fire)released=true;
        flood_level_select_tick(&select,i.key,i.fire&&released);
        render_level_select(renderer,*texture,&select,fb);
        next+=PRESENTATION_FRAME_MS;const Uint32 now=SDL_GetTicks();
        if((Sint32)(next-now)>0)SDL_Delay(next-now);
    }
    *level=select.selected_level;
    if(quit)return false;
    fade_out(renderer,*texture,fb,FLOOD_LEVEL_SELECT_W,FLOOD_LEVEL_SELECT_H);
    FloodGame *seed=calloc(1,sizeof(*seed));
    if(!seed){*status=3;return false;}
    flood_game_init(seed);
    if(!flood_game_load_level(seed,FLOOD_DATA_DIR,*level,err,sizeof(err))){
        fprintf(stderr,"load selected level %u: %s\n",*level,err);
        free(seed);*status=2;return false;
    }
    *rng_state=seed->rng_state;
    if(!direct_select){
        FloodProtection protection;
        if(!flood_protection_load(&protection,FLOOD_DATA_DIR,seed->rng_state,err,sizeof(err)))
            fprintf(stderr,"copy protection: %s\n",err);
        else{
            SDL_DestroyTexture(*texture);
            *texture=create_stage_texture(renderer,FLOOD_PROTECTION_W,FLOOD_PROTECTION_H);
            stage_size(window,renderer,FLOOD_PROTECTION_W,FLOOD_PROTECTION_H);
            if(!*texture){free(seed);*status=3;return false;}
            render_protection(renderer,*texture,&protection,fb);
            next=SDL_GetTicks();
            while(!protection.finished&&!quit){
                FloodInput i=mp_front_input(window,renderer,true,settings);
                if(i.quit)quit=true;
                if(protection.phase==1u&&protection.selection_pending&&!i.fire)
                    flood_protection_seed(&protection,(uint16_t)SDL_GetTicks());
                flood_protection_tick(&protection,i.x,i.key,i.fire);
                render_protection(renderer,*texture,&protection,fb);
                next+=PRESENTATION_FRAME_MS;const Uint32 now=SDL_GetTicks();
                if((Sint32)(next-now)>0)SDL_Delay(next-now);
            }
            if(!protection.passed)quit=true;
            *rng_state=protection.rng_state;
            if(protection_exit_fades(&protection,quit))
                fade_out(renderer,*texture,fb,FLOOD_PROTECTION_W,FLOOD_PROTECTION_H);
        }
    }
    free(seed);
    return !quit;
}

/* Enter the higher score first, then offer the same original name editor to
   the other player if their score still qualifies for the shared top ten. */
static bool mp_run_post_scores(SDL_Window *window,SDL_Renderer *renderer,
    SDL_Texture **texture,uint32_t *fb,uint8_t *finished_background,
    const uint32_t player_scores[2],const ReFloodSettings *settings,
    bool *quit,bool *selector_over_scores,int *status){
    char err[160]={0},score_err[160]={0};FloodHighScores scores;FloodTitle title;
    if(!flood_high_scores_load(&scores,FLOOD_DATA_DIR,0u,err,sizeof(err))){
        fprintf(stderr,"high scores: %s\n",err);*status=2;return false;
    }
    if(!high_scores_load_configuration(&scores,score_err,sizeof(score_err))&&score_err[0])
        fprintf(stderr,"high scores: %s; using original table\n",score_err);
    if(!flood_title_load(&title,FLOOD_DATA_DIR,err,sizeof(err))){
        fprintf(stderr,"title: %s\n",err);*status=2;return false;
    }
    SDL_DestroyTexture(*texture);
    *texture=create_stage_texture(renderer,FLOOD_HIGHSCORE_W,FLOOD_HIGHSCORE_H);
    stage_size(window,renderer,FLOOD_HIGHSCORE_W,FLOOD_HIGHSCORE_H);
    if(!*texture){*status=3;return false;}
    const unsigned order[2]={player_scores[1]>player_scores[0]?1u:0u,
        player_scores[1]>player_scores[0]?0u:1u};
    bool changed=false;Uint32 next=SDL_GetTicks();
    for(unsigned n=0;n<2u&&!(*quit);n++){
        const unsigned player=order[n];
        flood_high_scores_enter(&scores,player_scores[player]);
        if(scores.entry_index<0)continue;
        changed=true;
        while(scores.editing&&!*quit){
            FloodInput i=input(window,renderer,false,false);if(i.quit)*quit=true;
            mp_find_pads();
            const FloodInput actor=mp_device_input(settings->multiplayer_input[player],
                SDL_GetKeyboardState(NULL));
            flood_high_scores_tick(&scores,actor.x,actor.y,actor.fire);
            render_high_scores(renderer,*texture,&scores,fb);
            next+=PRESENTATION_FRAME_MS;const Uint32 now=SDL_GetTicks();
            if((Sint32)(next-now)>0)SDL_Delay(next-now);
        }
    }
    if(changed&&!*quit&&
        !high_scores_save_configuration(&scores,score_err,sizeof(score_err)))
        fprintf(stderr,"high scores: %s\n",score_err);
    if(*quit)return false;
    PostScoreAttract attract=post_score_attract_begin();
    render_high_scores(renderer,*texture,&scores,fb);
    while(!*quit){
        FloodInput i=mp_front_input(window,renderer,false,settings);
        if(i.quit){*quit=true;break;}
        if(post_score_attract_fire(&attract,i.fire)){
            *selector_over_scores=attract.show_scores;
            if(attract.show_scores)memcpy(finished_background,scores.planes,
                4u*FLOOD_LEVEL_SELECT_PLANE_BYTES);
            return true;
        }
        if(attract.show_scores)render_high_scores(renderer,*texture,&scores,fb);
        else render_title(renderer,*texture,&title,fb);
        post_score_attract_advance(&attract);
        next+=PRESENTATION_FRAME_MS;const Uint32 now=SDL_GetTicks();
        if((Sint32)(next-now)>0)SDL_Delay(next-now);
    }
    return false;
}

static void mp_present_split(SDL_Renderer *r,SDL_Texture *texture[2],
    const SDL_Rect play[2],const SDL_Rect hud[2],
    const SDL_Rect *play_source,const SDL_Rect *hud_source,
    const SDL_Rect *divider){
    SDL_SetRenderDrawColor(r,0u,0u,0u,255u);SDL_RenderClear(r);
    for(unsigned n=0;n<2u;n++){
        SDL_RenderCopy(r,texture[n],play_source,&play[n]);
        SDL_RenderCopy(r,texture[n],hud_source,&hud[n]);
    }
    SDL_RenderFillRect(r,divider);SDL_RenderPresent(r);
}

/* The original exit fade has sixteen 20 ms color steps. Fade the two already
   rendered views together, including their own in-game HUD strips. */
static void mp_fade_split(SDL_Renderer *r,SDL_Texture *texture[2],
    uint32_t *frames[2],const SDL_Rect play[2],const SDL_Rect hud[2],
    const SDL_Rect *play_source,const SDL_Rect *hud_source,
    const SDL_Rect *divider){
    Uint32 next=SDL_GetTicks();
    for(unsigned step=0;step<16u;step++){
        for(unsigned side=0;side<2u;side++){
            uint32_t *pixels=frames[side];
            for(unsigned n=0;n<FLOOD_VIEW_PIXELS;n++){
                const uint32_t c=pixels[n];uint32_t faded=0xff000000u;
                for(unsigned shift=0;shift<=16u;shift+=8u){
                    const uint32_t component=(c>>shift)&0xffu;
                    faded|=(component>17u?component-17u:0u)<<shift;
                }
                pixels[n]=faded;
            }
            update_stage_texture(texture[side],pixels,VIEW_W,VIEW_H);
        }
        mp_present_split(r,texture,play,hud,play_source,hud_source,divider);
        next+=20u;const Uint32 now=SDL_GetTicks();
        if((Sint32)(next-now)>0)SDL_Delay(next-now);
    }
}

/* Match the single-player 51-frame level banner and deferred protection cue.
   Both actors receive the first-frame protection/movement update. */
static bool mp_play_level_banner(SDL_Window *window,SDL_Renderer *r,
    const ReFloodSettings *settings,FloodMultiplayer *mp,FloodGame *view,
    SDL_Texture *texture[2],uint32_t *frame,uint32_t *frames[2],uint8_t *backing,
    const SDL_Rect play[2],const SDL_Rect hud[2],
    const SDL_Rect *play_source,const SDL_Rect *hud_source,
    const SDL_Rect *divider,FloodAudio *audio,SDL_AudioDeviceID device){
    Uint32 next=SDL_GetTicks();bool protection_cue=false;
    for(unsigned count=0;count<FLOOD_LEVEL_BANNER_FRAMES&&mp->game->running;count++){
        FloodInput modal=input(window,r,false,false);
        if(modal.quit)return true;
        mp_find_pads();const Uint8 *keys=SDL_GetKeyboardState(NULL);
        const FloodInput first=mp_device_input(settings->multiplayer_input[0],keys);
        const FloodInput second=mp_device_input(settings->multiplayer_input[1],keys);
        flood_multiplayer_banner_tick(mp,first,second,count==0u);
        if(count==0u)protection_cue=
            (mp->game->lives>0&&mp->game->player.invulnerable_timer==99u)||
            (mp->second->lives>0&&mp->second->player.invulnerable_timer==99u);
        flood_multiplayer_second_view(mp,view);
        const FloodGame *actors[2]={mp->game,view};
        for(unsigned n=0;n<2u;n++){
            build_level_banner_frame(actors[n],106,frame,backing);
            memcpy(frames[n],frame,FLOOD_VIEW_PIXELS*sizeof(*frame));
            update_stage_texture(texture[n],frame,VIEW_W,VIEW_H);
        }
        mp_present_split(r,texture,play,hud,play_source,hud_source,divider);
        next+=GAME_FRAME_MS;const Uint32 now=SDL_GetTicks();
        if((Sint32)(next-now)>0)SDL_Delay(next-now);
    }
    if(device&&protection_cue){
        SDL_LockAudioDevice(device);
        /* Trigger on every level entry. Let the audio callback consume its
           first buffer before gameplay can queue another effect on the same
           two channels. The trigger itself resets both routed channels. */
        flood_audio_trigger(audio,52u);
        SDL_UnlockAudioDevice(device);
        SDL_Delay(24u);
    }
    return false;
}

static int mp_run_gameplay(SDL_Window *window,SDL_Renderer *renderer,
    const ReFloodSettings *settings,unsigned level,uint16_t rng_state,
    uint32_t player_scores[2],bool *both_dead,bool *completed,
    const char *custom_header){
    char err[160]={0};int status=0;
    *both_dead=false;*completed=false;player_scores[0]=player_scores[1]=0u;
    FloodMultiplayer mp={.game=calloc(1,sizeof(FloodGame)),
        .second=calloc(1,sizeof(FloodGame))};
    FloodGame *view=malloc(sizeof(FloodGame));
    uint32_t *frame=malloc(FLOOD_VIEW_PIXELS*sizeof(*frame));
    uint32_t *frames[2]={malloc(FLOOD_VIEW_PIXELS*sizeof(uint32_t)),
        malloc(FLOOD_VIEW_PIXELS*sizeof(uint32_t))};
    uint8_t *backing=malloc(FLOOD_BACKING_PIXELS);
    SDL_Texture *texture[2]={NULL,NULL};
    char current_custom[EDITOR_BROWSER_PATH];
    if(!mp.game||!mp.second||!view||!frame||!frames[0]||!frames[1]||!backing){status=3;goto done;}
    if(custom_header){
        if(strlen(custom_header)>=sizeof(current_custom)){status=2;goto done;}
        strcpy(current_custom,custom_header);
    }
    flood_game_init(mp.game);
    const bool loaded=custom_header?
        flood_game_load_custom_level(mp.game,FLOOD_DATA_DIR,current_custom,err,sizeof(err)):
        flood_game_load_level(mp.game,FLOOD_DATA_DIR,level,err,sizeof(err));
    if(!loaded){
        fprintf(stderr,"multiplayer level: %s\n",err);status=2;goto done;
    }
    if(!custom_header)mp.game->rng_state=rng_state;
    flood_multiplayer_begin(&mp);
    uint8_t game_over_font[SETTINGS_FONT_BYTES];
    if(!settings_load_font(game_over_font,err,sizeof(err))){
        fprintf(stderr,"multiplayer font: %s\n",err);status=2;goto done;
    }
    stage_size(window,renderer,640,360);
    for(unsigned n=0;n<2u;n++)texture[n]=create_stage_texture(renderer,VIEW_W,VIEW_H);
    if(!texture[0]||!texture[1]){status=3;goto done;}
    FloodAudio audio;SDL_AudioDeviceID device=0;
    SDL_AudioSpec wanted={0},obtained={0};
    wanted.freq=44100;wanted.format=AUDIO_S16SYS;wanted.channels=2;wanted.samples=1024;
    wanted.callback=audio_callback;wanted.userdata=&audio;
    if(flood_audio_load(&audio,FLOOD_DATA_DIR,44100u,err,sizeof(err))){
        device=SDL_OpenAudioDevice(NULL,0,&wanted,&obtained,0);
        if(device)SDL_PauseAudioDevice(device,0);
    }
    const unsigned factor=scaler_output_factor();
    /* A centered 240x200 playfield crop scales exactly to 318x265. The
       original 320x8 HUD stays whole above it, including both edge gauges. */
    const SDL_Rect play_source={(int)(40u*factor),(int)(8u*factor),
        (int)(240u*factor),(int)(200u*factor)};
    const SDL_Rect hud_source={0,0,(int)(320u*factor),(int)(8u*factor)};
    SDL_Rect play[2]={
        {(int)factor,(int)(48u*factor),(int)(318u*factor),(int)(265u*factor)},
        {(int)(321u*factor),(int)(48u*factor),
            (int)(318u*factor),(int)(265u*factor)}};
    SDL_Rect hud[2]={{(int)factor,(int)(37u*factor),
        (int)(318u*factor),(int)(11u*factor)},
        {(int)(321u*factor),(int)(37u*factor),
        (int)(318u*factor),(int)(11u*factor)}};
    SDL_Rect divider={(int)(319u*factor),0,(int)(2u*factor),(int)(360u*factor)};
    bool quit=mp_play_level_banner(window,renderer,settings,&mp,view,
        texture,frame,frames,backing,play,hud,&play_source,&hud_source,
        &divider,&audio,device);
    Uint32 next=SDL_GetTicks();bool escaping=false;
    while(mp.game->running&&!quit){
        SDL_Event event;bool restart=false,pause=false;
        while(SDL_PollEvent(&event)){
            if(event.type==SDL_QUIT)quit=true;
            else if(event.type==SDL_CONTROLLERDEVICEADDED||
                    event.type==SDL_CONTROLLERDEVICEREMOVED)mp_close_pads();
            else if(event.type==SDL_KEYDOWN&&!event.key.repeat){
                if(event.key.keysym.sym==SDLK_ESCAPE)escaping=true;
                if(event.key.keysym.sym==SDLK_p)pause=true;
                if(event.key.keysym.sym==SDLK_r)restart=true;
                if(event.key.keysym.sym==SDLK_f){
                    const bool actual=(SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN_DESKTOP)!=0u;
                    apply_fullscreen(window,renderer,!actual);
                }
            }
        }
        if(quit)break;
        mp_find_pads();
        const Uint8 *keys=SDL_GetKeyboardState(NULL);
        if(keys[SDL_SCANCODE_ESCAPE])escaping=true;
        FloodInput first=mp_device_input(settings->multiplayer_input[0],keys);
        FloodInput second=mp_device_input(settings->multiplayer_input[1],keys);
        if(escaping){
            first.restart=!mp.terminal_animation;
            second.restart=!mp.terminal_animation;
            mp.terminal_animation=true;
            pause=false;restart=false;
        }
        if(restart){
            const int16_t first_lives=mp.game->lives,second_lives=mp.second->lives;
            mp_fade_split(renderer,texture,frames,play,hud,&play_source,
                &hud_source,&divider);
            const bool restarted=custom_header?
                flood_game_load_custom_level(mp.game,FLOOD_DATA_DIR,current_custom,err,sizeof(err)):
                flood_game_restart_begin(mp.game,FLOOD_DATA_DIR,err,sizeof(err));
            if(!restarted){
                fprintf(stderr,"multiplayer restart: %s\n",err);status=2;break;
            }
            flood_multiplayer_reset_second(&mp);
            quit=mp_play_level_banner(window,renderer,settings,&mp,view,
                texture,frame,frames,backing,play,hud,&play_source,
                &hud_source,&divider,&audio,device);
            if(quit)break;
            flood_multiplayer_restart_finish(&mp,first_lives,second_lives);
            next=SDL_GetTicks();continue;
        }
        if(pause){
            quit=mp_pause_until_fire(window,renderer,settings,first.fire||second.fire);
            next=SDL_GetTicks();continue;
        }
        flood_multiplayer_tick(&mp,first,second);
        if(mp.game->zap_message_pending)flood_complete_zap_message(mp.game);
        if(device){SDL_LockAudioDevice(device);
            for(unsigned n=0;n<mp.game->sound_queue_count;n++)
                flood_audio_trigger(&audio,mp.game->sound_queue[n]);
            SDL_UnlockAudioDevice(device);
        }
        char next_custom[EDITOR_BROWSER_PATH];
        const bool last_custom=custom_header&&mp.game->level_complete&&
            !reflood_custom_next_header(current_custom,next_custom,sizeof(next_custom));
        SDL_SetRenderDrawColor(renderer,0u,0u,0u,255u);
        SDL_RenderClear(renderer);
        render_world_panel(renderer,texture[0],mp.game,frame,backing,
            mp.second,0u,&play[0],&play_source);
        if(last_custom)custom_draw_congratulations(frame,game_over_font);
        else mp_draw_game_over(frame,game_over_font,mp.game->lives,
            mp.game->player.death_mode);
        if(last_custom||(mp.game->lives<=0&&!mp.game->player.death_mode))
            update_stage_texture(texture[0],frame,VIEW_W,VIEW_H);
        memcpy(frames[0],frame,FLOOD_VIEW_PIXELS*sizeof(*frame));
        flood_multiplayer_second_view(&mp,view);
        render_world_panel(renderer,texture[1],view,frame,backing,
            mp.game,1u,&play[1],&play_source);
        if(last_custom)custom_draw_congratulations(frame,game_over_font);
        else mp_draw_game_over(frame,game_over_font,mp.second->lives,
            mp.second->player.death_mode);
        if(last_custom||(mp.second->lives<=0&&!mp.second->player.death_mode))
            update_stage_texture(texture[1],frame,VIEW_W,VIEW_H);
        memcpy(frames[1],frame,FLOOD_VIEW_PIXELS*sizeof(*frame));
        mp_present_split(renderer,texture,play,hud,&play_source,&hud_source,&divider);
        if(mp.game->level_complete){
            const int16_t second_lives=mp.second->lives;
            const unsigned previous=mp.game->world.level_number;
            if(last_custom){
                const Uint32 until=SDL_GetTicks()+CUSTOM_CONGRATULATIONS_MS;
                while((Sint32)(until-SDL_GetTicks())>0&&!quit){
                    FloodInput modal=input(window,renderer,false,false);
                    if(modal.quit)quit=true;
                    mp_present_split(renderer,texture,play,hud,&play_source,
                        &hud_source,&divider);
                    SDL_Delay(PRESENTATION_FRAME_MS);
                }
                if(quit)break;
            }
            mp_fade_split(renderer,texture,frames,play,hud,&play_source,
                &hud_source,&divider);
            quit=wait_audio_idle(window,renderer,&audio,device);
            if(quit)break;
            if(custom_header){
                if(last_custom){*completed=true;break;}
                if(!flood_game_load_custom_level(mp.game,FLOOD_DATA_DIR,next_custom,
                    err,sizeof(err))){
                    fprintf(stderr,"multiplayer next custom level: %s\n",err);status=2;break;
                }
                strcpy(current_custom,next_custom);
            }else if(!flood_game_advance_level(mp.game,FLOOD_DATA_DIR,err,sizeof(err))){
                fprintf(stderr,"multiplayer advance: %s\n",err);status=2;break;
            }
            if(custom_header||mp.game->world.level_number!=previous){
                level=mp.game->world.level_number;
                flood_multiplayer_reset_second(&mp);
                mp.second->lives=second_lives;
                quit=mp_play_level_banner(window,renderer,settings,&mp,view,
                    texture,frame,frames,backing,play,hud,&play_source,
                    &hud_source,&divider,&audio,device);
                if(quit)break;
            }
            next=SDL_GetTicks();continue;
        }
        next+=game_frame_ms(saved_game_speed);
        const Uint32 now=SDL_GetTicks();
        if((Sint32)(next-now)>0)SDL_Delay(next-now);else next=now;
    }
    if(!quit&&status==0&&mp.game->lives<=0&&mp.second->lives<=0){
        mp_fade_split(renderer,texture,frames,play,hud,&play_source,
            &hud_source,&divider);
        if(device){SDL_LockAudioDevice(device);flood_audio_stop(&audio);
            SDL_UnlockAudioDevice(device);}
        *both_dead=true;
    }
    if(!quit&&status==0&&mp.game->game_complete){
        /* The ordinary level-42 exit has already faded the cavern. Match
           the single-player ending branch's second fade before credits. */
        mp_fade_split(renderer,texture,frames,play,hud,&play_source,
            &hud_source,&divider);
        *completed=true;
    }
    player_scores[0]=mp.game->score;player_scores[1]=mp.second->score;
    if(device)SDL_CloseAudioDevice(device);
done:
    mp_close_pads();
    for(unsigned n=0;n<2u;n++){
        SDL_DestroyTexture(texture[n]);
    }
    free(backing);free(frame);free(frames[0]);free(frames[1]);
    free(view);free(mp.second);free(mp.game);
    return status;
}

static bool mp_play_ending(SDL_Window *window,SDL_Renderer *renderer,
    SDL_Texture **texture,uint32_t *fb,const ReFloodSettings *settings,
    bool *quit,int *status){
    FloodEnding ending;char err[160]={0};
    if(!flood_ending_load(&ending,FLOOD_DATA_DIR,err,sizeof(err))){
        fprintf(stderr,"ending: %s\n",err);*status=2;return false;
    }
    SDL_DestroyTexture(*texture);
    *texture=create_stage_texture(renderer,FLOOD_ENDING_W,FLOOD_ENDING_H);
    stage_size(window,renderer,FLOOD_ENDING_W,FLOOD_ENDING_H);
    if(!*texture){*status=3;return false;}
    Uint32 next=SDL_GetTicks();
    while(!ending.finished&&!*quit){
        FloodInput i=mp_front_input(window,renderer,false,settings);
        if(i.quit)*quit=true;
        flood_ending_tick(&ending);render_ending(renderer,*texture,&ending,fb);
        next+=PRESENTATION_FRAME_MS;const Uint32 now=SDL_GetTicks();
        if((Sint32)(next-now)>0)SDL_Delay(next-now);
    }
    if(ending.finished&&!*quit)
        fade_out(renderer,*texture,fb,FLOOD_ENDING_W,FLOOD_ENDING_H);
    return ending.finished&&!*quit;
}

static int run_experimental_multiplayer(SDL_Window *window,SDL_Renderer *renderer,
    SDL_Texture **texture,uint32_t *fb,const ReFloodSettings *settings,
    unsigned level){
    int status=0;bool direct_select=false,over_scores=false;
    uint16_t rng_state=0u;
    uint8_t score_background[4*FLOOD_LEVEL_SELECT_PLANE_BYTES]={0};
    for(;;){
        if(!mp_play_frontend(window,renderer,texture,fb,&level,direct_select,
            over_scores,score_background,&rng_state,settings,&status))return status;
        direct_select=false;over_scores=false;
        uint32_t player_scores[2];bool both_dead=false,completed=false;
        status=mp_run_gameplay(window,renderer,settings,level,rng_state,
            player_scores,&both_dead,&completed,NULL);
        if(status||(!both_dead&&!completed))return status;
        bool quit=false;
        if(completed&&
            !mp_play_ending(window,renderer,texture,fb,settings,&quit,&status))
            return status;
        if(!mp_run_post_scores(window,renderer,texture,fb,score_background,
                player_scores,settings,&quit,&over_scores,&status))return status;
        direct_select=true;level=1u;
    }
}
