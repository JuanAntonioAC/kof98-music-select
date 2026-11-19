#pragma once

#ifndef KOF98_WCHAR
#define KOF98_WCHAR
typedef unsigned short wchar_t;
#endif

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef int i32;
typedef long long i64;
typedef unsigned short wch;

typedef unsigned long DWORD;
typedef long LONG;
typedef long long LONG_PTR;
typedef void* HANDLE;
typedef void* HWND;
typedef void* HMODULE;
typedef void* HDC;
typedef void* HGDIOBJ;
typedef void* HBRUSH;
typedef void* HFONT;
typedef void* HBITMAP;
typedef void* HCURSOR;
typedef LONG_PTR LRESULT;
typedef u64 WPARAM;
typedef i64 LPARAM;

#define WINAPI __stdcall
#define CALLBACK __stdcall
#define INVALID_HANDLE ((HANDLE)(long long)-1)
#define INVALID_FILE_ATTRIBUTES 0xFFFFFFFFu
#define GENERIC_READ 0x80000000u
#define FILE_SHARE_READ 1u
#define FILE_SHARE_WRITE 2u
#define OPEN_EXISTING 3u
#define OPEN_ALWAYS 4u
#define FILE_ATTRIBUTE_NORMAL 0x80u
#define FILE_ATTRIBUTE_DIRECTORY 0x10u
#define FILE_APPEND_DATA 4u
#define PAGE_READWRITE 4u
#define PAGE_EXECUTE_READWRITE 0x40u
#define MEM_COMMIT 0x1000u
#define MEM_RESERVE 0x2000u
#define CP_UTF8 65001u
#define MAX_PATH 260

#define WS_BORDER 0x00800000u
#define WS_POPUP 0x80000000u
#define WS_EX_TOPMOST 0x00000008u
#define WS_EX_TOOLWINDOW 0x00000080u
#define WS_EX_NOACTIVATE 0x08000000u
#define SW_HIDE 0
#define SW_SHOWNOACTIVATE 4
#define SW_SHOW 5
#define HWND_TOPMOST ((HWND)(long long)-1)
#define SWP_NOACTIVATE 0x0010u
#define SWP_SHOWWINDOW 0x0040u
#define STARTF_USESHOWWINDOW 0x00000001u
#define GW_OWNER 4u
#define TRANSPARENT 1
#define FW_NORMAL 400
#define FW_BOLD 700
#define DEFAULT_CHARSET 1
#define CLEARTYPE_QUALITY 5
#define SRCCOPY 0x00CC0020u
#define DT_CENTER 0x0001u
#define DT_VCENTER 0x0004u
#define DT_SINGLELINE 0x0020u
#define DT_NOPREFIX 0x0800u
#define DT_END_ELLIPSIS 0x8000u
#define WM_CLOSE 0x0010u
#define WM_KEYDOWN 0x0100u
#define WM_PAINT 0x000Fu
#define WM_TIMER 0x0113u
#define WM_LBUTTONDOWN 0x0201u
#define WM_LBUTTONDBLCLK 0x0203u
#define WM_HOTKEY 0x0312u
#define WM_APP 0x8000u
#define GWLP_WNDPROC (-4)
#define GWLP_HWNDPARENT (-8)
#define VK_PRIOR 0x21
#define VK_NEXT 0x22
#define VK_RETURN 0x0D
#define VK_ESCAPE 0x1B
#define VK_UP 0x26
#define VK_DOWN 0x28
#define VK_F8 0x77
#define MOD_SHIFT 0x0004u
#define MOD_CONTROL 0x0002u
#define STD_OUTPUT_HANDLE ((DWORD)-11)

typedef struct RECT_ {
    LONG left, top, right, bottom;
} RECT_;

typedef struct POINT_ {
    LONG x, y;
} POINT_;

typedef struct MSG_ {
    HWND hwnd;
    u32 message;
    u64 wParam;
    i64 lParam;
    DWORD time;
    POINT_ pt;
    DWORD priv;
} MSG_;

typedef LRESULT (CALLBACK *WNDPROC_)(HWND, u32, WPARAM, LPARAM);

typedef struct WNDCLASSEXW_ {
    u32 cbSize;
    u32 style;
    WNDPROC_ lpfnWndProc;
    int cbClsExtra;
    int cbWndExtra;
    HMODULE hInstance;
    void* hIcon;
    void* hCursor;
    void* hbrBackground;
    const wch* lpszMenuName;
    const wch* lpszClassName;
    void* hIconSm;
} WNDCLASSEXW_;

typedef struct PAINT_ {
    HDC hdc;
    int erase;
    RECT_ rc;
    int restore;
    int inc;
    u8 reserved[32];
} PAINT_;

typedef struct FIND_DATA_ {
    DWORD attributes;
    u32 times[6];
    DWORD size_high;
    DWORD size_low;
    DWORD reserved0;
    DWORD reserved1;
    wch name[260];
    wch alt[14];
} FIND_DATA_;

typedef struct STARTUPINFOW_ {
    DWORD cb;
    wch* reserved;
    wch* desktop;
    wch* title;
    DWORD x, y, xsize, ysize, xchars, ychars, fill, flags;
    u16 show;
    u16 reserved2;
    void* reserved2p;
    HANDLE stdin_h, stdout_h, stderr_h;
} STARTUPINFOW_;

typedef struct PROCESS_INFORMATION_ {
    HANDLE process;
    HANDLE thread;
    DWORD pid;
    DWORD tid;
} PROCESS_INFORMATION_;

typedef struct CRIT_ {
    void* debug;
    LONG lock;
    LONG recursion;
    HANDLE owner;
    HANDLE semaphore;
    u64 spin;
} CRIT_;

__declspec(dllimport) HANDLE WINAPI CreateFileW(const wch*, DWORD, DWORD, void*, DWORD, DWORD, HANDLE);
__declspec(dllimport) int WINAPI ReadFile(HANDLE, void*, DWORD, DWORD*, void*);
__declspec(dllimport) int WINAPI WriteFile(HANDLE, const void*, DWORD, DWORD*, void*);
__declspec(dllimport) int WINAPI CloseHandle(HANDLE);
__declspec(dllimport) DWORD WINAPI GetFileAttributesW(const wch*);
__declspec(dllimport) HANDLE WINAPI FindFirstFileW(const wch*, FIND_DATA_*);
__declspec(dllimport) int WINAPI FindNextFileW(HANDLE, FIND_DATA_*);
__declspec(dllimport) int WINAPI FindClose(HANDLE);
__declspec(dllimport) HMODULE WINAPI GetModuleHandleW(const wch*);
__declspec(dllimport) DWORD WINAPI GetModuleFileNameW(HMODULE, wch*, DWORD);
__declspec(dllimport) void* WINAPI GetProcAddress(HMODULE, const char*);
__declspec(dllimport) HMODULE WINAPI LoadLibraryW(const wch*);
__declspec(dllimport) int WINAPI VirtualProtect(void*, u64, DWORD, DWORD*);
__declspec(dllimport) void* WINAPI VirtualAlloc(void*, u64, DWORD, DWORD);
__declspec(dllimport) HANDLE WINAPI GetCurrentProcess(void);
__declspec(dllimport) int WINAPI FlushInstructionCache(HANDLE, const void*, u64);
__declspec(dllimport) HANDLE WINAPI CreateThread(void*, u64, DWORD (WINAPI *)(void*), void*, DWORD, DWORD*);
__declspec(dllimport) int WINAPI CreateProcessW(const wch*, wch*, void*, void*, int, DWORD, void*, const wch*, STARTUPINFOW_*, PROCESS_INFORMATION_*);
__declspec(dllimport) int WINAPI TerminateProcess(HANDLE, u32);
__declspec(dllimport) int WINAPI GetExitCodeProcess(HANDLE, DWORD*);
__declspec(dllimport) DWORD WINAPI WaitForSingleObject(HANDLE, DWORD);
__declspec(dllimport) int WINAPI DisableThreadLibraryCalls(HMODULE);
__declspec(dllimport) DWORD WINAPI GetCurrentProcessId(void);
__declspec(dllimport) DWORD WINAPI GetLastError(void);
__declspec(dllimport) u64 WINAPI GetTickCount64(void);
__declspec(dllimport) HANDLE WINAPI GetStdHandle(DWORD);
__declspec(dllimport) void WINAPI ExitProcess(u32);
__declspec(dllimport) u32 WINAPI GetSystemDirectoryW(wch*, u32);
__declspec(dllimport) int WINAPI MultiByteToWideChar(u32, DWORD, const char*, int, wch*, int);
__declspec(dllimport) int WINAPI WideCharToMultiByte(u32, DWORD, const wch*, int, char*, int, const char*, int*);
__declspec(dllimport) int WINAPI lstrcmpA(const char*, const char*);
__declspec(dllimport) void WINAPI Sleep(DWORD);
__declspec(dllimport) int WINAPI CreateDirectoryW(const wch*, void*);
__declspec(dllimport) DWORD WINAPI GetTempPathW(DWORD, wch*);
__declspec(dllimport) void WINAPI InitializeCriticalSection(CRIT_*);
__declspec(dllimport) void WINAPI EnterCriticalSection(CRIT_*);
__declspec(dllimport) void WINAPI LeaveCriticalSection(CRIT_*);

__declspec(dllimport) u16 WINAPI RegisterClassExW(const WNDCLASSEXW_*);
__declspec(dllimport) HWND WINAPI CreateWindowExW(DWORD, const wch*, const wch*, DWORD, int, int, int, int, HWND, void*, HMODULE, void*);
typedef LRESULT (CALLBACK *WndProcFn)(HWND, u32, WPARAM, LPARAM);
__declspec(dllimport) LRESULT WINAPI DefWindowProcW(HWND, u32, WPARAM, LPARAM);
__declspec(dllimport) LRESULT WINAPI CallWindowProcW(WndProcFn, HWND, u32, WPARAM, LPARAM);
__declspec(dllimport) LONG_PTR WINAPI SetWindowLongPtrW(HWND, int, LONG_PTR);
__declspec(dllimport) int WINAPI PostMessageW(HWND, u32, WPARAM, LPARAM);
__declspec(dllimport) int WINAPI ShowWindow(HWND, int);
__declspec(dllimport) int WINAPI SetWindowPos(HWND, HWND, int, int, int, int, u32);
__declspec(dllimport) int WINAPI SetForegroundWindow(HWND);
__declspec(dllimport) HWND WINAPI GetForegroundWindow(void);
__declspec(dllimport) int WINAPI AttachThreadInput(DWORD, DWORD, int);
__declspec(dllimport) DWORD WINAPI GetCurrentThreadId(void);
__declspec(dllimport) HWND WINAPI FindWindowExW(HWND, HWND, const wch*, const wch*);
__declspec(dllimport) DWORD WINAPI GetWindowThreadProcessId(HWND, DWORD*);
__declspec(dllimport) HWND WINAPI GetWindow(HWND, u32);
__declspec(dllimport) int WINAPI GetWindowRect(HWND, RECT_*);
__declspec(dllimport) int WINAPI IsWindowVisible(HWND);
__declspec(dllimport) int WINAPI InvalidateRect(HWND, const RECT_*, int);
__declspec(dllimport) HDC WINAPI BeginPaint(HWND, PAINT_*);
__declspec(dllimport) int WINAPI EndPaint(HWND, const PAINT_*);
__declspec(dllimport) int WINAPI GetClientRect(HWND, RECT_*);
__declspec(dllimport) int WINAPI FillRect(HDC, const RECT_*, HBRUSH);
__declspec(dllimport) int WINAPI DrawTextA(HDC, const char*, int, RECT_*, u32);
__declspec(dllimport) int WINAPI DrawTextW(HDC, const wch*, int, RECT_*, u32);
__declspec(dllimport) DWORD WINAPI SetTextColor(HDC, DWORD);
__declspec(dllimport) int WINAPI SetBkMode(HDC, int);
__declspec(dllimport) int WINAPI GetMessageW(MSG_*, HWND, u32, u32);
__declspec(dllimport) int WINAPI TranslateMessage(const MSG_*);
__declspec(dllimport) LRESULT WINAPI DispatchMessageW(const MSG_*);
__declspec(dllimport) u64 WINAPI SetTimer(HWND, u64, u32, void*);
__declspec(dllimport) int WINAPI RegisterHotKey(HWND, int, u32, u32);
__declspec(dllimport) HCURSOR WINAPI LoadCursorW(HMODULE, const wch*);
__declspec(dllimport) short WINAPI GetAsyncKeyState(int);

__declspec(dllimport) HDC WINAPI CreateCompatibleDC(HDC);
__declspec(dllimport) HBITMAP WINAPI CreateCompatibleBitmap(HDC, int, int);
__declspec(dllimport) HGDIOBJ WINAPI SelectObject(HDC, HGDIOBJ);
__declspec(dllimport) HFONT WINAPI CreateFontW(int, int, int, int, int, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, const wch*);
__declspec(dllimport) int WINAPI BitBlt(HDC, int, int, int, int, HDC, int, int, DWORD);
__declspec(dllimport) int WINAPI DeleteObject(HGDIOBJ);
__declspec(dllimport) int WINAPI DeleteDC(HDC);
__declspec(dllimport) HBRUSH WINAPI CreateSolidBrush(DWORD);
