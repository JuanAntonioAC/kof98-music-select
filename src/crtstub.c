int _fltused = 0x9875;

#pragma optimize("", off)
#pragma function(memcpy, memset)

void* memcpy(void* dst, const void* src, unsigned long long n) {
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;
    unsigned long long i;
    for (i = 0; i < n; ++i) d[i] = s[i];
    return dst;
}

void* memset(void* dst, int value, unsigned long long n) {
    unsigned char* d = (unsigned char*)dst;
    unsigned long long i;
    for (i = 0; i < n; ++i) d[i] = (unsigned char)value;
    return dst;
}
