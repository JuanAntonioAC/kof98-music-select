#include "bgm.h"
#include "library.h"
#include "menu.h"
#include "str.h"
#include "winapi.h"

static int g_fail;
static int g_checks;
static BgmCatalog g_catalog;
static Library g_test_lib;

static void out(const char* text) {
    DWORD w = 0;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    WriteFile(h, text, (DWORD)s_len(text), &w, 0);
}

static void expect(int cond, const char* msg) {
    ++g_checks;
    if (cond) return;
    out("FALLO: ");
    out(msg);
    out("\r\n");
    g_fail = 1;
}

static void write_file(const wchar_t* path, const void* data, int n) {
    HANDLE f = CreateFileW(path, GENERIC_READ | 0x40000000u, 0, 0, 2, FILE_ATTRIBUTE_NORMAL, 0);
    DWORD w = 0;
    if (f == INVALID_HANDLE) return;
    WriteFile(f, data, (DWORD)n, &w, 0);
    CloseHandle(f);
}

static int fake_ogg(unsigned char* b, int rate, int channels) {
    int i;
    for (i = 0; i < 96; ++i) b[i] = 0;
    b[0] = 'O'; b[1] = 'g'; b[2] = 'g'; b[3] = 'S';
    b[5] = 2;
    b[26] = 1;
    b[27] = 30;
    b[28] = 1;
    b[29] = 'v'; b[30] = 'o'; b[31] = 'r'; b[32] = 'b'; b[33] = 'i'; b[34] = 's';
    b[39] = (unsigned char)channels;
    b[40] = (unsigned char)(rate & 0xFF);
    b[41] = (unsigned char)((rate >> 8) & 0xFF);
    b[42] = (unsigned char)((rate >> 16) & 0xFF);
    b[43] = (unsigned char)((rate >> 24) & 0xFF);
    return 96;
}

static void make_ogg(const wchar_t* dir, const wchar_t* name, int rate, int channels) {
    wchar_t path[MAX_PATH];
    unsigned char b[96];
    int n = fake_ogg(b, rate, channels);
    w_copy(path, MAX_PATH, dir);
    w_cat(path, MAX_PATH, L"\\");
    w_cat(path, MAX_PATH, name);
    write_file(path, b, n);
}

static void join(wchar_t* out_path, const wchar_t* a, const wchar_t* b) {
    w_copy(out_path, MAX_PATH, a);
    w_cat(out_path, MAX_PATH, L"\\");
    w_cat(out_path, MAX_PATH, b);
}

static int find_folder(const Library* L, int parent, const char* name) {
    int i;
    for (i = 1; i < L->folder_count; ++i)
        if (L->folders[i].parent == parent && s_ieq(L->folders[i].name, name)) return i;
    return -1;
}

static const char* item_name(const Library* L, const LibItem* it) {
    if (it->kind == LIB_BACK) return "<volver>";
    if (it->kind == LIB_FOLDER) return L->folders[it->index].name;
    return L->cat.tracks[it->index].title;
}

static void add_bundled(Library* L, const char* stem, const char* title, int extra) {
    BgmTrack* t = &L->cat.tracks[L->cat.count++];
    memset(t, 0, sizeof(*t));
    s_copy(t->stem, BGM_STEM, stem);
    s_copy(t->title, BGM_TITLE, title);
    t->extra = extra;
}

static void test_catalog(void) {
    BgmCatalog* cat = &g_catalog;
    wchar_t redirected[260];
    char stem[32];
    memset(cat, 0, sizeof(*cat));
    expect(bgm_stem_from_path(L"D:\\game\\Data\\game\\audio\\bgm\\ps_n.ogg", stem, 32), "stem");
    expect(s_ieq(stem, "PS_N"), "stem mayusculas");
    expect(bgm_classify(cat, L"data/game/audio/bgm/PS_N.ogg") == BGM_OPEN_SELECT, "select");
    expect(bgm_classify(cat, L"data/game/audio/bgm/MAE_N.ogg") == BGM_OPEN_CUE, "entrada");
    expect(bgm_classify(cat, L"D:\\game\\Data\\game\\audio\\bgm\\WIN_N.ogg") == BGM_OPEN_IGNORE, "win");
    expect(bgm_classify(cat, L"D:\\game\\Data\\game\\audio\\voice\\PS_N.ogg") == BGM_OPEN_IGNORE, "fuera de bgm");
    s_copy(cat->tracks[0].stem, 32, "0730");
    s_copy(cat->tracks[0].title, 64, "Pista 0730");
    cat->tracks[0].duration_ms = 36000;
    cat->count = 1;
    expect(bgm_classify(cat, L"D:\\game\\Data\\game\\audio\\bgm\\0730.ogg") == BGM_OPEN_FIGHT, "pelea");
    expect(bgm_build_redirect(L"D:\\game\\Data\\game\\audio\\bgm\\0730.ogg", "KOF96GEES", redirected, 260), "redirect");
    expect(w_ieq(redirected, L"D:\\game\\Data\\game\\audio\\bgm\\KOF96GEES.ogg"), "ruta");
}

static void test_helpers(void) {
    unsigned char b[96];
    int rate = 0, ch = 0;
    char s1[16], s2[16];
    expect(lib_natural_cmp("Track 2", "Track 10") < 0, "orden natural 2 < 10");
    expect(lib_natural_cmp("track 02", "Track 2") == 0, "orden natural sin mayusculas ni ceros");
    expect(lib_natural_cmp("Album B", "album a") > 0, "orden alfabetico");
    expect(lib_parse_loop("12.5\r\n") == 0x41480000u, "loop 12.5 s");
    expect(lib_parse_loop("0.000000") == LIB_LOOP_FROM_START, "loop desde el inicio");
    expect(lib_parse_loop("abc") == LIB_LOOP_FROM_START, "loop invalido");
    fake_ogg(b, 44100, 2);
    expect(lib_ogg_check(b, 96, &rate, &ch) && rate == 44100 && ch == 2, "cabecera ogg vorbis");
    b[29] = 'X';
    expect(!lib_ogg_check(b, 96, &rate, &ch), "ogg que no es vorbis");
    expect(!lib_ogg_check((const unsigned char*)"ID3xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 40, &rate, &ch), "mp3 renombrado");
    lib_stem_for(L"Album\\Song.ogg", s1, 16);
    lib_stem_for(L"album/SONG.OGG", s2, 16);
    expect(s_ieq(s1, s2) && s1[0] == 'M' && s_len(s1) == 9, "id estable por ruta");
}

static void test_library(void) {
    Library* L = &g_test_lib;
    wchar_t root[MAX_PATH], music[MAX_PATH], dir[MAX_PATH], sub[MAX_PATH], path[MAX_PATH];
    LibItem items[LIB_ITEMS];
    int n, kof, sf, sf3, others, i, hits[8];
    unsigned r;
    GetTempPathW(MAX_PATH, root);
    w_cat(root, MAX_PATH, L"kofbgm-test");
    CreateDirectoryW(root, 0);
    join(music, root, L"Music");
    CreateDirectoryW(music, 0);
    join(dir, music, L"KOF 98 City Pop");
    CreateDirectoryW(dir, 0);
    make_ogg(dir, L"Track 10.ogg", 44100, 2);
    make_ogg(dir, L"Track 2.ogg", 44100, 2);
    join(path, dir, L"Track 2.loop");
    write_file(path, "12.5", 4);
    make_ogg(dir, L"bad48.ogg", 48000, 2);
    make_ogg(dir, L"mono.ogg", 44100, 1);
    join(path, dir, L"song.mp3");
    write_file(path, "ID3", 3);
    join(path, dir, L"cover.txt");
    write_file(path, "x", 1);
    join(dir, music, L"Street Fighter");
    CreateDirectoryW(dir, 0);
    join(sub, dir, L"SF3");
    CreateDirectoryW(sub, 0);
    make_ogg(sub, L"Theme.ogg", 44100, 2);
    join(dir, music, L"Vacia");
    CreateDirectoryW(dir, 0);
    join(sub, dir, L"Sub");
    CreateDirectoryW(sub, 0);
    make_ogg(music, L"suelta.ogg", 44100, 2);

    lib_reset(L);
    add_bundled(L, "0730", "ESAKA", 0);
    add_bundled(L, "CP98_01", "[KOF98 City Pop] Bundled", 1);
    add_bundled(L, "SF3_01", "[SF3 3rd Strike OST] Intro", 1);
    lib_assign_bundled(L, "KOF 98 UM - Soundtrack");
    n = lib_scan_root(L, music, 44100, 0, 0);
    lib_finish(L);
    expect(n == 4, "4 canciones validas en Music");
    expect(L->ignored == 3, "48 kHz, mono y mp3 ignorados");
    expect(s_ieq(L->cat.tracks[1].title, "Bundled"), "titulo sin [etiqueta]");

    kof = find_folder(L, 0, "KOF 98 City Pop");
    sf = find_folder(L, 0, "Street Fighter");
    others = find_folder(L, 0, "Otros");
    expect(kof > 0 && sf > 0 && others > 0, "carpetas detectadas");
    sf3 = find_folder(L, sf, "SF3");
    expect(sf3 > 0, "subcarpeta detectada");
    expect(find_folder(L, sf, "Street Fighter III 3rd Strike - Soundtrack") > 0, "album incluido dentro de Street Fighter");

    n = lib_items(L, 0, items, LIB_ITEMS);
    expect(n == 4, "raiz: 4 albumes (la carpeta vacia no aparece)");
    if (n == 4) {
        expect(s_ieq(item_name(L, &items[0]), "KOF 98 City Pop"), "raiz orden 1");
        expect(s_ieq(item_name(L, &items[1]), "KOF 98 UM - Soundtrack"), "raiz orden 2");
        expect(s_ieq(item_name(L, &items[2]), "Otros"), "raiz orden 3");
        expect(s_ieq(item_name(L, &items[3]), "Street Fighter"), "raiz orden 4");
    }
    for (i = 0; i < n; ++i) expect(items[i].kind == LIB_FOLDER, "raiz solo carpetas");

    n = lib_items(L, kof, items, LIB_ITEMS);
    expect(n == 4, "album: volver + 3 canciones");
    if (n == 4) {
        expect(items[0].kind == LIB_BACK, "primera fila: volver");
        expect(s_ieq(item_name(L, &items[1]), "Bundled"), "primero las incluidas");
        expect(s_ieq(item_name(L, &items[2]), "Track 2"), "Track 2 antes que Track 10");
        expect(s_ieq(item_name(L, &items[3]), "Track 10"), "Track 10 al final");
        expect(L->cat.tracks[items[2].index].loop_bits == 0x41480000u, "loop desde archivo .loop");
        expect(L->cat.tracks[items[3].index].loop_bits == LIB_LOOP_FROM_START, "sin .loop repite desde el inicio");
        {
            wchar_t red[MAX_PATH];
            expect(lib_redirect_path(L, L"X:\\bgm\\0730.ogg", L->cat.tracks[items[3].index].stem, red, MAX_PATH) &&
                   w_has(red, L"Track 10.ogg") && w_has(red, L"kofbgm-test"), "redireccion a la ruta del album");
        }
    }
    n = lib_items(L, sf, items, LIB_ITEMS);
    expect(n == 3 && items[0].kind == LIB_BACK && items[1].kind == LIB_FOLDER && items[2].kind == LIB_FOLDER, "Street Fighter: volver + 2 subcarpetas");

    for (i = 0; i < 8; ++i) hits[i] = 0;
    for (r = 0; r < 200; ++r) {
        int t = lib_random(L, kof, r * 2654435761u, 0);
        expect(t >= 0 && lib_in_folder(L, t, kof), "azar dentro del album");
    }
    for (r = 0; r < 200; ++r) {
        int t = lib_random(L, sf, r * 2654435761u, 0);
        expect(t >= 0 && lib_in_folder(L, t, sf), "azar dentro de Street Fighter (incluye subcarpetas)");
    }
    for (r = 0; r < 400; ++r) {
        int t = lib_random(L, 0, r * 2654435761u, 0);
        expect(t >= 0 && t < L->cat.count, "azar en toda la biblioteca");
        if (t >= 0 && t < 8) hits[t]++;
    }
    for (i = 0; i < L->cat.count && i < 8; ++i) expect(hits[i] > 0, "el azar de la raiz alcanza todas las canciones");
    {
        int only = lib_random(L, sf3, 7, L->cat.tracks[lib_items(L, sf3, items, LIB_ITEMS) > 1 ? items[1].index : 0].stem);
        expect(only >= 0, "album de una cancion: el azar no queda vacio");
    }
    {
        char first[BGM_STEM];
        int count = L->cat.count;
        s_copy(first, BGM_STEM, L->cat.tracks[count - 1].stem);
        lib_reset(L);
        lib_scan_root(L, music, 44100, 0, 0);
        lib_finish(L);
        expect(lib_track_index(L, first) >= 0, "mismo id al volver a escanear");
        lib_reset(L);
        lib_scan_root(L, music, 48000, 0, 0);
        lib_finish(L);
        expect(L->scanned == 1 && L->ignored == 6, "KOF 2002: solo acepta 48000 Hz");
    }
}

static void test_menu(void) {
    MenuPad m;
    MenuInput in;
    unsigned a;
    expect(menu_buttons(0, 30000, 0) == MENU_RIGHT, "stick derecha = pagina");
    expect(menu_buttons(0, -30000, 2000) == MENU_LEFT, "stick izquierda = pagina");
    expect(menu_buttons(0, 3000, -30000) == MENU_DOWN, "stick abajo");
    expect(menu_buttons(0x0040 | 0x1000, 0, 0) == 0, "L3 y A no son acciones del menu");

    menu_pad_reset(&m, 0);
    a = menu_pad_feed(&m, MENU_START, 1000);
    expect(a == 0, "START espera un instante por si llega D");
    a = menu_pad_feed(&m, MENU_START, 1050);
    expect(a == 0, "START sigue esperando");
    a = menu_pad_feed(&m, MENU_START, 1095);
    expect(a == MA_PICK, "START solo = elegir");
    a = menu_pad_feed(&m, MENU_START | MENU_D, 1200);
    expect(a == 0, "D despues de elegir con START mantenido no hace nada");
    a = menu_pad_feed(&m, 0, 1300);
    expect(a == 0, "soltar no hace nada");

    a = menu_pad_feed(&m, MENU_D, 2000);
    a |= menu_pad_feed(&m, 0, 2030);
    expect(a == MA_PICK, "toque corto de D = elegir");

    a = menu_pad_feed(&m, MENU_START, 3000);
    a |= menu_pad_feed(&m, MENU_START | MENU_D, 3040);
    expect(a == MA_RANDOM, "START y luego D = azar (sin elegir)");
    a = menu_pad_feed(&m, MENU_START | MENU_D, 3300);
    a |= menu_pad_feed(&m, MENU_D, 3400);
    a |= menu_pad_feed(&m, 0, 3500);
    expect(a == 0, "sin acciones dobles al soltar");

    a = menu_pad_feed(&m, MENU_START | MENU_D, 4000);
    expect(a == MA_RANDOM, "START + D a la vez = azar");
    menu_pad_feed(&m, 0, 4100);

    a = menu_pad_feed(&m, MENU_D, 5000);
    a |= menu_pad_feed(&m, MENU_D | MENU_START, 5020);
    expect(a == MA_RANDOM, "D y luego START = azar");
    menu_pad_feed(&m, 0, 5100);

    menu_pad_reset(&m, MENU_START);
    a = menu_pad_feed(&m, MENU_START, 6000);
    a |= menu_pad_feed(&m, MENU_START, 6200);
    expect(a == 0, "START mantenido al abrir no elige");
    a = menu_pad_feed(&m, 0, 6300);
    a |= menu_pad_feed(&m, MENU_BACK, 6400);
    expect(a == MA_BACK, "SELECT = volver");
    menu_pad_feed(&m, 0, 6500);

    a = menu_pad_feed(&m, MENU_DOWN, 7000);
    expect(a == MA_DOWN, "abajo");
    a = menu_pad_feed(&m, MENU_DOWN, 7300);
    expect(a == 0, "sin repeticion antes de tiempo");
    a = menu_pad_feed(&m, MENU_DOWN, 7000 + MENU_REPEAT_DELAY_MS);
    expect(a == MA_DOWN, "repeticion al mantener");
    menu_pad_feed(&m, 0, 7600);

    memset(&in, 0, sizeof(in));
    in.source = -1;
    menu_begin(&in, 1);
    a = menu_feed(&in, 5, MENU_RIGHT, 100);
    menu_end(&in);
    expect(a == 0, "mando nuevo con boton mantenido no actua");
    menu_begin(&in, 1);
    a = menu_feed(&in, 5, 0, 115);
    a |= menu_feed(&in, 6, 0, 115);
    menu_end(&in);
    menu_begin(&in, 1);
    a = menu_feed(&in, 5, MENU_RIGHT, 130);
    a |= menu_feed(&in, 6, MENU_RIGHT, 130);
    menu_end(&in);
    expect(menu_resolve(a) == MA_RIGHT, "duplicado de Steam: una sola pagina");
    menu_begin(&in, 1);
    menu_feed(&in, 6, 0, 145);
    menu_end(&in);
    expect(in.pads[0].used == 0 || in.pads[0].id != 5, "mando desconectado se olvida");
    menu_begin(&in, 0);
    a = menu_feed(&in, 6, MENU_RIGHT, 160);
    menu_end(&in);
    expect(a == 0, "cambio de fuente (selector/XInput) no dispara acciones");

    expect(menu_resolve(MA_PICK | MA_RANDOM) == MA_RANDOM, "azar tiene prioridad");
    expect(menu_resolve(MA_DOWN | MA_RIGHT) == MA_DOWN, "diagonal: solo una accion");

    expect(menu_step(0, 10, -1) == 9, "arriba en la primera fila va a la ultima");
    expect(menu_step(9, 10, 1) == 0, "abajo en la ultima va a la primera");
    expect(menu_page(3, 30, 14, 1) == 17, "pagina siguiente misma fila");
    expect(menu_page(17, 30, 14, 1) == 29, "ultima pagina incompleta");
    expect(menu_page(29, 30, 14, 1) == 1, "despues de la ultima vuelve a la primera");
    expect(menu_page(1, 30, 14, -1) == 29, "antes de la primera va a la ultima");
    expect(menu_page(5, 10, 14, 1) == 5, "una sola pagina");
    expect(menu_page(0, 0, 14, 1) == 0, "lista vacia");
    {
        int i, idx = 0, ok = 1;
        for (i = 0; i < 50; ++i) {
            int next = menu_page(idx, 30, 14, 1);
            if (next < 0 || next >= 30) ok = 0;
            idx = next;
        }
        expect(ok, "50 paginas seguidas sin salirse de la lista");
    }
}

__declspec(dllimport) DWORD WINAPI GetEnvironmentVariableW(const wch*, wch*, DWORD);

static void out_num(int n) {
    char digits[12], buf[16];
    int k = 0, i = 0;
    if (n < 0) { buf[i++] = '-'; n = -n; }
    do { digits[k++] = (char)('0' + n % 10); n /= 10; } while (n && k < 11);
    while (k > 0) buf[i++] = digits[--k];
    buf[i] = 0;
    out(buf);
}

static void dump_folder(const Library* L, int folder, int depth) {
    LibItem items[LIB_ITEMS];
    int n = lib_items(L, folder, items, LIB_ITEMS), i, tracks = 0;
    for (i = 0; i < n; ++i) {
        int d;
        if (items[i].kind == LIB_TRACK) { ++tracks; continue; }
        if (items[i].kind != LIB_FOLDER) continue;
        for (d = 0; d < depth; ++d) out("    ");
        out(L->folders[items[i].index].name);
        out(" (");
        out_num(L->folders[items[i].index].total);
        out(")\r\n");
        dump_folder(L, items[i].index, depth + 1);
    }
}

/* KOFBGM_DUMP=<bgm dir>|<Music dir>|<rate> prints the albums the mod would show. */
static int dump_real(void) {
    wchar_t arg[MAX_PATH * 2 + 16];
    wchar_t* music;
    wchar_t* rate;
    LibSpec spec;
    if (!GetEnvironmentVariableW(L"KOFBGM_DUMP", arg, MAX_PATH * 2 + 16)) return 0;
    music = arg;
    while (*music && *music != '|') ++music;
    if (*music) *music++ = 0;
    rate = music;
    while (*rate && *rate != '|') ++rate;
    if (*rate) *rate++ = 0;
    memset(&spec, 0, sizeof(spec));
    spec.bgm_dir = arg;
    spec.game_album = "Soundtrack del juego";
    spec.sample_rate = (rate[0] == '4' && rate[1] == '8') ? 48000 : 44100;
    spec.roots[0] = music;
    lib_build(&g_test_lib, &spec);
    out("pistas: ");
    out_num(g_test_lib.cat.count);
    out("  de Music: ");
    out_num(g_test_lib.scanned);
    out("  ignoradas: ");
    out_num(g_test_lib.ignored);
    out("\r\n");
    dump_folder(&g_test_lib, 0, 0);
    return 1;
}

void main(void) {
    if (dump_real()) ExitProcess(0);
    test_catalog();
    test_helpers();
    test_library();
    test_menu();
    if (g_fail) ExitProcess(1);
    {
        char buf[48];
        int n = g_checks, k = 0;
        char digits[12];
        s_copy(buf, 48, "ok (");
        do { digits[k++] = (char)('0' + n % 10); n /= 10; } while (n && k < 11);
        while (k > 0) { char one[2]; one[0] = digits[--k]; one[1] = 0; s_cat(buf, 48, one); }
        s_cat(buf, 48, " comprobaciones)\r\n");
        out(buf);
    }
    ExitProcess(0);
}
