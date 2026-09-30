#ifndef FLOOD_MULTIPLAYER_H
#define FLOOD_MULTIPLAYER_H
#include "flood_game.h"

/* Both allocations belong to the caller. Only game owns the cavern; second
   stores the other actor's state and never runs a whole-world tick. */
typedef struct {
    FloodGame *game;
    FloodGame *second;
    bool terminal_animation;
} FloodMultiplayer;

void flood_multiplayer_begin(FloodMultiplayer *multi);
void flood_multiplayer_tick(FloodMultiplayer *multi,FloodInput first,FloodInput second);
void flood_multiplayer_banner_tick(FloodMultiplayer *multi,FloodInput first,
    FloodInput second,bool first_frame);
void flood_multiplayer_second_view(const FloodMultiplayer *multi,FloodGame *view);
void flood_multiplayer_reset_second(FloodMultiplayer *multi);
void flood_multiplayer_restart_finish(FloodMultiplayer *multi,
    int16_t first_lives,int16_t second_lives);
uint8_t flood_multiplayer_actor_colour(uint8_t original);
#endif
