#ifndef REFLOOD_CUSTOM_MAP_H
#define REFLOOD_CUSTOM_MAP_H
#include <stdbool.h>
#include <stddef.h>

/* Find the immediately following numbered header in the same directory.
   Arbitrary filenames, gaps, and level_99 terminate the custom sequence. */
bool reflood_custom_next_header(const char *current,char *next,size_t capacity);

#endif
