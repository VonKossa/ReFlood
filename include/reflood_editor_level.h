#ifndef REFLOOD_EDITOR_LEVEL_H
#define REFLOOD_EDITOR_LEVEL_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#define EDITOR_W 128u
#define EDITOR_H 100u
#define EDITOR_MAP_BYTES (EDITOR_W*EDITOR_H)
#define EDITOR_TRIGGER_BYTES 640u
typedef struct {
    uint8_t header[16];
    uint8_t map[EDITOR_MAP_BYTES];
    uint8_t triggers[EDITOR_TRIGGER_BYTES];
    unsigned level;
    bool changed;
} EditorLevel;
void editor_level_new(EditorLevel *level, unsigned number);
bool editor_level_load(EditorLevel *level, const char *directory, unsigned number,
    char *error, size_t capacity);
bool editor_level_load_header(EditorLevel *level, const char *header_path,
    char *error, size_t capacity);
bool editor_level_save_named(const EditorLevel *level,const char *directory,
    const char *name,char *error,size_t capacity);
bool editor_level_named_exists(const char *directory,const char *name);
bool editor_level_ensure_output_directory(const char *output,char *error,size_t capacity);
bool editor_level_validate(const EditorLevel *level, char *error, size_t capacity);
bool editor_level_save(const EditorLevel *level, const char *directory,
    char *error, size_t capacity);
unsigned editor_level_bank(const EditorLevel *level);
unsigned editor_level_trigger_count(const EditorLevel *level);
#endif
