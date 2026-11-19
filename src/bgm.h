#pragma once

#ifndef KOF98_WCHAR
#define KOF98_WCHAR
typedef unsigned short wchar_t;
#endif

#define BGM_STEM 32
#define BGM_TITLE 128
#define BGM_MAX 1024
#define BGM_PATH 260

typedef struct BgmTrack {
    char stem[BGM_STEM];
    char title[BGM_TITLE];
    int duration_ms;
    int loop_ms;
    int has_loop;
    int extra;
    unsigned loop_bits;
    int album;
    int scanned;
    wchar_t file[BGM_PATH];
} BgmTrack;

typedef struct BgmCatalog {
    BgmTrack tracks[BGM_MAX];
    int count;
} BgmCatalog;

typedef enum BgmOpenKind {
    BGM_OPEN_IGNORE = 0,
    BGM_OPEN_SELECT,
    BGM_OPEN_CUE,
    BGM_OPEN_FIGHT
} BgmOpenKind;

void bgm_catalog_load(BgmCatalog* cat, const wchar_t* bgm_dir);
int bgm_stem_from_path(const wchar_t* path, char* stem, int stem_cap);
BgmOpenKind bgm_classify(const BgmCatalog* cat, const wchar_t* path);
const BgmTrack* bgm_find(const BgmCatalog* cat, const char* stem);
int bgm_build_redirect(const wchar_t* original, const char* stem, wchar_t* out, int out_cap);
