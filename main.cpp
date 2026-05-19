#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define PSAPI_VERSION 1

#include <windows.h>
#include <objidl.h>
#include <ole2.h>
#include <gdiplus.h>
#include <commctrl.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <shellapi.h>
#include <uxtheme.h>
#include <dwmapi.h>
#include <mmsystem.h>
#include <stdio.h>
#include <wchar.h>
#include <math.h>

#include "Resource.h"

#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "UxTheme.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "Winmm.lib")

using namespace Gdiplus;

void paintParentBackground(HWND hChild, HDC dst);

// --- Control IDs ---
#define ID_CLEAN_BUTTON  1001
#define ID_LOG_BOX     1002
#define ID_PROGRESS_BAR          1003
#define ID_THRESHOLD_EDIT    1004
#define ID_THRESHOLD_SPIN    1005
#define ID_AUTO_TOGGLE     1006
#define ID_LABEL_RAM      1007
#define ID_BTN_DEC        1008
#define ID_BTN_INC        1009
#define ID_STARTUP_TOGGLE 1014
#define ID_WIN_MIN        1015
#define ID_WIN_CLOSE      1016

// --- Tray ---
#define WM_TRAYICON       (WM_USER + 1)
#define WM_LIMPIEZA_FIN   (WM_USER + 2)
#define ID_TRAY_OPEN     2001
#define ID_TRAY_CLEAN   2002
#define ID_TRAY_EXIT     2003
#define ID_TIMER_MONITOR  2004
#define ID_TIMER_ANIM     2005
#define ID_TIMER_TOGGLE   1010
#define ID_TIMER_DEC      1011
#define ID_TIMER_INC      1012
#define ID_TIMER_EDIT     1013

// --- Palette ---
#define COL_BG_TOP        Color(255, 3, 5, 12)
#define COL_BG_BOT        Color(255, 1, 2, 6)
#define COL_CARD          Color(214, 7, 12, 20)
#define COL_CARD_BORDER   Color(255, 42, 78, 108)
#define COL_TEXT          Color(255, 224, 240, 248)
#define COL_TEXT_DIM      Color(255, 132, 162, 182)
#define COL_TEXT_FAINT    Color(255, 82, 102, 118)
#define COL_CYAN          Color(255, 0, 236, 255)
#define COL_VIOLET        Color(255, 164, 86, 255)
#define COL_MINT          Color(255, 44, 255, 176)
#define COL_PINK          Color(255, 255, 72, 182)
#define COL_AMBER         Color(255, 255, 184, 76)
#define COL_LIME          Color(255, 132, 255, 96)

#define RGB_CARD          RGB(7, 12, 20)
#define RGB_BG            RGB(0, 0, 0)
#define RGB_CONSOLE_BG    RGB(0, 0, 0)
#define RGB_TEXT          RGB(224, 240, 248)
#define RGB_TEXT_DIM      RGB(132, 162, 182)
#define RGB_CYAN          RGB(0, 236, 255)
#define RGB_MINT          RGB(44, 255, 176)

static const wchar_t* kAppName = L"RAMMY";
static const wchar_t* kTrayStartupValue = L"RAMMY";
static const int kTitlebarH = 42;

// --- Globals ---
HWND hLogBox      = NULL;
HWND hCleanButton          = NULL;
HWND hProgressBar          = NULL;
HWND hAutoToggle      = NULL;
HWND hThresholdEdit     = NULL;
HWND hDecButton         = NULL;
HWND hIncButton         = NULL;
HWND hTimerToggle    = NULL;
HWND hTimerDec       = NULL;
HWND hTimerEdit      = NULL;
HWND hTimerInc       = NULL;
HWND hStartupToggle  = NULL;
HWND hMinWindowButton      = NULL;
HWND hCloseWindowButton    = NULL;
HWND hTooltip        = NULL;
HWND hMainWindow        = NULL;

ULONG_PTR  gGdiToken        = 0;
Image*     gBgHero          = NULL;
// Static background cache - rerendered only on size change
HBITMAP    gBackgroundCache      = NULL;
int        gBackgroundCacheW     = 0;
int        gBackgroundCacheH     = 0;
bool       gMovingWindow = false;
HBRUSH     gBrushCard       = NULL;
HBRUSH     gBrushBg         = NULL;
HBRUSH     gBrushConsole    = NULL;
HICON      gAppIcon        = NULL;
HINSTANCE  gHInst           = NULL;
HFONT      gFontTitle       = NULL;
HFONT      gFontBig         = NULL;
HFONT      gFontMed         = NULL;
HFONT      gFontSmall       = NULL;
HFONT      gFontMono        = NULL;

NOTIFYICONDATA gNID         = {};
bool       gTrayIconAdded  = false;
bool       gCleaningNow  = false;
int        gRamThreshold       = 80;
bool       gAutoCleanEnabled      = true;
bool       gStartWithWindows   = false;

int        gTimerMins       = 30;
bool       gTimerEnabled     = false;
int        gTimerSegundos   = 0;
float      gTimerAnim       = 0.0f;
float      gStartupAnim     = 0.0f;

// Animation state
float      gRamCurrent       = 0.0f;   // current % shown (animated)
float      gRamTarget     = 0.0f;   // target %
DWORDLONG  gRamTotal        = 0;
DWORDLONG  gRamUsed        = 0;
DWORDLONG  gRamAvailable        = 0;
bool       gCleanButtonHover      = false;
bool       gCleanButtonPress      = false;
float      gCleanButtonGlow       = 0.0f;
float      gPulse           = 0.0f;
float      gToggleAnim      = 1.0f;   // 0 = off, 1 = on (animated)
bool       gDecButtonHover        = false;
bool       gIncButtonHover        = false;
bool       gMinButtonHover        = false;
bool       gCloseButtonHover      = false;
bool       gHighResTimer    = false;

bool       gTimerSliderHit  = false;
bool       gThreshSliderHit = false;

// Cache memory
DWORDLONG  gRamCache        = 0;

// --- NT types ---
typedef LONG NTSTATUS;
typedef NTSTATUS(NTAPI* PFN_NtSetSystemInformation)(ULONG, PVOID, ULONG);

#ifndef NT_SUCCESS
#define NT_SUCCESS(s) (((NTSTATUS)(s)) >= 0)
#endif

#ifndef QUOTA_LIMITS_HARDWS_MIN_DISABLE
#define QUOTA_LIMITS_HARDWS_MIN_DISABLE 0x00000002
#endif
#ifndef QUOTA_LIMITS_HARDWS_MAX_DISABLE
#define QUOTA_LIMITS_HARDWS_MAX_DISABLE 0x00000008
#endif

#define SystemMemoryListInformation             80
#define SystemFileCacheInformationEx            81
#define SystemCombinePhysicalMemoryInformation  130
#define SystemRegistryReconciliationInformation 155
#define MemoryEmptyWorkingSets                  2
#define MemoryFlushModifiedList                 3
#define MemoryPurgeStandbyList                  4
#define MemoryPurgeLowPriorityStandbyList       5

typedef struct _SYSTEM_FILECACHE_INFORMATION {
    ULONG_PTR CurrentSize;
    ULONG_PTR PeakSize;
    ULONG     PageFaultCount;
    ULONG_PTR MinimumWorkingSet;
    ULONG_PTR MaximumWorkingSet;
    ULONG_PTR CurrentSizeIncludingTransitionInPages;
    ULONG_PTR PeakSizeIncludingTransitionInPages;
    ULONG     TransitionRePurposeCount;
    ULONG     Flags;
} SYSTEM_FILECACHE_INFORMATION;

typedef struct _MEMORY_COMBINE_INFORMATION_EX {
    HANDLE    Handle;
    ULONG_PTR PagesCombined;
    ULONG     Flags;
} MEMORY_COMBINE_INFORMATION_EX;

// =========================================================
// Utilities
// =========================================================

double bytesAGB(DWORDLONG b) { return b / 1024.0 / 1024.0 / 1024.0; }

HICON loadAppIcon()
{
    return (HICON)LoadImageW(gHInst, MAKEINTRESOURCEW(IDI_APP_ICON),
        IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
}

Image* loadImageResource(WORD resourceId)
{
    HRSRC hRes = FindResourceW(gHInst, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
    if (!hRes) return NULL;

    DWORD size = SizeofResource(gHInst, hRes);
    if (!size) return NULL;

    HGLOBAL hLoaded = LoadResource(gHInst, hRes);
    if (!hLoaded) return NULL;

    const void* data = LockResource(hLoaded);
    if (!data) return NULL;

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!hMem) return NULL;

    void* dst = GlobalLock(hMem);
    if (!dst) {
        GlobalFree(hMem);
        return NULL;
    }

    memcpy(dst, data, size);
    GlobalUnlock(hMem);

    IStream* stream = NULL;
    if (FAILED(CreateStreamOnHGlobal(hMem, TRUE, &stream))) {
        GlobalFree(hMem);
        return NULL;
    }

    Image* img = Image::FromStream(stream);
    stream->Release();

    if (!img || img->GetLastStatus() != Ok) {
        if (img) delete img;
        return NULL;
    }

    return img;
}

void loadHeroImage()
{
    if (gBgHero) { delete gBgHero; gBgHero = NULL; }
    gBgHero = loadImageResource(IDR_BG_PNG);
}

void appendText(const wchar_t* text)
{
    int n = GetWindowTextLengthW(hLogBox);
    SendMessageW(hLogBox, EM_SETSEL, n, n);
    SendMessageW(hLogBox, EM_REPLACESEL, FALSE, (LPARAM)text);
    InvalidateRect(hLogBox, NULL, TRUE);
}

void appendLine(const wchar_t* text)
{
    appendText(text);
    appendText(L"\r\n");
}

DWORDLONG getAvailableRam()
{
    MEMORYSTATUSEX m; m.dwLength = sizeof(m);
    return GlobalMemoryStatusEx(&m) ? m.ullAvailPhys : 0;
}

DWORD getRamUsage()
{
    MEMORYSTATUSEX m; m.dwLength = sizeof(m);
    return GlobalMemoryStatusEx(&m) ? m.dwMemoryLoad : 0;
}

void updateStats()
{
    MEMORYSTATUSEX m; m.dwLength = sizeof(m);
    if (GlobalMemoryStatusEx(&m)) {
        gRamTotal    = m.ullTotalPhys;
        gRamAvailable    = m.ullAvailPhys;
        gRamUsed    = m.ullTotalPhys - m.ullAvailPhys;
        gRamTarget = (float)m.dwMemoryLoad;
    }
    PERFORMANCE_INFORMATION pi; pi.cb = sizeof(pi);
    if (GetPerformanceInfo(&pi, sizeof(pi)))
        gRamCache = (DWORDLONG)pi.SystemCache * pi.PageSize;
    else
        gRamCache = 0;
}

bool readStartWithWindows()
{
    HKEY key = NULL;
    LONG r = RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_QUERY_VALUE, &key);
    if (r != ERROR_SUCCESS) return false;

    wchar_t value[1024] = {};
    DWORD type = 0;
    DWORD cb = sizeof(value);
    r = RegQueryValueExW(key, kTrayStartupValue, NULL, &type, (LPBYTE)value, &cb);
    RegCloseKey(key);
    return r == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ) && value[0] != L'\0';
}

bool writeStartWithWindows(bool enable)
{
    HKEY key = NULL;
    LONG r = RegCreateKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, NULL, 0, KEY_SET_VALUE, NULL, &key, NULL);
    if (r != ERROR_SUCCESS) return false;

    if (!enable) {
        r = RegDeleteValueW(key, kTrayStartupValue);
        RegCloseKey(key);
        return r == ERROR_SUCCESS || r == ERROR_FILE_NOT_FOUND;
    }

    wchar_t exe[MAX_PATH] = {};
    if (!GetModuleFileNameW(NULL, exe, MAX_PATH)) {
        RegCloseKey(key);
        return false;
    }

    wchar_t cmd[MAX_PATH + 4] = {};
    swprintf_s(cmd, L"\"%s\"", exe);
    r = RegSetValueExW(key, kTrayStartupValue, 0, REG_SZ,
        (const BYTE*)cmd, (DWORD)((wcslen(cmd) + 1) * sizeof(wchar_t)));
    RegCloseKey(key);
    return r == ERROR_SUCCESS;
}

void showMemoryStats()
{
    MEMORYSTATUSEX m; m.dwLength = sizeof(m);
    if (!GlobalMemoryStatusEx(&m)) { appendLine(L"[ERR] Could not read memory."); return; }
    wchar_t buf[256];
    swprintf_s(buf, L"  RAM used        %lu%%",            m.dwMemoryLoad);            appendLine(buf);
    swprintf_s(buf, L"  RAM total       %.2f GB",          bytesAGB(m.ullTotalPhys));  appendLine(buf);
    swprintf_s(buf, L"  RAM available   %.2f GB",          bytesAGB(m.ullAvailPhys));  appendLine(buf);
    swprintf_s(buf, L"  RAM in use      %.2f GB",
        bytesAGB(m.ullTotalPhys - m.ullAvailPhys));                                    appendLine(buf);
}

// =========================================================
// Admin / privileges
// =========================================================

bool isAdministrator()
{
    BOOL es = FALSE; PSID sid = NULL;
    SID_IDENTIFIER_AUTHORITY auth = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&auth, 2,
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0, &sid))
    {
        CheckTokenMembership(NULL, sid, &es);
        FreeSid(sid);
    }
    return es == TRUE;
}

void relaunchAsAdministrator()
{
    wchar_t ruta[MAX_PATH];
    GetModuleFileNameW(NULL, ruta, MAX_PATH);
    ShellExecuteW(NULL, L"runas", ruta, NULL, NULL, SW_SHOWNORMAL);
}

bool enablePrivilege(LPCWSTR nombre)
{
    HANDLE tok = NULL; TOKEN_PRIVILEGES priv; LUID luid;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tok)) return false;
    if (!LookupPrivilegeValueW(NULL, nombre, &luid)) { CloseHandle(tok); return false; }
    priv.PrivilegeCount = 1;
    priv.Privileges[0].Luid       = luid;
    priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges(tok, FALSE, &priv, sizeof(priv), NULL, NULL);
    DWORD err = GetLastError();
    CloseHandle(tok);
    return err == ERROR_SUCCESS;
}

void enablePrivileges()
{
    appendLine(L"  Enabling privileges...");
    appendLine(enablePrivilege(SE_DEBUG_NAME)
        ? L"  [OK]  SeDebugPrivilege" : L"  [--]  SeDebugPrivilege");
    appendLine(enablePrivilege(SE_INCREASE_QUOTA_NAME)
        ? L"  [OK]  SeIncreaseQuotaPrivilege" : L"  [--]  SeIncreaseQuotaPrivilege");
    appendLine(enablePrivilege(SE_PROF_SINGLE_PROCESS_NAME)
        ? L"  [OK]  SeProfileSingleProcessPrivilege" : L"  [--]  SeProfileSingleProcessPrivilege");
#ifdef SE_INC_WORKING_SET_NAME
    appendLine(enablePrivilege(SE_INC_WORKING_SET_NAME)
        ? L"  [OK]  SeIncreaseWorkingSetPrivilege" : L"  [--]  SeIncreaseWorkingSetPrivilege");
#endif
    appendLine(L"");
}

// =========================================================
// NT internals
// =========================================================

PFN_NtSetSystemInformation getNtSetSystemInformation()
{
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) ntdll = LoadLibraryW(L"ntdll.dll");
    if (!ntdll) return NULL;
    return (PFN_NtSetSystemInformation)GetProcAddress(ntdll, "NtSetSystemInformation");
}

int cleanProcessesUltraAggressive()
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32 pe; pe.dwSize = sizeof(pe);
    int touched = 0;
    DWORD selfPid = GetCurrentProcessId();
    if (Process32First(snap, &pe)) do {
        // Skip critical / unprunable PIDs
        if (pe.th32ProcessID == 0 || pe.th32ProcessID == 4) continue;
        if (pe.th32ProcessID == selfPid) continue;

        HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_SET_QUOTA,
            FALSE, pe.th32ProcessID);
        if (h) {
            bool a = EmptyWorkingSet(h) != 0;
            bool b = SetProcessWorkingSetSize(h, (SIZE_T)-1, (SIZE_T)-1) != 0;
            bool c = SetProcessWorkingSetSizeEx(h, (SIZE_T)-1, (SIZE_T)-1,
                QUOTA_LIMITS_HARDWS_MIN_DISABLE | QUOTA_LIMITS_HARDWS_MAX_DISABLE) != 0;
            if (a || b || c) touched++;
            CloseHandle(h);
        }
    } while (Process32Next(snap, &pe));
    CloseHandle(snap);
    return touched;
}

// Final aggressive purge cycle, run after all passes
int finalIntensiveCycle()
{
    auto fn = getNtSetSystemInformation();
    if (!fn) return 0;
    int ok = 0;
    // 12-command sequence: interleave flushes + purges so each purge
    // sees freshly-flushed pages instead of pages still pending writeback.
    ULONG seq[] = {
        MemoryFlushModifiedList,
        MemoryPurgeLowPriorityStandbyList,
        MemoryPurgeStandbyList,
        MemoryFlushModifiedList,
        MemoryPurgeStandbyList,
        MemoryEmptyWorkingSets,
        MemoryFlushModifiedList,
        MemoryPurgeLowPriorityStandbyList,
        MemoryPurgeStandbyList,
        MemoryEmptyWorkingSets,
        MemoryFlushModifiedList,
        MemoryPurgeStandbyList,
    };
    for (auto c : seq) {
        if (NT_SUCCESS(fn(SystemMemoryListInformation, &c, sizeof(c)))) ok++;
        Sleep(45);
    }
    return ok;
}

// Stable RAM measurement: sample 2x, take max available (= memory most truly free)
DWORDLONG measureStableRam()
{
    DWORDLONG mx = 0;
    for (int i = 0; i < 2; i++) {
        DWORDLONG s = getAvailableRam();
        if (s > mx) mx = s;
        Sleep(120);
    }
    return mx;
}

int cleanWindowsMemoryLists()
{
    auto fn = getNtSetSystemInformation();
    if (!fn) { appendLine(L"  [ERR] NtSetSystemInformation unavailable."); return 0; }
    ULONG cmds[] = {
        MemoryEmptyWorkingSets, MemoryFlushModifiedList,
        MemoryPurgeStandbyList, MemoryPurgeLowPriorityStandbyList
    };
    const wchar_t* names[] = {
        L"EmptyWorkingSets", L"FlushModifiedList",
        L"PurgeStandbyList", L"PurgeLowPriorityStandbyList"
    };
    int ok = 0;
    for (int i = 0; i < 4; i++) {
        NTSTATUS s = fn(SystemMemoryListInformation, &cmds[i], sizeof(cmds[i]));
        wchar_t buf[256];
        if (NT_SUCCESS(s)) { swprintf_s(buf, L"  [OK]  %s", names[i]); ok++; }
        else swprintf_s(buf, L"  [--]  %s  0x%08lX", names[i], (unsigned long)s);
        appendLine(buf);
    }
    return ok;
}

bool cleanSystemFileCache()
{
    auto fn = getNtSetSystemInformation();
    if (!fn) return false;
    SYSTEM_FILECACHE_INFORMATION ci = {};
    ci.MinimumWorkingSet = (ULONG_PTR)-1;
    ci.MaximumWorkingSet = (ULONG_PTR)-1;
    ci.Flags = QUOTA_LIMITS_HARDWS_MIN_DISABLE | QUOTA_LIMITS_HARDWS_MAX_DISABLE;
    NTSTATUS s = fn(SystemFileCacheInformationEx, &ci, sizeof(ci));
    if (NT_SUCCESS(s)) { appendLine(L"  [OK]  SystemFileCacheInformationEx"); return true; }
    wchar_t buf[256];
    swprintf_s(buf, L"  [--]  SystemFileCacheInformationEx  0x%08lX", (unsigned long)s);
    appendLine(buf);
    return false;
}

// Documented Win32 API path to the file cache â€” complements the NT call.
// memreduct / WMC do not use this; calling both hits the cache through
// two independent kernel paths and tends to free more on busy systems.
bool cleanFileCacheApi()
{
    if (SetSystemFileCacheSize((SIZE_T)-1, (SIZE_T)-1, 0)) {
        appendLine(L"  [OK]  SetSystemFileCacheSize");
        return true;
    }
    wchar_t buf[128];
    swprintf_s(buf, L"  [--]  SetSystemFileCacheSize  err %lu", GetLastError());
    appendLine(buf);
    return false;
}

bool cleanRegistryCache()
{
    auto fn = getNtSetSystemInformation();
    if (!fn) return false;
    NTSTATUS s = fn(SystemRegistryReconciliationInformation, NULL, 0);
    if (NT_SUCCESS(s)) { appendLine(L"  [OK]  SystemRegistryReconciliationInformation"); return true; }
    wchar_t buf[256];
    swprintf_s(buf, L"  [--]  SystemRegistryReconciliationInformation  0x%08lX", (unsigned long)s);
    appendLine(buf);
    return false;
}

bool combinePhysicalMemory()
{
    auto fn = getNtSetSystemInformation();
    if (!fn) return false;
    MEMORY_COMBINE_INFORMATION_EX info = {};
    NTSTATUS s = fn(SystemCombinePhysicalMemoryInformation, &info, sizeof(info));
    wchar_t buf[256];
    if (NT_SUCCESS(s)) {
        swprintf_s(buf, L"  [OK]  CombinePhysicalMemory  pages %llu",
            (unsigned long long)info.PagesCombined);
        appendLine(buf); return true;
    }
    swprintf_s(buf, L"  [--]  CombinePhysicalMemory  0x%08lX", (unsigned long)s);
    appendLine(buf);
    return false;
}

int flushVolumeCaches()
{
    wchar_t drives[512];
    if (!GetLogicalDriveStringsW(511, drives)) return 0;
    int flushed = 0;
    wchar_t* d = drives;
    while (*d) {
        UINT driveType = GetDriveTypeW(d);
        if (driveType == DRIVE_FIXED || driveType == DRIVE_REMOVABLE) {
            wchar_t vol[8] = { L'\\', L'\\', L'.', L'\\', d[0], L':', L'\0' };
            HANDLE h = CreateFileW(vol, GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
            if (h != INVALID_HANDLE_VALUE) {
                if (FlushFileBuffers(h)) {
                    wchar_t buf[64];
                    swprintf_s(buf, L"  [OK]  FlushFileBuffers %s", vol);
                    appendLine(buf); flushed++;
                }
                CloseHandle(h);
            }
        }
        d += wcslen(d) + 1;
    }
    return flushed;
}

int aggressiveFlushCycle(int repetitions)
{
    auto fn = getNtSetSystemInformation();
    if (!fn) return 0;
    int ok = 0;
    for (int i = 0; i < repetitions; i++) {
        ULONG c;
        c = MemoryFlushModifiedList;
        if (NT_SUCCESS(fn(SystemMemoryListInformation, &c, sizeof(c)))) ok++;
        Sleep(30);
        c = MemoryPurgeStandbyList;
        if (NT_SUCCESS(fn(SystemMemoryListInformation, &c, sizeof(c)))) ok++;
        Sleep(30);
        c = MemoryPurgeLowPriorityStandbyList;
        if (NT_SUCCESS(fn(SystemMemoryListInformation, &c, sizeof(c)))) ok++;
        Sleep(30);
        c = MemoryEmptyWorkingSets;
        if (NT_SUCCESS(fn(SystemMemoryListInformation, &c, sizeof(c)))) ok++;
        Sleep(45);
    }
    return ok;
}

// =========================================================
// Cleanup thread
// =========================================================

DWORD WINAPI runCleanup(LPVOID)
{
    // Lower this thread to BELOW_NORMAL so cleanup does not freeze the desktop.
    // Process I/O priority also lowered to avoid hammering disk during flushes.
    HANDLE hThread = GetCurrentThread();
    int prevPrio = GetThreadPriority(hThread);
    SetThreadPriority(hThread, THREAD_PRIORITY_BELOW_NORMAL);

    appendLine(L"  ----------------------------------------------");
    appendLine(L"   RAMMY   //   CLEANUP STARTED");
    appendLine(L"  ----------------------------------------------");
    appendLine(L"");

    enablePrivileges();

    MEMORYSTATUSEX m0; m0.dwLength = sizeof(m0); GlobalMemoryStatusEx(&m0);
    DWORDLONG before    = m0.ullAvailPhys;
    DWORDLONG total    = m0.ullTotalPhys;
    DWORD     usageBefore = m0.dwMemoryLoad;

    appendLine(L"  [ INITIAL STATE ]");
    showMemoryStats();

    int totalProc = 0, totalCmd = 0, totalFC = 0;
    int totalReg  = 0, totalComb = 0, totalVol = 0, totalAgr = 0;

    const int PASSES = 2;
    for (int p = 1; p <= PASSES; p++) {
        wchar_t buf[64];
        appendLine(L"");
        swprintf_s(buf, L"  >> PASS  %d / %d", p, PASSES); appendLine(buf);
        appendLine(L"");

        int progress = (p * 100) / PASSES;

        appendLine(L"  [1] EmptyWorkingSet on all processes");
        int proc = cleanProcessesUltraAggressive(); totalProc += proc;
        swprintf_s(buf, L"      touched  %d", proc); appendLine(buf); appendLine(L"");
        SendMessageW(hProgressBar, PBM_SETPOS, progress - 25, 0);

        appendLine(L"  [2] Internal Windows memory lists");
        totalCmd += cleanWindowsMemoryLists(); appendLine(L"");
        SendMessageW(hProgressBar, PBM_SETPOS, progress - 20, 0);

        appendLine(L"  [3] System file cache");
        if (cleanSystemFileCache()) totalFC++; appendLine(L"");
        SendMessageW(hProgressBar, PBM_SETPOS, progress - 15, 0);

        appendLine(L"  [4] Registry cache");
        if (cleanRegistryCache()) totalReg++; appendLine(L"");

        appendLine(L"  [5] Physical memory combine");
        if (combinePhysicalMemory()) totalComb++; appendLine(L"");
        SendMessageW(hProgressBar, PBM_SETPOS, progress - 10, 0);

        appendLine(L"  [6] Volume flush");
        totalVol += flushVolumeCaches(); appendLine(L"");

        appendLine(L"  [7] Aggressive cycle x4");
        int agr = aggressiveFlushCycle(4); totalAgr += agr;
        swprintf_s(buf, L"      NT calls  %d / 16", agr); appendLine(buf);
        appendLine(L"");
        SendMessageW(hProgressBar, PBM_SETPOS, progress - 5, 0);

        appendLine(L"  [8] File cache (Win32 API)");
        cleanFileCacheApi(); appendLine(L"");
        SendMessageW(hProgressBar, PBM_SETPOS, progress - 3, 0);

        appendLine(L"  [9] Own process cleanup");
        EmptyWorkingSet(GetCurrentProcess());
        SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
        appendLine(L"");
        SendMessageW(hProgressBar, PBM_SETPOS, progress, 0);

        Sleep(120);
    }

    // Final intensive purge - catches cache that re-grew during passes
    appendLine(L"");
    appendLine(L"  >> FINAL INTENSIVE PURGE");
    int finalOk = finalIntensiveCycle();
    wchar_t bf[64];
    swprintf_s(bf, L"     NT calls  %d / 12", finalOk); appendLine(bf);

    SendMessageW(hProgressBar, PBM_SETPOS, 98, 0);

    // Stable measurement: kernel needs a moment to reconcile lists
    appendLine(L"");
    appendLine(L"  Stabilizing measurement...");
    Sleep(160);
    DWORDLONG after = measureStableRam();

    MEMORYSTATUSEX m1; m1.dwLength = sizeof(m1); GlobalMemoryStatusEx(&m1);
    DWORD usageAfter = m1.dwMemoryLoad;

    appendLine(L"");
    appendLine(L"  ----------------------------------------------");
    appendLine(L"   FINAL RESULT");
    appendLine(L"  ----------------------------------------------");
    appendLine(L"");

    wchar_t buf[256];
    swprintf_s(buf, L"  Processes         %d", totalProc); appendLine(buf);
    swprintf_s(buf, L"  NT commands       %d", totalCmd);  appendLine(buf);
    swprintf_s(buf, L"  Aggressive cycle  %d", totalAgr);  appendLine(buf);
    swprintf_s(buf, L"  Final purge       %d", finalOk);   appendLine(buf);
    swprintf_s(buf, L"  File cache        %d", totalFC);   appendLine(buf);
    swprintf_s(buf, L"  Registry cache    %d", totalReg);  appendLine(buf);
    swprintf_s(buf, L"  Page combines     %d", totalComb); appendLine(buf);
    swprintf_s(buf, L"  Volume flushes    %d", totalVol);  appendLine(buf);
    appendLine(L"");

    if (after > before) {
        DWORDLONG delta = after - before;
        double gb = bytesAGB(delta);
        double mb = delta / (1024.0 * 1024.0);
        double pct = (total > 0) ? (100.0 * (double)delta / (double)total) : 0.0;
        int points = (int)usageBefore - (int)usageAfter;
        if (gb >= 0.1)
            swprintf_s(buf, L"  >> FREED   %.2f GB   (%.1f %% RAM)", gb, pct);
        else
            swprintf_s(buf, L"  >> FREED   %.0f MB   (%.1f %% RAM)", mb, pct);
        appendLine(buf);
        swprintf_s(buf, L"     usage  %lu %%  ->  %lu %%   (%+d points)", usageBefore, usageAfter, -points);
        appendLine(buf);
    } else {
        appendLine(L"  >> FREED   0 MB   (RAM was already optimized)");
    }
    appendLine(L"");
    appendLine(L"  [ FINAL STATE ]");
    showMemoryStats();

    SendMessageW(hProgressBar, PBM_SETPOS, 100, 0);

    // Restore thread priority
    SetThreadPriority(hThread, prevPrio);

    updateStats();
    PostMessageW(hMainWindow, WM_LIMPIEZA_FIN, 0, 0);
    return 0;
}

void startCleanup()
{
    if (gCleaningNow) return;
    gCleaningNow = true;
    EnableWindow(hCleanButton, FALSE);
    SendMessageW(hProgressBar, PBM_SETPOS, 0, 0);
    ShowWindow(hProgressBar, SW_SHOW);
    SetWindowTextW(hLogBox, L"");
    RedrawWindow(hMainWindow, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    HANDLE h = CreateThread(NULL, 0, runCleanup, NULL, 0, NULL);
    if (h) {
        CloseHandle(h);
    } else {
        gCleaningNow = false;
        EnableWindow(hCleanButton, TRUE);
    }
}

// =========================================================
// System tray
// =========================================================

void addTrayIcon(HWND hwnd)
{
    gNID.cbSize           = sizeof(NOTIFYICONDATA);
    gNID.hWnd             = hwnd;
    gNID.uID              = 1;
    gNID.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    gNID.uCallbackMessage = WM_TRAYICON;
    gNID.hIcon            = gAppIcon;
    wcscpy_s(gNID.szTip, L"RAMMY - Monitor RAM");
    Shell_NotifyIconW(NIM_ADD, &gNID);
    gTrayIconAdded = true;
}

void removeTrayIcon()
{
    if (gTrayIconAdded) {
        Shell_NotifyIconW(NIM_DELETE, &gNID);
        gTrayIconAdded = false;
    }
}

void showTrayNotification(const wchar_t* titulo, const wchar_t* msg)
{
    if (!gTrayIconAdded) return;
    NOTIFYICONDATA nid  = gNID;
    nid.uFlags         |= NIF_INFO;
    nid.dwInfoFlags     = NIIF_INFO;
    nid.uTimeout        = 5000;
    wcscpy_s(nid.szInfoTitle, titulo);
    wcscpy_s(nid.szInfo, msg);
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

void showMainWindow(HWND hwnd)
{
    ShowWindow(hwnd, SW_RESTORE);
    SetForegroundWindow(hwnd);
    RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

// =========================================================
// Modern drawing helpers
// =========================================================

void roundedRectPath(GraphicsPath& path, float x, float y, float w, float h, float r)
{
    path.Reset();
    if (r > w/2) r = w/2;
    if (r > h/2) r = h/2;
    path.AddArc(x,         y,         r*2, r*2, 180, 90);
    path.AddArc(x + w - r*2, y,       r*2, r*2, 270, 90);
    path.AddArc(x + w - r*2, y + h - r*2, r*2, r*2, 0, 90);
    path.AddArc(x,         y + h - r*2, r*2, r*2, 90, 90);
    path.CloseFigure();
}

void drawCard(Graphics& g, float x, float y, float w, float h, float radius,
                 Color fill, Color border)
{
    GraphicsPath path;
    roundedRectPath(path, x, y, w, h, radius);

    // Soft shadow
    for (int i = 0; i < 5; i++) {
        GraphicsPath sh;
        roundedRectPath(sh, x - i, y + 3 + i, w + i*2, h + i + 1, radius + i);
        SolidBrush shB(Color((BYTE)(24 - i*3), 0, 0, 0));
        g.FillPath(&shB, &sh);
    }

    SolidBrush bF(fill);
    g.FillPath(&bF, &path);

    // Glass wash
    GraphicsPath gloss;
    roundedRectPath(gloss, x + 1.0f, y + 1.0f, w - 2.0f, h * 0.46f, radius - 1.0f);
    LinearGradientBrush glossBr(
        PointF(x, y), PointF(x, y + h * 0.46f),
        Color(34, 255, 255, 255),
        Color(0, 255, 255, 255));
    g.FillPath(&glossBr, &gloss);

    Pen bP(Color(120, border.GetR(), border.GetG(), border.GetB()), 1.0f);
    g.DrawPath(&bP, &path);

    Pen inP(Color(28, 255, 255, 255), 1.0f);
    g.DrawPath(&inP, &gloss);
}

void drawCenteredText(Graphics& g, const wchar_t* txt, Font& f, Color c,
                          RectF rc, StringAlignment ha = StringAlignmentCenter,
                          StringAlignment va = StringAlignmentCenter)
{
    StringFormat sf;
    sf.SetAlignment(ha);
    sf.SetLineAlignment(va);
    SolidBrush br(c);
    g.DrawString(txt, -1, &f, rc, &sf, &br);
}

// Heart visualization
static void drawShard(Graphics& g, float px, float py, float angle, float sw, float sh, Color c)
{
    float rad = angle * 0.01745329f;
    float ca = cosf(rad), sa = sinf(rad);
    PointF corners[4] = {
        { px + (-sw/2)*ca - (-sh/2)*sa, py + (-sw/2)*sa + (-sh/2)*ca },
        { px + ( sw/2)*ca - (-sh/2)*sa, py + ( sw/2)*sa + (-sh/2)*ca },
        { px + ( sw/2)*ca - ( sh/2)*sa, py + ( sw/2)*sa + ( sh/2)*ca },
        { px + (-sw/2)*ca - ( sh/2)*sa, py + (-sw/2)*sa + ( sh/2)*ca }
    };
    SolidBrush br(c);
    g.FillPolygon(&br, corners, 4);
}

void drawHeart(Graphics& g, float cx, float cy, float scale, float pct, float pulse)
{
    const int N = 300;
    PointF pts[N + 1];
    for (int i = 0; i <= N; i++) {
        float t = (float)i / N * 6.2831853f;
        float hx = 16.0f * sinf(t) * sinf(t) * sinf(t);
        float hy = -(13.0f * cosf(t) - 5.0f * cosf(2.0f*t) - 2.0f * cosf(3.0f*t) - cosf(4.0f*t));
        pts[i] = PointF(cx + hx * scale, cy + hy * scale);
    }
    GraphicsPath heartPath;
    heartPath.AddLines(pts, N + 1);
    heartPath.CloseFigure();

    g.SetSmoothingMode(SmoothingModeAntiAlias);

    // Base shell
    SolidBrush fillB(Color(235, 4, 10, 18));
    g.FillPath(&fillB, &heartPath);

    // Neon core stays inside heart only
    float loadMix = pct / 100.0f;
    float rgbPhase = pulse * 0.28f;
    float rr = 127.0f + 128.0f * sinf(rgbPhase + 0.0f + loadMix * 0.35f);
    float gg = 127.0f + 128.0f * sinf(rgbPhase + 2.094f + loadMix * 0.20f);
    float bb = 127.0f + 128.0f * sinf(rgbPhase + 4.188f + loadMix * 0.45f);
    Color rgbC(255, (BYTE)rr, (BYTE)gg, (BYTE)bb);
    Color heatC = pct < 60 ? COL_CYAN :
                  pct < 80 ? COL_LIME :
                             COL_PINK;
    BYTE mixR = (BYTE)(rgbC.GetR() * 0.55f + heatC.GetR() * 0.45f);
    BYTE mixG = (BYTE)(rgbC.GetG() * 0.55f + heatC.GetG() * 0.45f);
    BYTE mixB = (BYTE)(rgbC.GetB() * 0.55f + heatC.GetB() * 0.45f);
    Color glowC(255, mixR, mixG, mixB);
    {
        PathGradientBrush innerGlow(&heartPath);
        innerGlow.SetCenterPoint(PointF(cx, cy - scale * 0.8f));
        innerGlow.SetCenterColor(Color((BYTE)(82 + 18 * sinf(rgbPhase * 1.4f)),
            glowC.GetR(), glowC.GetG(), glowC.GetB()));
        Color surround[] = {
            Color(0, glowC.GetR(), glowC.GetG(), glowC.GetB())
        };
        int count = 1;
        innerGlow.SetSurroundColors(surround, &count);
        g.FillPath(&innerGlow, &heartPath);
    }

    // Internal circuit grid
    {
        GraphicsState gs = g.Save();
        g.SetClip(&heartPath);
        g.SetSmoothingMode(SmoothingModeNone);
        float top  = cy - 13.0f * scale;
        float bot  = cy + 17.0f * scale;
        float left = cx - 16.0f * scale;
        float right= cx + 16.0f * scale;
        float step = scale * 2.5f;
        Pen tracePen(Color(42, mixR, mixG, mixB), 1.0f);
        for (float y = top; y < bot; y += step)
            g.DrawLine(&tracePen, left, y, right, y);
        for (float x = left; x < right; x += step)
            g.DrawLine(&tracePen, x, top, x, bot);
        SolidBrush nodeBr(Color(64, mixR, mixG, mixB));
        for (float y = top + step * 0.5f; y < bot; y += step)
            for (float x = left + step * 0.5f; x < right; x += step)
                g.FillEllipse(&nodeBr, x - 1.5f, y - 1.5f, 3.0f, 3.0f);

        Pen wavePen(Color(78, glowC.GetR(), glowC.GetG(), glowC.GetB()), 2.0f);
        for (int k = 0; k < 3; k++) {
            GraphicsPath wave;
            float waveY = cy - scale * 4.0f + k * scale * 3.6f;
            for (int j = 0; j <= 48; j++) {
                float fx = left + (right - left) * j / 48.0f;
                float fy = waveY + sinf(j * 0.4f + rgbPhase * 2.1f + k) * scale * 0.45f;
                if (j == 0) wave.AddLine(fx, fy, fx + 0.1f, fy);
                else {
                    PointF last;
                    wave.GetLastPoint(&last);
                    wave.AddLine(last.X, last.Y, fx, fy);
                }
            }
            g.DrawPath(&wavePen, &wave);
        }
        g.Restore(gs);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
    }

    // Tight outline, no spill outside silhouette
    Pen outP(Color(245, glowC.GetR(), glowC.GetG(), glowC.GetB()), 2.0f);
    g.DrawPath(&outP, &heartPath);

    Pen shell(Color(44, 255, 255, 255), 4.5f);
    g.DrawPath(&shell, &heartPath);

    // Percentage text
    FontFamily fam(L"Segoe UI");
    Font fBig(&fam, 70, FontStyleBold, UnitPixel);
    Font fSmall(&fam, 13, FontStyleRegular, UnitPixel);

    wchar_t buf[64];
    swprintf_s(buf, L"%.1f%%", pct);

    // Shadow
    RectF rcP(cx - 130, cy - 58, 260, 92);
    RectF rcPsh(rcP.X + 2, rcP.Y + 2, rcP.Width, rcP.Height);
    drawCenteredText(g, buf, fBig, Color(140, 0, 0, 0), rcPsh);
    drawCenteredText(g, buf, fBig, Color(255, glowC.GetR(), glowC.GetG(), glowC.GetB()), rcP);

    swprintf_s(buf, L"%.1f GB / %.1f GB IN USE", bytesAGB(gRamUsed), bytesAGB(gRamTotal));
    RectF rcGB(cx - 160, cy + 42, 320, 22);
    drawCenteredText(g, buf, fSmall, Color(190, 160, 200, 215), rcGB);
}

void drawTimerSlider(Graphics& g, float x, float y, float w, int mins)
{
    FontFamily fam(L"Segoe UI");
    Font fLab(&fam, 9, FontStyleRegular, UnitPixel);
    Font fBubble(&fam, 9, FontStyleBold, UnitPixel);
    float ty = y + 10.0f;
    if (mins < 1) mins = 1;
    if (mins > 180) mins = 180;
    float tval = (float)(mins - 1) / 179.0f;

    Pen trackBg(Color(70, 34, 44, 60), 4.0f);
    trackBg.SetStartCap(LineCapRound); trackBg.SetEndCap(LineCapRound);
    g.DrawLine(&trackBg, x, ty, x + w, ty);

    float fx = x + tval * w;
    Pen trackGlow(Color(70, 0, 236, 255), 9.0f);
    trackGlow.SetStartCap(LineCapRound); trackGlow.SetEndCap(LineCapRound);
    g.DrawLine(&trackGlow, x, ty, fx, ty);
    Pen trackFill(Color(255, 0, 236, 255), 4.0f);
    trackFill.SetStartCap(LineCapRound); trackFill.SetEndCap(LineCapRound);
    g.DrawLine(&trackFill, x, ty, fx, ty);

    StringFormat sf; sf.SetAlignment(StringAlignmentCenter);
    const int marks[] = { 1, 60, 120, 180 };
    const wchar_t* labels[] = { L"1M", L"60M", L"120M", L"180M" };
    for (int i = 0; i < 4; i++) {
        float tx = x + (float)(marks[i] - 1) / 179.0f * w;
        Color tc = marks[i] <= mins ? (i % 2 == 0 ? COL_CYAN : COL_VIOLET) : Color(88, 80, 100, 120);
        SolidBrush tb(tc);
        g.FillEllipse(&tb, tx - 4.5f, ty - 4.5f, 9.0f, 9.0f);
        Color lc = abs(marks[i] - mins) <= 1 ? (i % 2 == 0 ? COL_CYAN : COL_VIOLET) : Color(140, 130, 155, 170);
        SolidBrush lb(lc);
        RectF rcL(tx - 24, ty + 12, 48, 14);
        g.DrawString(labels[i], -1, &fLab, rcL, &sf, &lb);
    }

    float thumbX = x + tval * w;
    Color thumbC = (((mins / 15) % 2) == 0) ? COL_CYAN : COL_VIOLET;
    SolidBrush glow(Color(90, thumbC.GetR(), thumbC.GetG(), thumbC.GetB()));
    g.FillEllipse(&glow, thumbX - 13.0f, ty - 13.0f, 26.0f, 26.0f);
    SolidBrush thumbFill(thumbC);
    g.FillEllipse(&thumbFill, thumbX - 8.0f, ty - 8.0f, 16.0f, 16.0f);
    Pen thumbBorder(Color(255, 255, 255, 255), 1.5f);
    g.DrawEllipse(&thumbBorder, thumbX - 8.0f, ty - 8.0f, 16.0f, 16.0f);

    wchar_t bubble[16];
    swprintf_s(bubble, L"%d MIN", mins);
    GraphicsPath bubblePath;
    roundedRectPath(bubblePath, thumbX - 30.0f, ty - 38.0f, 60.0f, 20.0f, 8.0f);
    SolidBrush bubbleFill(Color(220, 8, 16, 28));
    g.FillPath(&bubbleFill, &bubblePath);
    Pen bubbleBorder(Color(170, thumbC.GetR(), thumbC.GetG(), thumbC.GetB()), 1.0f);
    g.DrawPath(&bubbleBorder, &bubblePath);
    StringFormat sfB; sfB.SetAlignment(StringAlignmentCenter); sfB.SetLineAlignment(StringAlignmentCenter);
    SolidBrush bubbleText(COL_TEXT);
    g.DrawString(bubble, -1, &fBubble, RectF(thumbX - 30.0f, ty - 38.0f, 60.0f, 20.0f), &sfB, &bubbleText);
}

void drawThresholdSlider(Graphics& g, float x, float y, float w, int pct)
{
    FontFamily fam(L"Segoe UI");
    Font fLab(&fam, 9, FontStyleRegular, UnitPixel);
    Font fBubble(&fam, 9, FontStyleBold, UnitPixel);
    float ty = y + 10.0f;
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    float tval = (float)pct / 100.0f;

    Pen trackBg(Color(70, 34, 44, 60), 4.0f);
    trackBg.SetStartCap(LineCapRound); trackBg.SetEndCap(LineCapRound);
    g.DrawLine(&trackBg, x, ty, x + w, ty);

    float fx = x + tval * w;
    Pen trackGlow(Color(70, 44, 255, 176), 9.0f);
    trackGlow.SetStartCap(LineCapRound); trackGlow.SetEndCap(LineCapRound);
    g.DrawLine(&trackGlow, x, ty, fx, ty);
    Pen trackFill(Color(255, 44, 255, 176), 4.0f);
    trackFill.SetStartCap(LineCapRound); trackFill.SetEndCap(LineCapRound);
    g.DrawLine(&trackFill, x, ty, fx, ty);

    StringFormat sf; sf.SetAlignment(StringAlignmentCenter);
    const int marks[] = { 0, 25, 50, 75, 100 };
    const wchar_t* labels[] = { L"0%", L"25%", L"50%", L"75%", L"100%" };
    for (int i = 0; i < 5; i++) {
        float tx = x + (float)marks[i] / 100.0f * w;
        Color tc = marks[i] <= pct ? COL_MINT : Color(100, 80, 100, 120);
        SolidBrush tb(tc);
        g.FillEllipse(&tb, tx - 3.5f, ty - 3.5f, 7.0f, 7.0f);
        SolidBrush lb(abs(marks[i] - pct) <= 1 ? COL_MINT : Color(140, 130, 155, 170));
        RectF rcL(tx - 18, ty + 10, 36, 13);
        g.DrawString(labels[i], -1, &fLab, rcL, &sf, &lb);
    }

    float thumbX = x + tval * w;
    SolidBrush glow(Color(90, COL_MINT.GetR(), COL_MINT.GetG(), COL_MINT.GetB()));
    g.FillEllipse(&glow, thumbX - 12.0f, ty - 12.0f, 24.0f, 24.0f);
    SolidBrush thumbFill(COL_MINT);
    g.FillEllipse(&thumbFill, thumbX - 7.0f, ty - 7.0f, 14.0f, 14.0f);
    Pen thumbBorder(Color(255, 255, 255, 255), 1.5f);
    g.DrawEllipse(&thumbBorder, thumbX - 7.0f, ty - 7.0f, 14.0f, 14.0f);

    wchar_t bubble[16];
    swprintf_s(bubble, L"%d%%", pct);
    GraphicsPath bubblePath;
    roundedRectPath(bubblePath, thumbX - 24.0f, ty - 38.0f, 48.0f, 20.0f, 8.0f);
    SolidBrush bubbleFill(Color(220, 8, 16, 28));
    g.FillPath(&bubbleFill, &bubblePath);
    Pen bubbleBorder(Color(170, COL_MINT.GetR(), COL_MINT.GetG(), COL_MINT.GetB()), 1.0f);
    g.DrawPath(&bubbleBorder, &bubblePath);
    StringFormat sfB; sfB.SetAlignment(StringAlignmentCenter); sfB.SetLineAlignment(StringAlignmentCenter);
    SolidBrush bubbleText(COL_TEXT);
    g.DrawString(bubble, -1, &fBubble, RectF(thumbX - 24.0f, ty - 38.0f, 48.0f, 20.0f), &sfB, &bubbleText);
}

void drawTile(Graphics& g, float x, float y, float w, float h,
                 const wchar_t* label, const wchar_t* value, Color accent)
{
    drawCard(g, x, y, w, h, 12.0f, COL_CARD, COL_CARD_BORDER);

    // Accent left bar
    GraphicsPath bar;
    roundedRectPath(bar, x + 6, y + 8, 4, h - 16, 2);
    SolidBrush bb(accent);
    g.FillPath(&bb, &bar);

    FontFamily fam(L"Segoe UI");
    Font fLab(&fam, 11, FontStyleRegular, UnitPixel);
    Font fVal(&fam, 18, FontStyleBold, UnitPixel);

    StringFormat sf;
    sf.SetAlignment(StringAlignmentNear);
    sf.SetLineAlignment(StringAlignmentNear);

    SolidBrush bLab(COL_TEXT_DIM);
    RectF rcLab(x + 18, y + 8, w - 24, 16);
    g.DrawString(label, -1, &fLab, rcLab, &sf, &bLab);

    SolidBrush bVal(COL_TEXT);
    RectF rcVal(x + 18, y + 28, w - 24, h - 30);
    g.DrawString(value, -1, &fVal, rcVal, &sf, &bVal);
}

void drawToggle(LPDRAWITEMSTRUCT dis, float anim)
{
    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    bool press = (dis->itemState & ODS_SELECTED) != 0;

    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
    HBITMAP oldBmp = (HBITMAP)SelectObject(mem, bmp);

    paintParentBackground(dis->hwndItem, mem);

    Graphics g(mem);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    float radius = h / 2.0f;
    GraphicsPath path;
    roundedRectPath(path, 0, 0, (float)w, (float)h, radius);

    // Fill (interpolate off->on)
    Color off(255, 60, 66, 92);
    Color on1 = COL_MINT;
    Color on2 = COL_CYAN;
    BYTE rA = (BYTE)(off.GetR() + (on1.GetR() - off.GetR()) * anim);
    BYTE gA = (BYTE)(off.GetG() + (on1.GetG() - off.GetG()) * anim);
    BYTE bA = (BYTE)(off.GetB() + (on1.GetB() - off.GetB()) * anim);
    BYTE rB = (BYTE)(off.GetR() + (on2.GetR() - off.GetR()) * anim);
    BYTE gB = (BYTE)(off.GetG() + (on2.GetG() - off.GetG()) * anim);
    BYTE bB = (BYTE)(off.GetB() + (on2.GetB() - off.GetB()) * anim);

    LinearGradientBrush grad(
        PointF(0, 0), PointF((REAL)w, 0),
        Color(255, rA, gA, bA),
        Color(255, rB, gB, bB));
    g.FillPath(&grad, &path);
    if (press) {
        SolidBrush pressB(Color(55, 0, 0, 0));
        g.FillPath(&pressB, &path);
    }

    Pen border(press ? Color(220, 255, 255, 255) : Color(160, 140, 160, 200), 1.0f);
    g.DrawPath(&border, &path);

    // Knob slides left -> right
    float knobR = (h - 8) / 2.0f;
    float knobLeft  = knobR + 4.0f;
    float knobRight = w - knobR - 4.0f;
    float eased = anim * anim * (3.0f - 2.0f * anim);
    float knobCx = knobLeft + (knobRight - knobLeft) * eased;
    float knobCy = h / 2.0f;
    if (press) knobCy += 1.0f;

    SolidBrush sh(Color(95, 0, 0, 0));
    g.FillEllipse(&sh, knobCx - knobR + 1, knobCy - knobR + 1, knobR*2, knobR*2);

    SolidBrush kf(Color(255, 245, 248, 255));
    g.FillEllipse(&kf, knobCx - knobR, knobCy - knobR, knobR*2, knobR*2);

    Pen knobBorder(Color(180, 0, 180, 210), 1.0f);
    g.DrawEllipse(&knobBorder, knobCx - knobR, knobCy - knobR, knobR*2, knobR*2);

    // Subtle knob highlight
    SolidBrush kh(Color(120, 255, 255, 255));
    g.FillEllipse(&kh, knobCx - knobR + 2, knobCy - knobR + 1, knobR*2 - 4, knobR);

    BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
    SelectObject(mem, oldBmp);
    DeleteObject(bmp);
    DeleteDC(mem);
}

void drawTitleButton(LPDRAWITEMSTRUCT dis, const wchar_t* glyph, bool danger, bool hover)
{
    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    bool press = (dis->itemState & ODS_SELECTED) != 0;
    bool hot = hover || (dis->itemState & ODS_HOTLIGHT) != 0;

    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
    HBITMAP oldBmp = (HBITMAP)SelectObject(mem, bmp);
    paintParentBackground(dis->hwndItem, mem);

    Graphics g(mem);
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    GraphicsPath path;
    roundedRectPath(path, 2.0f, 2.0f, (float)w - 4.0f, (float)h - 4.0f, 8.0f);
    Color base = danger ? Color(110, 110, 28, 54) : Color(95, 18, 24, 38);
    Color edge = danger ? Color(170, 255, 86, 126) : Color(120, 120, 150, 175);
    if (hot) {
        base = danger ? Color(160, 165, 34, 72) : Color(145, 24, 34, 54);
        edge = danger ? Color(220, 255, 118, 152) : Color(180, 170, 205, 230);
    }
    if (press) base = Color(190, 12, 16, 28);
    SolidBrush fill(base);
    g.FillPath(&fill, &path);
    if (hot) {
        SolidBrush glow(Color(danger ? 55 : 42,
            danger ? 255 : 190,
            danger ? 98 : 220,
            danger ? 138 : 255));
        g.FillPath(&glow, &path);
    }
    Pen border(edge, 1.0f);
    g.DrawPath(&border, &path);

    FontFamily fam(L"Segoe UI");
    Font font(&fam, 13, FontStyleBold, UnitPixel);
    StringFormat sf; sf.SetAlignment(StringAlignmentCenter); sf.SetLineAlignment(StringAlignmentCenter);
    SolidBrush tb(Color(240, 232, 240, 248));
    g.DrawString(glyph, -1, &font, RectF(0, 0, (REAL)w, (REAL)h), &sf, &tb);

    BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
    SelectObject(mem, oldBmp);
    DeleteObject(bmp);
    DeleteDC(mem);
}

void drawRoundButton(LPDRAWITEMSTRUCT dis, const wchar_t* glyph, bool hover)
{
    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    bool press = (dis->itemState & ODS_SELECTED) != 0;

    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
    HBITMAP oldBmp = (HBITMAP)SelectObject(mem, bmp);

    paintParentBackground(dis->hwndItem, mem);

    Graphics g(mem);
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    GraphicsPath path;
    path.AddEllipse(0.0f, 0.0f, (REAL)w, (REAL)h);

    Color fill = press ? Color(255, 0, 180, 220) :
                 hover ? Color(220, 40, 80, 130) :
                         Color(180, 40, 48, 76);
    SolidBrush fb(fill);
    g.FillPath(&fb, &path);

    Color bc = press ? COL_CYAN :
               hover ? Color(255, 0, 229, 255) :
                       Color(180, 100, 120, 180);
    Pen bp(bc, 1.2f);
    g.DrawPath(&bp, &path);

    FontFamily fam(L"Segoe UI");
    Font font(&fam, 20, FontStyleBold, UnitPixel);
    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);
    RectF rcT(0, -1, (REAL)w, (REAL)h);

    Color tc = press ? Color(255, 255, 255, 255) :
               hover ? Color(255, 0, 229, 255) :
                       Color(255, 220, 230, 245);
    SolidBrush tb(tc);
    g.DrawString(glyph, -1, &font, rcT, &sf, &tb);

    BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
    SelectObject(mem, oldBmp);
    DeleteObject(bmp);
    DeleteDC(mem);
}

// Paint the parent window's background pixels at this child's location
// into the given DC. Lets owner-drawn rounded controls blend AA edges
// against the actual underlying gradient, no SetWindowRgn needed.
void paintParentBackground(HWND hChild, HDC dst)
{
    HWND parent = GetParent(hChild);
    if (!parent) return;
    POINT pt = { 0, 0 };
    MapWindowPoints(hChild, parent, &pt, 1);
    POINT prev;
    OffsetWindowOrgEx(dst, pt.x, pt.y, &prev);
    SendMessageW(parent, WM_PRINTCLIENT, (WPARAM)dst, PRF_CLIENT);
    SetWindowOrgEx(dst, prev.x, prev.y, NULL);
}

// Cover-fit (object-fit:cover): scale image to fill dest, crop the overflow
void drawImageCover(Graphics& g, Image* img, int dx, int dy, int dw, int dh)
{
    if (!img) return;
    int iw = (int)img->GetWidth();
    int ih = (int)img->GetHeight();
    if (iw <= 0 || ih <= 0) return;
    float ir = (float)iw / (float)ih;
    float dr = (float)dw / (float)dh;
    int sx, sy, sw, sh;
    if (ir > dr) {
        sh = ih;
        sw = (int)((float)ih * dr);
        sx = (iw - sw) / 2;
        sy = 0;
    } else {
        sw = iw;
        sh = (int)((float)iw / dr);
        sx = 0;
        sy = (ih - sh) / 2;
    }
    Rect dst(dx, dy, dw, dh);
    g.DrawImage(img, dst, sx, sy, sw, sh, UnitPixel);
}

void drawCleanButton(LPDRAWITEMSTRUCT dis)
{
    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    bool press    = (dis->itemState & ODS_SELECTED) != 0;
    bool disabled = (dis->itemState & ODS_DISABLED)  != 0;

    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
    HBITMAP oldBmp = (HBITMAP)SelectObject(mem, bmp);
    paintParentBackground(dis->hwndItem, mem);

    Graphics g(mem);
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    float rad = 8.0f;
    GraphicsPath path;
    roundedRectPath(path, 1.0f, 1.0f, (float)w - 2, (float)h - 2, rad);

    if (disabled) {
        SolidBrush dB(Color(200, 15, 25, 35));
        g.FillPath(&dB, &path);
        Pen dP(Color(140, 50, 70, 90), 1.5f);
        g.DrawPath(&dP, &path);

        FontFamily fam2(L"Segoe UI");
        Font fBtn2(&fam2, 14, FontStyleBold, UnitPixel);
        StringFormat sf2; sf2.SetAlignment(StringAlignmentCenter); sf2.SetLineAlignment(StringAlignmentCenter);
        SolidBrush tb2(Color(100, 80, 100, 110));
        RectF rcT2(16.0f, 0, (float)(w - 16), (float)h);
        g.DrawString(L"CLEAN", -1, &fBtn2, rcT2, &sf2, &tb2);
    } else {
        float glow = gCleanButtonGlow;
        if (glow > 0.01f) {
            for (int i = 0; i < 10; i++) {
                GraphicsPath gp;
                roundedRectPath(gp, (float)(1 - i), (float)(1 - i),
                                (float)(w - 2 + i*2), (float)(h - 2 + i*2), rad + i);
                BYTE a = (BYTE)((55.0f * glow) / (1.0f + i * 0.55f));
                Color gc = (i % 2 == 0) ? COL_CYAN : COL_VIOLET;
                Pen gP(Color(a, gc.GetR(), gc.GetG(), gc.GetB()), 1.0f);
                g.DrawPath(&gP, &gp);
            }
        }

        Color c1 = press ? Color(255, 12, 34, 52)  : Color(255, 8, 20, 34);
        Color c2 = press ? Color(255, 34, 10, 68) : Color(255, 10, 28, 52);
        LinearGradientBrush grad(PointF(0, 0), PointF(0, (float)h), c1, c2);
        g.FillPath(&grad, &path);

        Color bc = press ? COL_CYAN :
                   gCleanButtonHover ? COL_VIOLET :
                                 Color(190, 78, 118, 148);
        Pen bp(bc, 1.5f);
        g.DrawPath(&bp, &path);

        // Chip icon
        float icX = (float)w / 2.0f - 70;
        float icY = (float)h / 2.0f - 8;
        Color icC = press ? COL_CYAN : gCleanButtonHover ? COL_VIOLET : COL_MINT;
        Pen icP(icC, 1.5f);
        g.DrawRectangle(&icP, icX, icY, 15.0f, 14.0f);
        for (int k = 0; k < 3; k++) {
            float kx = icX + 3.0f + k * 4.5f;
            g.DrawLine(&icP, kx, icY, kx, icY - 4.0f);
            g.DrawLine(&icP, kx, icY + 14.0f, kx, icY + 18.0f);
        }

        FontFamily fam(L"Segoe UI");
        Font fBtn(&fam, 14, FontStyleBold, UnitPixel);
        StringFormat sf; sf.SetAlignment(StringAlignmentCenter); sf.SetLineAlignment(StringAlignmentCenter);
        SolidBrush tb(icC);
        RectF rcT(18.0f, 0, (float)(w - 18), (float)h);
        g.DrawString(L"CLEAN", -1, &fBtn, rcT, &sf, &tb);
    }

    BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
    SelectObject(mem, oldBmp);
    DeleteObject(bmp);
    DeleteDC(mem);
}

void renderBackgroundCache(HDC ref, int W, int H)
{
    if (gBackgroundCache) { DeleteObject(gBackgroundCache); gBackgroundCache = NULL; }
    gBackgroundCache = CreateCompatibleBitmap(ref, W, H);
    if (!gBackgroundCache) return;

    HDC dc = CreateCompatibleDC(ref);
    HBITMAP old = (HBITMAP)SelectObject(dc, gBackgroundCache);

    Graphics g(dc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    // Hero image background
    SolidBrush bg(COL_BG_BOT);
    g.FillRectangle(&bg, 0, 0, W, H);
    drawImageCover(g, gBgHero, 0, 0, W, H);
    SolidBrush shade(Color(178, 4, 5, 12));
    g.FillRectangle(&shade, 0, 0, W, H);
    SolidBrush topShade(Color(66, 0, 0, 0));
    g.FillRectangle(&topShade, 0, 0, W, 90);

    SelectObject(dc, old);
    DeleteDC(dc);
    gBackgroundCacheW = W;
    gBackgroundCacheH = H;
}

void invalidateBackgroundCache()
{
    if (gBackgroundCache) { DeleteObject(gBackgroundCache); gBackgroundCache = NULL; }
    gBackgroundCacheW = gBackgroundCacheH = 0;
}

// Layout helper — all measurements derived from window size
struct NxLayout {
    float W, H;
    float cardY, cardH, cw;
    float lx, mx, rx;
};
static NxLayout nxCalc(int W, int H)
{
    NxLayout l;
    l.W     = (float)W;
    l.H     = (float)H;
    l.cardH = 158.0f;
    l.cardY = l.H - l.cardH - 18.0f;
    float margin = 20.0f;
    l.cw    = (l.W - margin * 2.0f) / 3.0f - 8.0f;
    l.lx    = margin;
    l.mx    = margin + l.cw + 12.0f;
    l.rx    = margin + (l.cw + 12.0f) * 2.0f;
    return l;
}

void drawBackground(HWND hwnd, HDC hdc, const RECT* dirty)
{
    RECT rc; GetClientRect(hwnd, &rc);
    int W = rc.right, H = rc.bottom;

    RECT pr = { 0, 0, W, H };
    if (dirty) {
        pr = *dirty;
        if (pr.left < 0) pr.left = 0;
        if (pr.top  < 0) pr.top  = 0;
        if (pr.right  > W) pr.right  = W;
        if (pr.bottom > H) pr.bottom = H;
    }
    int pw = pr.right - pr.left;
    int ph = pr.bottom - pr.top;
    if (pw <= 0 || ph <= 0) return;

    if (!gBackgroundCache || gBackgroundCacheW != W || gBackgroundCacheH != H)
        renderBackgroundCache(hdc, W, H);

    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, pw, ph);
    HBITMAP oldBmp = (HBITMAP)SelectObject(mem, bmp);

    if (gBackgroundCache) {
        HDC cdc = CreateCompatibleDC(hdc);
        HBITMAP cold = (HBITMAP)SelectObject(cdc, gBackgroundCache);
        BitBlt(mem, 0, 0, pw, ph, cdc, pr.left, pr.top, SRCCOPY);
        SelectObject(cdc, cold);
        DeleteDC(cdc);
    }

    Graphics g(mem);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    g.TranslateTransform((REAL)-pr.left, (REAL)-pr.top);

    NxLayout L = nxCalc(W, H);
    FontFamily famUi(L"Segoe UI");
    StringFormat sfL; sfL.SetAlignment(StringAlignmentNear); sfL.SetLineAlignment(StringAlignmentNear);

    // -- Title ----------------------------------------------
    Font fTitle(&famUi, 18, FontStyleRegular, UnitPixel);
    SolidBrush titleBr(Color(210, 200, 216, 228));
    const wchar_t* title = L"created by sykron";
    float tx = 24.0f;
    float baseY = 14.0f;
    for (int i = 0; title[i] != L'\0'; i++) {
        wchar_t ch[2] = { title[i], L'\0' };
        float wy = baseY + sinf(gPulse * 0.55f + i * 0.38f) * 2.6f;
        g.DrawString(ch, -1, &fTitle, PointF(tx, wy), &sfL, &titleBr);
        tx += (title[i] == L' ') ? 6.0f : 8.3f;
    }

    // -- Heart ----------------------------------------------
    float heartCX = L.W / 2.0f;
    float heartCY = (L.cardY - 34.0f) / 2.0f + 22.0f;
    float heartScale = (L.cardY - 76.0f) / 30.0f;
    if (heartScale > 10.2f) heartScale = 10.2f;
    if (heartScale <  6.0f) heartScale =  6.0f;
    drawHeart(g, heartCX, heartCY, heartScale, gRamCurrent, gPulse);
    // -- Bottom three cards ---------------------------------
    float cy0 = L.cardY;
    float ch  = L.cardH;
    float cw  = L.cw;

    // LEFT card: Timer
    drawCard(g, L.lx, cy0, cw, ch, 8, COL_CARD, COL_CARD_BORDER);
    {
        Font fHead(&famUi, 10, FontStyleBold, UnitPixel);
        Font fMini(&famUi, 9, FontStyleRegular, UnitPixel);
        SolidBrush hBr(Color(185, 140, 176, 196));
        SolidBrush miniBr(Color(175, 110, 145, 166));
        SolidBrush cyanBr(COL_CYAN);
        StringFormat sfR; sfR.SetAlignment(StringAlignmentFar); sfR.SetLineAlignment(StringAlignmentNear);

        g.DrawString(L"TIMER", -1, &fHead,
                     PointF(L.lx + 14, cy0 + 14), &sfL, &hBr);
        g.DrawString(gTimerEnabled ? L"ON" : L"OFF", -1, &fMini,
                     RectF(L.lx, cy0 + 16, cw - 78, 16), &sfR, &cyanBr);
        g.DrawString(L"INTERVAL", -1, &fMini,
                     RectF(L.lx, cy0 + 38, cw - 20, 14), &sfR, &miniBr);

        // Timer countdown
        float slX = L.lx + 14, slW = cw - 28;
        drawTimerSlider(g, slX, cy0 + 86, slW, gTimerMins);
        if (gTimerEnabled) {
            Font fCd(&famUi, 9, FontStyleBold, UnitPixel);
            wchar_t cdBuf[32];
            swprintf_s(cdBuf, L"NEXT %02d:%02d", gTimerSegundos / 60, gTimerSegundos % 60);
            SolidBrush cdBr(COL_CYAN);
            StringFormat sfC; sfC.SetAlignment(StringAlignmentCenter);
            RectF rcCd(L.lx, cy0 + 126, cw, 14);
            g.DrawString(cdBuf, -1, &fCd, rcCd, &sfC, &cdBr);
        }
    }

    // CENTER: Stats + Button
    {
        Font fAv(&famUi, 12, FontStyleBold, UnitPixel);
        StringFormat sfCen; sfCen.SetAlignment(StringAlignmentCenter);

        wchar_t avBuf[40], cacheBuf[40];
        swprintf_s(avBuf,    L"FREE: %.1f GB", bytesAGB(gRamAvailable));
        swprintf_s(cacheBuf, L"CACHE: %llu MB", gRamCache / (1024*1024));

        SolidBrush avBr(COL_TEXT);
        RectF rcAv(L.mx, cy0 + 18, cw, 18);
        g.DrawString(avBuf, -1, &fAv, rcAv, &sfCen, &avBr);

        SolidBrush cacheBr(Color(180, 132, 255, 196));
        RectF rcCa(L.mx, cy0 + 38, cw, 18);
        g.DrawString(cacheBuf, -1, &fAv, rcCa, &sfCen, &cacheBr);

        // Glow behind the Optimize button (the Win32 button sits here)
        float btnW = cw - 30.0f;
        float btnH = 50.0f;
        float btnX = L.mx + 15.0f;
        float btnY = cy0 + 84.0f;
        BYTE bg2 = (BYTE)(70 + 40 * sinf(gPulse));
        for (int i = 7; i > 0; i--) {
            GraphicsPath gp;
            roundedRectPath(gp, btnX - i, btnY - i, btnW + i*2, btnH + i*2, 8.0f + i);
            Color edgeC = i % 2 == 0 ? COL_CYAN : COL_VIOLET;
            Pen gpn(Color((BYTE)(bg2 / (1.0f + i * 0.6f)), edgeC.GetR(), edgeC.GetG(), edgeC.GetB()), 1.0f);
            g.DrawPath(&gpn, &gp);
        }
    }

    // RIGHT card: Config
    drawCard(g, L.rx, cy0, cw, ch, 8, COL_CARD, COL_CARD_BORDER);
    {
        Font fHead(&famUi, 10, FontStyleBold, UnitPixel);
        Font fValue(&famUi, 24, FontStyleBold, UnitPixel);
        Font fMini(&famUi, 9, FontStyleRegular, UnitPixel);
        SolidBrush hBr(Color(185, 140, 176, 196));
        SolidBrush valueBr(COL_TEXT);

        g.DrawString(L"THRESHOLD", -1, &fHead,
                     PointF(L.rx + 14, cy0 + 14), &sfL, &hBr);
        g.DrawString(L"AUTO", -1, &fMini,
                     PointF(L.rx + cw - 92, cy0 + 18), &sfL, &hBr);

        wchar_t ub[8]; swprintf_s(ub, L"%d%%", gRamThreshold);
        g.DrawString(ub, -1, &fValue, PointF(L.rx + 14, cy0 + 30), &sfL, &valueBr);
        // Threshold percent slider
        float slX = L.rx + 14, slW = cw - 28;
        drawThresholdSlider(g, slX, cy0 + 78, slW, gRamThreshold);

        // Divider
        Pen divP(Color(34, 120, 156, 180), 1.0f);
        g.DrawLine(&divP, L.rx + 14, cy0 + 122, L.rx + cw - 14, cy0 + 122);

        g.DrawString(L"START WITH WINDOWS", -1, &fMini,
                     PointF(L.rx + 14, cy0 + 132), &sfL, &hBr);
    }

    BitBlt(hdc, pr.left, pr.top, pw, ph, mem, 0, 0, SRCCOPY);
    SelectObject(mem, oldBmp);
    DeleteObject(bmp);
    DeleteDC(mem);
}

// =========================================================
// Transparent console EDIT subclass
// Intercepts WM_ERASEBKGND, asks parent to draw its bg into
// the edit's DC at the right offset. Result: console text
// floats on top of the window background.
// =========================================================

LRESULT CALLBACK logBoxSubclassProc(HWND h, UINT msg, WPARAM wp, LPARAM lp,
                                     UINT_PTR /*idSub*/, DWORD_PTR /*ref*/)
{
    if (msg == WM_ERASEBKGND) {
        HDC hdc = (HDC)wp;
        HWND parent = GetParent(h);
        if (!parent) return 1;
        POINT pt = { 0, 0 };
        ClientToScreen(h, &pt);
        ScreenToClient(parent, &pt);
        POINT prev;
        OffsetWindowOrgEx(hdc, pt.x, pt.y, &prev);
        SendMessageW(parent, WM_PRINTCLIENT, (WPARAM)hdc, PRF_CLIENT);
        SetWindowOrgEx(hdc, prev.x, prev.y, NULL);
        return 1;
    }

    if (msg == WM_VSCROLL || msg == WM_MOUSEWHEEL || msg == EM_REPLACESEL ||
        msg == WM_SETTEXT || msg == WM_KEYUP) {
        LRESULT r = DefSubclassProc(h, msg, wp, lp);
        InvalidateRect(h, NULL, TRUE);
        return r;
    }

    return DefSubclassProc(h, msg, wp, lp);
}

// =========================================================
// Window procedure
// =========================================================

LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        hMainWindow = hwnd;

        RECT rcWin; GetClientRect(hwnd, &rcWin);
        int W = rcWin.right, H = rcWin.bottom;
        gStartWithWindows = readStartWithWindows();
        gStartupAnim   = gStartWithWindows ? 1.0f : 0.0f;

        // Fonts
        gFontTitle = CreateFontW(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        gFontBig = CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        gFontMed = CreateFontW(15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        gFontSmall = CreateFontW(13, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        gFontMono = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Cascadia Mono");

        // Derive layout (same math as nxCalc / drawBackground)
        float margin = 20.0f;
        float cardH  = 158.0f;
        float cardY  = (float)H - cardH - 18.0f;
        float cw     = ((float)W - margin * 2.0f) / 3.0f - 8.0f;
        float lxF    = margin;
        float mxF    = margin + cw + 12.0f;
        float rxF    = margin + (cw + 12.0f) * 2.0f;

        int lx = (int)lxF, mx = (int)mxF, rx = (int)rxF;
        int cy = (int)cardY;
        int icw = (int)cw;

        // Left card – threshold dec / edit / inc  (centred)
        int thX = lx + icw / 2 - 52;
        int thY = cy + 118;
        hDecButton = CreateWindowW(L"BUTTON", L"",
            WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | BS_OWNERDRAW,
            thX, thY, 26, 26, hwnd, (HMENU)ID_BTN_DEC, NULL, NULL);
        hThresholdEdit = CreateWindowW(L"EDIT", L"80",
            WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | ES_NUMBER | ES_CENTER,
            thX + 30, thY + 2, 52, 22, hwnd, (HMENU)ID_THRESHOLD_EDIT, NULL, NULL);
        hIncButton = CreateWindowW(L"BUTTON", L"",
            WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | BS_OWNERDRAW,
            thX + 86, thY, 26, 26, hwnd, (HMENU)ID_BTN_INC, NULL, NULL);
        ShowWindow(hDecButton, SW_HIDE);
        ShowWindow(hThresholdEdit, SW_HIDE);
        ShowWindow(hIncButton, SW_HIDE);

        // Centre – Optimize RAM button
        int btnW = icw - 30, btnH = 50;
        int btnX = mx + 15;
        int btnY = cy + 84;
        hCleanButton = CreateWindowW(L"BUTTON", L"",
            WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | BS_OWNERDRAW,
            btnX, btnY, btnW, btnH, hwnd, (HMENU)ID_CLEAN_BUTTON, NULL, NULL);

        // Progress bar under button (hidden; shown during cleanup)
        hProgressBar = CreateWindowW(PROGRESS_CLASSW, NULL,
            WS_CHILD | WS_CLIPSIBLINGS | PBS_SMOOTH,
            btnX, btnY + btnH + 4, btnW, 5, hwnd, (HMENU)ID_PROGRESS_BAR, NULL, NULL);
        SendMessageW(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendMessageW(hProgressBar, PBM_SETPOS, 0, 0);
        SetWindowTheme(hProgressBar, L"", L"");
        SendMessageW(hProgressBar, PBM_SETBARCOLOR, 0, RGB_CYAN);
        SendMessageW(hProgressBar, PBM_SETBKCOLOR,  0, RGB(10, 20, 28));

        // Right card – startup toggle (top-right area)
        int togW = 54, togH = 24;
        int stTogX = rx + icw - 126;
        hStartupToggle = CreateWindowW(L"BUTTON", L"",
            WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | BS_OWNERDRAW,
            stTogX, cy + 128, togW, togH, hwnd, (HMENU)ID_STARTUP_TOGGLE, NULL, NULL);

        // Right card – auto-cleanup toggle (bottom-right area)
        hAutoToggle = CreateWindowW(L"BUTTON", L"",
            WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | BS_OWNERDRAW,
            rx + icw - 62, cy + 12, togW, togH, hwnd, (HMENU)ID_AUTO_TOGGLE, NULL, NULL);
        hMinWindowButton = CreateWindowW(L"BUTTON", L"",
            WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | BS_OWNERDRAW,
            W - 90, 8, 34, 24, hwnd, (HMENU)ID_WIN_MIN, NULL, NULL);
        hCloseWindowButton = CreateWindowW(L"BUTTON", L"",
            WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | BS_OWNERDRAW,
            W - 50, 8, 34, 24, hwnd, (HMENU)ID_WIN_CLOSE, NULL, NULL);
        hTooltip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
            WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            hwnd, NULL, gHInst, NULL);
        if (hTooltip) {
            SetWindowPos(hTooltip, HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            TOOLINFOW ti = { sizeof(ti) };
            ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
            ti.hwnd = hwnd;
            ti.uId = (UINT_PTR)hMinWindowButton;
            ti.lpszText = (LPWSTR)L"Minimize";
            SendMessageW(hTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
            ti.uId = (UINT_PTR)hCloseWindowButton;
            ti.lpszText = (LPWSTR)L"Close";
            SendMessageW(hTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
        }

        // Left card - timer toggle + minutes input
        hTimerToggle = CreateWindowW(L"BUTTON", L"",
            WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | BS_OWNERDRAW,
            lx + icw - 68, cy + 10, 54, 24, hwnd, (HMENU)ID_TIMER_TOGGLE, NULL, NULL);
        hTimerEdit = CreateWindowW(L"EDIT", L"30",
            WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | ES_NUMBER | ES_CENTER,
            lx + 14, cy + 58, 84, 28, hwnd, (HMENU)ID_TIMER_EDIT, NULL, NULL);
        ShowWindow(hTimerEdit, SW_HIDE);

        // Hidden console (off-screen; keeps appendLine() working)
        hLogBox = CreateWindowW(L"EDIT", L"  RAMMY ready.\r\n",
            WS_CHILD | WS_CLIPSIBLINGS | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            0, H + 10, 2, 2, hwnd, (HMENU)ID_LOG_BOX, NULL, NULL);

        hTimerDec    = NULL;
        hTimerInc    = NULL;

        SendMessageW(hCleanButton,        WM_SETFONT, (WPARAM)gFontBig,   TRUE);
        SendMessageW(hLogBox,    WM_SETFONT, (WPARAM)gFontMono,  TRUE);
        SendMessageW(hAutoToggle,    WM_SETFONT, (WPARAM)gFontSmall, TRUE);
        SendMessageW(hStartupToggle,WM_SETFONT, (WPARAM)gFontSmall, TRUE);
        SendMessageW(hThresholdEdit,   WM_SETFONT, (WPARAM)gFontBig,   TRUE);
        SendMessageW(hTimerToggle,  WM_SETFONT, (WPARAM)gFontSmall, TRUE);
        SendMessageW(hTimerEdit,    WM_SETFONT, (WPARAM)gFontBig,   TRUE);

        gBrushCard    = CreateSolidBrush(RGB_CARD);
        gBrushBg      = CreateSolidBrush(RGB_BG);
        gBrushConsole = CreateSolidBrush(RGB_CONSOLE_BG);

        SetWindowSubclass(hLogBox, logBoxSubclassProc, 1, 0);

        // Dark title bar (Win10+)
        BOOL dark = TRUE;
        DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));

        if (!gAppIcon) gAppIcon = loadAppIcon();
        if (!gAppIcon) gAppIcon = LoadIcon(NULL, IDI_APPLICATION);
        SendMessageW(hwnd, WM_SETICON, ICON_BIG,   (LPARAM)gAppIcon);
        SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)gAppIcon);
        addTrayIcon(hwnd);

        updateStats();
        gRamCurrent = gRamTarget;

        if (timeBeginPeriod(1) == TIMERR_NOERROR) gHighResTimer = true;
        SetTimer(hwnd, ID_TIMER_MONITOR, 2000, NULL);
        SetTimer(hwnd, ID_TIMER_ANIM,    10,   NULL);

        InvalidateRect(hwnd, NULL, TRUE);
        break;
    }

    case WM_TIMER:
    {
        if (wp == ID_TIMER_MONITOR) {
            updateStats();

            wchar_t buf[128];
            swprintf_s(gNID.szTip, L"RAMMY  -  RAM  %.0f %%", gRamTarget);
            {
                NOTIFYICONDATA nid = gNID;
                nid.uFlags = NIF_TIP;
                Shell_NotifyIconW(NIM_MODIFY, &nid);
            }

            if (gAutoCleanEnabled && !gCleaningNow && (int)gRamTarget >= gRamThreshold) {
                swprintf_s(buf, L"RAM at %d%% >> Automatic cleanup triggered.", (int)gRamTarget);
                showTrayNotification(L"RAMMY - Auto", buf);
                startCleanup();
            }

            if (gTimerEnabled && !gCleaningNow) {
                gTimerSegundos -= 2;
                if (gTimerSegundos <= 0) {
                    gTimerSegundos = gTimerMins * 60;
                    showTrayNotification(L"RAMMY - Timer", L"Automatic timer cleanup.");
                    startCleanup();
                }
            }
        }
        else if (wp == ID_TIMER_ANIM) {
            // Smooth gauge animation
            float diff = gRamTarget - gRamCurrent;
            if (fabsf(diff) > 0.05f) {
                gRamCurrent += diff * 0.12f;
            } else {
                gRamCurrent = gRamTarget;
            }
            gPulse += 0.08f;

            // Smooth button glow toward target
            float target = (gCleanButtonHover || gCleanButtonPress) ? 1.0f : 0.0f;
            float gd = target - gCleanButtonGlow;
            bool botonGlowDirty = false;
            if (fabsf(gd) > 0.01f) {
                gCleanButtonGlow += gd * 0.18f;
                botonGlowDirty = true;
            } else if (gCleanButtonGlow != target) {
                gCleanButtonGlow = target;
                botonGlowDirty = true;
            }

            static DWORD lastFrameTick = 0;
            DWORD now = GetTickCount();
            float dt = lastFrameTick ? (float)(now - lastFrameTick) / 16.6667f : 1.0f;
            if (dt < 0.25f) dt = 0.25f;
            if (dt > 2.0f) dt = 2.0f;
            lastFrameTick = now;

            // Toggle slides
            float tgt = gAutoCleanEnabled ? 1.0f : 0.0f;
            float td = tgt - gToggleAnim;
            if (fabsf(td) > 0.001f) {
                gToggleAnim += td * (1.0f - powf(0.58f, dt));
                if (hAutoToggle) InvalidateRect(hAutoToggle, NULL, FALSE);
            } else if (gToggleAnim != tgt) {
                gToggleAnim = tgt;
                if (hAutoToggle) InvalidateRect(hAutoToggle, NULL, FALSE);
            }

            float tTimerTgt = gTimerEnabled ? 1.0f : 0.0f;
            float ttd = tTimerTgt - gTimerAnim;
            if (fabsf(ttd) > 0.001f) {
                gTimerAnim += ttd * (1.0f - powf(0.58f, dt));
                if (hTimerToggle) InvalidateRect(hTimerToggle, NULL, FALSE);
            } else if (gTimerAnim != tTimerTgt) {
                gTimerAnim = tTimerTgt;
                if (hTimerToggle) InvalidateRect(hTimerToggle, NULL, FALSE);
            }

            float tStartupTgt = gStartWithWindows ? 1.0f : 0.0f;
            float tsd = tStartupTgt - gStartupAnim;
            if (fabsf(tsd) > 0.001f) {
                gStartupAnim += tsd * (1.0f - powf(0.58f, dt));
                if (hStartupToggle) InvalidateRect(hStartupToggle, NULL, FALSE);
            } else if (gStartupAnim != tStartupTgt) {
                gStartupAnim = tStartupTgt;
                if (hStartupToggle) InvalidateRect(hStartupToggle, NULL, FALSE);
            }

            // Invalidate only the parts we need
            static DWORD lastHeavyAnim = 0;
            if (now - lastHeavyAnim >= 33) {
                lastHeavyAnim = now;
                RECT rcA; GetClientRect(hwnd, &rcA);
                // Heart + shards (upper area)
                RECT heartRc = { 0, 0, rcA.right, rcA.bottom - 180 };
                InvalidateRect(hwnd, &heartRc, FALSE);
                // Status dot
                RECT statusRc = { rcA.right - 220, 12, rcA.right, 52 };
                InvalidateRect(hwnd, &statusRc, FALSE);
                // Centre card (button glow + stats)
                NxLayout NLA = nxCalc(rcA.right, rcA.bottom);
                RECT centRc = { (int)NLA.mx - 4, (int)NLA.cardY, (int)(NLA.mx + NLA.cw) + 4, rcA.bottom };
                InvalidateRect(hwnd, &centRc, FALSE);
            }
            if (botonGlowDirty && hCleanButton) InvalidateRect(hCleanButton, NULL, FALSE);

            static int lastCdSec = -1;
            int curCdSec = gTimerEnabled ? gTimerSegundos : -1;
            if (curCdSec != lastCdSec) {
                lastCdSec = curCdSec;
                RECT rcA2; GetClientRect(hwnd, &rcA2);
                NxLayout NLA2 = nxCalc(rcA2.right, rcA2.bottom);
                int ctrlY = (int)NLA2.cardY;
                // Invalidate left card (countdown text sits there)
                RECT cdRc = { 0, ctrlY + 60, (int)NLA2.mx, ctrlY + 86 };
                InvalidateRect(hwnd, &cdRc, FALSE);
            }
        }
        break;
    }

    case WM_TRAYICON:
    {
        if (lp == WM_LBUTTONDBLCLK || lp == WM_LBUTTONUP) {
            showMainWindow(hwnd);
        } else if (lp == WM_RBUTTONUP) {
            POINT pt; GetCursorPos(&pt);
            HMENU menu = CreatePopupMenu();
            AppendMenuW(menu, MF_STRING, ID_TRAY_OPEN,   L"Open RAMMY");
            AppendMenuW(menu, MF_STRING, ID_TRAY_CLEAN, L"Clean RAM now");
            AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(menu, MF_STRING, ID_TRAY_EXIT,   L"Exit");
            SetForegroundWindow(hwnd);
            TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(menu);
        }
        break;
    }

    case WM_LIMPIEZA_FIN:
    {
        gCleaningNow = false;
        gRamCurrent = gRamTarget;
        EnableWindow(hCleanButton, TRUE);
        ShowWindow(hProgressBar, SW_HIDE);
        InvalidateRect(hwnd, NULL, FALSE);
        RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
        break;
    }

    case WM_COMMAND:
    {
        WORD id = LOWORD(wp);
        if (id == ID_CLEAN_BUTTON) {
            startCleanup();
        } else if (id == ID_STARTUP_TOGGLE) {
            bool nextValue = !gStartWithWindows;
            if (writeStartWithWindows(nextValue)) {
                gStartWithWindows = nextValue;
                gStartupAnim += ((gStartWithWindows ? 1.0f : 0.0f) - gStartupAnim) * 0.45f;
                RedrawWindow(hStartupToggle, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
            } else {
                MessageBoxW(hwnd,
                    L"Could not update Start with Windows.",
                    L"RAMMY", MB_ICONWARNING | MB_OK);
            }
        } else if (id == ID_AUTO_TOGGLE) {
            gAutoCleanEnabled = !gAutoCleanEnabled;
            gToggleAnim += ((gAutoCleanEnabled ? 1.0f : 0.0f) - gToggleAnim) * 0.45f;
            RedrawWindow(hAutoToggle, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
        } else if (id == ID_BTN_DEC) {
            if (gRamThreshold > 10) {
                gRamThreshold--;
                wchar_t b[8]; swprintf_s(b, L"%d", gRamThreshold);
                SetWindowTextW(hThresholdEdit, b);
            }
        } else if (id == ID_BTN_INC) {
            if (gRamThreshold < 99) {
                gRamThreshold++;
                wchar_t b[8]; swprintf_s(b, L"%d", gRamThreshold);
                SetWindowTextW(hThresholdEdit, b);
            }
        } else if (id == ID_THRESHOLD_EDIT && HIWORD(wp) == EN_CHANGE) {
            wchar_t buf[8];
            GetWindowTextW(hThresholdEdit, buf, 8);
            int val = _wtoi(buf);
            if (val >= 10 && val <= 99) gRamThreshold = val;
        } else if (id == ID_TIMER_TOGGLE) {
            gTimerEnabled = !gTimerEnabled;
            if (gTimerMins < 1) gTimerMins = 1;
            gTimerSegundos = gTimerEnabled ? gTimerMins * 60 : 0;
            gTimerAnim += ((gTimerEnabled ? 1.0f : 0.0f) - gTimerAnim) * 0.45f;
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == ID_TIMER_EDIT && HIWORD(wp) == EN_CHANGE) {
            wchar_t buf[16];
            GetWindowTextW(hTimerEdit, buf, 16);
            int val = _wtoi(buf);
            if (val >= 1 && val <= 999) {
                gTimerMins = val;
                if (gTimerEnabled) gTimerSegundos = gTimerMins * 60;
            }
        } else if (id == ID_TRAY_OPEN) {
            showMainWindow(hwnd);
        } else if (id == ID_TRAY_CLEAN) {
            startCleanup();
        } else if (id == ID_WIN_MIN) {
            ShowWindow(hwnd, SW_MINIMIZE);
        } else if (id == ID_WIN_CLOSE) {
            DestroyWindow(hwnd);
        } else if (id == ID_TRAY_EXIT) {
            DestroyWindow(hwnd);
        }
        break;
    }

    case WM_SIZE:
    {
        if (wp == SIZE_MINIMIZED) {
            ShowWindow(hwnd, SW_HIDE);
        } else {
            RECT rcNow; GetClientRect(hwnd, &rcNow);
            NxLayout NL = nxCalc(rcNow.right, rcNow.bottom);

            int lx = (int)NL.lx;
            int mx = (int)NL.mx;
            int rx = (int)NL.rx;
            int cy = (int)NL.cardY;
            int icw = (int)NL.cw;

            int thX = lx + icw / 2 - 52;
            int thY = cy + 118;
            if (hDecButton)     MoveWindow(hDecButton, thX, thY, 26, 26, TRUE);
            if (hThresholdEdit) MoveWindow(hThresholdEdit, thX + 30, thY + 2, 52, 22, TRUE);
            if (hIncButton)     MoveWindow(hIncButton, thX + 86, thY, 26, 26, TRUE);

            int btnW = icw - 30, btnH = 50;
            int btnX = mx + 15;
            int btnY = cy + 84;
            if (hCleanButton) MoveWindow(hCleanButton, btnX, btnY, btnW, btnH, TRUE);
            if (hProgressBar) MoveWindow(hProgressBar, btnX, btnY + btnH + 4, btnW, 5, TRUE);

            int togW = 54, togH = 24;
            if (hStartupToggle) MoveWindow(hStartupToggle, rx + icw -  62, cy + 128, togW, togH, TRUE);
            if (hAutoToggle)     MoveWindow(hAutoToggle,     rx + icw -  62, cy +  12, togW, togH, TRUE);
            if (hTimerToggle)   MoveWindow(hTimerToggle,   lx + icw -  68, cy +  10, 54, 24, TRUE);
            if (hTimerEdit)     MoveWindow(hTimerEdit,     lx + 14,        cy +  58, 84, 28, TRUE);
            if (hMinWindowButton)     MoveWindow(hMinWindowButton,     rcNow.right - 90, 8, 34, 24, TRUE);
            if (hCloseWindowButton)   MoveWindow(hCloseWindowButton,   rcNow.right - 50, 8, 34, 24, TRUE);

            invalidateBackgroundCache();
        }
        break;
    }

    case WM_ENTERSIZEMOVE:
    {
        gMovingWindow = true;
        KillTimer(hwnd, ID_TIMER_ANIM);
        break;
    }

    case WM_EXITSIZEMOVE:
    {
        gMovingWindow = false;
        SetTimer(hwnd, ID_TIMER_ANIM, 10, NULL);
        InvalidateRect(hwnd, NULL, FALSE);
        break;
    }

    case WM_NCHITTEST:
    {
        LRESULT hit = DefWindowProcW(hwnd, msg, wp, lp);
        if (hit == HTCLIENT) {
            POINT pt = { (int)(short)LOWORD(lp), (int)(short)HIWORD(lp) };
            ScreenToClient(hwnd, &pt);
            RECT rc; GetClientRect(hwnd, &rc);
            const int grip = 8;
            if (pt.x < grip && pt.y < grip) return HTTOPLEFT;
            if (pt.x > rc.right - grip && pt.y < grip) return HTTOPRIGHT;
            if (pt.x < grip && pt.y > rc.bottom - grip) return HTBOTTOMLEFT;
            if (pt.x > rc.right - grip && pt.y > rc.bottom - grip) return HTBOTTOMRIGHT;
            if (pt.x < grip) return HTLEFT;
            if (pt.x > rc.right - grip) return HTRIGHT;
            if (pt.y < grip) return HTTOP;
            if (pt.y > rc.bottom - grip) return HTBOTTOM;
            if (pt.y < kTitlebarH) {
                HWND child = ChildWindowFromPoint(hwnd, pt);
                if (child == hwnd) return HTCAPTION;
            }
        }
        return hit;
    }

    case WM_ERASEBKGND:
        return 1;  // we paint full bg in WM_PAINT

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        drawBackground(hwnd, hdc, &ps.rcPaint);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_PRINTCLIENT:
    {
        drawBackground(hwnd, (HDC)wp, NULL);
        return 0;
    }

    case WM_CTLCOLOREDIT:
    {
        HDC  hdc   = (HDC)wp;
        HWND hCtrl = (HWND)lp;
        if (hCtrl == hLogBox) {
            SetTextColor(hdc, RGB_MINT);
            SetBkMode(hdc, TRANSPARENT);
            return (INT_PTR)GetStockObject(NULL_BRUSH);
        }
        // Threshold + timer edit
        SetTextColor(hdc, RGB_CYAN);
        SetBkColor(hdc, RGB_CARD);
        return (INT_PTR)gBrushCard;
    }

    case WM_CTLCOLORSTATIC:
    {
        HDC  hdc   = (HDC)wp;
        HWND hCtrl = (HWND)lp;
        if (hCtrl == hLogBox) {
            SetTextColor(hdc, RGB_MINT);
            SetBkMode(hdc, TRANSPARENT);
            return (INT_PTR)GetStockObject(NULL_BRUSH);
        }
        SetTextColor(hdc, RGB_TEXT);
        SetBkColor(hdc, RGB_CARD);
        SetBkMode(hdc, TRANSPARENT);
        return (INT_PTR)gBrushCard;
    }

    case WM_CTLCOLORBTN:
    {
        SetTextColor((HDC)wp, RGB_TEXT);
        SetBkColor((HDC)wp, RGB_CARD);
        return (INT_PTR)gBrushCard;
    }

    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lp;
        if (dis->CtlID == ID_CLEAN_BUTTON) {
            drawCleanButton(dis);
            return TRUE;
        }
        if (dis->CtlID == ID_AUTO_TOGGLE) {
            drawToggle(dis, gToggleAnim);
            return TRUE;
        }
        if (dis->CtlID == ID_STARTUP_TOGGLE) {
            drawToggle(dis, gStartupAnim);
            return TRUE;
        }
        if (dis->CtlID == ID_BTN_DEC) {
            drawRoundButton(dis, L"-", gDecButtonHover);
            return TRUE;
        }
        if (dis->CtlID == ID_BTN_INC) {
            drawRoundButton(dis, L"+", gIncButtonHover);
            return TRUE;
        }
        if (dis->CtlID == ID_TIMER_TOGGLE) {
            drawToggle(dis, gTimerAnim);
            return TRUE;
        }
        if (dis->CtlID == ID_WIN_MIN) {
            drawTitleButton(dis, L"—", false, gMinButtonHover);
            return TRUE;
        }
        if (dis->CtlID == ID_WIN_CLOSE) {
            drawTitleButton(dis, L"×", true, gCloseButtonHover);
            return TRUE;
        }
        break;
    }

    case WM_MOUSEMOVE:
    {
        int mx2 = (int)(short)LOWORD(lp);
        int my2 = (int)(short)HIWORD(lp);
        POINT pt = { mx2, my2 };

        // Slider drag
        if (GetCapture() == hwnd) {
            RECT rcC2; GetClientRect(hwnd, &rcC2);
            NxLayout NL2 = nxCalc(rcC2.right, rcC2.bottom);
            if (gTimerSliderHit) {
                float slX = NL2.lx + 14, slW = NL2.cw - 28;
                float t = ((float)mx2 - slX) / slW;
                if (t < 0) t = 0; if (t > 1) t = 1;
                gTimerMins = 1 + (int)floorf(t * 179.0f + 0.5f);
                if (gTimerMins < 1) gTimerMins = 1;
                if (gTimerMins > 180) gTimerMins = 180;
                if (gTimerEnabled) gTimerSegundos = gTimerMins * 60;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            if (gThreshSliderHit) {
                float slX = NL2.rx + 14, slW = NL2.cw - 28;
                float t = ((float)mx2 - slX) / slW;
                if (t < 0) t = 0; if (t > 1) t = 1;
                gRamThreshold = (int)floorf(t * 100.0f + 0.5f);
                if (gRamThreshold < 0) gRamThreshold = 0;
                if (gRamThreshold > 100) gRamThreshold = 100;
                wchar_t b[8]; swprintf_s(b, L"%d", gRamThreshold);
                if (hThresholdEdit) SetWindowTextW(hThresholdEdit, b);
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }

        auto check = [&](HWND h, bool& flag) {
            if (!h) return;
            RECT rb; GetWindowRect(h, &rb);
            POINT tl = { rb.left, rb.top }, br2 = { rb.right, rb.bottom };
            ScreenToClient(hwnd, &tl);
            ScreenToClient(hwnd, &br2);
            RECT loc = { tl.x, tl.y, br2.x, br2.y };
            bool over = PtInRect(&loc, pt);
            if (over != flag) { flag = over; InvalidateRect(h, NULL, FALSE); }
        };
        check(hCleanButton,  gCleanButtonHover);
        check(hDecButton, gDecButtonHover);
        check(hIncButton, gIncButtonHover);
        check(hMinWindowButton, gMinButtonHover);
        check(hCloseWindowButton, gCloseButtonHover);

        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tme);
        break;
    }

    case WM_MOUSELEAVE:
    {
        if (gCleanButtonHover) { gCleanButtonHover = false; InvalidateRect(hCleanButton, NULL, FALSE); }
        if (gDecButtonHover)   { gDecButtonHover   = false; InvalidateRect(hDecButton, NULL, FALSE); }
        if (gIncButtonHover)   { gIncButtonHover   = false; InvalidateRect(hIncButton, NULL, FALSE); }
        if (gMinButtonHover)   { gMinButtonHover   = false; InvalidateRect(hMinWindowButton, NULL, FALSE); }
        if (gCloseButtonHover) { gCloseButtonHover = false; InvalidateRect(hCloseWindowButton, NULL, FALSE); }
        break;
    }

    case WM_LBUTTONDOWN:
    {
        RECT rcC; GetClientRect(hwnd, &rcC);
        int W2 = rcC.right, H2 = rcC.bottom;
        NxLayout NL = nxCalc(W2, H2);
        int mx2 = (int)(short)LOWORD(lp);
        int my2 = (int)(short)HIWORD(lp);

        // Timer slider bounds
        {
            float slX = NL.lx + 14, slW = NL.cw - 28;
            float ty  = NL.cardY + 86 + 10.0f;
            if (my2 >= (int)(ty - 12) && my2 <= (int)(ty + 28) &&
                mx2 >= (int)(slX - 10) && mx2 <= (int)(slX + slW + 10))
            {
                float t = ((float)mx2 - slX) / slW;
                if (t < 0) t = 0; if (t > 1) t = 1;
                gTimerMins = 1 + (int)floorf(t * 179.0f + 0.5f);
                if (gTimerMins < 1) gTimerMins = 1;
                if (gTimerMins > 180) gTimerMins = 180;
                if (gTimerEnabled) gTimerSegundos = gTimerMins * 60;
                gTimerSliderHit = true;
                SetCapture(hwnd);
                InvalidateRect(hwnd, NULL, FALSE);
                break;
            }
        }

        // Threshold slider bounds
        {
            float slX = NL.rx + 14, slW = NL.cw - 28;
            float ty  = NL.cardY + 78 + 10.0f;
            if (my2 >= (int)(ty - 12) && my2 <= (int)(ty + 28) &&
                mx2 >= (int)(slX - 10) && mx2 <= (int)(slX + slW + 10))
            {
                float t = ((float)mx2 - slX) / slW;
                if (t < 0) t = 0; if (t > 1) t = 1;
                gRamThreshold  = (int)floorf(t * 100.0f + 0.5f);
                wchar_t b[8]; swprintf_s(b, L"%d", gRamThreshold);
                if (hThresholdEdit) SetWindowTextW(hThresholdEdit, b);
                gThreshSliderHit = true;
                SetCapture(hwnd);
                InvalidateRect(hwnd, NULL, FALSE);
                break;
            }
        }
        break;
    }

    case WM_LBUTTONUP:
    {
        if (GetCapture() == hwnd) ReleaseCapture();
        gTimerSliderHit = false;
        gThreshSliderHit = false;
        break;
    }

    case WM_CAPTURECHANGED:
    {
        gTimerSliderHit = false;
        gThreshSliderHit = false;
        break;
    }

    case WM_DESTROY:
    {
        KillTimer(hwnd, ID_TIMER_MONITOR);
        KillTimer(hwnd, ID_TIMER_ANIM);
        if (gHighResTimer) {
            timeEndPeriod(1);
            gHighResTimer = false;
        }
        removeTrayIcon();
        if (gBgHero)        { delete gBgHero; gBgHero = NULL; }
        if (gBrushCard)      { DeleteObject(gBrushCard); gBrushCard = NULL; }
        if (gBrushBg)        { DeleteObject(gBrushBg); gBrushBg = NULL; }
        if (gBrushConsole)   { DeleteObject(gBrushConsole); gBrushConsole = NULL; }
        if (gAppIcon)       { DestroyIcon(gAppIcon); gAppIcon = NULL; }
        if (gFontTitle)      { DeleteObject(gFontTitle); }
        if (gFontBig)        { DeleteObject(gFontBig); }
        if (gFontMed)        { DeleteObject(gFontMed); }
        if (gFontSmall)      { DeleteObject(gFontSmall); }
        if (gFontMono)       { DeleteObject(gFontMono); }
        invalidateBackgroundCache();
        PostQuitMessage(0);
        break;
    }

    default:
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    return 0;
}

// =========================================================
// Entry point
// =========================================================

int WINAPI wWinMain(
    _In_     HINSTANCE hInst,
    _In_opt_ HINSTANCE /*hPrev*/,
    _In_     PWSTR     /*cmdLine*/,
    _In_     int       nShow)
{
    if (!isAdministrator()) { relaunchAsAdministrator(); return 0; }

    gHInst = hInst;

    GdiplusStartupInput gdipInput;
    GdiplusStartup(&gGdiToken, &gdipInput, NULL);
    loadHeroImage();

    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_PROGRESS_CLASS | ICC_UPDOWN_CLASS | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    const wchar_t WINDOW_CLASS[] = L"RammyCyberClass";
    WNDCLASSW wc   = {};
    wc.lpfnWndProc   = windowProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = WINDOW_CLASS;
    wc.hbrBackground = NULL;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    gAppIcon = loadAppIcon();
    wc.hIcon         = gAppIcon;
    if (!wc.hIcon) wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    RegisterClassW(&wc);

    HWND window = CreateWindowExW(0, WINDOW_CLASS,
        L"RAMMY",
        WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_SYSMENU | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1060, 680,
        NULL, NULL, hInst, NULL);

    if (!window) { GdiplusShutdown(gGdiToken); return 0; }

    ShowWindow(window, nShow);
    UpdateWindow(window);

    MSG mensaje = {};
    while (GetMessageW(&mensaje, NULL, 0, 0)) {
        TranslateMessage(&mensaje);
        DispatchMessageW(&mensaje);
    }

    GdiplusShutdown(gGdiToken);
    return 0;
}
