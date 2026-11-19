#include "winapi.h"
#include "str.h"

static void out(const char* text) {
    DWORD w = 0;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), text, (DWORD)s_len(text), &w, 0);
}

static void out_num(DWORD n) {
    char d[16];
    char buf[16];
    int i = 0;
    int j = 0;
    if (n == 0) { out("0"); return; }
    while (n && i < 15) {
        d[i++] = (char)('0' + (n % 10));
        n /= 10;
    }
    while (i > 0) buf[j++] = d[--i];
    buf[j] = 0;
    out(buf);
}

static void check(const wchar_t* dll, const char* name) {
    HMODULE m = LoadLibraryW(dll);
    void* p = m ? GetProcAddress(m, name) : 0;
    if (p) return;
    out("MISSING ");
    out(name);
    out(" err=");
    out_num(GetLastError());
    out("\r\n");
}

void main(void) {
    const char* k[] = {
        "VirtualProtect","CreateThread","DisableThreadLibraryCalls","GetCurrentProcessId","GetTickCount64",
        "GetSystemDirectoryW","lstrcmpA","Sleep","LoadLibraryW","EnterCriticalSection","LeaveCriticalSection",
        "FindFirstFileW","FindNextFileW","FindClose","MultiByteToWideChar","WideCharToMultiByte","GetProcAddress",
        "GetModuleFileNameW","GetModuleHandleW","GetFileAttributesW","CloseHandle","CreateFileW","WriteFile",
        "ReadFile","InitializeCriticalSection", 0
    };
    const char* u[] = {
        "GetWindowRect","IsWindowVisible","InvalidateRect","BeginPaint","EndPaint","GetClientRect","FillRect",
        "DrawTextA","SetTextColor","SetBkMode","GetMessageW","TranslateMessage","DispatchMessageW","SetTimer",
        "RegisterHotKey","LoadCursorW","GetWindow","GetWindowThreadProcessId","FindWindowExW","SetForegroundWindow",
        "ShowWindow","DefWindowProcW","CreateWindowExW","RegisterClassExW","SetWindowPos", 0
    };
    const char* g[] = {
        "CreateCompatibleBitmap","SelectObject","CreateFontW","BitBlt","DeleteObject","DeleteDC","CreateSolidBrush",
        "CreateCompatibleDC", 0
    };
    int i;
    HMODULE h;
    for (i = 0; k[i]; ++i) check(L"kernel32.dll", k[i]);
    for (i = 0; u[i]; ++i) check(L"user32.dll", u[i]);
    for (i = 0; g[i]; ++i) check(L"gdi32.dll", g[i]);
    out("checks done\r\n");
    h = LoadLibraryW(L"D:\\KOF98-Music-Select\\dist\\Kof98Bgm.dll");
    out("handle=");
    out_num((DWORD)(u64)h);
    out(" err=");
    out_num(GetLastError());
    out("\r\n");
    ExitProcess(h ? 0 : 1);
}
