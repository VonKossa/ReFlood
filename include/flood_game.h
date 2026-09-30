#ifndef FLOOD_GAME_H
#define FLOOD_GAME_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define FLOOD_MAP_W 128
#define FLOOD_MAP_H 100
#define FLOOD_TILE_SIZE 16
#define FLOOD_OBJECT_MAX 128
#define FLOOD_ACTION_MAX 8
#define FLOOD_ACTION_RENDER_MAX 64
#define FLOOD_MATILDA_HISTORY 256
#define FLOOD_TILE_COUNT 256
#define FLOOD_TILE_PIXELS (FLOOD_TILE_SIZE*FLOOD_TILE_SIZE)
#define FLOOD_SPRITE_COUNT 230
#define FLOOD_MAX_SPRITE_W 32
#define FLOOD_MAX_SPRITE_H 32
#define FLOOD_MAX_SPRITE_PIXELS (FLOOD_MAX_SPRITE_W*FLOOD_MAX_SPRITE_H)
#define FLOOD_TRIGGER_DATA_SIZE 640
#define FLOOD_HUD_W 320
#define FLOOD_HUD_H 8
#define FLOOD_HUD_ROW_BYTES (FLOOD_HUD_W/8)
#define FLOOD_HUD_BYTES (FLOOD_HUD_ROW_BYTES*FLOOD_HUD_H)
#define FLOOD_HUD_GLYPH_COUNT 0x70
#define FLOOD_HUD_FETCH_X 16
#define FLOOD_HUD_VISIBLE_X 0
#define FLOOD_HUD_BACKING_Y 16
#define FLOOD_HUD_VISIBLE_Y 0
#define FLOOD_AMIGA_ROW_BYTES 44
#define FLOOD_VIEW_W 320
#define FLOOD_VIEW_H 208
#define FLOOD_VIEW_PIXELS (FLOOD_VIEW_W*FLOOD_VIEW_H)
#define FLOOD_BACKING_W 352
#define FLOOD_BACKING_X 16
#define FLOOD_BACKING_PIXELS (FLOOD_BACKING_W*FLOOD_VIEW_H)
#define FLOOD_ENDING_W 320
#define FLOOD_ENDING_H 208
#define FLOOD_ENDING_BACKING_H 240
#define FLOOD_ENDING_ROW_BYTES 44
#define FLOOD_ENDING_PLANE_BYTES 0x2940
#define FLOOD_ENDING_SCREEN_BYTES 0xA500
#define FLOOD_ENDING_ASSET_BYTES 0x1E500
#define FLOOD_ENDING_EVENT_COUNT 75
#define FLOOD_SOUND_QUEUE_MAX 32
#define FLOOD_FX_TRACK_BYTES 2111
#define FLOOD_FX_INSTRUMENT_BYTES 97348
#define FLOOD_MUSIC_TRACK_BYTES 9344
#define FLOOD_MUSIC_INSTRUMENT_BYTES 120548
#define FLOOD_AUDIO_CHANNELS 4
#define FLOOD_INTRO_W 320
#define FLOOD_INTRO_H 240
#define FLOOD_INTRO_CONTENT_W 160
#define FLOOD_INTRO_CONTENT_H 96
#define FLOOD_INTRO_CONTENT_X 92
#define FLOOD_INTRO_CONTENT_Y 72
#define FLOOD_INTRO_BACKING_W 320
#define FLOOD_INTRO_BACKING_H 500
#define FLOOD_INTRO_ROW_BYTES 40
#define FLOOD_INTRO_PLANE_BYTES 20000
#define FLOOD_INTRO_EVENT_BYTES 3280
#define FLOOD_TITLE_W 320
#define FLOOD_TITLE_H 208
#define FLOOD_TITLE_BACKING_W 352
#define FLOOD_TITLE_BACKING_H 240
#define FLOOD_TITLE_ROW_BYTES 44
#define FLOOD_TITLE_PLANE_BYTES 10560
#define FLOOD_LEVEL_SELECT_W 320
#define FLOOD_LEVEL_SELECT_H 208
#define FLOOD_LEVEL_SELECT_BACKING_H 240
#define FLOOD_LEVEL_SELECT_ROW_BYTES 44
#define FLOOD_LEVEL_SELECT_PLANE_BYTES 10560
#define FLOOD_LEVEL_SELECT_PASSWORD_COUNT 48
#define FLOOD_LEVEL_SELECT_FONT_BYTES 960
#define FLOOD_PROTECTION_W 320
#define FLOOD_PROTECTION_H 208
#define FLOOD_PROTECTION_PIXELS (FLOOD_PROTECTION_W*FLOOD_PROTECTION_H)
#define FLOOD_PROTECTION_LANGUAGES 4
#define FLOOD_PROTECTION_RECORDS 20
#define FLOOD_PROTECTION_ACTIVE_RECORDS 19
#define FLOOD_PROTECTION_RECORD_BYTES 74
#define FLOOD_PROTECTION_CHALLENGE_BYTES 5920
#define FLOOD_PROTECTION_MESSAGE_BYTES 367
#define FLOOD_HIGHSCORE_W 320
#define FLOOD_HIGHSCORE_H 208
#define FLOOD_HIGHSCORE_BACKING_H 240
#define FLOOD_HIGHSCORE_ROW_BYTES 44
#define FLOOD_HIGHSCORE_PLANE_BYTES 10560
#define FLOOD_HIGHSCORE_ENTRIES 5
#define FLOOD_HIGHSCORE_NAME_CHARS 11
#define FLOOD_HIGHSCORE_GLYPHS 43
#define FLOOD_HIGHSCORE_GLYPH_BYTES 240
#define FLOOD_TILE_OVERFETCH_COLS 22
#define FLOOD_TILE_OVERFETCH_ROWS 14
#define FLOOD_OVERLAY_TILE_COUNT 42
#define FLOOD_OVERLAY_BANK_COUNT 2
#define FLOOD_OVERLAY_TILE_PIXELS FLOOD_TILE_PIXELS
#define FLOOD_OVERLAY_BANK_BYTES 0x0A80
#define FLOOD_OVERLAY_BYTES (FLOOD_OVERLAY_BANK_COUNT*FLOOD_OVERLAY_BANK_BYTES)
#define FLOOD_ATTR_WATER_BLOCK 0x01u
#define FLOOD_ATTR_SOLID 0x02u
#define FLOOD_ATTR_SLOPE 0x04u
/* General lethal-surface bit; BLOCKA $3D is the confirmed Sparkling Fungi. */
#define FLOOD_ATTR_FATAL 0x40u

typedef struct {
    int16_t x,y,dx,dy,life_force,air;
    uint8_t anim_phase,hbank,vbank,pose_code,death_mode,death_phase;
    uint8_t contact_mask,raw_contact_mask,contact_count,attachment_mode,attachment_ticks;
    bool pose_active;
    uint16_t contact_aux;
    int8_t raw_x,raw_y,surface_x,surface_y;
    uint16_t parachute_timer,balloon_timer,invulnerable_timer,orange_timer;
    uint8_t forced_up_timer;
} FloodPlayer;

/* The single 18-byte auxiliary record at $7DA46.  Its State-0 entry handles
   $DC/$E7/$E8 independently of the ordinary item-cell cache. */
typedef struct {
    int16_t x,y,phase,dy,aux;
    uint16_t state;
    uint16_t unused[3];
} FloodQuiffyEffect;

typedef struct {
    uint8_t width,height;
    uint8_t pixels[FLOOD_MAX_SPRITE_PIXELS];
    uint8_t mask[FLOOD_MAX_SPRITE_PIXELS];
} FloodSprite;

typedef struct {
    uint8_t terrain[FLOOD_MAP_W*FLOOD_MAP_H];
    uint8_t render_terrain[FLOOD_MAP_W*FLOOD_MAP_H];
    uint8_t attr[FLOOD_TILE_COUNT];
    uint8_t tile_pixels[FLOOD_TILE_COUNT*FLOOD_TILE_PIXELS];
    uint8_t overlay_pixels[FLOOD_OVERLAY_BANK_COUNT*FLOOD_OVERLAY_TILE_COUNT*FLOOD_OVERLAY_TILE_PIXELS];
    uint8_t water[FLOOD_MAP_W*FLOOD_MAP_H];
    uint8_t render_water[FLOOD_MAP_W*FLOOD_MAP_H];
    uint8_t trigger_data[FLOOD_TRIGGER_DATA_SIZE];
    uint16_t trigger_count;
    bool water_active,mechanism_enabled,mechanisms_paused,render_terrain_valid;
    int16_t water_source_x,water_source_y,bounds_x,bounds_y;
    uint8_t bounds_tile;
    uint16_t water_pause,water_speed;
    uint16_t water_scan_row;
    uint32_t water_age;
    int16_t remaining_food;
    uint8_t block_bank;
    uint8_t level_number;
} FloodWorld;

typedef struct { int16_t x,y; uint8_t frame; } FloodHistorySample;
typedef struct {
    FloodHistorySample history[FLOOD_MATILDA_HISTORY];
    uint16_t write_pos,read_pos,hold;
    bool visible;
    int16_t x,y;
    uint16_t sprite_id;
} FloodMatilda;
typedef struct {
    int16_t x,y,dx,dy,origin_x,origin_y,render_x,render_y;
    uint16_t aux,timer;
    uint8_t state,anim,sprite,sprite_base,state_flags;
    bool active;
} FloodObject;

/* Exact 18-byte record at $7D0F6: nine consecutive big-endian words. */
typedef struct {
    int16_t x,y,dx,dy;
    uint16_t anim,state,aux,timer,target;
} FloodAction;

typedef struct { int16_t x,y; uint16_t sprite_id; } FloodActionRender;

typedef struct {
    FloodPlayer player;
    /* Set only during experimental multiplayer action dispatch. */
    FloodPlayer *multiplayer_opponent;
    FloodWorld world;
    FloodSprite sprites[FLOOD_SPRITE_COUNT];
    FloodMatilda matilda;
    FloodObject objects[FLOOD_OBJECT_MAX];
    FloodAction actions[FLOOD_ACTION_MAX];
    FloodQuiffyEffect quiffy_effect;
    FloodActionRender action_renders[FLOOD_ACTION_RENDER_MAX];
    uint8_t hud_template[FLOOD_HUD_BYTES];
    uint8_t hud_glyphs[FLOOD_HUD_GLYPH_COUNT*FLOOD_HUD_H];
    uint8_t hud_mask[FLOOD_HUD_BYTES];
    uint8_t hud_digits[10];
    uint32_t score;
    int16_t camera_x,camera_y;
    uint16_t rng_state;
    int16_t lives;
    int16_t escape_life_force;
    uint32_t ticks;
    uint16_t last_pickup_tile,last_special_tile;
    uint8_t last_sound_id;
    uint8_t sound_queue[FLOOD_SOUND_QUEUE_MAX],sound_queue_count;
    uint8_t action_render_count;
    uint8_t render_tile_phase,render_buffer_index;
    uint16_t selected_weapon_state,fire_ticks;
    uint8_t weapon_main_offset,weapon_tip_offset;
    uint16_t transport_timer;
    uint8_t transport_effect_stage;
    int16_t transport_x,transport_y;
    int16_t transport_render_x,transport_render_y;
    int16_t action_target_x,action_target_y;
    int16_t quiffy_effect_render_x,quiffy_effect_render_y;
    uint16_t quiffy_effect_sprite;
    bool weapon_pose_active,dispatch_suppressed,render_dispatch_suppressed;
    bool quiffy_effect_pose_active,quiffy_effect_render_active;
    bool player_blink_active;
    bool level_complete,game_complete;
    bool special_entry_offset;
    bool transport_render_prejump;
    bool zap_message_pending;
    bool ouch_visible,damage_contact_previous,water_contact_previous;
    bool slope_correction_previous;
    bool escape_death_active;
    bool sound_pending;
    bool running;
} FloodGame;

typedef struct {
    int8_t x,y;
    bool fire,restart,pause,restart_level,quit;
    uint8_t key;
} FloodInput;
typedef struct { uint8_t x_flags,y_flags,corner_flags; } FloodCollisionResult;
typedef struct { uint8_t code,hbank,vbank; bool active; } FloodPoseSelection;
typedef struct {
    uint8_t camera_phase,bplcon1_shift,bplcon1,frame_byte_offset;
    uint8_t backing_x,fetch_x,visible_x;
    int16_t compositor_base_bias;
} FloodHudPlacement;

typedef struct {
    uint8_t source[FLOOD_ENDING_ASSET_BYTES];
    uint8_t screen[FLOOD_ENDING_SCREEN_BYTES];
    uint16_t palette[16];
    uint16_t event_index,hold;
    uint32_t frames;
    bool loaded,finished;
} FloodEnding;

typedef struct {
    const uint8_t *data;
    uint16_t base,value;
    uint8_t phase,ticks,repeats,flags,loop_adjust;
} FloodAudioEnvelope;

typedef struct {
    size_t track_start,track_pos,sequence_start,sequence_pos;
    uint16_t base_period,period;
    uint8_t delay,delay_reset,transpose,volume,sample_id;
    bool active,loop_track,loop_sequence;
    FloodAudioEnvelope volume_env,pitch_env;
    size_t sample_start,sample_length;
    uint64_t sample_phase,sample_step;
    uint8_t dma_state;
    bool sample_loop,sample_playing;
} FloodAudioChannel;

typedef struct {
    uint8_t tracks[FLOOD_MUSIC_TRACK_BYTES];
    uint8_t instruments[FLOOD_MUSIC_INSTRUMENT_BYTES];
    FloodAudioChannel channel[FLOOD_AUDIO_CHANNELS];
    size_t track_size,instrument_size,song_base,track_table,volume_macros,pitch_macros;
    uint32_t sample_rate;
    uint64_t tick_accumulator;
    uint8_t speed,speed_counter;
    bool loaded,music_mode;
} FloodAudio;

typedef struct {
    uint8_t planes[6*FLOOD_INTRO_PLANE_BYTES];
    uint8_t events[FLOOD_INTRO_EVENT_BYTES];
    uint16_t palette[32],frame;
    size_t event_pos;
    bool loaded,finished,skipped;
} FloodIntro;

typedef struct {
    uint8_t planes[5*FLOOD_TITLE_PLANE_BYTES];
    uint16_t palette[32];
    bool loaded;
} FloodTitle;

typedef struct {
    uint8_t planes[4*FLOOD_LEVEL_SELECT_PLANE_BYTES];
    uint8_t font[FLOOD_LEVEL_SELECT_FONT_BYTES];
    uint8_t panel[126],correct[20],incorrect[20];
    uint8_t passwords[FLOOD_LEVEL_SELECT_PASSWORD_COUNT][4];
    uint16_t palette[16],status_timer;
    uint8_t selected_level,result_level,cursor,last_key;
    char saved_password[5];
    bool loaded,finished;
} FloodLevelSelect;

#define FLOOD_ZAP_W 160
#define FLOOD_ZAP_H 48
#define FLOOD_ZAP_X 96
#define FLOOD_ZAP_Y 72
#define FLOOD_ZAP_PIXELS (FLOOD_ZAP_W*FLOOD_ZAP_H)
#define FLOOD_LEVEL_BANNER_FRAMES 51
typedef struct {
    uint8_t pixels[FLOOD_ZAP_PIXELS];
    uint8_t font[FLOOD_LEVEL_SELECT_FONT_BYTES],panel[126];
    uint8_t passwords[FLOOD_LEVEL_SELECT_PASSWORD_COUNT][4];
    uint16_t palette[16];
    uint8_t level;
    bool loaded,finished;
} FloodZapMessage;

typedef struct {
    uint8_t pixels[FLOOD_PROTECTION_PIXELS];
    uint8_t font[FLOOD_LEVEL_SELECT_FONT_BYTES];
    uint8_t challenges[FLOOD_PROTECTION_CHALLENGE_BYTES];
    uint8_t messages[FLOOD_PROTECTION_MESSAGE_BYTES];
    uint16_t message_offsets[16],flag_layout[16][3],palette[16];
    FloodSprite symbols[17];
    char answer[14];
    uint16_t rng_state;
    uint8_t language,challenge,phase,cursor,last_key,failures;
    int8_t last_horizontal;
    bool selection_pending,ack_fire_seen,loaded,finished,passed,exhausted;
} FloodProtection;

typedef struct {
    uint8_t base_planes[4*FLOOD_HIGHSCORE_PLANE_BYTES];
    uint8_t planes[4*FLOOD_HIGHSCORE_PLANE_BYTES];
    uint8_t glyphs[FLOOD_HIGHSCORE_GLYPHS*FLOOD_HIGHSCORE_GLYPH_BYTES];
    uint32_t scores[FLOOD_HIGHSCORE_ENTRIES];
    char names[FLOOD_HIGHSCORE_ENTRIES][12];
    uint16_t palette[16],glyph_offset,target_offset;
    int16_t glyph_step;
    int8_t entry_index;
    uint8_t cursor;
    bool horizontal_ready,fire_seen,loaded,editing,finished;
} FloodHighScores;

void flood_game_init(FloodGame *g);
bool flood_game_load_level(FloodGame *g,const char *data_dir,unsigned level,char *err,size_t errcap);
bool flood_game_restart_begin(FloodGame *g,const char *data_dir,char *err,size_t errcap);
/* Load the three companion level files named by a selected *_header.bin path;
   presentation, block, and sprite assets still come from data_dir. */
bool flood_game_load_custom_level(FloodGame *g,const char *data_dir,
    const char *header_path,char *err,size_t errcap);
void flood_game_restart_finish(FloodGame *g);
bool flood_game_advance_level(FloodGame *g,const char *data_dir,char *err,size_t errcap);
void flood_game_tick(FloodGame *g,FloodInput in);
void flood_game_level_banner_tick(FloodGame *g,FloodInput in,bool first_frame);
bool flood_ending_load(FloodEnding *ending,const char *data_dir,char *err,size_t errcap);
void flood_ending_tick(FloodEnding *ending);
uint8_t flood_ending_pixel(const FloodEnding *ending,unsigned x,unsigned y);
bool flood_audio_load(FloodAudio *audio,const char *data_dir,uint32_t sample_rate,char *err,size_t errcap);
bool flood_music_load(FloodAudio *audio,const char *data_dir,uint32_t sample_rate,char *err,size_t errcap);
void flood_audio_trigger(FloodAudio *audio,uint8_t sound_id);
void flood_audio_tick(FloodAudio *audio);
void flood_audio_mix(FloodAudio *audio,int16_t *stereo,size_t frames);
void flood_audio_stop(FloodAudio *audio);
bool flood_audio_is_playing(const FloodAudio *audio);
bool flood_intro_load(FloodIntro *intro,const char *data_dir,char *err,size_t errcap);
void flood_intro_tick(FloodIntro *intro,bool fire);
uint8_t flood_intro_pixel(const FloodIntro *intro,unsigned x,unsigned y);
bool flood_title_load(FloodTitle *title,const char *data_dir,char *err,size_t errcap);
uint8_t flood_title_pixel(const FloodTitle *title,unsigned x,unsigned y);
bool flood_level_select_load(FloodLevelSelect *select,const char *data_dir,
    unsigned initial_level,char *err,size_t errcap);
void flood_level_select_set_background(FloodLevelSelect *select,
    const uint8_t planes[4*FLOOD_LEVEL_SELECT_PLANE_BYTES]);
void flood_level_select_tick(FloodLevelSelect *select,uint8_t key,bool fire);
uint8_t flood_level_select_pixel(const FloodLevelSelect *select,unsigned x,unsigned y);
bool flood_zap_message_load(FloodZapMessage *message,const char *data_dir,
    unsigned level,char *err,size_t errcap);
void flood_zap_message_tick(FloodZapMessage *message,bool fire);
uint8_t flood_zap_message_pixel(const FloodZapMessage *message,unsigned x,unsigned y);
bool flood_protection_load(FloodProtection *protection,const char *data_dir,
    uint16_t rng_seed,char *err,size_t errcap);
void flood_protection_seed(FloodProtection *protection,uint16_t raster_seed);
void flood_protection_tick(FloodProtection *protection,int8_t horizontal,
    uint8_t key,bool fire);
uint8_t flood_protection_pixel(const FloodProtection *protection,unsigned x,unsigned y);
bool flood_high_scores_load(FloodHighScores *scores,const char *data_dir,
    uint32_t current_score,char *err,size_t errcap);
void flood_high_scores_enter(FloodHighScores *scores,uint32_t current_score);
void flood_high_scores_tick(FloodHighScores *scores,int8_t horizontal,
    int8_t vertical,bool fire);
uint8_t flood_high_scores_pixel(const FloodHighScores *scores,unsigned x,unsigned y);

uint16_t flood_tile_index_at(int16_t x,int16_t y);
uint8_t flood_tile_attr_at(const FloodGame *g,int16_t x,int16_t y);
FloodCollisionResult flood_query_collision(const FloodGame *g,uint16_t x,uint16_t y,int16_t dx,int16_t dy,uint16_t width,uint16_t height);
uint8_t flood_water_fill_at(const FloodGame *g,int16_t x,int16_t y);
void flood_reset_water(FloodGame *g);
bool flood_water_step(FloodGame *g);
void flood_rebuild_water(FloodGame *g);
void flood_update_water(FloodGame *g);
void flood_matilda_add_death_delay(FloodGame *g);
uint8_t flood_tile_pixel(const FloodGame *g,uint8_t tile,unsigned x,unsigned y);
const FloodSprite *flood_sprite(const FloodGame *g,unsigned sprite_id);
uint16_t flood_player_sprite_id(const FloodGame *g);
bool flood_player_should_draw(const FloodGame *g);
uint16_t flood_weapon_tip_sprite_id(const FloodGame *g);
uint8_t flood_select_quiffy_corner_pose(uint8_t contacts,uint8_t hbank,uint8_t vbank);
FloodPoseSelection flood_select_quiffy_pose(uint16_t contacts,uint8_t raw_contacts,
    uint8_t contact_count,int8_t raw_y,uint8_t anim_phase,uint8_t hbank,
    uint8_t vbank,uint8_t previous_code,bool material_present);
uint16_t flood_build_quiffy_contacts(FloodGame *g);
bool flood_initialize_marker(FloodGame *g,uint16_t map_index,uint8_t marker);
unsigned flood_activate_trigger(FloodGame *g,uint16_t event_id,uint16_t mode);
void flood_update_state12(FloodGame *g);
void flood_update_state14(FloodGame *g);
void flood_update_state1_chain(FloodGame *g);
void flood_update_state5(FloodGame *g);
void flood_update_states_6_9_15(FloodGame *g);
void flood_update_state8(FloodGame *g);
void flood_update_mechanisms_11_18(FloodGame *g);
void flood_update_states_2_13_19_21(FloodGame *g);
void flood_update_actions(FloodGame *g);
void flood_update_hud(FloodGame *g);
FloodHudPlacement flood_hud_placement(uint16_t camera_x);
void flood_update_camera(FloodGame *g);
uint8_t flood_render_tile_id(const FloodGame *g,uint8_t tile);
void flood_build_tile_window(const FloodGame *g,uint8_t out[FLOOD_VIEW_PIXELS]);
void flood_composite_overlay_window(const FloodGame *g,uint8_t out[FLOOD_VIEW_PIXELS]);
void flood_build_tile_backing(const FloodGame *g,uint8_t out[FLOOD_BACKING_PIXELS]);
void flood_composite_overlay_backing(const FloodGame *g,uint8_t out[FLOOD_BACKING_PIXELS]);
uint8_t flood_transport_effect_pixel(const uint8_t backing[FLOOD_BACKING_PIXELS],
    unsigned x,unsigned y,unsigned stage);
uint16_t flood_fade_colour(uint16_t colour,unsigned steps);
void flood_complete_zap_message(FloodGame *g);
void flood_prepare_object_dispatch(FloodGame *g);
void flood_finish_object_dispatch(FloodGame *g);
void flood_resolve_quiffy_contacts(FloodPlayer *p,uint8_t contacts);
void flood_maintain_quiffy_attachment(FloodPlayer *p);
#endif
