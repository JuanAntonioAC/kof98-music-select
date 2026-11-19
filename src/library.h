#pragma once

#include "bgm.h"

/* Music library shown as folders ("albums").
   Folder 0 is the root. Every track belongs to exactly one folder:
   - the game's own soundtrack goes into one album,
   - the bundled extra tracks are grouped by their "[Album] Title" tag,
   - every subfolder of a Music folder becomes an album (nested folders too). */

#define LIB_FOLDERS 256
#define LIB_NAME 96
#define LIB_ITEMS (BGM_MAX + LIB_FOLDERS + 1)
#define LIB_DEPTH 6
#define LIB_LOOP_FROM_START 0x3A83126Fu

typedef struct LibFolder {
    char name[LIB_NAME];
    int parent;
    int total;
} LibFolder;

typedef struct Library {
    BgmCatalog cat;
    LibFolder folders[LIB_FOLDERS];
    int folder_count;
    int scanned;
    int ignored;
} Library;

enum { LIB_BACK = 1, LIB_FOLDER = 2, LIB_TRACK = 3 };

typedef struct LibItem {
    int kind;
    int index;
} LibItem;

typedef void (*LibLogFn)(const char* text);

typedef struct LibSpec {
    const wchar_t* bgm_dir;
    const char* game_album;
    int sample_rate;
    const wchar_t* roots[2];
    int log_files;
    LibLogFn log;
} LibSpec;

void lib_build(Library* L, const LibSpec* spec);
void lib_reset(Library* L);
int lib_folder(Library* L, int parent, const char* name);
int lib_folder_path(Library* L, const char* path);
void lib_assign_bundled(Library* L, const char* game_album);
int lib_scan_root(Library* L, const wchar_t* root, int sample_rate, int log_files, LibLogFn log);
void lib_finish(Library* L);

int lib_items(const Library* L, int folder, LibItem* out, int max);
int lib_item_find(const LibItem* items, int n, int kind, int index);
int lib_track_index(const Library* L, const char* stem);
int lib_random(const Library* L, int folder, unsigned r, const char* avoid);
int lib_in_folder(const Library* L, int track, int folder);
void lib_crumb(const Library* L, int folder, char* out, int cap);
int lib_redirect_path(const Library* L, const wchar_t* original, const char* stem, wchar_t* out, int cap);

int lib_ogg_check(const unsigned char* head, int n, int* rate, int* channels);
int lib_natural_cmp(const char* a, const char* b);
unsigned lib_parse_loop(const char* text);
void lib_stem_for(const wchar_t* rel, char* stem, int cap);
