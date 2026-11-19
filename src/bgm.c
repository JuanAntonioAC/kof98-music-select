#include "bgm.h"
#include "str.h"
#include "winapi.h"
#include "extra_tracks.h"

static const char* k_menu[] = {
    "TITLE_N", "TITLE_LOOP_N", "PS_N", "R_PS_N", "HTP_N", "CON_S_N", "MAE_N",
    "WIN_N", "RANKING_N", "BOSS_B_N", "M_DEMO_N", "CONTI_N", "SR_N_L", "SR_N_S",
    "HCNC_N", "I_RAN_N", "0002", "0751", "0755",
    "0723", "0724", "0725", "0726", "072B", "072C", "072D", "072E", "072F",
    "0749", "074A", "074B", "0750", "0752", "0754", "0756", 0
};

typedef struct NameRow { const char* stem; const char* title; } NameRow;

static const NameRow k_stage[] = {
    { "0730", "ESAKA? (equipo protagonista)" },
    { "0731", "Kurikinton (Fatal Fury)" },
    { "0732", "Art of Fight (Art of Fighting)" },
    { "0733", "Rumbling on the City (Ikari)" },
    { "0734", "Shudder Gong (Psycho Soldier)" },
    { "0735", "Fairy (chicas)" },
    { "0736", "Soul Town (Kim)" },
    { "0737", "Bloody (Orochi team)" },
    { "0738", "Stormy Saxophone 2 (Yagami)" },
    { "0739", "In Spite of One's Age (masters)" },
    { "073A", "Slum No.5 (Sports)" },
    { "073B", "XXX (Omega Rugal)" },
    { "073C", "C62 (Yamazaki)" },
    { "073D", "Blue Mary's Blues" },
    { "073E", "London March (Billy)" },
    { "073F", "Stormy Saxophone (Iori)" },
    { "0740", "Still Green (Shingo)" },
    { "0741", "The RR (Rugal)" },
    { "0742", "ESAKA '94 - KYO ALTERNO" },
    { "0743", "Rhythmic Hallucination (Yashiro)" },
    { "0744", "Fanatic Waltz (Shermie)" },
    { "0745", "Mad Fantasy (Chris)" },
    { "0746", "Hey! (chicas 94)" },
    { "0747", "ESAKA Forever (Kyo vs Iori)" },
    { "0748", "Cool Jam (Iori vs Kyo)" },
};

static const NameRow k_names[] = {
    { "KOF96GEES", "GEESE - Stiff Neck" },
    { "KOF96KRAUSER", "KRAUSER - Requiem K.626" },
    { "KOF96BIG", "MR. BIG - Dust Man" },
    { "KOF96GOENITZ", "GOENITZ - Trash Head" },
    { "KOF96JYOSEI", "CHICAS 96 - Get'n Up" },
    { "KOF96ATHENA", "ATHENA - Psycho Soldier Remix" },
    { "KOF96RYUUKO", "RYO 96 - Kamikiri" },
    { "KOF97BOUSOU", "IORI/LEONA - Control Crisis" },
    { "KOF97OROCHI", "OROCHI - The Origin of Mind" },
    { "KOF94RYUUKO", "RYO 94 - Ryuko no Hen" },
    { "KOF95JAPAN", "ESAKA - Funky (equipo 95)" },
    { "KOF95GAROU", "TERRY 95 - Club M" },
    { "KOF95IKARI", "IKARI 95 - Desert Requiem" },
    { "KOF95KING", "KING - Ground Creeping Bass" },
    { "ARF2EIJI", "EIJI - Way of the Shinobi" },
    { "ARF2YURI", "YURI - Diet" },
    { "KASUMI", "KASUMI - Art of Fight" },
    { "GD3YAMAZAKI", "YAMAZAKI - C62 Shirokuni" },
    { "GD3MARY", "MARY - Kiss Me" },
    { "RBBILLY", "BILLY - N.D.R" },
    { "RB2MAI", "MAI - Mai Mai Kyuun" },
    { "RB2GEESE", "GEESE - Soy Sauce" },
    { "0721", "Stormy Saxophone (Iori)" },
    { "0722", "XXX (Omega Rugal)" },
    { "0728", "Big Boss" },
    { "0729", "Blood" },
    { "072A", "Black Noah" },
    { "0753", "Get Advantage" },
};

static int is_menu(const char* stem) {
    int i;
    for (i = 0; k_menu[i]; ++i)
        if (s_ieq(stem, k_menu[i])) return 1;
    return 0;
}

static int is_extra(const char* stem) {
    int i;
    for (i = 0; i < (int)(sizeof(k_extra) / sizeof(k_extra[0])); ++i)
        if (s_ieq(stem, k_extra[i].stem)) return 1;
    return 0;
}

static int has_ci(const char* s, const char* token) {
    int n, m, i, j;
    if (!s || !token) return 0;
    n = s_len(s);
    m = s_len(token);
    for (i = 0; i + m <= n; ++i) {
        for (j = 0; j < m; ++j) {
            char a = s[i + j];
            char b = token[j];
            if (a >= 'a' && a <= 'z') a = (char)(a - 32);
            if (b >= 'a' && b <= 'z') b = (char)(b - 32);
            if (a != b) break;
        }
        if (j == m) return 1;
    }
    return 0;
}

/* Hide jingles and duplicate rips from the picker. Fight themes (0730, KOF95*) stay. */
static int is_picker_skip(const char* stem) {
    static const char* bad[] = {
        "WIN", "TITLE", "INTRO", "MAE", "RANK", "HTP", "DEMO", "CONTI",
        "SELECT", "STAFF", "HOWTO", "CONTINUE", 0
    };
    int i;
    if (!stem || !stem[0]) return 1;
    for (i = 0; bad[i]; ++i)
        if (has_ci(stem, bad[i])) return 1;
    /* 0730_D15 / WIN_N_PS / KOF96GEES_PS — duplicates, not the real theme. */
    if (stem[4] == '_') {
        int hex = 1;
        for (i = 0; i < 4; ++i) {
            char c = stem[i];
            if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) hex = 0;
        }
        if (hex) return 1;
    }
    if (has_ci(stem, "_PS")) return 1;
    return 0;
}

static const char* title_for(const char* stem) {
    int i;
    for (i = 0; i < (int)(sizeof(k_stage) / sizeof(k_stage[0])); ++i)
        if (s_ieq(stem, k_stage[i].stem)) return k_stage[i].title;
    for (i = 0; i < (int)(sizeof(k_names) / sizeof(k_names[0])); ++i)
        if (s_ieq(stem, k_names[i].stem)) return k_names[i].title;
    return 0;
}

static int parse_clock(const char* text) {
    int min, sec, ms = 0;
    const char* colon = s_chr(text, ':');
    const char* dot;
    char frac[4];
    int n = 0;
    if (!colon) return 0;
    min = s_atoi(text);
    sec = s_atoi(colon + 1);
    dot = s_chr(colon + 1, '.');
    if (dot) {
        const char* p = dot + 1;
        while (*p >= '0' && *p <= '9' && n < 3) frac[n++] = *p++;
        while (n < 3) frac[n++] = '0';
        frac[3] = 0;
        ms = s_atoi(frac);
    }
    return ((min * 60 + sec) * 1000) + ms;
}

static void add_track(BgmCatalog* cat, const char* stem, int duration_ms, int has_loop) {
    BgmTrack* t;
    const char* title;
    if (cat->count >= BGM_MAX || !stem || !stem[0] || is_menu(stem) || is_extra(stem) || is_picker_skip(stem)) return;
    if (duration_ms < 2500) return;
    if (bgm_find(cat, stem)) return;
    t = &cat->tracks[cat->count++];
    s_copy(t->stem, BGM_STEM, stem);
    s_upper(t->stem);
    title = title_for(t->stem);
    if (title) s_copy(t->title, BGM_TITLE, title);
    else {
        s_copy(t->title, BGM_TITLE, "Pista ");
        s_cat(t->title, BGM_TITLE, t->stem);
    }
    t->duration_ms = duration_ms;
    t->has_loop = has_loop;
    t->loop_ms = has_loop ? duration_ms : -1;
}

static void load_loop_csv(BgmCatalog* cat, const wchar_t* bgm_dir) {
    wchar_t path[MAX_PATH];
    HANDLE f;
    char buf[8192];
    DWORD got = 0;
    int i = 0;
    w_copy(path, MAX_PATH, bgm_dir);
    w_cat(path, MAX_PATH, L"\\loop.csv");
    f = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (f == INVALID_HANDLE) return;
    ReadFile(f, buf, (DWORD)sizeof(buf) - 1, &got, 0);
    CloseHandle(f);
    buf[got] = 0;
    while (buf[i]) {
        int start = i;
        char stem[BGM_STEM];
        const char* comma;
        int n;
        while (buf[i] && buf[i] != '\n') ++i;
        if (buf[i] == '\n') { buf[i] = 0; ++i; }
        comma = s_chr(buf + start, ',');
        if (!comma) continue;
        n = (int)(comma - (buf + start));
        if (n <= 0 || n >= BGM_STEM) continue;
        memcpy(stem, buf + start, (unsigned long long)n);
        stem[n] = 0;
        add_track(cat, stem, parse_clock(comma + 1), 1);
    }
}

static int file_exists(const wchar_t* path) {
    DWORD a = GetFileAttributesW(path);
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

void bgm_catalog_load(BgmCatalog* cat, const wchar_t* bgm_dir) {
    wchar_t pattern[MAX_PATH];
    FIND_DATA_ fd;
    HANDLE h;
    memset(cat, 0, sizeof(*cat));
    if (!bgm_dir || !bgm_dir[0]) return;
    load_loop_csv(cat, bgm_dir);
    w_copy(pattern, MAX_PATH, bgm_dir);
    w_cat(pattern, MAX_PATH, L"\\*.ogg");
    h = FindFirstFileW(pattern, &fd);
    if (h == INVALID_HANDLE) return;
    do {
        char stem[BGM_STEM];
        wchar_t full[MAX_PATH];
        int n, i;
        if (fd.attributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        n = WideCharToMultiByte(CP_UTF8, 0, fd.name, -1, stem, BGM_STEM, 0, 0);
        if (n <= 0) continue;
        for (i = 0; stem[i]; ++i) {
            if (stem[i] == '.') { stem[i] = 0; break; }
        }
        w_copy(full, MAX_PATH, bgm_dir);
        w_cat(full, MAX_PATH, L"\\");
        w_cat(full, MAX_PATH, fd.name);
        if (!file_exists(full)) continue;
        if (!bgm_find(cat, stem) && !is_menu(stem)) add_track(cat, stem, 8000, 0);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    {
        int a, b;
        for (a = 1; a < cat->count; ++a) {
            BgmTrack key = cat->tracks[a];
            int ka = (key.title[0] == 'G') ? 0 : ((key.title[0] == 'E') ? 1 : ((key.stem[0] >= '0' && key.stem[0] <= '9') ? 3 : 2));
            b = a - 1;
            while (b >= 0) {
                int kb = (cat->tracks[b].title[0] == 'G') ? 0 : ((cat->tracks[b].title[0] == 'E') ? 1 : ((cat->tracks[b].stem[0] >= '0' && cat->tracks[b].stem[0] <= '9') ? 3 : 2));
                if (kb < ka || (kb == ka && lstrcmpA(cat->tracks[b].title, key.title) <= 0)) break;
                cat->tracks[b + 1] = cat->tracks[b];
                --b;
            }
            cat->tracks[b + 1] = key;
        }
    }
    {
        int i;
        for (i = 0; i < (int)(sizeof(k_extra) / sizeof(k_extra[0])) && cat->count < BGM_MAX; ++i) {
            wchar_t full[MAX_PATH];
            wchar_t file[BGM_STEM];
            BgmTrack* t;
            if (MultiByteToWideChar(CP_UTF8, 0, k_extra[i].stem, -1, file, BGM_STEM) <= 0) continue;
            w_copy(full, MAX_PATH, bgm_dir);
            w_cat(full, MAX_PATH, L"\\");
            w_cat(full, MAX_PATH, file);
            w_cat(full, MAX_PATH, L".ogg");
            if (!file_exists(full) || bgm_find(cat, k_extra[i].stem)) continue;
            t = &cat->tracks[cat->count++];
            s_copy(t->stem, BGM_STEM, k_extra[i].stem);
            s_copy(t->title, BGM_TITLE, k_extra[i].title);
            t->duration_ms = 0;
            t->has_loop = 1;
            t->loop_ms = -1;
            t->extra = 1;
            t->loop_bits = k_extra[i].loop_bits;
        }
    }
}

int bgm_stem_from_path(const wchar_t* path, char* stem, int stem_cap) {
    const wchar_t* slash;
    const wchar_t* name;
    const wchar_t* dot;
    int n, i;
    if (!path || !stem || stem_cap < 2) return 0;
    slash = w_rchr(path, L'\\');
    if (!slash) slash = w_rchr(path, L'/');
    name = slash ? slash + 1 : path;
    dot = w_rchr(name, L'.');
    if (!dot || !w_ieq(dot, L".ogg")) return 0;
    n = WideCharToMultiByte(CP_UTF8, 0, name, (int)(dot - name), stem, stem_cap - 1, 0, 0);
    if (n <= 0) return 0;
    stem[n] = 0;
    for (i = 0; stem[i]; ++i)
        if (stem[i] >= 'a' && stem[i] <= 'z') stem[i] = (char)(stem[i] - 32);
    return 1;
}

const BgmTrack* bgm_find(const BgmCatalog* cat, const char* stem) {
    int i;
    if (!cat || !stem) return 0;
    for (i = 0; i < cat->count; ++i)
        if (s_ieq(cat->tracks[i].stem, stem)) return &cat->tracks[i];
    return 0;
}

BgmOpenKind bgm_classify(const BgmCatalog* cat, const wchar_t* path) {
    char stem[BGM_STEM];
    if (!bgm_stem_from_path(path, stem, BGM_STEM)) return BGM_OPEN_IGNORE;
    if (!w_has(path, L"\\bgm\\") && !w_has(path, L"/bgm/")) return BGM_OPEN_IGNORE;
    if (s_ieq(stem, "PS_N")) return BGM_OPEN_SELECT;
    if (s_ieq(stem, "MAE_N")) return BGM_OPEN_CUE;
    if (is_menu(stem)) return BGM_OPEN_IGNORE;
    if (cat && bgm_find(cat, stem)) return BGM_OPEN_FIGHT;
    return BGM_OPEN_IGNORE;
}

int bgm_build_redirect(const wchar_t* original, const char* stem, wchar_t* out, int out_cap) {
    const wchar_t* slash;
    int dir, n;
    wchar_t file[80];
    if (!original || !stem || !stem[0] || !out || out_cap < 16) return 0;
    slash = w_rchr(original, L'\\');
    if (!slash) slash = w_rchr(original, L'/');
    if (!slash) return 0;
    dir = (int)(slash - original);
    n = MultiByteToWideChar(CP_UTF8, 0, stem, -1, file, 70);
    if (n <= 0) return 0;
    if (dir + 1 + w_len(file) + 5 >= out_cap) return 0;
    memcpy(out, original, (unsigned long long)dir * sizeof(wchar_t));
    out[dir] = slash[0];
    w_copy(out + dir + 1, out_cap - dir - 1, file);
    w_cat(out, out_cap, L".ogg");
    return 1;
}
