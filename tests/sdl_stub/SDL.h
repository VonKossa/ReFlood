#ifndef SDL_H
#define SDL_H
#include <stdint.h>
typedef uint8_t Uint8;typedef int16_t Sint16;typedef uint32_t Uint32;typedef int32_t Sint32;
typedef int SDL_Keycode;typedef int32_t SDL_JoystickID;
typedef struct SDL_Window SDL_Window;typedef struct SDL_Renderer SDL_Renderer;typedef struct SDL_Texture SDL_Texture;
typedef struct {int x,y,w,h;} SDL_Rect;
typedef struct SDL_GameController SDL_GameController;typedef struct SDL_Joystick SDL_Joystick;
typedef unsigned SDL_AudioDeviceID;
typedef enum {SDL_ScaleModeNearest,SDL_ScaleModeLinear,SDL_ScaleModeBest} SDL_ScaleMode;
typedef enum {SDL_CONTROLLER_AXIS_INVALID=-1,SDL_CONTROLLER_AXIS_LEFTX,
    SDL_CONTROLLER_AXIS_LEFTY} SDL_GameControllerAxis;
typedef enum {SDL_CONTROLLER_BUTTON_INVALID=-1,SDL_CONTROLLER_BUTTON_A,
    SDL_CONTROLLER_BUTTON_BACK,SDL_CONTROLLER_BUTTON_DPAD_UP,
    SDL_CONTROLLER_BUTTON_DPAD_DOWN,SDL_CONTROLLER_BUTTON_DPAD_LEFT,
    SDL_CONTROLLER_BUTTON_DPAD_RIGHT,SDL_CONTROLLER_BUTTON_X} SDL_GameControllerButton;
typedef struct {int freq;unsigned short format;Uint8 channels;unsigned short samples;
    void (*callback)(void *,Uint8 *,int);void *userdata;} SDL_AudioSpec;
typedef struct {int type;struct {Uint8 repeat;struct {SDL_Keycode sym;} keysym;} key;
    struct {int which;} cdevice;struct {int y;} wheel;
    struct {Uint8 button,clicks;int x,y;} button;
    struct {Uint8 button;} cbutton;} SDL_Event;
#define SDL_INIT_VIDEO 1u
#define SDL_INIT_AUDIO 2u
#define SDL_INIT_EVENTS 4u
#define SDL_INIT_GAMECONTROLLER 8u
#define SDL_QUIT 0x100
#define SDL_KEYDOWN 0x300
#define SDL_MOUSEBUTTONDOWN 0x401
#define SDL_MOUSEWHEEL 0x403
#define SDL_BUTTON_LEFT 1u
#define SDL_CONTROLLERDEVICEADDED 0x650
#define SDL_CONTROLLERDEVICEREMOVED 0x651
#define SDL_CONTROLLERBUTTONDOWN 0x653
#define SDLK_RETURN 13
#define SDLK_KP_ENTER 271
#define SDLK_BACKSPACE 8
#define SDLK_DELETE 127
#define SDLK_a 97
#define SDLK_w 119
#define SDLK_s 115
#define SDLK_f 102
#define SDLK_p 112
#define SDLK_r 114
#define SDLK_z 122
#define SDLK_0 48
#define SDLK_9 57
#define SDLK_SPACE 32
#define SDLK_ESCAPE 27
#define SDLK_UP 273
#define SDLK_DOWN 274
#define SDLK_PAGEUP 280
#define SDLK_PAGEDOWN 281
#define SDLK_HOME 278
#define SDLK_END 279
#define SDL_SCANCODE_RIGHT 79
#define SDL_SCANCODE_LEFT 80
#define SDL_SCANCODE_DOWN 81
#define SDL_SCANCODE_UP 82
#define SDL_SCANCODE_SPACE 44
#define SDL_SCANCODE_ESCAPE 41
#define SDL_SCANCODE_RCTRL 228
#define SDL_SCANCODE_LCTRL 224
#define SDL_SCANCODE_A 4
#define SDL_SCANCODE_D 7
#define SDL_SCANCODE_S 22
#define SDL_SCANCODE_W 26
#define SDL_SCANCODE_P 19
#define SDL_SCANCODE_R 21
#define AUDIO_S16SYS 0x8010
#define SDL_WINDOWPOS_CENTERED 0
#define SDL_WINDOW_FULLSCREEN_DESKTOP 0x00001001u
#define SDL_RENDERER_ACCELERATED 1u
#define SDL_RENDERER_PRESENTVSYNC 2u
#define SDL_PIXELFORMAT_ARGB8888 1u
#define SDL_TEXTUREACCESS_STREAMING 1
#define SDL_VERSION_ATLEAST(x,y,z) 1
int SDL_Init(Uint32);void SDL_Quit(void);int SDL_PollEvent(SDL_Event *);
const Uint8 *SDL_GetKeyboardState(int *);SDL_Window *SDL_CreateWindow(const char *,int,int,int,int,Uint32);
int SDL_SetWindowFullscreen(SDL_Window *,Uint32);
Uint32 SDL_GetWindowFlags(SDL_Window *);void SDL_SetWindowPosition(SDL_Window *,int,int);
SDL_Renderer *SDL_CreateRenderer(SDL_Window *,int,Uint32);int SDL_RenderSetLogicalSize(SDL_Renderer *,int,int);
SDL_Texture *SDL_CreateTexture(SDL_Renderer *,Uint32,int,int,int);void SDL_DestroyTexture(SDL_Texture *);
int SDL_SetTextureScaleMode(SDL_Texture *,SDL_ScaleMode);
void SDL_DestroyRenderer(SDL_Renderer *);void SDL_DestroyWindow(SDL_Window *);void SDL_SetWindowSize(SDL_Window *,int,int);
int SDL_UpdateTexture(SDL_Texture *,const void *,const void *,int);int SDL_RenderClear(SDL_Renderer *);
int SDL_SetRenderDrawColor(SDL_Renderer *,Uint8,Uint8,Uint8,Uint8);
int SDL_RenderCopy(SDL_Renderer *,SDL_Texture *,const void *,const void *);void SDL_RenderPresent(SDL_Renderer *);
int SDL_RenderFillRect(SDL_Renderer *,const SDL_Rect *);
SDL_AudioDeviceID SDL_OpenAudioDevice(const char *,int,const SDL_AudioSpec *,SDL_AudioSpec *,int);
void SDL_PauseAudioDevice(SDL_AudioDeviceID,int);void SDL_LockAudioDevice(SDL_AudioDeviceID);
void SDL_UnlockAudioDevice(SDL_AudioDeviceID);void SDL_CloseAudioDevice(SDL_AudioDeviceID);
Uint32 SDL_GetTicks(void);void SDL_Delay(Uint32);
int SDL_NumJoysticks(void);int SDL_IsGameController(int);
SDL_GameController *SDL_GameControllerOpen(int);void SDL_GameControllerClose(SDL_GameController *);
const char *SDL_GameControllerNameForIndex(int);
int SDL_GameControllerGetAttached(SDL_GameController *);
Sint16 SDL_GameControllerGetAxis(SDL_GameController *,SDL_GameControllerAxis);
Uint8 SDL_GameControllerGetButton(SDL_GameController *,SDL_GameControllerButton);
SDL_Joystick *SDL_GameControllerGetJoystick(SDL_GameController *);
SDL_JoystickID SDL_JoystickInstanceID(SDL_Joystick *);
#endif
