#ifndef REFLOOD_EDITOR_BROWSER_H
#define REFLOOD_EDITOR_BROWSER_H
#include <stdbool.h>
#include <stddef.h>
#define EDITOR_BROWSER_PATH 1024
#define EDITOR_BROWSER_ENTRIES 512
typedef struct {
    char name[256];
    bool directory;
} EditorBrowserEntry;
typedef struct {
    char directory[EDITOR_BROWSER_PATH];
    EditorBrowserEntry entries[EDITOR_BROWSER_ENTRIES];
    unsigned count,selected,offset;
} EditorBrowser;
bool editor_browser_open(EditorBrowser *browser,const char *directory,char *error,size_t capacity);
bool editor_browser_parent(EditorBrowser *browser,char *error,size_t capacity);
/* Open a directory, or return a selected header path in selected_path. */
bool editor_browser_activate(EditorBrowser *browser,char *selected_path,size_t path_capacity,
    char *error,size_t error_capacity);
#endif
