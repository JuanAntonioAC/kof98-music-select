#pragma once

#ifndef KOF98_WCHAR
#define KOF98_WCHAR
typedef unsigned short wchar_t;
#endif

int s_len(const char* s);
int s_ieq(const char* a, const char* b);
void s_copy(char* dst, int cap, const char* src);
void s_cat(char* dst, int cap, const char* src);
const char* s_chr(const char* s, char c);
int s_atoi(const char* s);
void s_upper(char* s);

int w_len(const wchar_t* s);
void w_copy(wchar_t* dst, int cap, const wchar_t* src);
void w_cat(wchar_t* dst, int cap, const wchar_t* src);
const wchar_t* w_rchr(const wchar_t* s, wchar_t c);
int w_ieq(const wchar_t* a, const wchar_t* b);
int w_has(const wchar_t* s, const wchar_t* part);
