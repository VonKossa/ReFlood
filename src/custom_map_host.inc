/* The custom-map path is deliberately separate from the original campaign. */
#define CUSTOM_BROWSER_ROWS 9u

static void custom_browser_move(EditorBrowser *browser,int delta){
    if(!browser->count)return;
    int next=(int)browser->selected+delta;
    if(next<0)next=0;
    if(next>=(int)browser->count)next=(int)browser->count-1;
    browser->selected=(unsigned)next;
    if(browser->selected<browser->offset)browser->offset=browser->selected;
    if(browser->selected>=browser->offset+CUSTOM_BROWSER_ROWS)
        browser->offset=browser->selected-CUSTOM_BROWSER_ROWS+1u;
}
static void render_custom_browser(SDL_Renderer *renderer,SDL_Texture *texture,
    uint32_t *fb,const uint8_t font[SETTINGS_FONT_BYTES],const EditorBrowser *browser,
    const char *message){
    for(size_t n=0;n<(size_t)VIEW_W*VIEW_H;n++)fb[n]=0xff000000u;
    settings_draw_centered(fb,font,10,"CHOOSE CUSTOM MAP");
    const size_t length=strlen(browser->directory);
    settings_draw_text(fb,font,8,28,length>37u?browser->directory+length-37u:browser->directory);
    settings_draw_text(fb,font,16,49,"[..] PARENT DIRECTORY");
    for(unsigned row=0;row<CUSTOM_BROWSER_ROWS&&browser->offset+row<browser->count;row++){
        const unsigned index=browser->offset+row;
        const EditorBrowserEntry *item=&browser->entries[index];
        const int y=64+(int)row*13;
        if(index==browser->selected)for(int py=y-2;py<y+10;py++)
            for(int px=10;px<310;px++)fb[(size_t)py*VIEW_W+px]=0xff26384cu;
        char entry[40];
        snprintf(entry,sizeof(entry),"%s %.32s",item->directory?"[DIR]":"     ",item->name);
        settings_draw_text(fb,font,16,y,entry);
    }
    if(!browser->count)settings_draw_text(fb,font,16,72,"NO LEVEL HEADERS HERE");
    if(message&&message[0])settings_draw_text(fb,font,8,181,message);
    settings_draw_text(fb,font,8,198,"ENTER OPEN  X/DEL CLEAR  ESC BACK");
    update_stage_texture(texture,fb,VIEW_W,VIEW_H);
    SDL_RenderClear(renderer);SDL_RenderCopy(renderer,texture,NULL,NULL);SDL_RenderPresent(renderer);
}
static CustomBrowserResult custom_browser_activate(EditorBrowser *browser,
    char *chosen,size_t chosen_capacity,char *message,size_t message_capacity){
    char path[EDITOR_BROWSER_PATH]={0},error[160];
    if(!editor_browser_activate(browser,path,sizeof(path),error,sizeof(error))){
        snprintf(message,message_capacity,"%.38s",error);return CUSTOM_BROWSER_CANCEL;
    }
    if(!path[0])return CUSTOM_BROWSER_CANCEL; /* Opened a directory. */
    FloodGame *probe=malloc(sizeof(*probe));
    if(!probe){snprintf(message,message_capacity,"OUT OF MEMORY");return CUSTOM_BROWSER_CANCEL;}
    flood_game_init(probe);
    const bool valid=flood_game_load_custom_level(probe,FLOOD_DATA_DIR,path,error,sizeof(error));
    free(probe);
    if(!valid){snprintf(message,message_capacity,"%.38s",error);return CUSTOM_BROWSER_CANCEL;}
    if(strlen(path)>=chosen_capacity){
        snprintf(message,message_capacity,"SELECTED PATH IS TOO LONG");return CUSTOM_BROWSER_CANCEL;
    }
    strcpy(chosen,path);return CUSTOM_BROWSER_SELECTED;
}
static CustomBrowserResult run_custom_browser(SDL_Window *window,SDL_Renderer *renderer,
    SDL_Texture *texture,uint32_t *fb,const uint8_t font[SETTINGS_FONT_BYTES],
    char *chosen,size_t chosen_capacity){
    (void)window;
    EditorBrowser browser;char error[160],message[48]={0};
    if(!editor_browser_open(&browser,"custom/levels",error,sizeof(error))){
        if(!editor_browser_open(&browser,".",error,sizeof(error)))return CUSTOM_BROWSER_CANCEL;
        snprintf(message,sizeof(message),"CUSTOM/LEVELS MISSING - USING ./");
    }
    chosen[0]='\0';Uint32 next=SDL_GetTicks();
    for(;;){
        render_custom_browser(renderer,texture,fb,font,&browser,message);
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            if(event.type==SDL_QUIT)return CUSTOM_BROWSER_QUIT;
            if(event.type==SDL_CONTROLLERDEVICEADDED){close_game_controller();find_game_controller();}
            if(event.type==SDL_CONTROLLERDEVICEREMOVED)remove_game_controller(event.cdevice.which);
            if(event.type==SDL_MOUSEWHEEL)custom_browser_move(&browser,-event.wheel.y*3);
            if(event.type==SDL_MOUSEBUTTONDOWN&&event.button.button==SDL_BUTTON_LEFT){
                const int x=event.button.x,y=event.button.y;
                if(x>=8&&x<310&&y>=45&&y<62){
                    if(!editor_browser_parent(&browser,error,sizeof(error)))
                        snprintf(message,sizeof(message),"%.38s",error);
                    else message[0]='\0';
                }else if(x>=10&&x<310&&y>=61&&y<61+(int)CUSTOM_BROWSER_ROWS*13){
                    const unsigned index=browser.offset+(unsigned)(y-61)/13u;
                    if(index<browser.count){
                        browser.selected=index;
                        if(event.button.clicks>=2u){
                            CustomBrowserResult result=custom_browser_activate(&browser,chosen,
                                chosen_capacity,message,sizeof(message));
                            if(result==CUSTOM_BROWSER_SELECTED)return result;
                        }
                    }
                }
            }
            SDL_Keycode key=0;
            if(event.type==SDL_KEYDOWN&&!event.key.repeat)key=event.key.keysym.sym;
            if(event.type==SDL_CONTROLLERBUTTONDOWN){
                switch(event.cbutton.button){
                case SDL_CONTROLLER_BUTTON_A:key=SDLK_RETURN;break;
                case SDL_CONTROLLER_BUTTON_BACK:key=SDLK_ESCAPE;break;
                case SDL_CONTROLLER_BUTTON_X:key=SDLK_DELETE;break;
                case SDL_CONTROLLER_BUTTON_DPAD_UP:key=SDLK_UP;break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:key=SDLK_DOWN;break;
                default:break;
                }
            }
            if(key==SDLK_ESCAPE)return CUSTOM_BROWSER_CANCEL;
            if(key==SDLK_DELETE)return CUSTOM_BROWSER_CLEARED;
            if(key==SDLK_UP||key==SDLK_w)custom_browser_move(&browser,-1);
            else if(key==SDLK_DOWN||key==SDLK_s)custom_browser_move(&browser,1);
            else if(key==SDLK_PAGEUP)custom_browser_move(&browser,-(int)CUSTOM_BROWSER_ROWS);
            else if(key==SDLK_PAGEDOWN)custom_browser_move(&browser,(int)CUSTOM_BROWSER_ROWS);
            else if(key==SDLK_HOME){browser.selected=0;browser.offset=0;}
            else if(key==SDLK_END&&browser.count){
                browser.selected=browser.count-1u;
                browser.offset=browser.count>CUSTOM_BROWSER_ROWS?
                    browser.count-CUSTOM_BROWSER_ROWS:0u;
            }else if(key==SDLK_BACKSPACE){
                if(!editor_browser_parent(&browser,error,sizeof(error)))
                    snprintf(message,sizeof(message),"%.38s",error);
                else message[0]='\0';
            }else if(key==SDLK_RETURN||key==SDLK_KP_ENTER){
                CustomBrowserResult result=custom_browser_activate(&browser,chosen,
                    chosen_capacity,message,sizeof(message));
                if(result==CUSTOM_BROWSER_SELECTED)return result;
            }
        }
        next+=PRESENTATION_FRAME_MS;const Uint32 now=SDL_GetTicks();
        if((Sint32)(next-now)>0)SDL_Delay(next-now);else next=now;
    }
}

static bool run_custom_map_session(SDL_Window *window,SDL_Renderer *renderer,
    SDL_Texture **texture,uint32_t *fb,const char *header){
    uint8_t *world=malloc(FLOOD_BACKING_PIXELS);
    FloodGame *game=malloc(sizeof(*game));char error[160];
    char current_header[EDITOR_BROWSER_PATH];
    if(strlen(header)>=sizeof(current_header)){free(world);free(game);return false;}
    strcpy(current_header,header);
    if(!world||!game){free(world);free(game);return false;}
    flood_game_init(game);
    if(!flood_game_load_custom_level(game,FLOOD_DATA_DIR,current_header,error,sizeof(error))){
        fprintf(stderr,"custom level: %s\n",error);free(world);free(game);return false;
    }
    SDL_DestroyTexture(*texture);*texture=create_stage_texture(renderer,VIEW_W,VIEW_H);
    stage_size(window,renderer,VIEW_W,VIEW_H);
    if(!*texture){free(world);free(game);return false;}
    FloodAudio audio;SDL_AudioDeviceID device=0;SDL_AudioSpec wanted={0},obtained={0};
    wanted.freq=44100;wanted.format=AUDIO_S16SYS;wanted.channels=2;wanted.samples=1024;
    wanted.callback=audio_callback;wanted.userdata=&audio;
    if(flood_audio_load(&audio,FLOOD_DATA_DIR,(uint32_t)wanted.freq,error,sizeof(error))){
        device=SDL_OpenAudioDevice(NULL,0,&wanted,&obtained,0);
        if(device)SDL_PauseAudioDevice(device,0);
    }else fprintf(stderr,"audio: %s\n",error);
    bool quit=play_level_banner(window,renderer,*texture,game,&audio,device,fb,world);
    Uint32 next=SDL_GetTicks();
    while(game->running&&!quit){
        FloodInput i=input(window,renderer,false,false);if(i.quit)quit=true;
        if(quit)break;
        if(i.restart_level){
            fade_out(renderer,*texture,fb,VIEW_W,VIEW_H);
            if(!flood_game_load_custom_level(game,FLOOD_DATA_DIR,current_header,error,sizeof(error))){
                fprintf(stderr,"restart custom level: %s\n",error);break;
            }
            quit=play_level_banner(window,renderer,*texture,game,&audio,device,fb,world);
            if(!quit)flood_game_restart_finish(game);
            next=SDL_GetTicks();continue;
        }
        if(i.pause){quit=pause_until_fire(window,renderer,i.fire);next=SDL_GetTicks();continue;}
        flood_game_tick(game,i);
        if(device){SDL_LockAudioDevice(device);
            for(unsigned n=0;n<game->sound_queue_count;n++)flood_audio_trigger(&audio,game->sound_queue[n]);
            SDL_UnlockAudioDevice(device);
        }
        render_world(renderer,*texture,game,fb,world);
        if(game->zap_message_pending){
            FloodZapMessage message;
            if(!flood_zap_message_load(&message,FLOOD_DATA_DIR,game->world.level_number,
                error,sizeof(error))){
                fprintf(stderr,"zap message: %s\n",error);
                flood_complete_zap_message(game);
            }else{
                Uint32 modal_next=SDL_GetTicks();render_zap(renderer,*texture,&message,fb,world);
                while(!message.finished&&!quit){
                    FloodInput modal=input(window,renderer,false,false);if(modal.quit)quit=true;
                    flood_zap_message_tick(&message,modal.fire);
                    render_zap(renderer,*texture,&message,fb,world);
                    modal_next+=20u;const Uint32 now=SDL_GetTicks();
                    if((Sint32)(modal_next-now)>0)SDL_Delay(modal_next-now);
                }
                if(message.finished){
                    const unsigned resume_sound=game->sound_queue_count;
                    flood_complete_zap_message(game);
                    if(device){SDL_LockAudioDevice(device);
                        for(unsigned n=resume_sound;n<game->sound_queue_count;n++)
                            flood_audio_trigger(&audio,game->sound_queue[n]);
                        SDL_UnlockAudioDevice(device);
                    }
                    render_world(renderer,*texture,game,fb,world);
                }
            }
            next=SDL_GetTicks();
        }
        if(game->level_complete){
            char next_header[EDITOR_BROWSER_PATH];
            const bool has_next=reflood_custom_next_header(current_header,
                next_header,sizeof(next_header));
            if(!has_next){
                uint8_t font[SETTINGS_FONT_BYTES];
                if(settings_load_font(font,error,sizeof(error))){
                    custom_draw_congratulations(fb,font);
                    update_stage_texture(*texture,fb,VIEW_W,VIEW_H);
                    const Uint32 until=SDL_GetTicks()+CUSTOM_CONGRATULATIONS_MS;
                    while((Sint32)(until-SDL_GetTicks())>0&&!quit){
                        FloodInput modal=input(window,renderer,false,false);
                        if(modal.quit)quit=true;
                        SDL_RenderClear(renderer);
                        SDL_RenderCopy(renderer,*texture,NULL,NULL);
                        SDL_RenderPresent(renderer);
                        SDL_Delay(PRESENTATION_FRAME_MS);
                    }
                }else fprintf(stderr,"custom congratulations font: %s\n",error);
            }
            if(quit)break;
            fade_out(renderer,*texture,fb,VIEW_W,VIEW_H);
            quit=wait_audio_idle(window,renderer,&audio,device);
            if(quit)break;
            if(!has_next)break;
            if(!flood_game_load_custom_level(game,FLOOD_DATA_DIR,next_header,
                error,sizeof(error))){
                fprintf(stderr,"next custom level: %s\n",error);break;
            }
            strcpy(current_header,next_header);
            quit=play_level_banner(window,renderer,*texture,game,&audio,device,fb,world);
            next=SDL_GetTicks();continue;
        }
        next+=game_frame_ms(saved_game_speed);const Uint32 now=SDL_GetTicks();
        if((Sint32)(next-now)>0)SDL_Delay(next-now);else next=now;
    }
    if(device)SDL_CloseAudioDevice(device);
    free(world);free(game);
    stage_size(window,renderer,VIEW_W,VIEW_H);
    return quit;
}
#undef CUSTOM_BROWSER_ROWS
