#ifndef REFLOOD_EDITOR_TILE_HELP_H
#define REFLOOD_EDITOR_TILE_HELP_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Returns a short, bank-aware description for one map byte. */
void editor_tile_description(unsigned tile,uint8_t attributes,bool have_attributes,
    char *text,size_t capacity);

#endif
