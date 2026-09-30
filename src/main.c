#include "flood_game.h"
#include "flood_multiplayer.h"
#include "reflood_editor_browser.h"
#include "reflood_custom_map.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#ifndef FLOOD_DATA_DIR
#define FLOOD_DATA_DIR "./data"
#endif
#ifdef FLOOD_USE_SDL
#include <SDL.h>
#define VIEW_W FLOOD_VIEW_W
#define VIEW_H FLOOD_VIEW_H
#define PRESENTATION_FRAME_MS 20u
#define INTRO_FRAME_MS 20u
#define INTRO_MUSIC_LEAD_MS 8354u
#define POST_SCORE_SCREEN_FRAMES 200u
#define CUSTOM_CONGRATULATIONS_MS 3000u
#define REFLOOD_WINDOW_TITLE "ReFlood"
#ifndef REFLOOD_CONFIG_DIR
#define REFLOOD_CONFIG_DIR "./config"
#endif
#define REFLOOD_CONFIG_PATH REFLOOD_CONFIG_DIR "/configuration"
#define REFLOOD_CONFIG_TEMP_PATH REFLOOD_CONFIG_DIR "/configuration.tmp"
#define REFLOOD_HISCORE_PATH REFLOOD_CONFIG_DIR "/hiscore"
#define REFLOOD_HISCORE_TEMP_PATH REFLOOD_CONFIG_DIR "/hiscore.tmp"
#define REFLOOD_HISCORE_HEADER "REFLOOD-HISCORE-1"
#define SETTINGS_FONT_BYTES 960u
/* Cycle-exact A500/PAL calibration: consecutive original $D7CE entries land
   exactly three 50 Hz video frames apart (3 * 20 ms).  The same cadence drives
   all 51 level-banner states. */
#define GAME_FRAME_MS 60u
#define TURBO_GAME_FRAME_MS (GAME_FRAME_MS/2u)
static const uint32_t palette[16]={
0xFF000000u,0xFF332233u,0xFF554488u,0xFF668899u,
0xFF661100u,0xFF773300u,0xFFBB6600u,0xFFCC9922u,
0xFF334411u,0xFF445511u,0xFF556600u,0xFF667700u,
0xFF223366u,0xFF336677u,0xFF8899AAu,0xFFCCBB99u};
typedef enum {
    REFLOOD_SCALER_NEAREST,
    REFLOOD_SCALER_LINEAR,
    REFLOOD_SCALER_SCALE2X,
    REFLOOD_SCALER_COUNT
} ReFloodScaler;
typedef enum {
    REFLOOD_GAME_SPEED_NORMAL,
    REFLOOD_GAME_SPEED_TURBO,
    REFLOOD_GAME_SPEED_COUNT
} ReFloodGameSpeed;
typedef enum {
    REFLOOD_INPUT_KEYBOARD,
    REFLOOD_INPUT_GAMEPAD,
    REFLOOD_INPUT_COUNT
} ReFloodInputMode;
static bool fullscreen=false;
static int stage_width=VIEW_W,stage_height=VIEW_H;
static ReFloodScaler active_scaler=REFLOOD_SCALER_NEAREST;
static ReFloodScaler saved_scaler=REFLOOD_SCALER_NEAREST;
static ReFloodGameSpeed saved_game_speed=REFLOOD_GAME_SPEED_NORMAL;
static unsigned saved_player1_choice=0u;
static SDL_GameController *game_controller=NULL;
static uint32_t *scaled_framebuffer=NULL;
static size_t scaled_framebuffer_pixels=0u;
typedef struct {
    bool fullscreen;
    ReFloodScaler scaler;
    ReFloodGameSpeed game_speed;
    unsigned multiplayer_input[2];
    bool multiplayer;
    bool custom_map;
    char custom_header[EDITOR_BROWSER_PATH];
    unsigned selected;
    bool vertical_ready,horizontal_ready,confirm_ready;
} ReFloodSettings;

static ReFloodSettings settings_defaults(void){
    ReFloodSettings settings={0};
    settings.scaler=REFLOOD_SCALER_NEAREST;
    settings.game_speed=REFLOOD_GAME_SPEED_NORMAL;
    settings.multiplayer_input[0]=0u;settings.multiplayer_input[1]=1u;
    settings.vertical_ready=settings.horizontal_ready=settings.confirm_ready=true;
    return settings;
}
static const char *scaler_config_name(ReFloodScaler scaler){
    static const char *const names[REFLOOD_SCALER_COUNT]={"nearest","linear","scale2x"};
    return scaler<REFLOOD_SCALER_COUNT?names[scaler]:names[0];
}
static const char *scaler_menu_name(ReFloodScaler scaler){
    static const char *const names[REFLOOD_SCALER_COUNT]={"NEAREST","LINEAR","SCALE2X"};
    return scaler<REFLOOD_SCALER_COUNT?names[scaler]:names[0];
}
static const char *game_speed_config_name(ReFloodGameSpeed speed){
    static const char *const names[REFLOOD_GAME_SPEED_COUNT]={"normal","turbo"};
    return speed<REFLOOD_GAME_SPEED_COUNT?names[speed]:names[0];
}
static const char *game_speed_menu_name(ReFloodGameSpeed speed){
    static const char *const names[REFLOOD_GAME_SPEED_COUNT]={"NORMAL","TURBO"};
    return speed<REFLOOD_GAME_SPEED_COUNT?names[speed]:names[0];
}
static const char *player_config_name(unsigned choice){
    static const char *const names[4]={"arrows","wasd","gamepad1","gamepad2"};
    return choice<4u?names[choice]:names[0];
}
static Uint32 game_frame_ms(ReFloodGameSpeed speed){
    return speed==REFLOOD_GAME_SPEED_TURBO?TURBO_GAME_FRAME_MS:GAME_FRAME_MS;
}
static bool settings_value_matches(const char *value,const char *word){
    const size_t length=strlen(word);
    if(strncmp(value,word,length)!=0)return false;
    value+=length;while(*value==' '||*value=='\t'||*value=='\r'||*value=='\n')value++;
    return *value=='\0';
}
static bool settings_parse_fullscreen(const char *line,bool *value){
    const char *p=line;
    while(*p==' '||*p=='\t')p++;
    if(strncmp(p,"fullscreen",10u)!=0)return false;
    p+=10u;while(*p==' '||*p=='\t')p++;
    if(*p++!='=')return false;
    while(*p==' '||*p=='\t')p++;
    if(settings_value_matches(p,"1")||settings_value_matches(p,"true")||
        settings_value_matches(p,"fullscreen")){*value=true;return true;}
    if(settings_value_matches(p,"0")||settings_value_matches(p,"false")||
        settings_value_matches(p,"windowed")){*value=false;return true;}
    return false;
}
static bool settings_parse_scaler(const char *line,ReFloodScaler *value){
    const char *p=line;
    while(*p==' '||*p=='\t')p++;
    if(strncmp(p,"scaler",6u)!=0)return false;
    p+=6u;while(*p==' '||*p=='\t')p++;
    if(*p++!='=')return false;
    while(*p==' '||*p=='\t')p++;
    for(unsigned n=0;n<REFLOOD_SCALER_COUNT;n++)if(settings_value_matches(p,scaler_config_name((ReFloodScaler)n))){
        *value=(ReFloodScaler)n;return true;
    }
    return false;
}
static bool settings_parse_game_speed(const char *line,ReFloodGameSpeed *value){
    const char *p=line;
    while(*p==' '||*p=='\t')p++;
    if(strncmp(p,"game_speed",10u)!=0)return false;
    p+=10u;while(*p==' '||*p=='\t')p++;
    if(*p++!='=')return false;
    while(*p==' '||*p=='\t')p++;
    for(unsigned n=0;n<REFLOOD_GAME_SPEED_COUNT;n++)
        if(settings_value_matches(p,game_speed_config_name((ReFloodGameSpeed)n))){
            *value=(ReFloodGameSpeed)n;return true;
        }
    return false;
}
static bool settings_parse_input(const char *line,ReFloodInputMode *value){
    const char *p=line;
    while(*p==' '||*p=='\t')p++;
    if(strncmp(p,"input",5u)!=0)return false;
    p+=5u;while(*p==' '||*p=='\t')p++;
    if(*p++!='=')return false;
    while(*p==' '||*p=='\t')p++;
    static const char *const legacy_names[REFLOOD_INPUT_COUNT]={"keyboard","gamepad"};
    for(unsigned n=0;n<REFLOOD_INPUT_COUNT;n++)
        if(settings_value_matches(p,legacy_names[n])){
            *value=(ReFloodInputMode)n;return true;
        }
    return false;
}
static bool settings_parse_player(const char *line,const char *name,unsigned *value){
    const char *p=line;
    while(*p==' '||*p=='\t')p++;
    const size_t length=strlen(name);
    if(strncmp(p,name,length)!=0)return false;
    p+=length;while(*p==' '||*p=='\t')p++;
    if(*p++!='=')return false;
    while(*p==' '||*p=='\t')p++;
    for(unsigned n=0;n<4u;n++)if(settings_value_matches(p,player_config_name(n))){
        *value=n;return true;
    }
    return false;
}
static void settings_load_configuration(ReFloodSettings *settings){
    FILE *file=fopen(REFLOOD_CONFIG_PATH,"rb");char line[96];
    if(!file)return;
    bool player1_seen=false;
    while(fgets(line,sizeof(line),file)){
        bool value;
        if(settings_parse_fullscreen(line,&value)){settings->fullscreen=value;continue;}
        if(settings_parse_scaler(line,&settings->scaler))continue;
        if(settings_parse_game_speed(line,&settings->game_speed))continue;
        if(settings_parse_player(line,"player1",&settings->multiplayer_input[0])){
            player1_seen=true;continue;
        }
        if(settings_parse_player(line,"player2",&settings->multiplayer_input[1]))continue;
        if(!player1_seen){
            ReFloodInputMode legacy;
            if(settings_parse_input(line,&legacy))
                settings->multiplayer_input[0]=legacy==REFLOOD_INPUT_GAMEPAD?2u:0u;
        }
    }
    fclose(file);
}
static bool settings_make_config_directory(void){
#ifdef _WIN32
    if(_mkdir(REFLOOD_CONFIG_DIR)==0)return true;
#else
    if(mkdir(REFLOOD_CONFIG_DIR,0777)==0)return true;
#endif
    return errno==EEXIST;
}
static bool settings_save_configuration(const ReFloodSettings *settings,char *err,size_t errcap){
    if(!settings_make_config_directory()){
        snprintf(err,errcap,"cannot create %s",REFLOOD_CONFIG_DIR);return false;
    }
    FILE *file=fopen(REFLOOD_CONFIG_TEMP_PATH,"wb");
    if(!file){snprintf(err,errcap,"cannot write %s",REFLOOD_CONFIG_TEMP_PATH);return false;}
    const bool wrote=fprintf(file,
        "fullscreen=%u\nscaler=%s\ngame_speed=%s\nplayer1=%s\nplayer2=%s\n",
        settings->fullscreen?1u:0u,scaler_config_name(settings->scaler),
        game_speed_config_name(settings->game_speed),
        player_config_name(settings->multiplayer_input[0]),
        player_config_name(settings->multiplayer_input[1]))>0;
    const bool closed=fclose(file)==0;
    if(!wrote||!closed){remove(REFLOOD_CONFIG_TEMP_PATH);snprintf(err,errcap,"cannot finish configuration write");return false;}
    if(rename(REFLOOD_CONFIG_TEMP_PATH,REFLOOD_CONFIG_PATH)!=0){
        /* Some Windows C runtimes do not replace an existing destination. */
        if(remove(REFLOOD_CONFIG_PATH)!=0&&errno!=ENOENT){
            remove(REFLOOD_CONFIG_TEMP_PATH);snprintf(err,errcap,"cannot replace %s",REFLOOD_CONFIG_PATH);return false;
        }
        if(rename(REFLOOD_CONFIG_TEMP_PATH,REFLOOD_CONFIG_PATH)!=0){
            remove(REFLOOD_CONFIG_TEMP_PATH);snprintf(err,errcap,"cannot install %s",REFLOOD_CONFIG_PATH);return false;
        }
    }
    return true;
}
static bool high_score_name_valid(const char name[12]){
    for(unsigned n=0;n<FLOOD_HIGHSCORE_NAME_CHARS;n++)
        if((uint8_t)name[n]<0x30u||(uint8_t)name[n]>0x5au)return false;
    return name[FLOOD_HIGHSCORE_NAME_CHARS]=='\0';
}
static bool high_scores_load_configuration(FloodHighScores *scores,char *err,size_t errcap){
    if(errcap)err[0]='\0';
    FILE *file=fopen(REFLOOD_HISCORE_PATH,"rb");char line[96];
    if(!file){if(errno!=ENOENT)snprintf(err,errcap,"cannot open %s",REFLOOD_HISCORE_PATH);return false;}
    uint32_t loaded_scores[FLOOD_HIGHSCORE_ENTRIES];
    char loaded_names[FLOOD_HIGHSCORE_ENTRIES][12];
    if(!fgets(line,sizeof(line),file)||strcmp(line,REFLOOD_HISCORE_HEADER "\n")!=0){
        snprintf(err,errcap,"invalid header in %s",REFLOOD_HISCORE_PATH);fclose(file);return false;
    }
    for(unsigned n=0;n<FLOOD_HIGHSCORE_ENTRIES;n++){
        if(!fgets(line,sizeof(line),file)){
            snprintf(err,errcap,"incomplete %s",REFLOOD_HISCORE_PATH);fclose(file);return false;
        }
        char *tab=strchr(line,'\t');
        if(!tab){snprintf(err,errcap,"invalid row in %s",REFLOOD_HISCORE_PATH);fclose(file);return false;}
        *tab='\0';char *end=NULL;errno=0;const unsigned long value=strtoul(line,&end,10);
        char *name=tab+1u;name[strcspn(name,"\r\n")]='\0';
        if(errno||!line[0]||*end||value>UINT32_MAX||strlen(name)!=FLOOD_HIGHSCORE_NAME_CHARS){
            snprintf(err,errcap,"invalid row in %s",REFLOOD_HISCORE_PATH);fclose(file);return false;
        }
        loaded_scores[n]=(uint32_t)value;memcpy(loaded_names[n],name,FLOOD_HIGHSCORE_NAME_CHARS+1u);
        if(!high_score_name_valid(loaded_names[n])||(n&&loaded_scores[n]>loaded_scores[n-1u])){
            snprintf(err,errcap,"invalid table in %s",REFLOOD_HISCORE_PATH);fclose(file);return false;
        }
    }
    fclose(file);memcpy(scores->scores,loaded_scores,sizeof(loaded_scores));
    memcpy(scores->names,loaded_names,sizeof(loaded_names));return true;
}
static bool high_scores_save_configuration(const FloodHighScores *scores,char *err,size_t errcap){
    if(errcap)err[0]='\0';
    if(!settings_make_config_directory()){
        snprintf(err,errcap,"cannot create %s",REFLOOD_CONFIG_DIR);return false;
    }
    FILE *file=fopen(REFLOOD_HISCORE_TEMP_PATH,"wb");
    if(!file){snprintf(err,errcap,"cannot write %s",REFLOOD_HISCORE_TEMP_PATH);return false;}
    bool wrote=fprintf(file,"%s\n",REFLOOD_HISCORE_HEADER)>0;
    for(unsigned n=0;n<FLOOD_HIGHSCORE_ENTRIES&&wrote;n++)
        wrote=fprintf(file,"%lu\t%.11s\n",(unsigned long)scores->scores[n],scores->names[n])>0;
    const bool closed=fclose(file)==0;
    if(!wrote||!closed){remove(REFLOOD_HISCORE_TEMP_PATH);snprintf(err,errcap,"cannot finish high-score write");return false;}
    if(rename(REFLOOD_HISCORE_TEMP_PATH,REFLOOD_HISCORE_PATH)!=0){
        if(remove(REFLOOD_HISCORE_PATH)!=0&&errno!=ENOENT){
            remove(REFLOOD_HISCORE_TEMP_PATH);snprintf(err,errcap,"cannot replace %s",REFLOOD_HISCORE_PATH);return false;
        }
        if(rename(REFLOOD_HISCORE_TEMP_PATH,REFLOOD_HISCORE_PATH)!=0){
            remove(REFLOOD_HISCORE_TEMP_PATH);snprintf(err,errcap,"cannot install %s",REFLOOD_HISCORE_PATH);return false;
        }
    }
    return true;
}
static unsigned scaler_output_factor(void){
    return active_scaler==REFLOOD_SCALER_SCALE2X?2u:1u;
}
static void scale2x_frame(const uint32_t *source,unsigned width,unsigned height,uint32_t *dest){
    const unsigned dest_width=width*2u;
    for(unsigned y=0;y<height;y++)for(unsigned x=0;x<width;x++){
        const uint32_t e=source[(size_t)y*width+x];
        const uint32_t b=source[(size_t)(y?y-1u:y)*width+x];
        const uint32_t d=source[(size_t)y*width+(x?x-1u:x)];
        const uint32_t f=source[(size_t)y*width+(x+1u<width?x+1u:x)];
        const uint32_t h=source[(size_t)(y+1u<height?y+1u:y)*width+x];
        uint32_t e0=e,e1=e,e2=e,e3=e;
        if(b!=h&&d!=f){
            if(d==b)e0=d;
            if(b==f)e1=f;
            if(d==h)e2=d;
            if(h==f)e3=f;
        }
        const size_t out=(size_t)(y*2u)*dest_width+x*2u;
        dest[out]=e0;dest[out+1u]=e1;dest[out+dest_width]=e2;dest[out+dest_width+1u]=e3;
    }
}
static SDL_Texture *create_stage_texture(SDL_Renderer *renderer,unsigned width,unsigned height){
    const unsigned factor=scaler_output_factor();
#if !SDL_VERSION_ATLEAST(2,0,12)
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,
        active_scaler==REFLOOD_SCALER_LINEAR?"linear":"nearest");
#endif
    SDL_Texture *texture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,(int)(width*factor),(int)(height*factor));
    if(!texture)return NULL;
#if SDL_VERSION_ATLEAST(2,0,12)
    SDL_SetTextureScaleMode(texture,active_scaler==REFLOOD_SCALER_LINEAR?
        SDL_ScaleModeLinear:SDL_ScaleModeNearest);
#endif
    return texture;
}
static int update_stage_texture(SDL_Texture *texture,const uint32_t *pixels,
    unsigned width,unsigned height){
    if(active_scaler!=REFLOOD_SCALER_SCALE2X)
        return SDL_UpdateTexture(texture,NULL,pixels,(int)(width*sizeof(*pixels)));
    const size_t needed=(size_t)width*height*4u;
    if(needed>scaled_framebuffer_pixels){
        uint32_t *larger=realloc(scaled_framebuffer,needed*sizeof(*larger));
        if(!larger)return -1;
        scaled_framebuffer=larger;scaled_framebuffer_pixels=needed;
    }
    scale2x_frame(pixels,width,height,scaled_framebuffer);
    return SDL_UpdateTexture(texture,NULL,scaled_framebuffer,
        (int)(width*2u*sizeof(*scaled_framebuffer)));
}
static void clear_present_buffers(SDL_Renderer *renderer){
    if(!renderer)return;
    SDL_SetRenderDrawColor(renderer,0u,0u,0u,255u);
    /* Present twice so both buffers used by affected SDL backends are black
       after a desktop-fullscreen transition. */
    for(unsigned n=0;n<2u;n++){SDL_RenderClear(renderer);SDL_RenderPresent(renderer);}
}
static bool apply_fullscreen(SDL_Window *window,SDL_Renderer *renderer,bool desired){
    if(SDL_SetWindowFullscreen(window,desired?SDL_WINDOW_FULLSCREEN_DESKTOP:0u)!=0)return false;
    fullscreen=desired;
    if(!desired){
        SDL_SetWindowSize(window,stage_width*2,stage_height*2);
        SDL_SetWindowPosition(window,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED);
    }
    const unsigned factor=scaler_output_factor();
    SDL_RenderSetLogicalSize(renderer,stage_width*(int)factor,stage_height*(int)factor);
    clear_present_buffers(renderer);return true;
}
static void close_game_controller(void){
    if(game_controller){SDL_GameControllerClose(game_controller);game_controller=NULL;}
}
static void open_game_controller(int device_index){
    if(game_controller||device_index<0||!SDL_IsGameController(device_index))return;
    game_controller=SDL_GameControllerOpen(device_index);
}
static void find_game_controller(void){
    if(game_controller&&SDL_GameControllerGetAttached(game_controller))return;
    close_game_controller();
    const int count=SDL_NumJoysticks();
    unsigned ordinal=0u;
    const unsigned wanted=saved_player1_choice>=2u?saved_player1_choice-2u:0u;
    for(int n=0;n<count&&!game_controller;n++)if(SDL_IsGameController(n)){
        if(ordinal++==wanted)open_game_controller(n);
    }
}
static void remove_game_controller(SDL_JoystickID instance){
    if(!game_controller)return;
    SDL_Joystick *joystick=SDL_GameControllerGetJoystick(game_controller);
    if(joystick&&SDL_JoystickInstanceID(joystick)==instance)close_game_controller();
}
static FloodInput input(SDL_Window *window,SDL_Renderer *renderer,bool text_entry,
    bool accept_both_devices){
    FloodInput i={0};SDL_Event e;bool pause_key=false,restart_level_key=false;
    while(SDL_PollEvent(&e)){
        if(e.type==SDL_QUIT)i.quit=true;
        else if(e.type==SDL_CONTROLLERDEVICEADDED){
            close_game_controller();find_game_controller();
        }
        else if(e.type==SDL_CONTROLLERDEVICEREMOVED)remove_game_controller(e.cdevice.which);
        else if(e.type==SDL_KEYDOWN){
            const SDL_Keycode key=e.key.keysym.sym;
            if(!text_entry&&!e.key.repeat){
                if(key==SDLK_p)pause_key=true;
                else if(key==SDLK_r)restart_level_key=true;
            }
            if(key==SDLK_f&&!text_entry&&!e.key.repeat){
                const bool actual=(SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN_DESKTOP)!=0u;
                const bool desired=!actual;
                apply_fullscreen(window,renderer,desired);
            }else if(key==SDLK_RETURN||key==SDLK_KP_ENTER)i.key=0x0du;
            else if(key==SDLK_BACKSPACE||key==SDLK_DELETE)i.key=0x7fu;
            else if(key>=SDLK_a&&key<=SDLK_z)i.key=(uint8_t)('A'+key-SDLK_a);
            else if(key>=SDLK_0&&key<=SDLK_9)i.key=(uint8_t)('0'+key-SDLK_0);
            else if(key==SDLK_SPACE)i.key=' ';
        }
    }
    const bool arrows=accept_both_devices||saved_player1_choice==0u;
    const bool wasd=!accept_both_devices&&saved_player1_choice==1u;
    const bool gamepad=accept_both_devices||saved_player1_choice>=2u;
    if(arrows||wasd){
        const Uint8*k=SDL_GetKeyboardState(NULL);
        i.x=arrows?(k[SDL_SCANCODE_RIGHT]?1:0)-(k[SDL_SCANCODE_LEFT]?1:0):
            (k[SDL_SCANCODE_D]?1:0)-(k[SDL_SCANCODE_A]?1:0);
        i.y=arrows?(k[SDL_SCANCODE_DOWN]?1:0)-(k[SDL_SCANCODE_UP]?1:0):
            (k[SDL_SCANCODE_S]?1:0)-(k[SDL_SCANCODE_W]?1:0);
        i.fire=arrows?k[SDL_SCANCODE_RCTRL]:k[SDL_SCANCODE_LCTRL];
        i.pause=pause_key;i.restart_level=restart_level_key;
    }
    i.restart=SDL_GetKeyboardState(NULL)[SDL_SCANCODE_ESCAPE]!=0;
    if(gamepad){
        find_game_controller();
        if(game_controller){
            const Sint16 axis_x=SDL_GameControllerGetAxis(game_controller,
                SDL_CONTROLLER_AXIS_LEFTX);
            const Sint16 axis_y=SDL_GameControllerGetAxis(game_controller,
                SDL_CONTROLLER_AXIS_LEFTY);
            const bool left=SDL_GameControllerGetButton(game_controller,
                SDL_CONTROLLER_BUTTON_DPAD_LEFT)||axis_x<-16000;
            const bool right=SDL_GameControllerGetButton(game_controller,
                SDL_CONTROLLER_BUTTON_DPAD_RIGHT)||axis_x>16000;
            const bool up=SDL_GameControllerGetButton(game_controller,
                SDL_CONTROLLER_BUTTON_DPAD_UP)||axis_y<-16000;
            const bool down=SDL_GameControllerGetButton(game_controller,
                SDL_CONTROLLER_BUTTON_DPAD_DOWN)||axis_y>16000;
            const int pad_x=(right?1:0)-(left?1:0);
            const int pad_y=(down?1:0)-(up?1:0);
            i.x+=pad_x;i.y+=pad_y;
            if(i.x>1)i.x=1;else if(i.x<-1)i.x=-1;
            if(i.y>1)i.y=1;else if(i.y<-1)i.y=-1;
            i.fire=i.fire||SDL_GameControllerGetButton(game_controller,
                SDL_CONTROLLER_BUTTON_A)!=0;
            i.restart=i.restart||SDL_GameControllerGetButton(game_controller,
                SDL_CONTROLLER_BUTTON_BACK)!=0;
        }
    }
    return i;
}
static bool pause_until_fire(SDL_Window *window,SDL_Renderer *renderer,bool initial_fire){
    /* $D370-$D38A: wait for fire down, then wait for fire up.  If fire is
       already held when P arrives, only its release remains to be observed. */
    bool fire_seen=initial_fire;Uint32 next=SDL_GetTicks();
    for(;;){
        const FloodInput i=input(window,renderer,false,false);
        if(i.quit)return true;
        if(!fire_seen){if(i.fire)fire_seen=true;}
        else if(!i.fire)return false;
        next+=20u;const Uint32 now=SDL_GetTicks();
        if((Sint32)(next-now)>0)SDL_Delay(next-now);else next=now;
    }
}
static unsigned settings_tick(ReFloodSettings *settings,int vertical,int horizontal,bool confirm){
    enum { SETTINGS_ROWS=9u };
    unsigned result=0u;
    if(!vertical)settings->vertical_ready=true;
    else if(settings->vertical_ready){
        settings->selected=(settings->selected+(vertical>0?1u:SETTINGS_ROWS-1u))%SETTINGS_ROWS;
        settings->vertical_ready=false;
    }
    if(!horizontal)settings->horizontal_ready=true;
    else if(settings->horizontal_ready){
        if(settings->selected==0u){settings->fullscreen=!settings->fullscreen;result|=1u;}
        else if(settings->selected==1u){
            settings->scaler=(ReFloodScaler)((settings->scaler+(horizontal>0?1u:2u))%
                REFLOOD_SCALER_COUNT);result|=1u;
        }else if(settings->selected==2u){
            settings->game_speed=settings->game_speed==REFLOOD_GAME_SPEED_NORMAL?
                REFLOOD_GAME_SPEED_TURBO:REFLOOD_GAME_SPEED_NORMAL;result|=1u;
        }else if(settings->selected==3u||settings->selected==4u){
            unsigned *choice=&settings->multiplayer_input[settings->selected-3u];
            *choice=(*choice+(horizontal>0?1u:3u))%4u;result|=1u;
        }
        settings->horizontal_ready=false;
    }
    if(!confirm)settings->confirm_ready=true;
    else if(settings->confirm_ready){
        if(settings->selected==0u){settings->fullscreen=!settings->fullscreen;result|=1u;}
        else if(settings->selected==1u){
            settings->scaler=(ReFloodScaler)((settings->scaler+1u)%REFLOOD_SCALER_COUNT);result|=1u;
        }else if(settings->selected==2u){
            settings->game_speed=settings->game_speed==REFLOOD_GAME_SPEED_NORMAL?
                REFLOOD_GAME_SPEED_TURBO:REFLOOD_GAME_SPEED_NORMAL;result|=1u;
        }else if(settings->selected==3u||settings->selected==4u){
            unsigned *choice=&settings->multiplayer_input[settings->selected-3u];
            *choice=(*choice+1u)%4u;result|=1u;
        }else if(settings->selected==5u)result|=2u;
        else if(settings->selected==6u)result|=8u;
        else {settings->multiplayer=settings->selected==8u;result|=4u;}
        settings->confirm_ready=false;
    }
    return result;
}
static bool settings_load_font(uint8_t font[SETTINGS_FONT_BYTES],char *err,size_t errcap){
    char path[512];snprintf(path,sizeof(path),"%s/presentation/LEVEL_font.bin",FLOOD_DATA_DIR);
    FILE *file=fopen(path,"rb");
    if(!file){snprintf(err,errcap,"cannot open %.120s",path);return false;}
    const size_t got=fread(font,1u,SETTINGS_FONT_BYTES,file);fclose(file);
    if(got!=SETTINGS_FONT_BYTES){snprintf(err,errcap,"short read from %.120s",path);return false;}
    return true;
}
static void settings_draw_text(uint32_t *fb,const uint8_t font[SETTINGS_FONT_BYTES],
    int x,int y,const char *text){
    for(;*text;text++,x+=8){
        const uint8_t ch=(uint8_t)*text;
        if(ch==':'){
            for(unsigned gy=0;gy<2u;gy++)for(unsigned gx=0;gx<2u;gx++){
                const int px=x+3+(int)gx,py0=y+1+(int)gy,py1=y+5+(int)gy;
                if(px>=0&&px<VIEW_W&&py0>=0&&py0<VIEW_H)fb[(size_t)py0*VIEW_W+px]=0xffffffffu;
                if(px>=0&&px<VIEW_W&&py1>=0&&py1<VIEW_H)fb[(size_t)py1*VIEW_W+px]=0xffffffffu;
            }
            continue;
        }
        if(ch<0x14u||ch>=0x8cu)continue;
        const uint8_t *glyph=font+(size_t)(ch-0x14u)*8u;
        for(unsigned gy=0;gy<8u;gy++)for(unsigned gx=0;gx<8u;gx++)
            if((glyph[gy]&(uint8_t)(0x80u>>gx))&&x+(int)gx>=0&&x+(int)gx<VIEW_W&&
                y+(int)gy>=0&&y+(int)gy<VIEW_H)
                fb[(size_t)(y+(int)gy)*VIEW_W+x+(int)gx]=0xffffffffu;
    }
}
static void settings_draw_centered(uint32_t *fb,const uint8_t font[SETTINGS_FONT_BYTES],
    int y,const char *text){
    settings_draw_text(fb,font,(VIEW_W-(int)strlen(text)*8)/2,y,text);
}
static void custom_draw_congratulations(uint32_t *fb,
    const uint8_t font[SETTINGS_FONT_BYTES]){
    settings_draw_centered(fb,font,104,"CONGRATULATIONS");
}
static const char *multiplayer_choice_name(unsigned choice){
    static char display_name[29];
    if(choice==0u)return "ARROWS / RCTRL";
    if(choice==1u)return "WASD / LCTRL";
    unsigned found=0u;
    for(int device=0;device<SDL_NumJoysticks();device++)if(SDL_IsGameController(device)){
        if(found++==choice-2u){
            const char *name=SDL_GameControllerNameForIndex(device);
            if(!name)return "CONTROLLER";
            size_t count=0u;
            while(name[count]&&count<sizeof(display_name)-1u){
                unsigned char ch=(unsigned char)name[count];
                display_name[count]=(char)(ch>='a'&&ch<='z'?ch-'a'+'A':
                    ch>=32u&&ch<=126u?ch:'?');
                count++;
            }
            display_name[count]='\0';return display_name;
        }
    }
    return "CONTROLLER DISCONNECTED";
}
static bool multiplayer_choice_connected(unsigned choice){
    if(choice<2u)return true;
    unsigned found=0u;
    for(int device=0;device<SDL_NumJoysticks();device++)
        if(SDL_IsGameController(device))found++;
    return found>choice-2u;
}
typedef enum {CUSTOM_BROWSER_CANCEL,CUSTOM_BROWSER_SELECTED,CUSTOM_BROWSER_CLEARED,
    CUSTOM_BROWSER_QUIT} CustomBrowserResult;
static CustomBrowserResult run_custom_browser(SDL_Window *window,SDL_Renderer *renderer,
    SDL_Texture *texture,uint32_t *fb,const uint8_t font[SETTINGS_FONT_BYTES],
    char *chosen,size_t chosen_capacity);
static void render_settings(SDL_Renderer *renderer,SDL_Texture *texture,uint32_t *fb,
    const uint8_t font[SETTINGS_FONT_BYTES],const ReFloodSettings *settings){
    char display[32],scaler[32],game_speed[32],p1[40],p2[40];
    for(size_t n=0;n<(size_t)VIEW_W*VIEW_H;n++)fb[n]=0xff000000u;
    settings_draw_centered(fb,font,15,"SETTINGS");
    snprintf(display,sizeof(display),"DISPLAY: %s",settings->fullscreen?"FULLSCREEN":"WINDOWED");
    snprintf(scaler,sizeof(scaler),"SCALER: %s",scaler_menu_name(settings->scaler));
    snprintf(game_speed,sizeof(game_speed),"GAME SPEED: %s",
        game_speed_menu_name(settings->game_speed));
    snprintf(p1,sizeof(p1),"P1: %.28s",multiplayer_choice_name(settings->multiplayer_input[0]));
    snprintf(p2,sizeof(p2),"P2: %.28s",multiplayer_choice_name(settings->multiplayer_input[1]));
    const char *rows[]={display,scaler,game_speed,p1,p2,
        "SAVE SETTINGS","CHOOSE CUSTOM MAP","START SINGLEPLAYER GAME",
        "START MULTIPLAYER (EXPERIMENTAL)"};
    for(unsigned row=0;row<9u;row++)settings_draw_centered(fb,font,32+(int)row*18,rows[row]);
    const char *selected=rows[settings->selected];
    const int selected_y=32+(int)settings->selected*18;
    settings_draw_text(fb,font,(VIEW_W-(int)strlen(selected)*8)/2-16,
        selected_y,">");
    if(settings->multiplayer_input[0]==settings->multiplayer_input[1]||
       !multiplayer_choice_connected(settings->multiplayer_input[0])||
       !multiplayer_choice_connected(settings->multiplayer_input[1]))
        settings_draw_centered(fb,font,196,"CHOOSE TWO CONNECTED INPUTS");
    else if(settings->custom_map){
        const char *name=strrchr(settings->custom_header,'/');
        const char *backslash=strrchr(settings->custom_header,'\\');
        if(backslash&&(!name||backslash>name))name=backslash;
        name=name?name+1:settings->custom_header;
        char status[40];snprintf(status,sizeof(status),"MAP: %.33s",name);
        settings_draw_centered(fb,font,196,status);
    }
    update_stage_texture(texture,fb,VIEW_W,VIEW_H);
    SDL_RenderClear(renderer);SDL_RenderCopy(renderer,texture,NULL,NULL);SDL_RenderPresent(renderer);
}
static bool run_settings(SDL_Window *window,SDL_Renderer *renderer,SDL_Texture *texture,
    uint32_t *fb,ReFloodSettings *settings,char *err,size_t errcap){
    uint8_t font[SETTINGS_FONT_BYTES];
    if(!settings_load_font(font,err,errcap))return false;
    bool start=false;Uint32 next=SDL_GetTicks();
    render_settings(renderer,texture,fb,font,settings);
    while(!start){
        FloodInput i=input(window,renderer,true,true);if(i.quit)return false;
        const bool confirm=i.fire||i.key==0x0du;
        const unsigned action=settings_tick(settings,i.y,i.x,confirm);
        if(action&2u){
            bool applied=true;
            if(settings->fullscreen!=fullscreen)
                applied=apply_fullscreen(window,renderer,settings->fullscreen);
            if(!applied)settings->fullscreen=fullscreen;
            else if(!settings_save_configuration(settings,err,errcap))
                fprintf(stderr,"configuration: %s\n",err);
            else {saved_scaler=settings->scaler;saved_game_speed=settings->game_speed;
                saved_player1_choice=settings->multiplayer_input[0];}
        }
        if(action&4u){
            if(!settings->multiplayer||
               (settings->multiplayer_input[0]!=settings->multiplayer_input[1]&&
                multiplayer_choice_connected(settings->multiplayer_input[0])&&
                multiplayer_choice_connected(settings->multiplayer_input[1])))start=true;
        }
        if(action&8u){
            char selection[EDITOR_BROWSER_PATH];
            const CustomBrowserResult result=run_custom_browser(window,renderer,texture,fb,
                font,selection,sizeof(selection));
            if(result==CUSTOM_BROWSER_QUIT)return false;
            if(result==CUSTOM_BROWSER_SELECTED){
                strcpy(settings->custom_header,selection);settings->custom_map=true;
            }else if(result==CUSTOM_BROWSER_CLEARED){
                settings->custom_header[0]='\0';settings->custom_map=false;
            }
        }
        render_settings(renderer,texture,fb,font,settings);
        next+=PRESENTATION_FRAME_MS;const Uint32 now=SDL_GetTicks();
        if((Sint32)(next-now)>0)SDL_Delay(next-now);else next=now;
    }
    saved_player1_choice=settings->multiplayer_input[0];
    close_game_controller();
    return true;
}
static void stage_size(SDL_Window *window,SDL_Renderer *renderer,int width,int height){
    stage_width=width;stage_height=height;
    /* Desktop fullscreen survives logical-size changes. Reissuing the mode
       request here could leave SDL's cached and actual fullscreen states out
       of sync, making a later F toggle fail. */
    if(!fullscreen){
        SDL_SetWindowSize(window,width*2,height*2);
        SDL_SetWindowPosition(window,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED);
    }
    const unsigned factor=scaler_output_factor();
    SDL_RenderSetLogicalSize(renderer,width*(int)factor,height*(int)factor);
    clear_present_buffers(renderer);
}
static bool intro_music_lead(SDL_Window *window,SDL_Renderer *renderer,
    SDL_Texture *texture,uint32_t *fb){
    memset(fb,0,VIEW_W*VIEW_H*sizeof(*fb));
    update_stage_texture(texture,fb,VIEW_W,VIEW_H);
    SDL_RenderClear(renderer);SDL_RenderCopy(renderer,texture,NULL,NULL);
    SDL_RenderPresent(renderer);
    const Uint32 end=SDL_GetTicks()+INTRO_MUSIC_LEAD_MS;
    while((Sint32)(end-SDL_GetTicks())>0){
        FloodInput i=input(window,renderer,false,false);if(i.quit)return true;
        Uint32 remaining=end-SDL_GetTicks();SDL_Delay(remaining>20u?20u:remaining);
    }
    return false;
}
static void audio_callback(void *userdata,Uint8 *stream,int bytes){
    flood_audio_mix((FloodAudio *)userdata,(int16_t *)stream,(size_t)bytes/(2u*sizeof(int16_t)));
}
static bool wait_audio_idle(SDL_Window *window,SDL_Renderer *renderer,FloodAudio *audio,SDL_AudioDeviceID device){
    if(!device)return false;
    for(;;){
        SDL_LockAudioDevice(device);
        const bool playing=flood_audio_is_playing(audio);
        SDL_UnlockAudioDevice(device);
        if(!playing)return false;
        FloodInput i=input(window,renderer,false,false);if(i.quit)return true;
        SDL_Delay(10u);
    }
}
static void composite_sprite(uint8_t *fb,const FloodGame *g,unsigned sprite_id,int x,int y,int camx,int camy){
    const FloodSprite *s=flood_sprite(g,sprite_id); if(!s)return;
    for(unsigned sy=0;sy<s->height;sy++)for(unsigned sx=0;sx<s->width;sx++){
        const unsigned pi=sy*FLOOD_MAX_SPRITE_W+sx; if(!s->mask[pi])continue;
        const int dx=x+(int)sx-camx+FLOOD_BACKING_X,dy=y+(int)sy-camy;
        if(dx<0||dy<0||dx>=FLOOD_BACKING_W||dy>=VIEW_H)continue;
        fb[dy*FLOOD_BACKING_W+dx]=(uint8_t)(s->pixels[pi]&15u);
    }
}
static void composite_sprite_variant(uint8_t *fb,const FloodGame *g,unsigned sprite_id,
    int x,int y,int camx,int camy,bool second,bool ghost){
    if(!second){composite_sprite(fb,g,sprite_id,x,y,camx,camy);return;}
    const FloodSprite *s=flood_sprite(g,sprite_id);if(!s)return;
    for(unsigned sy=0;sy<s->height;sy++)for(unsigned sx=0;sx<s->width;sx++){
        const unsigned pi=sy*FLOOD_MAX_SPRITE_W+sx;if(!s->mask[pi])continue;
        const int dx=x+(int)sx-camx+FLOOD_BACKING_X,dy=y+(int)sy-camy;
        if(dx<0||dy<0||dx>=FLOOD_BACKING_W||dy>=VIEW_H)continue;
        uint8_t colour=flood_multiplayer_actor_colour(s->pixels[pi]);
        /* The source goggles use their original cool lens and cream glare.
           Matilda's two eye pixels occupy opposite positions when facing left. */
        if(!ghost&&(s->pixels[pi]==3u||s->pixels[pi]==15u))colour=s->pixels[pi];
        if(ghost&&sprite_id>=0xd2u&&sprite_id<=0xd9u&&sy==13u&&
            (sprite_id<=0xd5u?(sx==12u||sx==14u):(sx==17u||sx==19u)))
            colour=16u; /* Red eye RGB is resolved after the indexed world pass. */
        fb[dy*FLOOD_BACKING_W+dx]=colour;
    }
}
static void composite_hud(uint8_t *fb,const FloodGame *g,int camx){
    /* The Amiga backing-buffer X and BPLCON1 delay vary with camx&15, but
       their sum is always fetch X=16, which is visible-window X=0. */
    const FloodHudPlacement placement=flood_hud_placement((uint16_t)camx);
    const int origin_x=placement.visible_x+FLOOD_BACKING_X,origin_y=FLOOD_HUD_VISIBLE_Y;
    for(unsigned y=0;y<FLOOD_HUD_H;y++)for(unsigned bx=0;bx<FLOOD_HUD_ROW_BYTES;bx++){
        const uint8_t bits=g->hud_mask[y*FLOOD_HUD_ROW_BYTES+bx];
        const uint8_t colour=(uint8_t)(bx<4u?13u:bx>=36u?5u:15u);
        for(unsigned bit=0;bit<8u;bit++)if(bits&(uint8_t)(0x80u>>bit))
            fb[(origin_y+(int)y)*FLOOD_BACKING_W+origin_x+(int)bx*8+(int)bit]=colour;
    }
}
static void render_world_panel(SDL_Renderer *r,SDL_Texture *tex,const FloodGame *g,
    uint32_t *fb,uint8_t *world,const FloodGame *other,unsigned slot,
    const SDL_Rect *destination,const SDL_Rect *source){
    int camx=g->camera_x,camy=g->camera_y;
    flood_build_tile_backing(g,world);
    if(g->zap_message_pending){
        for(unsigned y=0;y<VIEW_H;y++)for(unsigned x=0;x<VIEW_W;x++)
            fb[y*VIEW_W+x]=palette[world[y*FLOOD_BACKING_W+x+FLOOD_BACKING_X]&15u];
        update_stage_texture(tex,fb,VIEW_W,VIEW_H);
        if(!destination)SDL_RenderClear(r);
        SDL_RenderCopy(r,tex,source,destination);
        if(!destination)SDL_RenderPresent(r);
        return;
    }
    for(unsigned n=0;n<g->action_render_count&&!g->render_dispatch_suppressed;n++){
        const FloodActionRender *a=&g->action_renders[n];
        composite_sprite(world,g,a->sprite_id,a->x,a->y,camx,camy);
    }
    for(unsigned n=0;n<FLOOD_OBJECT_MAX&&!g->render_dispatch_suppressed;n++)if(g->objects[n].active&&g->objects[n].state!=19u){
        const FloodObject *o=&g->objects[n];
        if(o->state==22u){
            if(o->state_flags&1u){
                if(o->anim)composite_sprite(world,g,o->sprite,o->x,o->y,camx,camy);
            }else{
                composite_sprite(world,g,o->sprite_base,o->x+16,o->y,camx,camy);
                composite_sprite(world,g,(unsigned)(o->sprite_base+2u),o->x,o->y,camx,camy);
            }
            composite_sprite(world,g,(unsigned)(o->sprite_base+3u),o->origin_x,o->origin_y,camx,camy);
        }else if(o->state==1u||o->state==5u||o->state==6u||o->state==8u||o->state==9u||o->state==10u||o->state==12u||o->state==14u||o->state==15u){
            if(o->sprite)composite_sprite(world,g,o->sprite,o->render_x,o->render_y,camx,camy);
        }
        else if(o->state==2u||o->state==13u||o->state==20u||o->state==21u){
            if(o->sprite)composite_sprite(world,g,o->sprite,o->render_x,o->render_y,camx,camy);
        }
    }
    if(other){
        /* Attachments belong to actors, so draw the remote one in this
           camera too, including during their protected blink frame. */
        if(other->lives>0&&other->player.parachute_timer)
            composite_sprite(world,g,0xc6u,other->player.x,
                other->player.y-24,camx,camy);
        if(other->lives>0&&other->player.balloon_timer)
            composite_sprite(world,g,0xc7u,other->player.x,
                other->player.y-24,camx,camy);
        for(unsigned n=0;n<other->action_render_count&&!other->render_dispatch_suppressed;n++){
            const FloodActionRender *a=&other->action_renders[n];
            composite_sprite(world,g,a->sprite_id,a->x,a->y,camx,camy);
        }
        if(other->lives>0&&other->matilda.visible)
            composite_sprite_variant(world,g,other->matilda.sprite_id,
                other->matilda.x,other->matilda.y,camx,camy,slot==0u,true);
        /* The other actor uses the shared display-buffer phase for the same
           protection blink as the local actor. Their private state stores
           the pose, including the original death-cross animation. */
        if((other->lives>0||other->player.death_mode)&&
            (!(other->player_blink_active||other->player.invulnerable_timer)||
                g->render_buffer_index==0u))
            composite_sprite_variant(world,g,flood_player_sprite_id(other),
                other->player.x,other->player.y,camx,camy,
                slot==0u&&other->player.death_mode!=2u,false);
        if(other->ouch_visible)
            composite_sprite(world,g,0xa4u,other->player.x+8,
                other->player.y-16,camx,camy);
    }
    if((!other||g->lives>0)&&g->matilda.visible)
        composite_sprite_variant(world,g,g->matilda.sprite_id,g->matilda.x,
            g->matilda.y,camx,camy,slot==1u,true);
    /* $D01C-$D076 draws the carried parachute/balloon before Quiffy's
       protection blink gate, so the attachment remains visible on hidden
       player phases. */
    if(g->player.parachute_timer)
        composite_sprite(world,g,0xc6u,g->player.x,g->player.y-24,camx,camy);
    if(g->player.balloon_timer)
        composite_sprite(world,g,0xc7u,g->player.x,g->player.y-24,camx,camy);
    if(g->quiffy_effect_render_active)
        composite_sprite(world,g,g->quiffy_effect_sprite,g->quiffy_effect_render_x,
            g->quiffy_effect_render_y,camx,camy);
    else if((!other||g->lives>0||g->player.death_mode)&&flood_player_should_draw(g)){
        const int px=g->transport_render_prejump?g->transport_render_x:g->player.x;
        const int py=g->transport_render_prejump?g->transport_render_y:g->player.y;
        /* $D0F4-$D12C: sustained flame draws a second 32-pixel piece beside
           Quiffy.  Normal fire uses $D0/$D1; failure offsets it to $DA/$DD. */
        if(g->weapon_pose_active&&g->fire_ticks>=4u)
            composite_sprite(world,g,flood_weapon_tip_sprite_id(g),
                px+(g->player.hbank-2)*16,py,camx,camy);
        composite_sprite_variant(world,g,flood_player_sprite_id(g),px,py,camx,camy,
            slot==1u&&g->player.death_mode!=2u,false);
    }
    if(g->ouch_visible)
        composite_sprite(world,g,0xa4u,g->player.x+8,g->player.y-16,camx,camy);
    /* $ED66 follows the complete $CAD2 actor dispatcher, so dynamic water
       modifies its two destination planes after every sprite has drawn. */
    flood_composite_overlay_backing(g,world);
    composite_hud(world,g,camx);
    for(unsigned y=0;y<VIEW_H;y++)for(unsigned x=0;x<VIEW_W;x++)
        {
            const uint8_t colour=flood_transport_effect_pixel(world,x,y,
                g->transport_effect_stage);
            fb[y*VIEW_W+x]=(colour&16u)?0xffe33224u:palette[colour&15u];
        }
    update_stage_texture(tex,fb,VIEW_W,VIEW_H);
    if(!destination)SDL_RenderClear(r);
    SDL_RenderCopy(r,tex,source,destination);
    if(!destination)SDL_RenderPresent(r);
}
static void render_world(SDL_Renderer *r,SDL_Texture *tex,const FloodGame *g,uint32_t *fb,uint8_t *world){
    render_world_panel(r,tex,g,fb,world,NULL,0u,NULL,NULL);
}
static uint32_t ending_colour(uint16_t amiga){
    return 0xff000000u|((uint32_t)((amiga>>8)&15u)*17u<<16)|
        ((uint32_t)((amiga>>4)&15u)*17u<<8)|(uint32_t)(amiga&15u)*17u;
}
static void render_ending(SDL_Renderer *r,SDL_Texture *tex,const FloodEnding *ending,uint32_t *fb){
    for(unsigned y=0;y<FLOOD_ENDING_H;y++)for(unsigned x=0;x<FLOOD_ENDING_W;x++)
        fb[y*FLOOD_ENDING_W+x]=ending_colour(ending->palette[flood_ending_pixel(ending,x,y)]);
    update_stage_texture(tex,fb,FLOOD_ENDING_W,FLOOD_ENDING_H);
    SDL_RenderClear(r);SDL_RenderCopy(r,tex,NULL,NULL);SDL_RenderPresent(r);
}
static void render_intro(SDL_Renderer *r,SDL_Texture *tex,const FloodIntro *intro,uint32_t *fb){
    for(unsigned y=0;y<FLOOD_INTRO_H;y++)for(unsigned x=0;x<FLOOD_INTRO_W;x++)
        fb[y*FLOOD_INTRO_W+x]=ending_colour(intro->palette[flood_intro_pixel(intro,x,y)]);
    update_stage_texture(tex,fb,FLOOD_INTRO_W,FLOOD_INTRO_H);
    SDL_RenderClear(r);SDL_RenderCopy(r,tex,NULL,NULL);SDL_RenderPresent(r);
}
static void render_title(SDL_Renderer *r,SDL_Texture *tex,const FloodTitle *title,uint32_t *fb){
    for(unsigned y=0;y<FLOOD_TITLE_H;y++)for(unsigned x=0;x<FLOOD_TITLE_W;x++)
        fb[y*FLOOD_TITLE_W+x]=ending_colour(title->palette[flood_title_pixel(title,x,y)]);
    update_stage_texture(tex,fb,FLOOD_TITLE_W,FLOOD_TITLE_H);
    SDL_RenderClear(r);SDL_RenderCopy(r,tex,NULL,NULL);SDL_RenderPresent(r);
}
static void render_level_select(SDL_Renderer *r,SDL_Texture *tex,
    const FloodLevelSelect *select,uint32_t *fb){
    for(unsigned y=0;y<FLOOD_LEVEL_SELECT_H;y++)for(unsigned x=0;x<FLOOD_LEVEL_SELECT_W;x++)
        fb[y*FLOOD_LEVEL_SELECT_W+x]=ending_colour(
            select->palette[flood_level_select_pixel(select,x,y)]);
    update_stage_texture(tex,fb,FLOOD_LEVEL_SELECT_W,FLOOD_LEVEL_SELECT_H);
    SDL_RenderClear(r);SDL_RenderCopy(r,tex,NULL,NULL);SDL_RenderPresent(r);
}
static void render_zap(SDL_Renderer *r,SDL_Texture *tex,
    const FloodZapMessage *message,uint32_t *fb,const uint8_t *world){
    for(unsigned y=0;y<VIEW_H;y++)for(unsigned x=0;x<VIEW_W;x++){
        uint8_t colour=world[(size_t)y*FLOOD_BACKING_W+x+FLOOD_BACKING_X];
        if(x>=FLOOD_ZAP_X&&x<FLOOD_ZAP_X+FLOOD_ZAP_W&&
           y>=FLOOD_ZAP_Y&&y<FLOOD_ZAP_Y+FLOOD_ZAP_H)
            colour=flood_zap_message_pixel(message,x-FLOOD_ZAP_X,y-FLOOD_ZAP_Y);
        fb[y*VIEW_W+x]=ending_colour(message->palette[colour&15u]);
    }
    update_stage_texture(tex,fb,VIEW_W,VIEW_H);
    SDL_RenderClear(r);SDL_RenderCopy(r,tex,NULL,NULL);SDL_RenderPresent(r);
}
static void build_level_banner_frame(const FloodGame *g,int banner_x,
    uint32_t *fb,uint8_t *world){
    flood_build_tile_backing(g,world);composite_hud(world,g,g->camera_x);
    for(unsigned n=0;n<20u;n++){
        composite_sprite(world,g,0x4au,g->camera_x+(int)n*16,g->camera_y+100,
            g->camera_x,g->camera_y);
        composite_sprite(world,g,0x4bu,g->camera_x+(int)n*16,g->camera_y+116,
            g->camera_x,g->camera_y);
    }
    static const uint8_t decoration[5]={0x47u,0x48u,0x49u,0x48u,0x47u};
    static const int offset[5]={0,14,28,42,56};
    for(unsigned n=0;n<5u;n++)if(banner_x+offset[n]>0)
        composite_sprite(world,g,decoration[n],g->camera_x+banner_x+offset[n],
            g->camera_y+108,g->camera_x,g->camera_y);
    if(banner_x+84>0)composite_sprite(world,g,(unsigned)(0x3du+g->world.level_number/10u),
        g->camera_x+banner_x+84,g->camera_y+108,g->camera_x,g->camera_y);
    if(banner_x+98>0)composite_sprite(world,g,(unsigned)(0x3du+g->world.level_number%10u),
        g->camera_x+banner_x+96,g->camera_y+108,g->camera_x,g->camera_y);
    for(unsigned y=0;y<VIEW_H;y++)for(unsigned x=0;x<VIEW_W;x++)
        fb[y*VIEW_W+x]=palette[world[y*FLOOD_BACKING_W+x+FLOOD_BACKING_X]&15u];
}
static void render_level_banner(SDL_Renderer *r,SDL_Texture *tex,
    const FloodGame *g,int banner_x,uint32_t *fb,uint8_t *world){
    build_level_banner_frame(g,banner_x,fb,world);
    update_stage_texture(tex,fb,VIEW_W,VIEW_H);
    SDL_RenderClear(r);SDL_RenderCopy(r,tex,NULL,NULL);SDL_RenderPresent(r);
}
static bool play_level_banner(SDL_Window *window,SDL_Renderer *r,SDL_Texture *tex,FloodGame *g,
    FloodAudio *audio,SDL_AudioDeviceID audio_device,uint32_t *fb,uint8_t *world){
    Uint32 next=SDL_GetTicks();
    bool level_start_sound_deferred=false;
    for(unsigned frame=0;frame<FLOOD_LEVEL_BANNER_FRAMES&&g->running;frame++){
        FloodInput i=input(window,r,false,false);if(i.quit)return true;
        flood_game_level_banner_tick(g,i,frame==0u);
        if(audio_device){SDL_LockAudioDevice(audio_device);
            for(unsigned n=0;n<g->sound_queue_count;n++){
                /* $D07C-$D0A2 queues protection-start sound $34 on the
                   banner's first gameplay update.  Hold that one cue until
                   the blocking banner has completed; other effects retain
                   their ordinary dispatch timing. */
                if(frame==0u&&g->sound_queue[n]==52u)
                    level_start_sound_deferred=true;
                else flood_audio_trigger(audio,g->sound_queue[n]);
            }
            SDL_UnlockAudioDevice(audio_device);
        }
        render_level_banner(r,tex,g,106,fb,world);
        next+=GAME_FRAME_MS;const Uint32 now=SDL_GetTicks();if((Sint32)(next-now)>0)SDL_Delay(next-now);
    }
    /* The actual level-start cue is protection-start sound $34.  Start its
       paired tracks at the transition into active play.  Milestone 128's
       separate sound-7 injection was based on a resource index and is gone. */
    if(g->running&&audio_device&&level_start_sound_deferred){
        SDL_LockAudioDevice(audio_device);flood_audio_trigger(audio,52u);
        SDL_UnlockAudioDevice(audio_device);
    }
    return false;
}
static void render_protection(SDL_Renderer *r,SDL_Texture *tex,
    const FloodProtection *protection,uint32_t *fb){
    for(unsigned y=0;y<FLOOD_PROTECTION_H;y++)for(unsigned x=0;x<FLOOD_PROTECTION_W;x++)
        fb[y*FLOOD_PROTECTION_W+x]=ending_colour(
            protection->palette[flood_protection_pixel(protection,x,y)]);
    update_stage_texture(tex,fb,FLOOD_PROTECTION_W,FLOOD_PROTECTION_H);
    SDL_RenderClear(r);SDL_RenderCopy(r,tex,NULL,NULL);SDL_RenderPresent(r);
}
static void render_high_scores(SDL_Renderer *r,SDL_Texture *tex,
    const FloodHighScores *scores,uint32_t *fb){
    for(unsigned y=0;y<FLOOD_HIGHSCORE_H;y++)for(unsigned x=0;x<FLOOD_HIGHSCORE_W;x++)
        fb[y*FLOOD_HIGHSCORE_W+x]=ending_colour(
            scores->palette[flood_high_scores_pixel(scores,x,y)]);
    update_stage_texture(tex,fb,FLOOD_HIGHSCORE_W,FLOOD_HIGHSCORE_H);
    SDL_RenderClear(r);SDL_RenderCopy(r,tex,NULL,NULL);SDL_RenderPresent(r);
}
static void fade_out(SDL_Renderer *r,SDL_Texture *tex,uint32_t *fb,unsigned width,unsigned height){
    Uint32 next=SDL_GetTicks();
    for(unsigned step=0;step<16u;step++){
        for(unsigned n=0;n<width*height;n++){
            const uint32_t c=fb[n];uint32_t faded=0xff000000u;
            for(unsigned shift=0;shift<=16u;shift+=8u){
                const uint32_t component=(c>>shift)&0xffu;
                faded|=(component>17u?component-17u:0u)<<shift;
            }
            fb[n]=faded;
        }
        update_stage_texture(tex,fb,width,height);
        SDL_RenderClear(r);SDL_RenderCopy(r,tex,NULL,NULL);SDL_RenderPresent(r);
        next+=20u;const Uint32 now=SDL_GetTicks();if((Sint32)(next-now)>0)SDL_Delay(next-now);
    }
}
static bool protection_exit_fades(const FloodProtection *protection,bool quit){
    /* $AD66 follows a successful initial language/copy-protection route.
       Closing the host or failing the challenge is not a gameplay handoff. */
    return protection->passed&&!quit;
}
static bool game_over_exit_fades(const FloodGame *g,bool quit){
    /* The common original gameplay exit at $B2FC calls $A49E before the
       final-life path constructs the credits/high-score display. */
    return !quit&&!g->game_complete&&g->lives<=0;
}
static void reset_after_game_over(FloodGame *g,unsigned *level){
    /* The original post-score path resets lives, selected cavern, and score
       before returning directly to PLAY LEVEL. */
    flood_game_init(g);*level=1u;
}
static bool protection_required(bool direct_level_after_scores){
    /* Language/copy protection belongs to the initial front-end route only.
       The post-score PLAY LEVEL panel returns straight to the cavern. */
    return !direct_level_after_scores;
}
static size_t presentation_framebuffer_pixels(void){
    return (size_t)FLOOD_INTRO_W*FLOOD_INTRO_H;
}
static bool ending_opens_post_score_loop(bool ending_finished,bool quit){
    return ending_finished&&!quit;
}
typedef struct {
    unsigned screen_frames;
    bool show_scores,fire_released;
} PostScoreAttract;
static PostScoreAttract post_score_attract_begin(void){
    PostScoreAttract state={0u,true,false};return state;
}
static bool post_score_attract_fire(PostScoreAttract *state,bool fire){
    if(!fire){state->fire_released=true;return false;}
    return state->fire_released;
}
static void post_score_attract_advance(PostScoreAttract *state){
    state->screen_frames++;
    if(state->screen_frames==POST_SCORE_SCREEN_FRAMES){
        state->screen_frames=0u;state->show_scores=!state->show_scores;
    }
}
static bool run_post_score_loop(SDL_Window *window,SDL_Renderer *renderer,
    SDL_Texture **texture,uint32_t *fb,uint8_t *finished_background,
    uint32_t score,char *err,size_t errcap,bool *quit,
    bool *selector_over_scores){
    FloodHighScores scores;FloodTitle title;
    if(!flood_high_scores_load(&scores,FLOOD_DATA_DIR,0u,err,errcap)){
        fprintf(stderr,"high scores: %s\n",err);return false;
    }
    char score_err[160]={0};
    if(!high_scores_load_configuration(&scores,score_err,sizeof(score_err))&&score_err[0])
        fprintf(stderr,"high scores: %s; using original table\n",score_err);
    flood_high_scores_enter(&scores,score);
    const bool score_changed=scores.entry_index>=0;
    if(!flood_title_load(&title,FLOOD_DATA_DIR,err,errcap)){
        fprintf(stderr,"title: %s\n",err);return false;
    }
    SDL_DestroyTexture(*texture);*texture=create_stage_texture(renderer,
        FLOOD_HIGHSCORE_W,FLOOD_HIGHSCORE_H);
    stage_size(window,renderer,FLOOD_HIGHSCORE_W,FLOOD_HIGHSCORE_H);
    Uint32 next=SDL_GetTicks();
    /* A qualifying score completes the original name editor before the
       ordinary title/score attract loop takes control. */
    while(*texture&&scores.editing&&!*quit){
        FloodInput i=input(window,renderer,false,false);if(i.quit)*quit=true;
        flood_high_scores_tick(&scores,i.x,i.y,i.fire);
        render_high_scores(renderer,*texture,&scores,fb);
        next+=20u;const Uint32 now=SDL_GetTicks();if((Sint32)(next-now)>0)SDL_Delay(next-now);
    }
    if(score_changed&&!scores.editing&&!*quit&&
        !high_scores_save_configuration(&scores,score_err,sizeof(score_err)))
        fprintf(stderr,"high scores: %s\n",score_err);
    if(!*texture||*quit)return false;

    PostScoreAttract attract=post_score_attract_begin();
    render_high_scores(renderer,*texture,&scores,fb);
    while(!*quit){
        FloodInput i=input(window,renderer,false,false);if(i.quit){*quit=true;break;}
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
#include "multiplayer_host.inc"
#include "custom_map_host.inc"
int main(int argc,char **argv){
    unsigned level=1;if(argc>1)level=(unsigned)strtoul(argv[1],NULL,10);
    ReFloodSettings settings=settings_defaults();settings_load_configuration(&settings);
    saved_scaler=settings.scaler;
    saved_game_speed=settings.game_speed;
    saved_player1_choice=settings.multiplayer_input[0];
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_EVENTS|SDL_INIT_GAMECONTROLLER))return 1;
    find_game_controller();
    fullscreen=settings.fullscreen;
    SDL_Window*w=SDL_CreateWindow(REFLOOD_WINDOW_TITLE,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
        VIEW_W*2,VIEW_H*2,settings.fullscreen?SDL_WINDOW_FULLSCREEN_DESKTOP:0u);
    SDL_Renderer*r=SDL_CreateRenderer(w,-1,SDL_RENDERER_ACCELERATED);
    SDL_RenderSetLogicalSize(r,VIEW_W,VIEW_H);
    SDL_Texture*tex=create_stage_texture(r,VIEW_W,VIEW_H);
    /* The intro retains a 320x240 logical canvas even though the other
       presentation screens expose 208 rows.  Size the shared buffer for the
       largest stage rather than coupling it to the title viewport. */
    uint32_t*fb=malloc(presentation_framebuffer_pixels()*sizeof(*fb));char err[160]={0};
    if(!w||!r||!tex||!fb){free(fb);close_game_controller();SDL_Quit();return 3;}
    /* This is deliberately the first interactive state. No level, intro, or
       audio content is initialized until a start option has been selected. */
show_settings_menu:
    if(!run_settings(w,r,tex,fb,&settings,err,sizeof(err))){
        if(err[0])fprintf(stderr,"settings: %s\n",err);
        free(fb);SDL_DestroyTexture(tex);SDL_DestroyRenderer(r);SDL_DestroyWindow(w);
        close_game_controller();SDL_Quit();return 0;
    }
    if(settings.custom_map&&!settings.multiplayer){
        active_scaler=settings.scaler;
        const bool quit=run_custom_map_session(w,r,&tex,fb,settings.custom_header);
        if(!quit&&tex)goto show_settings_menu;
        free(scaled_framebuffer);free(fb);SDL_DestroyTexture(tex);
        SDL_DestroyRenderer(r);SDL_DestroyWindow(w);close_game_controller();
        SDL_Quit();return tex?0:3;
    }
    if(settings.multiplayer){
        int result=0;bool custom_finished=false;
        if(settings.multiplayer_input[0]==settings.multiplayer_input[1]){
            fprintf(stderr,"multiplayer: assign different devices to the two players\n");
            result=2;
        }else{
            active_scaler=settings.scaler;
            if(settings.custom_map){
                uint32_t scores[2];bool both_dead=false,completed=false;
                result=mp_run_gameplay(w,r,&settings,level,0u,scores,&both_dead,&completed,
                    settings.custom_header);
                custom_finished=both_dead||completed;
            }else result=run_experimental_multiplayer(w,r,&tex,fb,&settings,level);
        }
        if(custom_finished&&result==0){stage_size(w,r,VIEW_W,VIEW_H);goto show_settings_menu;}
        free(scaled_framebuffer);free(fb);SDL_DestroyTexture(tex);
        SDL_DestroyRenderer(r);SDL_DestroyWindow(w);close_game_controller();
        SDL_Quit();return result;
    }
    active_scaler=saved_scaler;
    SDL_DestroyTexture(tex);tex=create_stage_texture(r,VIEW_W,VIEW_H);
    stage_size(w,r,VIEW_W,VIEW_H);
    if(!tex){free(fb);SDL_DestroyRenderer(r);SDL_DestroyWindow(w);
        close_game_controller();SDL_Quit();return 3;}
    uint8_t*world=malloc(FLOOD_BACKING_PIXELS);FloodGame g;flood_game_init(&g);
    uint8_t post_score_background[4*FLOOD_LEVEL_SELECT_PLANE_BYTES];
    bool level_select_over_scores=false;
    bool direct_level_after_scores=false;
    if(!world){free(fb);SDL_DestroyTexture(tex);SDL_DestroyRenderer(r);SDL_DestroyWindow(w);
        close_game_controller();SDL_Quit();return 3;}
    if(!flood_game_load_level(&g,FLOOD_DATA_DIR,level,err,sizeof(err))){
        fprintf(stderr,"load level %u: %s\n",level,err);free(world);free(fb);
        SDL_DestroyTexture(tex);SDL_DestroyRenderer(r);SDL_DestroyWindow(w);
        close_game_controller();SDL_Quit();return 2;
    }
    FloodAudio audio;SDL_AudioDeviceID audio_device=0;SDL_AudioSpec wanted={0},obtained={0};
    wanted.freq=44100;wanted.format=AUDIO_S16SYS;wanted.channels=2;wanted.samples=1024;
    wanted.callback=audio_callback;wanted.userdata=&audio;
    if(flood_music_load(&audio,FLOOD_DATA_DIR,(uint32_t)wanted.freq,err,sizeof(err))){
        audio_device=SDL_OpenAudioDevice(NULL,0,&wanted,&obtained,0);
        if(audio_device)SDL_PauseAudioDevice(audio_device,0);
    }else fprintf(stderr,"audio: %s\n",err);
    FloodIntro intro;bool intro_quit=false;
    intro_quit=intro_music_lead(w,r,tex,fb);
    if(!flood_intro_load(&intro,FLOOD_DATA_DIR,err,sizeof(err)))fprintf(stderr,"intro: %s\n",err);
    else if(!intro_quit){
        SDL_DestroyTexture(tex);tex=create_stage_texture(r,FLOOD_INTRO_W,FLOOD_INTRO_H);
        stage_size(w,r,FLOOD_INTRO_W,FLOOD_INTRO_H);
        Uint32 next=SDL_GetTicks();
        while(tex&&!intro.finished&&!intro_quit){
            FloodInput i=input(w,r,false,false);if(i.quit)intro_quit=true;
            flood_intro_tick(&intro,i.fire);render_intro(r,tex,&intro,fb);
            next+=INTRO_FRAME_MS;const Uint32 now=SDL_GetTicks();if((Sint32)(next-now)>0)SDL_Delay(next-now);
        }
        SDL_DestroyTexture(tex);tex=create_stage_texture(r,VIEW_W,VIEW_H);
        stage_size(w,r,VIEW_W,VIEW_H);
    }
    if(!intro_quit){
        FloodTitle title;
        if(!flood_title_load(&title,FLOOD_DATA_DIR,err,sizeof(err)))fprintf(stderr,"title: %s\n",err);
        else{
            SDL_DestroyTexture(tex);tex=create_stage_texture(r,FLOOD_TITLE_W,FLOOD_TITLE_H);
            stage_size(w,r,FLOOD_TITLE_W,FLOOD_TITLE_H);render_title(r,tex,&title,fb);
            bool released=false,accepted=false;Uint32 next=SDL_GetTicks();
            while(tex&&!accepted&&!intro_quit){
                FloodInput i=input(w,r,false,false);if(i.quit)intro_quit=true;
                if(!i.fire)released=true;else if(released)accepted=true;
                /* A desktop-fullscreen transition clears both renderer back
                   buffers.  The title is otherwise static, so redraw it on
                   every wait iteration just like the interactive screens. */
                render_title(r,tex,&title,fb);
                next+=20u;const Uint32 now=SDL_GetTicks();if((Sint32)(next-now)>0)SDL_Delay(next-now);
            }
            SDL_DestroyTexture(tex);tex=create_stage_texture(r,VIEW_W,VIEW_H);
            stage_size(w,r,VIEW_W,VIEW_H);
        }
    }
show_level_select:
    if(!intro_quit){
        if(audio_device){SDL_LockAudioDevice(audio_device);flood_audio_stop(&audio);SDL_UnlockAudioDevice(audio_device);}
        FloodLevelSelect select;
        if(!flood_level_select_load(&select,FLOOD_DATA_DIR,level,err,sizeof(err)))
            fprintf(stderr,"level selector: %s\n",err);
        else{
            if(level_select_over_scores)
                flood_level_select_set_background(&select,post_score_background);
            level_select_over_scores=false;
            SDL_DestroyTexture(tex);tex=create_stage_texture(r,
                FLOOD_LEVEL_SELECT_W,FLOOD_LEVEL_SELECT_H);
            stage_size(w,r,FLOOD_LEVEL_SELECT_W,FLOOD_LEVEL_SELECT_H);
            render_level_select(r,tex,&select,fb);
            bool released=false;Uint32 next=SDL_GetTicks();
            while(tex&&!select.finished&&!intro_quit){
                FloodInput i=input(w,r,true,false);if(i.quit)intro_quit=true;
                if(!i.fire)released=true;
                flood_level_select_tick(&select,i.key,i.fire&&released);
                render_level_select(r,tex,&select,fb);
                next+=20u;const Uint32 now=SDL_GetTicks();if((Sint32)(next-now)>0)SDL_Delay(next-now);
            }
            level=select.selected_level;
            if(tex&&!intro_quit)fade_out(r,tex,fb,FLOOD_LEVEL_SELECT_W,FLOOD_LEVEL_SELECT_H);
            SDL_DestroyTexture(tex);tex=create_stage_texture(r,VIEW_W,VIEW_H);
            stage_size(w,r,VIEW_W,VIEW_H);
        }
    }
    if(intro_quit)g.running=false;
    else if(!flood_game_load_level(&g,FLOOD_DATA_DIR,level,err,sizeof(err))){
        fprintf(stderr,"load selected level %u: %s\n",level,err);g.running=false;
    }
    if(g.running&&protection_required(direct_level_after_scores)){
        FloodProtection protection;
        if(!flood_protection_load(&protection,FLOOD_DATA_DIR,g.rng_state,err,sizeof(err)))
            fprintf(stderr,"copy protection: %s\n",err);
        else{
            SDL_DestroyTexture(tex);tex=create_stage_texture(r,
                FLOOD_PROTECTION_W,FLOOD_PROTECTION_H);
            stage_size(w,r,FLOOD_PROTECTION_W,FLOOD_PROTECTION_H);
            render_protection(r,tex,&protection,fb);Uint32 next=SDL_GetTicks();
            while(tex&&!protection.finished&&!intro_quit){
                FloodInput i=input(w,r,true,false);if(i.quit)intro_quit=true;
                if(protection.phase==1u&&protection.selection_pending&&!i.fire)
                    flood_protection_seed(&protection,(uint16_t)SDL_GetTicks());
                flood_protection_tick(&protection,i.x,i.key,i.fire);
                render_protection(r,tex,&protection,fb);
                next+=20u;const Uint32 now=SDL_GetTicks();if((Sint32)(next-now)>0)SDL_Delay(next-now);
            }
            g.rng_state=protection.rng_state;
            if(!protection.passed)g.running=false;
            if(tex&&protection_exit_fades(&protection,intro_quit))
                fade_out(r,tex,fb,FLOOD_PROTECTION_W,FLOOD_PROTECTION_H);
            SDL_DestroyTexture(tex);tex=create_stage_texture(r,VIEW_W,VIEW_H);
            stage_size(w,r,VIEW_W,VIEW_H);
        }
    }
    direct_level_after_scores=false;
    if(audio_device&&g.running){
        SDL_LockAudioDevice(audio_device);
        if(!flood_audio_load(&audio,FLOOD_DATA_DIR,(uint32_t)wanted.freq,err,sizeof(err)))fprintf(stderr,"audio: %s\n",err);
        SDL_UnlockAudioDevice(audio_device);
    }else if(g.running&&flood_audio_load(&audio,FLOOD_DATA_DIR,(uint32_t)wanted.freq,err,sizeof(err))){
        audio_device=SDL_OpenAudioDevice(NULL,0,&wanted,&obtained,0);
        if(audio_device)SDL_PauseAudioDevice(audio_device,0);
    }
    bool game_quit=false;
    if(g.running)game_quit=play_level_banner(w,r,tex,&g,&audio,audio_device,fb,world);
    Uint32 gameplay_next=SDL_GetTicks();
    while(g.running&&!game_quit){FloodInput i=input(w,r,false,false);if(i.quit)game_quit=true;
        if(!game_quit&&i.restart_level){
            /* $D33C-$D362 calls reset/setup (whose $A3D2 entry fades), runs
               the complete banner, and deducts the life only afterward. */
            fade_out(r,tex,fb,VIEW_W,VIEW_H);
            if(!flood_game_restart_begin(&g,FLOOD_DATA_DIR,err,sizeof(err))){
                fprintf(stderr,"restart level: %s\n",err);g.running=false;
            }else if(g.running){
                game_quit=play_level_banner(w,r,tex,&g,&audio,audio_device,fb,world);
                if(!game_quit)flood_game_restart_finish(&g);
            }
            gameplay_next=SDL_GetTicks();continue;
        }
        if(!game_quit&&i.pause){
            game_quit=pause_until_fire(w,r,i.fire);
            gameplay_next=SDL_GetTicks();continue;
        }
        flood_game_tick(&g,i);
        if(audio_device){SDL_LockAudioDevice(audio_device);for(unsigned n=0;n<g.sound_queue_count;n++)flood_audio_trigger(&audio,g.sound_queue[n]);SDL_UnlockAudioDevice(audio_device);}
        render_world(r,tex,&g,fb,world);
        if(g.zap_message_pending&&!game_quit){
            FloodZapMessage message;
            if(!flood_zap_message_load(&message,FLOOD_DATA_DIR,g.world.level_number,err,sizeof(err))){
                fprintf(stderr,"zap message: %s\n",err);flood_complete_zap_message(&g);
            }else{
                Uint32 next=SDL_GetTicks();render_zap(r,tex,&message,fb,world);
                while(!message.finished&&!game_quit){
                    FloodInput modal=input(w,r,false,false);if(modal.quit)game_quit=true;
                    flood_zap_message_tick(&message,modal.fire);render_zap(r,tex,&message,fb,world);
                    next+=20u;const Uint32 now=SDL_GetTicks();if((Sint32)(next-now)>0)SDL_Delay(next-now);
                }
                if(message.finished){
                    const unsigned resume_sound=g.sound_queue_count;
                    flood_complete_zap_message(&g);
                    if(audio_device){SDL_LockAudioDevice(audio_device);
                        for(unsigned n=resume_sound;n<g.sound_queue_count;n++)
                            flood_audio_trigger(&audio,g.sound_queue[n]);
                        SDL_UnlockAudioDevice(audio_device);
                    }
                    render_world(r,tex,&g,fb,world);
                }
                gameplay_next=SDL_GetTicks();
            }
        }
        if(g.running&&g.level_complete){
            fade_out(r,tex,fb,VIEW_W,VIEW_H);
            game_quit=wait_audio_idle(w,r,&audio,audio_device);
        }
        const unsigned old_level=g.world.level_number;
        if(g.running&&!game_quit&&!flood_game_advance_level(&g,FLOOD_DATA_DIR,err,sizeof(err))){fprintf(stderr,"advance level: %s\n",err);g.running=false;}
        if(g.running&&g.world.level_number!=old_level){
            game_quit=play_level_banner(w,r,tex,&g,&audio,audio_device,fb,world);
            gameplay_next=SDL_GetTicks();
        }
        gameplay_next+=game_frame_ms(saved_game_speed);const Uint32 gameplay_now=SDL_GetTicks();
        if((Sint32)(gameplay_next-gameplay_now)>0)SDL_Delay(gameplay_next-gameplay_now);
        else gameplay_next=gameplay_now;
    }
    /* Level 42 first passes through A3D2's ordinary cavern fade, then the
       $9872 ending branch performs the original second all-black delay. */
    if(g.game_complete&&!game_quit)fade_out(r,tex,fb,VIEW_W,VIEW_H);
    if(game_over_exit_fades(&g,game_quit)){
        fade_out(r,tex,fb,VIEW_W,VIEW_H);
        if(audio_device){SDL_LockAudioDevice(audio_device);flood_audio_stop(&audio);SDL_UnlockAudioDevice(audio_device);}
        const bool return_to_level_select=run_post_score_loop(w,r,&tex,fb,
            post_score_background,g.score,err,sizeof(err),&game_quit,
            &level_select_over_scores);
        if(return_to_level_select){
            reset_after_game_over(&g,&level);intro_quit=false;
            direct_level_after_scores=true;
            goto show_level_select;
        }
    }
    if(g.game_complete&&!game_quit){
        FloodEnding ending;bool ending_finished=false;
        if(!flood_ending_load(&ending,FLOOD_DATA_DIR,err,sizeof(err)))fprintf(stderr,"ending: %s\n",err);
        else{
            SDL_DestroyTexture(tex);tex=create_stage_texture(r,FLOOD_ENDING_W,FLOOD_ENDING_H);
            stage_size(w,r,FLOOD_ENDING_W,FLOOD_ENDING_H);
            Uint32 next=SDL_GetTicks();
            while(tex&&fb&&!ending.finished&&!game_quit){
                FloodInput i=input(w,r,false,false);if(i.quit)game_quit=true;
                flood_ending_tick(&ending);render_ending(r,tex,&ending,fb);
                next+=20u;const Uint32 now=SDL_GetTicks();if((Sint32)(next-now)>0)SDL_Delay(next-now);
            }
            ending_finished=ending.finished&&!game_quit;
            if(tex&&fb&&ending_finished)fade_out(r,tex,fb,FLOOD_ENDING_W,FLOOD_ENDING_H);
        }
        /* $15AD2 returns to the same score/credits path used after the last
           life.  Preserve the completed board as the selector background. */
        if(ending_opens_post_score_loop(ending_finished,game_quit)){
            if(audio_device){SDL_LockAudioDevice(audio_device);flood_audio_stop(&audio);SDL_UnlockAudioDevice(audio_device);}
            if(run_post_score_loop(w,r,&tex,fb,post_score_background,g.score,
                err,sizeof(err),&game_quit,&level_select_over_scores)){
                reset_after_game_over(&g,&level);intro_quit=false;
                direct_level_after_scores=true;
                goto show_level_select;
            }
        }
    }
    if(audio_device)SDL_CloseAudioDevice(audio_device);
    free(scaled_framebuffer);free(world);free(fb);SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(r);SDL_DestroyWindow(w);close_game_controller();SDL_Quit();return 0;
}
#else
int main(int argc,char **argv){
    unsigned level=1;if(argc>1)level=(unsigned)strtoul(argv[1],NULL,10);FloodGame g;flood_game_init(&g);char err[160];
    if(!flood_game_load_level(&g,FLOOD_DATA_DIR,level,err,sizeof(err))){fprintf(stderr,"load level %u: %s\n",level,err);return 2;}
    for(int n=0;n<300&&g.running;n++){FloodInput i={0};i.x=(n<180);flood_game_tick(&g,i);
        if(!flood_game_advance_level(&g,FLOOD_DATA_DIR,err,sizeof(err))){fprintf(stderr,"advance level: %s\n",err);return 2;}}
    printf("level=%u bank=%c ticks=%u current=(%d,%d) life=%d sprite=$%02X matilda=%d complete=%d\n",g.world.level_number,'A'+g.world.block_bank-1,g.ticks,g.player.x,g.player.y,g.player.life_force,flood_player_sprite_id(&g),g.matilda.visible,g.game_complete);return 0;
}
#endif
