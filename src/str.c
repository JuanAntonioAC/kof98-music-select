#include "str.h"

int s_len(const char* s) {
    int n = 0;
    if (!s) return 0;
    while (s[n]) ++n;
    return n;
}

static char lower(char c) {
    if (c >= 'A' && c <= 'Z') return (char)(c + 32);
    return c;
}

int s_ieq(const char* a, const char* b) {
    int i;
    if (!a || !b) return 0;
    for (i = 0; lower(a[i]) == lower(b[i]); ++i)
        if (!a[i]) return 1;
    return 0;
}

void s_copy(char* dst, int cap, const char* src) {
    int i = 0;
    if (!dst || cap <= 0) return;
    if (!src) src = "";
    while (src[i] && i + 1 < cap) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = 0;
}

void s_cat(char* dst, int cap, const char* src) {
    int n = s_len(dst);
    int i = 0;
    if (!dst || !src || cap <= 0) return;
    while (src[i] && n + 1 < cap) dst[n++] = src[i++];
    dst[n] = 0;
}

const char* s_chr(const char* s, char c) {
    if (!s) return 0;
    while (*s) {
        if (*s == c) return s;
        ++s;
    }
    return 0;
}

int s_atoi(const char* s) {
    int n = 0;
    int sign = 1;
    if (!s) return 0;
    if (*s == '-') { sign = -1; ++s; }
    while (*s >= '0' && *s <= '9') {
        n = n * 10 + (*s - '0');
        ++s;
    }
    return n * sign;
}

void s_upper(char* s) {
    if (!s) return;
    while (*s) {
        if (*s >= 'a' && *s <= 'z') *s = (char)(*s - 32);
        ++s;
    }
}

int w_len(const wchar_t* s) {
    int n = 0;
    if (!s) return 0;
    while (s[n]) ++n;
    return n;
}

void w_copy(wchar_t* dst, int cap, const wchar_t* src) {
    int i = 0;
    if (!dst || cap <= 0) return;
    if (!src) src = L"";
    while (src[i] && i + 1 < cap) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = 0;
}

void w_cat(wchar_t* dst, int cap, const wchar_t* src) {
    int n = w_len(dst);
    int i = 0;
    if (!dst || !src) return;
    while (src[i] && n + 1 < cap) dst[n++] = src[i++];
    dst[n] = 0;
}

const wchar_t* w_rchr(const wchar_t* s, wchar_t c) {
    const wchar_t* hit = 0;
    if (!s) return 0;
    while (*s) {
        if (*s == c) hit = s;
        ++s;
    }
    return hit;
}

int w_ieq(const wchar_t* a, const wchar_t* b) {
    if (!a || !b) return 0;
    while (*a && *b) {
        wchar_t ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca = (wchar_t)(ca + 32);
        if (cb >= 'A' && cb <= 'Z') cb = (wchar_t)(cb + 32);
        if (ca != cb) return 0;
        ++a;
        ++b;
    }
    return *a == *b;
}

int w_has(const wchar_t* s, const wchar_t* part) {
    int n, m, i, j;
    if (!s || !part) return 0;
    n = w_len(s);
    m = w_len(part);
    for (i = 0; i + m <= n; ++i) {
        for (j = 0; j < m; ++j) {
            wchar_t a = s[i + j], b = part[j];
            if (a >= 'A' && a <= 'Z') a = (wchar_t)(a + 32);
            if (b >= 'A' && b <= 'Z') b = (wchar_t)(b + 32);
            if (a != b) break;
        }
        if (j == m) return 1;
    }
    return 0;
}
