#include "library.h"
#include "str.h"
#include "winapi.h"

typedef struct AlbumTag { const char* tag; const char* path; } AlbumTag;

static const AlbumTag k_tags[] = {
    { "KOF98 City Pop", "KOF 98 City Pop" },
    { "KOF99 City Pop", "KOF 99 City Pop" },
    { "KOF99 OST", "KOF 99 Soundtrack" },
    { "KOF99 OST 2016", "KOF 99 Soundtrack 2016" },
    { "KOF99 Arrange", "KOF 99 Arrange" },
    { "SF3 3rd Strike OST", "Street Fighter/Street Fighter III 3rd Strike - Soundtrack" },
    { "SF3 3rd Strike Arranged", "Street Fighter/Street Fighter III 3rd Strike - Arranged" },
    { "Ultra Street Fighter II", "Street Fighter/Ultra Street Fighter II" },
};

static const char* k_audio_ext[] = { "mp3", "flac", "wav", "m4a", "aac", "wma", "opus", "aiff", "aif", "mp4", 0 };

void lib_reset(Library* L) {
    memset(L, 0, sizeof(*L));
    L->folder_count = 1;
    L->folders[0].parent = -1;
}

int lib_folder(Library* L, int parent, const char* name) {
    int i;
    LibFolder* f;
    if (!name || !name[0]) return parent;
    for (i = 1; i < L->folder_count; ++i)
        if (L->folders[i].parent == parent && s_ieq(L->folders[i].name, name)) return i;
    if (L->folder_count >= LIB_FOLDERS) return parent;
    f = &L->folders[L->folder_count];
    memset(f, 0, sizeof(*f));
    s_copy(f->name, LIB_NAME, name);
    f->parent = parent;
    return L->folder_count++;
}

int lib_folder_path(Library* L, const char* path) {
    char part[LIB_NAME];
    int folder = 0;
    int n = 0;
    if (!path) return 0;
    for (;; ++path) {
        if (*path == '/' || *path == 0) {
            part[n] = 0;
            if (n) folder = lib_folder(L, folder, part);
            n = 0;
            if (!*path) break;
        } else if (n + 1 < LIB_NAME) {
            part[n++] = *path;
        }
    }
    return folder;
}

static const char* album_for_tag(const char* tag) {
    int i;
    for (i = 0; i < (int)(sizeof(k_tags) / sizeof(k_tags[0])); ++i)
        if (s_ieq(tag, k_tags[i].tag)) return k_tags[i].path;
    return tag;
}

void lib_assign_bundled(Library* L, const char* game_album) {
    int game = lib_folder(L, 0, game_album);
    int i;
    for (i = 0; i < L->cat.count; ++i) {
        BgmTrack* t = &L->cat.tracks[i];
        const char* close;
        char tag[LIB_NAME];
        int n;
        if (t->scanned) continue;
        t->album = game;
        if (!t->extra || t->title[0] != '[') continue;
        close = s_chr(t->title, ']');
        if (!close) continue;
        n = (int)(close - t->title) - 1;
        if (n <= 0 || n >= LIB_NAME) continue;
        memcpy(tag, t->title + 1, (unsigned long long)n);
        tag[n] = 0;
        t->album = lib_folder_path(L, album_for_tag(tag));
        ++close;
        while (*close == ' ') ++close;
        if (*close) {
            char rest[BGM_TITLE];
            s_copy(rest, BGM_TITLE, close);
            s_copy(t->title, BGM_TITLE, rest);
        }
    }
}

static char lower_ch(char c) {
    return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
}

static int is_digit(char c) {
    return c >= '0' && c <= '9';
}

int lib_natural_cmp(const char* a, const char* b) {
    if (!a) a = "";
    if (!b) b = "";
    while (*a && *b) {
        if (is_digit(*a) && is_digit(*b)) {
            const char* sa;
            const char* sb;
            int la, lb, i;
            while (*a == '0' && is_digit(a[1])) ++a;
            while (*b == '0' && is_digit(b[1])) ++b;
            sa = a;
            sb = b;
            while (is_digit(*a)) ++a;
            while (is_digit(*b)) ++b;
            la = (int)(a - sa);
            lb = (int)(b - sb);
            if (la != lb) return la < lb ? -1 : 1;
            for (i = 0; i < la; ++i)
                if (sa[i] != sb[i]) return sa[i] < sb[i] ? -1 : 1;
            continue;
        } else {
            char ca = lower_ch(*a), cb = lower_ch(*b);
            if (ca != cb) return (unsigned char)ca < (unsigned char)cb ? -1 : 1;
            ++a;
            ++b;
        }
    }
    if (*a) return 1;
    if (*b) return -1;
    return 0;
}

unsigned lib_parse_loop(const char* text) {
    int whole = 0, ms = 0, digits = 0;
    union { float f; unsigned u; } v;
    if (!text) return LIB_LOOP_FROM_START;
    while (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n' || (unsigned char)*text == 0xEF ||
           (unsigned char)*text == 0xBB || (unsigned char)*text == 0xBF) ++text;
    if (!is_digit(*text)) return LIB_LOOP_FROM_START;
    while (is_digit(*text) && whole < 100000) whole = whole * 10 + (*text++ - '0');
    if (*text == '.' || *text == ',') {
        ++text;
        while (is_digit(*text) && digits < 3) {
            ms = ms * 10 + (*text++ - '0');
            ++digits;
        }
        while (digits < 3) { ms *= 10; ++digits; }
    }
    ms += whole * 1000;
    if (ms <= 0) return LIB_LOOP_FROM_START;
    v.f = (float)ms / 1000.0f;
    return v.u;
}

void lib_stem_for(const wchar_t* rel, char* stem, int cap) {
    static const char hex[] = "0123456789ABCDEF";
    unsigned h = 2166136261u;
    int i;
    if (cap < 10) { if (cap > 0) stem[0] = 0; return; }
    for (; rel && *rel; ++rel) {
        wchar_t c = *rel;
        if (c >= 'A' && c <= 'Z') c = (wchar_t)(c + 32);
        if (c == '/') c = '\\';
        h ^= (unsigned)(c & 0xFF);
        h *= 16777619u;
        h ^= (unsigned)(c >> 8);
        h *= 16777619u;
    }
    stem[0] = 'M';
    for (i = 0; i < 8; ++i) stem[1 + i] = hex[(h >> (28 - i * 4)) & 15];
    stem[9] = 0;
}

int lib_ogg_check(const unsigned char* head, int n, int* rate, int* channels) {
    int segs, p;
    if (rate) *rate = 0;
    if (channels) *channels = 0;
    if (!head || n < 28 || head[0] != 'O' || head[1] != 'g' || head[2] != 'g' || head[3] != 'S') return 0;
    segs = head[26];
    p = 27 + segs;
    if (p + 16 > n) return 0;
    if (head[p] != 1 || head[p + 1] != 'v' || head[p + 2] != 'o' || head[p + 3] != 'r' || head[p + 4] != 'b' ||
        head[p + 5] != 'i' || head[p + 6] != 's') return 0;
    if (channels) *channels = head[p + 11];
    if (rate) *rate = (int)((unsigned)head[p + 12] | ((unsigned)head[p + 13] << 8) | ((unsigned)head[p + 14] << 16) | ((unsigned)head[p + 15] << 24));
    return 1;
}

static int read_head(const wchar_t* path, unsigned char* buf, int cap) {
    HANDLE f = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    DWORD got = 0;
    if (f == INVALID_HANDLE) return -1;
    if (!ReadFile(f, buf, (DWORD)cap, &got, 0)) got = 0;
    CloseHandle(f);
    return (int)got;
}

static int ext_is(const wchar_t* name, const char* ext) {
    const wchar_t* dot = w_rchr(name, L'.');
    int i;
    if (!dot) return 0;
    ++dot;
    for (i = 0; ext[i]; ++i) {
        wchar_t c = dot[i];
        if (c >= 'A' && c <= 'Z') c = (wchar_t)(c + 32);
        if (c != (wchar_t)ext[i]) return 0;
    }
    return dot[i] == 0;
}

static void to_utf8(const wchar_t* w, int wlen, char* out, int cap) {
    int n = WideCharToMultiByte(CP_UTF8, 0, w, wlen, out, cap - 1, 0, 0);
    if (n < 0) n = 0;
    out[n] = 0;
}

static void log_file(LibLogFn log, const char* why, const wchar_t* path) {
    char line[BGM_PATH * 3 + 96];
    char file[BGM_PATH * 3];
    if (!log) return;
    to_utf8(path, -1, file, (int)sizeof(file));
    s_copy(line, (int)sizeof(line), "musica ignorada (");
    s_cat(line, (int)sizeof(line), why);
    s_cat(line, (int)sizeof(line), "): ");
    s_cat(line, (int)sizeof(line), file);
    log(line);
}

static unsigned loop_for_file(const wchar_t* path) {
    wchar_t side[BGM_PATH];
    const wchar_t* dot;
    char text[64];
    int n;
    w_copy(side, BGM_PATH, path);
    dot = w_rchr(side, L'.');
    if (!dot) return LIB_LOOP_FROM_START;
    side[dot - side] = 0;
    if (w_len(side) + 6 >= BGM_PATH) return LIB_LOOP_FROM_START;
    w_cat(side, BGM_PATH, L".loop");
    n = read_head(side, (unsigned char*)text, (int)sizeof(text) - 1);
    if (n <= 0) return LIB_LOOP_FROM_START;
    text[n] = 0;
    return lib_parse_loop(text);
}

typedef struct ScanCtx {
    Library* L;
    int rate;
    int log_files;
    LibLogFn log;
    int others;
} ScanCtx;

static void add_file(ScanCtx* c, const wchar_t* abs, const wchar_t* rel, const wchar_t* name, int folder) {
    unsigned char head[160];
    int got, rate = 0, ch = 0, n;
    char stem[BGM_STEM];
    BgmTrack* t;
    const wchar_t* dot;
    if (w_len(abs) >= BGM_PATH - 1) {
        c->L->ignored++;
        if (c->log_files) log_file(c->log, "ruta demasiado larga", abs);
        return;
    }
    got = read_head(abs, head, (int)sizeof(head));
    if (!lib_ogg_check(head, got, &rate, &ch)) {
        c->L->ignored++;
        if (c->log_files) log_file(c->log, "no es OGG Vorbis", abs);
        return;
    }
    if (rate != c->rate) {
        c->L->ignored++;
        if (c->log_files) log_file(c->log, c->rate == 48000 ? "no es de 48000 Hz; preparala con TrackPrep" : "no es de 44100 Hz; preparala con TrackPrep", abs);
        return;
    }
    if (ch != 2) {
        c->L->ignored++;
        if (c->log_files) log_file(c->log, "no es estereo; preparala con TrackPrep", abs);
        return;
    }
    if (c->L->cat.count >= BGM_MAX) {
        c->L->ignored++;
        if (c->log_files) log_file(c->log, "biblioteca llena", abs);
        return;
    }
    lib_stem_for(rel, stem, BGM_STEM);
    if (bgm_find(&c->L->cat, stem)) return;
    if (folder == 0) {
        if (!c->others) c->others = lib_folder(c->L, 0, "Otros");
        folder = c->others;
    }
    t = &c->L->cat.tracks[c->L->cat.count++];
    memset(t, 0, sizeof(*t));
    s_copy(t->stem, BGM_STEM, stem);
    dot = w_rchr(name, L'.');
    n = dot ? (int)(dot - name) : w_len(name);
    to_utf8(name, n, t->title, BGM_TITLE);
    if (!t->title[0]) s_copy(t->title, BGM_TITLE, stem);
    w_copy(t->file, BGM_PATH, abs);
    t->album = folder;
    t->scanned = 1;
    t->extra = 1;
    t->has_loop = 1;
    t->loop_ms = -1;
    t->loop_bits = loop_for_file(abs);
    c->L->scanned++;
}

static void scan_dir(ScanCtx* c, const wchar_t* dir, const wchar_t* rel, int folder, int depth) {
    wchar_t pattern[BGM_PATH + 4];
    FIND_DATA_ fd;
    HANDLE h;
    if (w_len(dir) + 3 >= BGM_PATH) return;
    w_copy(pattern, BGM_PATH + 4, dir);
    w_cat(pattern, BGM_PATH + 4, L"\\*");
    h = FindFirstFileW(pattern, &fd);
    if (h == INVALID_HANDLE) return;
    do {
        wchar_t abs[BGM_PATH + 4];
        wchar_t sub_rel[BGM_PATH + 4];
        if (fd.name[0] == '.' || fd.name[0] == 0) continue;
        if (w_len(dir) + 1 + w_len(fd.name) >= BGM_PATH) {
            c->L->ignored++;
            continue;
        }
        w_copy(abs, BGM_PATH + 4, dir);
        w_cat(abs, BGM_PATH + 4, L"\\");
        w_cat(abs, BGM_PATH + 4, fd.name);
        w_copy(sub_rel, BGM_PATH + 4, rel);
        if (sub_rel[0]) w_cat(sub_rel, BGM_PATH + 4, L"\\");
        w_cat(sub_rel, BGM_PATH + 4, fd.name);
        if (fd.attributes & FILE_ATTRIBUTE_DIRECTORY) {
            char name[LIB_NAME];
            if (depth >= LIB_DEPTH) continue;
            to_utf8(fd.name, -1, name, LIB_NAME);
            scan_dir(c, abs, sub_rel, lib_folder(c->L, folder, name), depth + 1);
        } else if (ext_is(fd.name, "ogg")) {
            add_file(c, abs, sub_rel, fd.name, folder);
        } else {
            int i;
            for (i = 0; k_audio_ext[i]; ++i) {
                if (!ext_is(fd.name, k_audio_ext[i])) continue;
                c->L->ignored++;
                if (c->log_files) log_file(c->log, "formato no compatible; conviertela con TrackPrep", abs);
                break;
            }
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

int lib_scan_root(Library* L, const wchar_t* root, int sample_rate, int log_files, LibLogFn log) {
    ScanCtx c;
    DWORD a;
    int before = L->scanned;
    if (!root || !root[0]) return 0;
    a = GetFileAttributesW(root);
    if (a == INVALID_FILE_ATTRIBUTES || !(a & FILE_ATTRIBUTE_DIRECTORY)) return 0;
    memset(&c, 0, sizeof(c));
    c.L = L;
    c.rate = sample_rate;
    c.log_files = log_files;
    c.log = log;
    scan_dir(&c, root, L"", 0, 0);
    return L->scanned - before;
}

void lib_finish(Library* L) {
    int i;
    for (i = 0; i < L->folder_count; ++i) L->folders[i].total = 0;
    for (i = 0; i < L->cat.count; ++i) {
        int f = L->cat.tracks[i].album;
        int guard = 0;
        while (f >= 0 && f < L->folder_count && guard++ < LIB_FOLDERS) {
            L->folders[f].total++;
            f = L->folders[f].parent;
        }
    }
}

void lib_build(Library* L, const LibSpec* spec) {
    int r;
    lib_reset(L);
    bgm_catalog_load(&L->cat, spec->bgm_dir);
    lib_assign_bundled(L, spec->game_album);
    for (r = 0; r < 2; ++r) {
        if (!spec->roots[r] || !spec->roots[r][0]) continue;
        if (r == 1 && spec->roots[0] && w_ieq(spec->roots[0], spec->roots[1])) continue;
        lib_scan_root(L, spec->roots[r], spec->sample_rate, spec->log_files, spec->log);
    }
    lib_finish(L);
}

static int track_before(const Library* L, int a, int b) {
    const BgmTrack* ta = &L->cat.tracks[a];
    const BgmTrack* tb = &L->cat.tracks[b];
    if (ta->scanned != tb->scanned) return !ta->scanned;
    if (ta->scanned) {
        int c = lib_natural_cmp(ta->title, tb->title);
        if (c) return c < 0;
    }
    return a < b;
}

int lib_items(const Library* L, int folder, LibItem* out, int max) {
    int n = 0, first, i, j;
    if (!L || !out || max <= 0) return 0;
    if (folder < 0 || folder >= L->folder_count) folder = 0;
    if (folder != 0) {
        out[n].kind = LIB_BACK;
        out[n].index = L->folders[folder].parent;
        ++n;
    }
    first = n;
    for (i = 1; i < L->folder_count && n < max; ++i) {
        if (L->folders[i].parent != folder || L->folders[i].total <= 0) continue;
        j = n;
        while (j > first && lib_natural_cmp(L->folders[out[j - 1].index].name, L->folders[i].name) > 0) {
            out[j] = out[j - 1];
            --j;
        }
        out[j].kind = LIB_FOLDER;
        out[j].index = i;
        ++n;
    }
    first = n;
    for (i = 0; i < L->cat.count && n < max; ++i) {
        if (L->cat.tracks[i].album != folder) continue;
        j = n;
        while (j > first && track_before(L, i, out[j - 1].index)) {
            out[j] = out[j - 1];
            --j;
        }
        out[j].kind = LIB_TRACK;
        out[j].index = i;
        ++n;
    }
    return n;
}

int lib_item_find(const LibItem* items, int n, int kind, int index) {
    int i;
    for (i = 0; i < n; ++i)
        if (items[i].kind == kind && items[i].index == index) return i;
    return -1;
}

int lib_track_index(const Library* L, const char* stem) {
    int i;
    if (!L || !stem || !stem[0]) return -1;
    for (i = 0; i < L->cat.count; ++i)
        if (s_ieq(L->cat.tracks[i].stem, stem)) return i;
    return -1;
}

int lib_in_folder(const Library* L, int track, int folder) {
    int f, guard = 0;
    if (track < 0 || track >= L->cat.count) return 0;
    if (folder <= 0) return 1;
    f = L->cat.tracks[track].album;
    while (f >= 0 && f < L->folder_count && guard++ < LIB_FOLDERS) {
        if (f == folder) return 1;
        f = L->folders[f].parent;
    }
    return 0;
}

int lib_random(const Library* L, int folder, unsigned r, const char* avoid) {
    int i, n = 0, pick;
    int avoid_idx = lib_track_index(L, avoid);
    for (i = 0; i < L->cat.count; ++i)
        if (lib_in_folder(L, i, folder)) ++n;
    if (n <= 0) return -1;
    if (n > 1 && avoid_idx >= 0 && lib_in_folder(L, avoid_idx, folder)) {
        pick = (int)(r % (unsigned)(n - 1));
        for (i = 0; i < L->cat.count; ++i) {
            if (i == avoid_idx || !lib_in_folder(L, i, folder)) continue;
            if (pick-- == 0) return i;
        }
        return -1;
    }
    pick = (int)(r % (unsigned)n);
    for (i = 0; i < L->cat.count; ++i) {
        if (!lib_in_folder(L, i, folder)) continue;
        if (pick-- == 0) return i;
    }
    return -1;
}

void lib_crumb(const Library* L, int folder, char* out, int cap) {
    int chain[LIB_DEPTH + 8];
    int n = 0, f = folder;
    if (!out || cap <= 0) return;
    out[0] = 0;
    while (f > 0 && f < L->folder_count && n < (int)(sizeof(chain) / sizeof(chain[0]))) {
        chain[n++] = f;
        f = L->folders[f].parent;
    }
    while (n > 0) {
        s_cat(out, cap, L->folders[chain[--n]].name);
        if (n) s_cat(out, cap, " / ");
    }
}

int lib_redirect_path(const Library* L, const wchar_t* original, const char* stem, wchar_t* out, int cap) {
    const BgmTrack* t = bgm_find(&L->cat, stem);
    if (t && t->file[0]) {
        if (w_len(t->file) >= cap) return 0;
        w_copy(out, cap, t->file);
        return 1;
    }
    return bgm_build_redirect(original, stem, out, cap);
}
