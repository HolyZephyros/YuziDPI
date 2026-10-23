#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include "dpi_engine.h"
#include "i18n.h"
#include "ota_updater.h"
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <objidl.h>
#include <gdiplus.h>

using namespace Gdiplus;

#define WM_TRAYICON       (WM_USER + 100)
#define WM_ENGINE_LOG     (WM_USER + 101)
#define WM_ENGINE_STATE   (WM_USER + 102)

// Kontrol ID'leri
#define ID_SCOPE_TAB_0    110
#define ID_SCOPE_TAB_1    111
#define ID_SCOPE_TAB_2    112

#define ID_STAGE_BTN_1    121
#define ID_STAGE_BTN_2    122
#define ID_STAGE_BTN_3    123
#define ID_STAGE_BTN_4    124
#define ID_STAGE_BTN_5    125
#define ID_STAGE_BTN_6    126

#define ID_CHK_AUTOCONN   131
#define ID_CHK_WINSTART   132
#define ID_CHK_MINSTART   133
#define ID_BTN_LANG       140

#define ID_BTN_ADD_EXE    202
#define ID_BTN_ADD_FOLDER 203
#define ID_EDIT_DOMAIN    204
#define ID_BTN_ADD_DOMAIN 205
#define ID_BTN_REMOVE     207

#define ID_BTN_STARTSTOP  303

#define ID_TRAY_OPEN      401
#define ID_TRAY_TOGGLE    402
#define ID_TRAY_EXIT      403

#define ID_LANG_MENU_BASE 500

// Birebir VS Code / Antigravity Catppuccin Mocha Mat Renk Paleti (Sıfır Gradyan, Sıfır Parlama)
#define CTP_CRUST         RGB(17, 17, 27)    // #11111B — Input ve liste iç zemini
#define CTP_MANTLE        RGB(24, 24, 37)    // #181825 — Ana pencere zemini
#define CTP_BASE          RGB(30, 30, 46)    // #1E1E2E — Kart / panel zemini
#define CTP_SURFACE_BTN   RGB(36, 36, 54)    // #242436 — Pasif buton zemini
#define CTP_SURFACE0      RGB(49, 50, 68)    // #313244 — İnce 1px çerçeve kenarı
#define CTP_SURFACE1      RGB(69, 71, 90)    // #45475A — Seçili/hover çerçeve kenarı
#define CTP_OVERLAY0      RGB(108, 112, 134) // #6C7086 — İkincil soluk metin
#define CTP_SUBTEXT0      RGB(166, 173, 200) // #A6ADC8 — Alt açıklama metni
#define CTP_TEXT          RGB(205, 214, 244) // #CDD6F4 — Ana metin

#define CTP_MAUVE         RGB(203, 166, 247) // #CBA6F7 — Ana vurgu (Mauve)
#define CTP_MAUVE_BG      RGB(43, 37, 64)    // #2B2540 — Seçili kutu mat Mauve zemini
#define CTP_BLUE          RGB(137, 180, 250) // #89B4FA — Site etiketi
#define CTP_GREEN         RGB(166, 227, 161) // #A6E3A1 — Uygulama / Aktif etiketi
#define CTP_PEACH         RGB(250, 179, 135) // #FAB387 — Klasör etiketi
#define CTP_RED           RGB(243, 139, 168) // #F38BA8 — Durdur / Sil vurgu
#define CTP_RED_BG        RGB(54, 33, 46)    // #36212E — Durdur / Sil mat zemini

static HWND g_hMainWnd        = NULL;
static HWND g_hBtnLang        = NULL;
static HWND g_hScopeTabs[3]   = { NULL, NULL, NULL };
static HWND g_hStageBtns[6]   = { NULL, NULL, NULL, NULL, NULL, NULL };
static HWND g_hChkAutoConn    = NULL;
static HWND g_hChkWinStart    = NULL;
static HWND g_hChkMinStart    = NULL;

static HWND g_hBtnAddExe      = NULL;
static HWND g_hBtnAddFolder   = NULL;
static HWND g_hEditDomain     = NULL;
static HWND g_hBtnAddDomain   = NULL;
static HWND g_hBtnRemove      = NULL;
static HWND g_hBtnStartStop   = NULL;

static HFONT  g_hFontTitle    = NULL;
static HFONT  g_hFontSection  = NULL;
static HFONT  g_hFontStageNum = NULL;
static HFONT  g_hFontNormal   = NULL;
static HFONT  g_hFontBold     = NULL;
static HFONT  g_hFontSmall    = NULL;
static HFONT  g_hFontSmallB   = NULL;
static HFONT  g_hFontBtnBig   = NULL;

static HBRUSH g_hBrMantle     = NULL;
static HBRUSH g_hBrBase       = NULL;
static HBRUSH g_hBrCrust      = NULL;
static HICON  g_hIconBig      = NULL;
static HICON  g_hIconSmall    = NULL;
static int    g_dpi           = 96;

static ULONG_PTR g_gdiplusToken = 0;

static bool      g_chkStateAutoConn = false;
static bool      g_chkStateWinStart = false;
static bool      g_chkStateMinStart = false;
static AppLangId g_lang             = LANG_ID_TR;

struct FilteredTargetRow {
    int engineIndex;
    TunnelItemType type;
    std::wstring displayName;
    std::wstring pathOrDomain;
};
static std::vector<FilteredTargetRow> g_visibleRows;
static int  g_selectedRow   = -1;
static int  g_scrollOffsetY = 0;
static int  g_hoverRow      = -1;
static bool g_hoverRowDel   = false;

static ZeroDpiEngine g_engine;
static NOTIFYICONDATAW g_nid{};
static std::wstring g_iniPath;
static UINT g_uMsgTaskbarCreated = 0;

static LONG WINAPI YuziCrashSafeFilter(EXCEPTION_POINTERS*) {
    g_engine.EnableSystemSmartPac(false);
    if (g_nid.hWnd) {
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        g_nid.hWnd = NULL;
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

static const UiStrings& L() {
    return g_locales[(int)g_lang];
}

static int S(int px) {
    return MulDiv(px, g_dpi, 96);
}

static Color ToGdipColor(COLORREF c, BYTE alpha = 255) {
    return Color(alpha, GetRValue(c), GetGValue(c), GetBValue(c));
}

static RECT GetListViewportRect() {
    RECT rc = { S(32), S(356), S(608), S(606) };
    return rc;
}

static int GetRowHeightPx() {
    return S(34);
}

static void TrimWorkingSet() {
    SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
    EmptyWorkingSet(GetCurrentProcess());
}

static double GetCurrentRamMb() {
    PROCESS_MEMORY_COUNTERS pmc{};
    pmc.cb = sizeof(pmc);
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return (double)pmc.WorkingSetSize / (1024.0 * 1024.0);
    }
    return 1.2;
}

static std::wstring GetConfigIniPath() {
    wchar_t exePath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring path(exePath);
    size_t pos = path.find_last_of(L"\\/");
    if (pos != std::wstring::npos) path.resize(pos + 1);
    path += L"yuzidpi.ini";
    return path;
}

static void OnEngineLog(const wchar_t* msg) {
    if (!msg) return;
    if (g_hMainWnd && IsWindow(g_hMainWnd)) {
        PostMessageW(g_hMainWnd, WM_ENGINE_LOG, 0, 0);
    }
}

static void OnEngineState(BypassStage stage, bool running, int latencyMs) {
    if (g_hMainWnd && IsWindow(g_hMainWnd)) {
        WPARAM wp = ((WPARAM)stage & 0xFF) | (((WPARAM)(latencyMs & 0xFFFF)) << 8);
        PostMessageW(g_hMainWnd, WM_ENGINE_STATE, wp, (LPARAM)(running ? 1 : 0));
    }
}

static void BuildRoundRectPath(GraphicsPath& path, REAL x, REAL y, REAL w, REAL h, REAL rad) {
    path.Reset();
    if (rad <= 0.5f) {
        path.AddRectangle(RectF(x, y, w, h));
        return;
    }
    REAL d = rad * 2.0f;
    if (d > w) d = w;
    if (d > h) d = h;
    path.AddArc(x, y, d, d, 180.0f, 90.0f);
    path.AddArc(x + w - d, y, d, d, 270.0f, 90.0f);
    path.AddArc(x + w - d, y + h - d, d, d, 0.0f, 90.0f);
    path.AddArc(x, y + h - d, d, d, 90.0f, 90.0f);
    path.CloseFigure();
}

static void DrawFlatBox(Graphics& g, const RECT& rc, int radiusPx, COLORREF fillCol, COLORREF borderCol, REAL borderW = 1.0f) {
    RectF rf((REAL)rc.left + 0.5f, (REAL)rc.top + 0.5f,
             (REAL)(rc.right - rc.left) - 1.0f, (REAL)(rc.bottom - rc.top) - 1.0f);
    GraphicsPath path;
    BuildRoundRectPath(path, rf.X, rf.Y, rf.Width, rf.Height, (REAL)S(radiusPx));

    SolidBrush br(ToGdipColor(fillCol));
    g.FillPath(&br, &path);

    if (borderW > 0.0f) {
        Pen pen(ToGdipColor(borderCol), borderW);
        g.DrawPath(&pen, &path);
    }
}

static void DrawFlatBadge(HDC hdc, Graphics& g, const RECT& rc, int radiusPx,
                          const wchar_t* text, COLORREF fillCol, COLORREF borderCol, COLORREF textCol, HFONT font) {
    DrawFlatBox(g, rc, radiusPx, fillCol, borderCol, 1.0f);
    HGDIOBJ oldF = SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textCol);
    RECT rcTxt = rc;
    DrawTextW(hdc, text, -1, &rcTxt, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldF);
}

static void ClampListScroll() {
    RECT rcView = GetListViewportRect();
    int viewH = (rcView.bottom - rcView.top) - S(12);
    int contentH = (int)g_visibleRows.size() * GetRowHeightPx();
    int maxScroll = contentH - viewH;
    if (maxScroll < 0) maxScroll = 0;
    if (g_scrollOffsetY < 0) g_scrollOffsetY = 0;
    if (g_scrollOffsetY > maxScroll) g_scrollOffsetY = maxScroll;
}

static void RefreshTargetsListView() {
    TunnelScopeMode scope = g_engine.GetScopeMode();

    bool showListControls = (scope != SCOPE_FULL_SYSTEM);
    int cmdShow = showListControls ? SW_SHOW : SW_HIDE;
    if (g_hBtnAddExe) ShowWindow(g_hBtnAddExe, cmdShow);
    if (g_hBtnAddFolder) ShowWindow(g_hBtnAddFolder, cmdShow);
    if (g_hEditDomain) ShowWindow(g_hEditDomain, cmdShow);
    if (g_hBtnAddDomain) ShowWindow(g_hBtnAddDomain, cmdShow);
    if (g_hBtnRemove) ShowWindow(g_hBtnRemove, cmdShow);

    if (scope == SCOPE_ALL_EXCEPT_EXCLUDED) {
        if (g_hBtnAddDomain) SetWindowTextW(g_hBtnAddDomain, L().btnAddExcl);
        if (g_hEditDomain) SendMessageW(g_hEditDomain, EM_SETCUEBANNER, FALSE, (LPARAM)L().cueExclDomain);
    } else if (scope == SCOPE_ONLY_LISTED) {
        if (g_hBtnAddDomain) SetWindowTextW(g_hBtnAddDomain, L().btnAddSite);
        if (g_hEditDomain) SendMessageW(g_hEditDomain, EM_SETCUEBANNER, FALSE, (LPARAM)L().cueAddDomain);
    }

    g_visibleRows.clear();
    if (showListControls) {
        bool wantExcluded = (scope == SCOPE_ALL_EXCEPT_EXCLUDED);
        std::vector<TunnelTargetItem> items = g_engine.GetTunnelItems();
        for (int i = 0; i < (int)items.size(); ++i) {
            if (items[i].isExcluded != wantExcluded) continue;
            FilteredTargetRow row;
            row.engineIndex = i;
            row.type = items[i].type;
            row.displayName = items[i].displayName;
            row.pathOrDomain = items[i].pathOrDomain;
            g_visibleRows.push_back(row);
        }
    }

    if (g_selectedRow >= (int)g_visibleRows.size()) {
        g_selectedRow = (int)g_visibleRows.size() - 1;
    }
    ClampListScroll();
    if (g_hMainWnd) InvalidateRect(g_hMainWnd, NULL, FALSE);
}

static void SaveCurrentUiSettings() {
    g_engine.SetUseSystemPac(true);
    g_engine.SetAutoLaunchApps(true);
    ZeroDpiEngine::SetWindowsAutostart(g_chkStateWinStart, g_chkStateMinStart);
    g_engine.SaveConfig(g_iniPath, g_chkStateAutoConn, true, g_chkStateMinStart);
    WritePrivateProfileStringW(L"Settings", L"Language", L().code, g_iniPath.c_str());
}

static void ApplyLanguageToControls() {
    if (g_hBtnAddExe)    SetWindowTextW(g_hBtnAddExe, L().btnAddExe);
    if (g_hBtnAddFolder) SetWindowTextW(g_hBtnAddFolder, L().btnAddFolder);
    if (g_hBtnRemove)    SetWindowTextW(g_hBtnRemove, L().btnRemoveSel);
    if (g_hChkAutoConn)  SetWindowTextW(g_hChkAutoConn, L().chkAutoConn);
    if (g_hChkWinStart)  SetWindowTextW(g_hChkWinStart, L().chkWinStart);
    if (g_hChkMinStart)  SetWindowTextW(g_hChkMinStart, L().chkMinStart);

    RefreshTargetsListView();

    for (int i = 0; i < 3; ++i) if (g_hScopeTabs[i]) InvalidateRect(g_hScopeTabs[i], NULL, FALSE);
    for (int i = 0; i < 6; ++i) if (g_hStageBtns[i]) InvalidateRect(g_hStageBtns[i], NULL, FALSE);
    if (g_hBtnLang)      InvalidateRect(g_hBtnLang, NULL, FALSE);
    if (g_hBtnAddDomain) InvalidateRect(g_hBtnAddDomain, NULL, FALSE);
    if (g_hBtnAddExe)    InvalidateRect(g_hBtnAddExe, NULL, FALSE);
    if (g_hBtnAddFolder) InvalidateRect(g_hBtnAddFolder, NULL, FALSE);
    if (g_hBtnRemove)    InvalidateRect(g_hBtnRemove, NULL, FALSE);
    if (g_hChkAutoConn)  InvalidateRect(g_hChkAutoConn, NULL, FALSE);
    if (g_hChkWinStart)  InvalidateRect(g_hChkWinStart, NULL, FALSE);
    if (g_hChkMinStart)  InvalidateRect(g_hChkMinStart, NULL, FALSE);
    if (g_hBtnStartStop) InvalidateRect(g_hBtnStartStop, NULL, FALSE);
    if (g_hMainWnd)      InvalidateRect(g_hMainWnd, NULL, FALSE);
}

static void UpdateStatusUi() {
    TrimWorkingSet();
    if (g_hBtnStartStop) InvalidateRect(g_hBtnStartStop, NULL, FALSE);
    if (g_hMainWnd) InvalidateRect(g_hMainWnd, NULL, FALSE);
}

static void ToggleStartStop() {
    SaveCurrentUiSettings();
    if (g_engine.IsRunning()) {
        g_engine.Stop();
        UpdateStatusUi();
    } else {
        g_engine.Start(3131, 1080);
        UpdateStatusUi();
    }
}

static void BrowseAndAddExe(HWND hwnd) {
    wchar_t fileBuf[MAX_PATH] = {0};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"*.exe\0*.exe\0*.*\0*.*\0";
    ofn.lpstrFile = fileBuf;
    ofn.nMaxFile = MAX_PATH;
    bool isExclMode = (g_engine.GetScopeMode() == SCOPE_ALL_EXCEPT_EXCLUDED);
    ofn.lpstrTitle = isExclMode ? L().dlgPickExeExcl : L().dlgPickExeTunnel;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileNameW(&ofn) && wcslen(fileBuf) > 0) {
        g_engine.AddTunnelItem(ITEM_APP_EXE, fileBuf, isExclMode);
        RefreshTargetsListView();
        SaveCurrentUiSettings();
    }
}

static void BrowseAndAddFolder(HWND hwnd) {
    wchar_t displayName[MAX_PATH] = {0};
    BROWSEINFOW bi{};
    bi.hwndOwner = hwnd;
    bi.pszDisplayName = displayName;
    bool isExclMode = (g_engine.GetScopeMode() == SCOPE_ALL_EXCEPT_EXCLUDED);
    bi.lpszTitle = isExclMode ? L().dlgPickFolderExcl : L().dlgPickFolderTunnel;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_USENEWUI;

    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        wchar_t folderPath[MAX_PATH] = {0};
        if (SHGetPathFromIDListW(pidl, folderPath) && wcslen(folderPath) > 0) {
            g_engine.AddTunnelItem(ITEM_FOLDER, folderPath, isExclMode);
            RefreshTargetsListView();
            SaveCurrentUiSettings();
        }
        CoTaskMemFree(pidl);
    }
}

static void SetupTrayIcon(HWND hwnd) {
    if (g_nid.hWnd) return;
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = g_hIconSmall ? g_hIconSmall : LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(1));
    wcscpy_s(g_nid.szTip, L"YuziDPI");
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

static void ShowTrayMenu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_OPEN, L().trayOpen);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_TOGGLE, g_engine.IsRunning() ? L().trayStop : L().trayStart);
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L().trayExit);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
}

static void ShowLanguagePopupMenu(HWND hwnd) {
    RECT rcBtn{};
    GetWindowRect(g_hBtnLang, &rcBtn);
    HMENU hMenu = CreatePopupMenu();
    for (int i = 0; i < LANG_ID_COUNT; ++i) {
        UINT flags = MF_STRING | ((i == (int)g_lang) ? MF_CHECKED : 0);
        AppendMenuW(hMenu, flags, ID_LANG_MENU_BASE + i, g_locales[i].menuLabel);
    }
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN, rcBtn.left, rcBtn.bottom + S(2), 0, hwnd, NULL);
    DestroyMenu(hMenu);
}

// VS Code / Antigravity Tarzı Mat Catppuccin Mocha Kontrol Çizimi (6px Köşe, 1px İnce Kenar)
static void DrawNeumorphicControl(LPDRAWITEMSTRUCT dis) {
    HDC hdcOut = dis->hDC;
    RECT rc = dis->rcItem;
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0) return;

    int id = (int)dis->CtlID;
    bool pressed = (dis->itemState & ODS_SELECTED) != 0;

    HDC hdc = CreateCompatibleDC(hdcOut);
    HBITMAP hbm = CreateCompatibleBitmap(hdcOut, w, h);
    HGDIOBJ oldBm = SelectObject(hdc, hbm);

    // Üst bardaki dil seçici Mantle (#181825) üzerinde, diğer butonlar Base (#1E1E2E) kart üzerinde
    FillRect(hdc, &rc, (id == ID_BTN_LANG) ? g_hBrMantle : g_hBrBase);

    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    // 0) ÜST BAR DİL SEÇİCİ BUTONU (19 Dil)
    if (id == ID_BTN_LANG) {
        DrawFlatBox(g, rc, 5, pressed ? CTP_CRUST : CTP_BASE, CTP_SURFACE0, 1.0f);
        wchar_t langTxt[32];
        _snwprintf_s(langTxt, _countof(langTxt), _TRUNCATE, L"%s ▾", L().shortCode);
        HGDIOBJ oldF = SelectObject(hdc, g_hFontSmallB);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, CTP_SUBTEXT0);
        DrawTextW(hdc, langTxt, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, oldF);

        BitBlt(hdcOut, 0, 0, w, h, hdc, 0, 0, SRCCOPY);
        SelectObject(hdc, oldBm);
        DeleteObject(hbm);
        DeleteDC(hdc);
        return;
    }

    // A) ÇALIŞMA KAPSAMI — 3 Mat Kutu (6px Köşe)
    if (id >= ID_SCOPE_TAB_0 && id <= ID_SCOPE_TAB_2) {
        int tabIdx = id - ID_SCOPE_TAB_0;
        bool active = ((int)g_engine.GetScopeMode() == tabIdx);

        COLORREF fillCol   = active ? CTP_MAUVE_BG : (pressed ? CTP_CRUST : CTP_SURFACE_BTN);
        COLORREF borderCol = active ? CTP_MAUVE    : CTP_SURFACE0;
        DrawFlatBox(g, rc, 6, fillCol, borderCol, 1.0f);

        const wchar_t* mainTitles[3] = { L().scope0Title, L().scope1Title, L().scope2Title };
        const wchar_t* subTitles[3]  = { L().scope0Sub,   L().scope1Sub,   L().scope2Sub };

        RECT rcTop = { rc.left + S(12), rc.top + S(7), rc.right - S(8), rc.top + S(27) };
        RECT rcSub = { rc.left + S(12), rc.top + S(26), rc.right - S(8), rc.bottom - S(6) };

        SetBkMode(hdc, TRANSPARENT);
        HGDIOBJ oldFont = SelectObject(hdc, g_hFontBold);
        SetTextColor(hdc, active ? CTP_MAUVE : CTP_TEXT);
        DrawTextW(hdc, mainTitles[tabIdx], -1, &rcTop, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        SelectObject(hdc, g_hFontSmall);
        SetTextColor(hdc, active ? CTP_SUBTEXT0 : CTP_OVERLAY0);
        DrawTextW(hdc, subTitles[tabIdx], -1, &rcSub, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(hdc, oldFont);

        BitBlt(hdcOut, 0, 0, w, h, hdc, 0, 0, SRCCOPY);
        SelectObject(hdc, oldBm);
        DeleteObject(hbm);
        DeleteDC(hdc);
        return;
    }

    // B) DPI KADEMESİ — 6 Mat Kutu (1..6, 6px Köşe)
    if (id >= ID_STAGE_BTN_1 && id <= ID_STAGE_BTN_6) {
        int stNum = id - ID_STAGE_BTN_1 + 1;
        bool active = (g_engine.GetSelectedMode() == stNum);

        COLORREF fillCol   = active ? CTP_MAUVE_BG : (pressed ? CTP_CRUST : CTP_SURFACE_BTN);
        COLORREF borderCol = active ? CTP_MAUVE    : CTP_SURFACE0;
        DrawFlatBox(g, rc, 6, fillCol, borderCol, 1.0f);

        wchar_t numTxt[16];
        _snwprintf_s(numTxt, _countof(numTxt), _TRUNCATE, L"%d", stNum);

        RECT rcNum = { rc.left, rc.top + S(6), rc.right, rc.top + S(32) };
        RECT rcSub = { rc.left + S(2), rc.top + S(31), rc.right - S(2), rc.bottom - S(5) };

        SetBkMode(hdc, TRANSPARENT);
        HGDIOBJ oldFont = SelectObject(hdc, g_hFontStageNum);
        SetTextColor(hdc, active ? CTP_MAUVE : CTP_TEXT);
        DrawTextW(hdc, numTxt, -1, &rcNum, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, active ? g_hFontSmallB : g_hFontSmall);
        SetTextColor(hdc, active ? CTP_SUBTEXT0 : CTP_OVERLAY0);
        DrawTextW(hdc, L().stageLabels[stNum - 1], -1, &rcSub, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(hdc, oldFont);

        BitBlt(hdcOut, 0, 0, w, h, hdc, 0, 0, SRCCOPY);
        SelectObject(hdc, oldBm);
        DeleteObject(hbm);
        DeleteDC(hdc);
        return;
    }

    // C) ALT KOMPAKT MAT ANAHTARLAR (Oto Bağlan, Windows ile Aç, Gizli Başlat)
    if (id == ID_CHK_AUTOCONN || id == ID_CHK_WINSTART || id == ID_CHK_MINSTART) {
        bool checked = false;
        if (id == ID_CHK_AUTOCONN) checked = g_chkStateAutoConn;
        else if (id == ID_CHK_WINSTART) checked = g_chkStateWinStart;
        else if (id == ID_CHK_MINSTART) checked = g_chkStateMinStart;

        int swW = S(36);
        int swH = S(18);
        int swY = rc.top + (rc.bottom - rc.top - swH) / 2;
        RectF trackRf((REAL)(rc.left + S(2)), (REAL)swY, (REAL)swW, (REAL)swH);
        GraphicsPath trackPath;
        BuildRoundRectPath(trackPath, trackRf.X, trackRf.Y, trackRf.Width, trackRf.Height, trackRf.Height * 0.5f);

        SolidBrush trBr(ToGdipColor(checked ? CTP_MAUVE_BG : CTP_CRUST));
        g.FillPath(&trBr, &trackPath);
        Pen trPen(ToGdipColor(checked ? CTP_MAUVE : CTP_SURFACE0), 1.0f);
        g.DrawPath(&trPen, &trackPath);

        REAL pad = (REAL)S(3);
        REAL knobD = trackRf.Height - pad * 2.0f;
        REAL knobX = checked ? (trackRf.X + trackRf.Width - pad - knobD) : (trackRf.X + pad);
        REAL knobY = trackRf.Y + pad;

        SolidBrush knobBr(ToGdipColor(checked ? CTP_MAUVE : CTP_OVERLAY0));
        g.FillEllipse(&knobBr, knobX, knobY, knobD, knobD);

        wchar_t label[64] = {0};
        GetWindowTextW(dis->hwndItem, label, _countof(label));
        RECT rcTxt = { (int)(trackRf.X + trackRf.Width) + S(8), rc.top, rc.right, rc.bottom };
        HGDIOBJ oldF = SelectObject(hdc, g_hFontNormal);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, checked ? CTP_TEXT : CTP_SUBTEXT0);
        DrawTextW(hdc, label, -1, &rcTxt, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(hdc, oldF);

        BitBlt(hdcOut, 0, 0, w, h, hdc, 0, 0, SRCCOPY);
        SelectObject(hdc, oldBm);
        DeleteObject(hbm);
        DeleteDC(hdc);
        return;
    }

    // D) ANA BAŞLAT / DURDUR BUTONU (6px Köşe, Mat Yüzey)
    if (id == ID_BTN_STARTSTOP) {
        bool running = g_engine.IsRunning();
        COLORREF fillCol   = running ? CTP_RED_BG   : CTP_MAUVE_BG;
        COLORREF borderCol = running ? CTP_RED      : CTP_MAUVE;
        COLORREF textCol   = running ? CTP_RED      : CTP_MAUVE;
        if (pressed) fillCol = CTP_CRUST;

        DrawFlatBox(g, rc, 6, fillCol, borderCol, 1.0f);

        const wchar_t* mainTxt = running ? L().btnStop : L().btnStart;
        HGDIOBJ oldF = SelectObject(hdc, g_hFontBold);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, textCol);
        DrawTextW(hdc, mainTxt, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, oldF);

        BitBlt(hdcOut, 0, 0, w, h, hdc, 0, 0, SRCCOPY);
        SelectObject(hdc, oldBm);
        DeleteObject(hbm);
        DeleteDC(hdc);
        return;
    }

    // E) "+ Site Ekle" ve Alt Aksiyon Butonları (+ Uygulama, + Klasör Seç, Seçiliyi Kaldır)
    if (id == ID_BTN_ADD_DOMAIN) {
        DrawFlatBox(g, rc, 6, pressed ? CTP_CRUST : CTP_MAUVE_BG, CTP_MAUVE, 1.0f);
        wchar_t text[64] = {0};
        GetWindowTextW(dis->hwndItem, text, _countof(text));
        HGDIOBJ oldF = SelectObject(hdc, g_hFontBold);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, CTP_MAUVE);
        DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(hdc, oldF);

        BitBlt(hdcOut, 0, 0, w, h, hdc, 0, 0, SRCCOPY);
        SelectObject(hdc, oldBm);
        DeleteObject(hbm);
        DeleteDC(hdc);
        return;
    }

    bool isRemove = (id == ID_BTN_REMOVE);
    COLORREF fillCol   = isRemove ? (pressed ? CTP_CRUST : CTP_RED_BG) : (pressed ? CTP_CRUST : CTP_SURFACE_BTN);
    COLORREF borderCol = CTP_SURFACE0;
    DrawFlatBox(g, rc, 6, fillCol, borderCol, 1.0f);

    wchar_t text[64] = {0};
    GetWindowTextW(dis->hwndItem, text, _countof(text));
    HGDIOBJ oldFont = SelectObject(hdc, g_hFontNormal);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, isRemove ? CTP_RED : CTP_TEXT);
    DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    SelectObject(hdc, oldFont);

    BitBlt(hdcOut, 0, 0, w, h, hdc, 0, 0, SRCCOPY);
    SelectObject(hdc, oldBm);
    DeleteObject(hbm);
    DeleteDC(hdc);
}

static void PaintCustomGlassTargetList(HDC hdc, Graphics& g) {
    RECT rcView = GetListViewportRect();
    int pad = S(4);
    RECT rcClip = { rcView.left + pad, rcView.top + pad, rcView.right - pad, rcView.bottom - pad };

    if (g_visibleRows.empty()) {
        TunnelScopeMode scope = g_engine.GetScopeMode();
        HGDIOBJ oldF = SelectObject(hdc, g_hFontBold);
        SetTextColor(hdc, CTP_SUBTEXT0);
        RECT rcE1 = { rcView.left + S(20), rcView.top + S(82), rcView.right - S(20), rcView.top + S(108) };
        DrawTextW(hdc,
            (scope == SCOPE_ALL_EXCEPT_EXCLUDED)
                ? L().emptyExcludedTitle
                : L().emptyTunneledTitle,
            -1, &rcE1, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, g_hFontNormal);
        SetTextColor(hdc, CTP_OVERLAY0);
        RECT rcE2 = { rcView.left + S(40), rcView.top + S(112), rcView.right - S(40), rcView.top + S(162) };
        DrawTextW(hdc,
            (scope == SCOPE_ALL_EXCEPT_EXCLUDED)
                ? L().emptyExcludedSub
                : L().emptyTunneledSub,
            -1, &rcE2, DT_CENTER | DT_WORDBREAK);
        SelectObject(hdc, oldF);
        return;
    }

    int savedDC = SaveDC(hdc);
    IntersectClipRect(hdc, rcClip.left, rcClip.top, rcClip.right, rcClip.bottom);

    Region prevRegion;
    g.GetClip(&prevRegion);
    g.SetClip(Rect(rcClip.left, rcClip.top, rcClip.right - rcClip.left, rcClip.bottom - rcClip.top), CombineModeIntersect);

    int rowH = GetRowHeightPx();
    int viewH = rcClip.bottom - rcClip.top;
    int totalH = (int)g_visibleRows.size() * rowH;
    bool hasScrollbar = (totalH > viewH);
    int rightMargin = hasScrollbar ? S(10) : S(2);

    HGDIOBJ oldF = SelectObject(hdc, g_hFontNormal);

    for (int i = 0; i < (int)g_visibleRows.size(); ++i) {
        int topY = rcClip.top + i * rowH - g_scrollOffsetY;
        int botY = topY + rowH;
        if (botY <= rcClip.top || topY >= rcClip.bottom) continue;

        bool selected = (i == g_selectedRow);
        bool hovered  = (i == g_hoverRow);

        RECT rcRowBox = {
            rcClip.left + S(2),
            topY + S(1),
            rcClip.right - rightMargin - S(2),
            botY - S(1)
        };

        if (selected) {
            DrawFlatBox(g, rcRowBox, 5, RGB(36, 36, 54), CTP_SURFACE1, 1.0f);
        } else if (hovered) {
            DrawFlatBox(g, rcRowBox, 5, RGB(26, 26, 40), RGB(38, 39, 54), 1.0f);
        }

        const auto& item = g_visibleRows[i];
        const wchar_t* typeStr = L().tagSite;
        COLORREF badgeTxtCol = CTP_BLUE;
        COLORREF badgeBgCol  = RGB(25, 32, 48);
        COLORREF badgeBrdCol = RGB(42, 56, 84);

        if (item.type == ITEM_APP_EXE) {
            typeStr = L().tagExe;
            badgeTxtCol = CTP_GREEN;
            badgeBgCol  = RGB(24, 38, 34);
            badgeBrdCol = RGB(42, 68, 58);
        } else if (item.type == ITEM_FOLDER) {
            typeStr = L().tagFolder;
            badgeTxtCol = CTP_PEACH;
            badgeBgCol  = RGB(40, 31, 29);
            badgeBrdCol = RGB(72, 52, 44);
        }

        RECT rcBadge = { rcRowBox.left + S(8), topY + S(6), rcRowBox.left + S(68), botY - S(6) };
        DrawFlatBadge(hdc, g, rcBadge, 4, typeStr, badgeBgCol, badgeBrdCol, badgeTxtCol, g_hFontSmall);

        SelectObject(hdc, selected ? g_hFontBold : g_hFontNormal);
        SetTextColor(hdc, selected ? CTP_TEXT : CTP_SUBTEXT0);
        RECT rcName = { rcRowBox.left + S(78), topY, rcRowBox.left + S(248), botY };
        DrawTextW(hdc, item.displayName.c_str(), -1, &rcName, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        SelectObject(hdc, g_hFontSmall);
        SetTextColor(hdc, CTP_OVERLAY0);
        RECT rcPath = { rcRowBox.left + S(254), topY, rcRowBox.right - S(28), botY };
        DrawTextW(hdc, item.pathOrDomain.c_str(), -1, &rcPath, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_PATH_ELLIPSIS);

        bool delHot = (hovered && g_hoverRowDel);
        RECT rcDel = { rcRowBox.right - S(24), topY + S(5), rcRowBox.right - S(4), botY - S(5) };
        if (delHot) {
            DrawFlatBox(g, rcDel, 4, CTP_RED_BG, CTP_RED, 1.0f);
        }
        SelectObject(hdc, g_hFontNormal);
        SetTextColor(hdc, delHot ? CTP_RED : CTP_OVERLAY0);
        DrawTextW(hdc, L"×", -1, &rcDel, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    if (hasScrollbar) {
        REAL sbW = (REAL)S(4);
        REAL sbX = (REAL)(rcClip.right - S(6));
        REAL sbTop = (REAL)(rcClip.top + S(2));
        REAL sbTrackH = (REAL)(viewH - S(4));

        REAL thumbH = sbTrackH * ((REAL)viewH / (REAL)totalH);
        if (thumbH < (REAL)S(24)) thumbH = (REAL)S(24);
        int maxScroll = totalH - viewH;
        REAL thumbY = sbTop + ((maxScroll > 0) ? ((REAL)g_scrollOffsetY / (REAL)maxScroll) * (sbTrackH - thumbH) : 0.0f);

        GraphicsPath thPath;
        BuildRoundRectPath(thPath, sbX, thumbY, sbW, thumbH, 2.0f);
        SolidBrush thBr(ToGdipColor(CTP_SURFACE1));
        g.FillPath(&thBr, &thPath);
    }

    SelectObject(hdc, oldF);
    g.SetClip(&prevRegion, CombineModeReplace);
    RestoreDC(hdc, savedDC);
}

static void PaintNeumorphicDashboard(HWND hwnd, HDC hdcWin) {
    RECT rcClient;
    GetClientRect(hwnd, &rcClient);
    int w = rcClient.right - rcClient.left;
    int h = rcClient.bottom - rcClient.top;
    if (w <= 0 || h <= 0) return;

    TunnelScopeMode scope = g_engine.GetScopeMode();

    HDC hdc = CreateCompatibleDC(hdcWin);
    HBITMAP hbm = CreateCompatibleBitmap(hdcWin, w, h);
    HGDIOBJ oldBm = SelectObject(hdc, hbm);

    // 1. Ana Pencere Zemini: Catppuccin Mantle (#181825)
    FillRect(hdc, &rcClient, g_hBrMantle);
    SetBkMode(hdc, TRANSPARENT);

    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    // 2. Üst Başlık + Sürüm Etiketi + Canlı Durum Rozeti
    HGDIOBJ oldFont = SelectObject(hdc, g_hFontTitle);
    SetTextColor(hdc, CTP_TEXT);
    RECT rcTitle = { S(20), S(12), S(115), S(42) };
    DrawTextW(hdc, L"YuziDPI", -1, &rcTitle, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, g_hFontSmall);
    SetTextColor(hdc, CTP_OVERLAY0);
    RECT rcVer = { S(106), S(16), S(170), S(42) };
    DrawTextW(hdc, YUZIDPI_VERSION_WSTR, -1, &rcVer, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    bool running = g_engine.IsRunning();
    BypassStage curStage = g_engine.GetActiveStage();
    double ramMb = GetCurrentRamMb();

    wchar_t pillTxt[128];
    if (running) {
        _snwprintf_s(pillTxt, _countof(pillTxt), _TRUNCATE, L().statusActiveFmt, (int)curStage);
    } else {
        _snwprintf_s(pillTxt, _countof(pillTxt), _TRUNCATE, L().statusReadyFmt, ramMb);
    }

    RECT rcTopPill = { S(456), S(14), S(620), S(38) };
    if (running) {
        DrawFlatBadge(hdc, g, rcTopPill, 5, pillTxt,
            RGB(24, 38, 34), RGB(46, 78, 66), CTP_GREEN, g_hFontSmallB);
    } else {
        DrawFlatBadge(hdc, g, rcTopPill, 5, pillTxt,
            CTP_BASE, CTP_SURFACE0, CTP_OVERLAY0, g_hFontSmall);
    }

    // 3. KART 1: Çalışma Kapsamı & DPI Kademesi
    RECT rcCard1 = { S(18), S(48), S(622), S(238) };
    DrawFlatBox(g, rcCard1, 8, CTP_BASE, CTP_SURFACE0, 1.0f);

    SelectObject(hdc, g_hFontSmallB);
    SetTextColor(hdc, CTP_OVERLAY0);
    RECT rcSec1 = { S(32), S(60), S(320), S(78) };
    DrawTextW(hdc, L().secScope, -1, &rcSec1, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT rcSec2 = { S(32), S(144), S(300), S(162) };
    DrawTextW(hdc, L().secStage, -1, &rcSec2, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, g_hFontSmall);
    SetTextColor(hdc, CTP_OVERLAY0);
    RECT rcHint = { S(310), S(144), S(608), S(162) };
    DrawTextW(hdc, L().stageHint, -1, &rcHint, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

    // 4. KART 2: Tünellenen Hedefler Paneli
    RECT rcCard2 = { S(18), S(250), S(622), S(664) };
    DrawFlatBox(g, rcCard2, 8, CTP_BASE, CTP_SURFACE0, 1.0f);

    wchar_t card2Title[96];
    const wchar_t* badgeTxt = L().badgeOnlyListed;
    COLORREF badgeBg  = CTP_MAUVE_BG;
    COLORREF badgeBrd = CTP_SURFACE1;
    COLORREF badgeCol = CTP_MAUVE;

    if (scope == SCOPE_ALL_EXCEPT_EXCLUDED) {
        _snwprintf_s(card2Title, _countof(card2Title), _TRUNCATE, L().targetsExcludedFmt, (int)g_visibleRows.size());
        badgeTxt = L().badgeExcluded;
        badgeBg  = CTP_RED_BG;
        badgeBrd = RGB(88, 48, 66);
        badgeCol = CTP_RED;
    } else if (scope == SCOPE_FULL_SYSTEM) {
        wcscpy_s(card2Title, L().targetsFullTitle);
        badgeTxt = L().badgeFullActive;
        badgeBg  = RGB(24, 38, 34);
        badgeBrd = RGB(46, 78, 66);
        badgeCol = CTP_GREEN;
    } else {
        _snwprintf_s(card2Title, _countof(card2Title), _TRUNCATE, L().targetsTunneledFmt, (int)g_visibleRows.size());
    }

    SelectObject(hdc, g_hFontSmallB);
    SetTextColor(hdc, CTP_SUBTEXT0);
    RECT rcCT = { S(32), S(264), S(430), S(286) };
    DrawTextW(hdc, card2Title, -1, &rcCT, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT rcCB = { S(444), S(262), S(608), S(286) };
    DrawFlatBadge(hdc, g, rcCB, 4, badgeTxt, badgeBg, badgeBrd, badgeCol, g_hFontSmall);

    if (scope == SCOPE_FULL_SYSTEM) {
        RECT rcFullWell = { S(32), S(298), S(608), S(648) };
        DrawFlatBox(g, rcFullWell, 6, CTP_CRUST, CTP_SURFACE0, 1.0f);

        SelectObject(hdc, g_hFontBold);
        SetTextColor(hdc, CTP_TEXT);
        RECT rcFT1 = { S(56), S(434), S(584), S(462) };
        DrawTextW(hdc, L().fullSystemTitle, -1, &rcFT1, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, g_hFontNormal);
        SetTextColor(hdc, CTP_OVERLAY0);
        RECT rcFT2 = { S(84), S(468), S(556), S(528) };
        DrawTextW(hdc, L().fullSystemSub, -1, &rcFT2, DT_CENTER | DT_WORDBREAK);
    } else {
        RECT rcInputWell = { S(32), S(298), S(478), S(340) };
        DrawFlatBox(g, rcInputWell, 6, CTP_CRUST, CTP_SURFACE0, 1.0f);

        RECT rcListWell = GetListViewportRect();
        DrawFlatBox(g, rcListWell, 6, CTP_CRUST, CTP_SURFACE0, 1.0f);

        PaintCustomGlassTargetList(hdc, g);
    }

    // 5. KART 3: Alt Ayar Barı
    RECT rcCard3 = { S(18), S(676), S(622), S(742) };
    DrawFlatBox(g, rcCard3, 8, CTP_BASE, CTP_SURFACE0, 1.0f);

    SelectObject(hdc, oldFont);

    BitBlt(hdcWin, 0, 0, w, h, hdc, 0, 0, SRCCOPY);
    SelectObject(hdc, oldBm);
    DeleteObject(hbm);
    DeleteDC(hdc);
}

static void CreateUiControls(HWND hwnd) {
    HINSTANCE hInst = GetModuleHandleW(NULL);

    // 0. Üst Bar Dil Seçici Buton (19 Dil)
    g_hBtnLang = CreateWindowExW(0, L"BUTTON", L"TR",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        S(392), S(14), S(56), S(24),
        hwnd, (HMENU)ID_BTN_LANG, hInst, NULL);

    // 1. Çalışma Kapsamı — 3 Mat Kutu
    const wchar_t* scopeNames[3] = { L().scope0Title, L().scope1Title, L().scope2Title };
    int scW = S(186);
    int scGap = S(9);
    for (int i = 0; i < 3; ++i) {
        g_hScopeTabs[i] = CreateWindowExW(0, L"BUTTON", scopeNames[i],
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            S(32) + i * (scW + scGap), S(84), scW, S(48),
            hwnd, (HMENU)(INT_PTR)(ID_SCOPE_TAB_0 + i), hInst, NULL);
    }

    // 2. DPI Kademesi — 6 Mat Kutu (1..6)
    int stW = S(90);
    int stGap = S(7);
    for (int i = 0; i < 6; ++i) {
        wchar_t lbl[16];
        _snwprintf_s(lbl, _countof(lbl), _TRUNCATE, L"%d", i + 1);
        g_hStageBtns[i] = CreateWindowExW(0, L"BUTTON", lbl,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            S(32) + i * (stW + stGap), S(168), stW, S(54),
            hwnd, (HMENU)(INT_PTR)(ID_STAGE_BTN_1 + i), hInst, NULL);
    }

    // 3. Tünellenenler Kartı — Üst Satır: Alan Adı Girişi + "+ Site Ekle"
    g_hEditDomain = CreateWindowExW(0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        S(44), S(310), S(422), S(20), hwnd, (HMENU)ID_EDIT_DOMAIN, hInst, NULL);
    SendMessageW(g_hEditDomain, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    SendMessageW(g_hEditDomain, EM_SETCUEBANNER, FALSE, (LPARAM)L().cueAddDomain);

    g_hBtnAddDomain = CreateWindowExW(0, L"BUTTON", L().btnAddSite,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        S(486), S(298), S(122), S(42), hwnd, (HMENU)ID_BTN_ADD_DOMAIN, hInst, NULL);

    // 4. Tünellenenler Kartı — Alt Satır: 3 Mat Buton
    g_hBtnAddExe = CreateWindowExW(0, L"BUTTON", L().btnAddExe,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        S(32), S(616), S(186), S(34), hwnd, (HMENU)ID_BTN_ADD_EXE, hInst, NULL);

    g_hBtnAddFolder = CreateWindowExW(0, L"BUTTON", L().btnAddFolder,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        S(227), S(616), S(186), S(34), hwnd, (HMENU)ID_BTN_ADD_FOLDER, hInst, NULL);

    g_hBtnRemove = CreateWindowExW(0, L"BUTTON", L().btnRemoveSel,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        S(422), S(616), S(186), S(34), hwnd, (HMENU)ID_BTN_REMOVE, hInst, NULL);

    // 5. Alt Kart — 3 Anahtar + Ana Başlat / Durdur Butonu
    g_hChkAutoConn = CreateWindowExW(0, L"BUTTON", L().chkAutoConn,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        S(32), S(692), S(126), S(34), hwnd, (HMENU)ID_CHK_AUTOCONN, hInst, NULL);

    g_hChkWinStart = CreateWindowExW(0, L"BUTTON", L().chkWinStart,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        S(166), S(692), S(146), S(34), hwnd, (HMENU)ID_CHK_WINSTART, hInst, NULL);

    g_hChkMinStart = CreateWindowExW(0, L"BUTTON", L().chkMinStart,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        S(320), S(692), S(122), S(34), hwnd, (HMENU)ID_CHK_MINSTART, hInst, NULL);

    g_hBtnStartStop = CreateWindowExW(0, L"BUTTON", L().btnStart,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        S(468), S(689), S(140), S(40), hwnd, (HMENU)ID_BTN_STARTSTOP, hInst, NULL);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_hMainWnd = hwnd;
            CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
            CleanupOldOtaFiles();

            BOOL useDarkMode = TRUE;
            DwmSetWindowAttribute(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &useDarkMode, sizeof(useDarkMode));
            DwmSetWindowAttribute(hwnd, 19, &useDarkMode, sizeof(useDarkMode));

            COLORREF captionCol = CTP_MANTLE;
            DwmSetWindowAttribute(hwnd, 35 /* DWMWA_CAPTION_COLOR */, &captionCol, sizeof(captionCol));

            g_iniPath = GetConfigIniPath();

            bool autoConn = false;
            bool autoLaunch = true;
            bool startMin = false;
            g_engine.LoadConfig(g_iniPath, autoConn, autoLaunch, startMin);
            g_engine.SetUseSystemPac(true);
            g_engine.SetAutoLaunchApps(true);

            wchar_t langBuf[16] = {0};
            GetPrivateProfileStringW(L"Settings", L"Language", L"", langBuf, _countof(langBuf), g_iniPath.c_str());
            g_lang = ParseLangCode(langBuf);

            g_chkStateAutoConn = autoConn;
            g_chkStateMinStart = startMin;
            g_chkStateWinStart = ZeroDpiEngine::IsWindowsAutostartEnabled();

            CreateUiControls(hwnd);
            RefreshTargetsListView();
            g_engine.SetCallbacks(OnEngineLog, OnEngineState);

            if (autoConn) {
                ToggleStartStop();
            } else {
                g_engine.EnableSystemSmartPac(false);
            }

            // Sormadan Tam Otomatik (Sessiz) OTA Kontrolü — Yalnızca ilk açılışta 1 kez bakar
            HANDLE hOta = CreateThread(NULL, 0, SilentOtaWorkerThreadProc, hwnd, 0, NULL);
            if (hOta) CloseHandle(hOta);

            SetTimer(hwnd, 1, 800, NULL);
            TrimWorkingSet();
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            PaintNeumorphicDashboard(hwnd, hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_MOUSEWHEEL: {
            if (g_engine.GetScopeMode() != SCOPE_FULL_SYSTEM && !g_visibleRows.empty()) {
                int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
                g_scrollOffsetY -= (zDelta / WHEEL_DELTA) * GetRowHeightPx();
                ClampListScroll();
                RECT rcView = GetListViewportRect();
                InvalidateRect(hwnd, &rcView, FALSE);
            }
            return 0;
        }

        case WM_MOUSEMOVE: {
            if (g_engine.GetScopeMode() != SCOPE_FULL_SYSTEM && !g_visibleRows.empty()) {
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                RECT rcView = GetListViewportRect();
                int pad = S(4);
                RECT rcClip = { rcView.left + pad, rcView.top + pad, rcView.right - pad, rcView.bottom - pad };

                int newHover = -1;
                bool newHoverDel = false;
                if (PtInRect(&rcClip, pt)) {
                    int relY = pt.y - rcClip.top + g_scrollOffsetY;
                    int idx = relY / GetRowHeightPx();
                    if (idx >= 0 && idx < (int)g_visibleRows.size()) {
                        newHover = idx;
                        int viewH = rcClip.bottom - rcClip.top;
                        int totalH = (int)g_visibleRows.size() * GetRowHeightPx();
                        int rightMargin = (totalH > viewH) ? S(10) : S(2);
                        int rowRight = rcClip.right - rightMargin - S(2);
                        if (pt.x >= rowRight - S(26) && pt.x <= rowRight - S(2)) {
                            newHoverDel = true;
                        }
                    }
                }
                if (newHover != g_hoverRow || newHoverDel != g_hoverRowDel) {
                    g_hoverRow = newHover;
                    g_hoverRowDel = newHoverDel;
                    InvalidateRect(hwnd, &rcView, FALSE);
                }
            }
            return 0;
        }

        case WM_LBUTTONDOWN: {
            if (g_engine.GetScopeMode() != SCOPE_FULL_SYSTEM && !g_visibleRows.empty()) {
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                RECT rcView = GetListViewportRect();
                int pad = S(4);
                RECT rcClip = { rcView.left + pad, rcView.top + pad, rcView.right - pad, rcView.bottom - pad };

                if (PtInRect(&rcClip, pt)) {
                    int relY = pt.y - rcClip.top + g_scrollOffsetY;
                    int idx = relY / GetRowHeightPx();
                    if (idx >= 0 && idx < (int)g_visibleRows.size()) {
                        int viewH = rcClip.bottom - rcClip.top;
                        int totalH = (int)g_visibleRows.size() * GetRowHeightPx();
                        int rightMargin = (totalH > viewH) ? S(10) : S(2);
                        int rowRight = rcClip.right - rightMargin - S(2);

                        if (pt.x >= rowRight - S(26) && pt.x <= rowRight - S(2)) {
                            int engIdx = g_visibleRows[idx].engineIndex;
                            g_engine.RemoveTunnelItem(engIdx);
                            RefreshTargetsListView();
                            SaveCurrentUiSettings();
                        } else {
                            g_selectedRow = idx;
                            InvalidateRect(hwnd, &rcView, FALSE);
                        }
                    }
                }
            }
            return 0;
        }

        case WM_TIMER: {
            if (wParam == 1) {
                KillTimer(hwnd, 1);
                UpdateStatusUi();
            }
            return 0;
        }

        case WM_OTA_RESTART: {
            bool wasHidden = !IsWindowVisible(hwnd);
            g_engine.SetCallbacks(nullptr, nullptr);
            g_engine.Stop();
            g_engine.EnableSystemSmartPac(false);
            if (g_nid.hWnd) {
                Shell_NotifyIconW(NIM_DELETE, &g_nid);
                g_nid.hWnd = NULL;
            }
            ShowWindow(hwnd, SW_HIDE);
            std::wstring exePath = GetCurrentExeFullPath();
            wchar_t cmdLine[MAX_PATH + 32] = {0};
            if (wasHidden) {
                _snwprintf_s(cmdLine, _countof(cmdLine), _TRUNCATE, L"\"%s\" --minimized", exePath.c_str());
            } else {
                _snwprintf_s(cmdLine, _countof(cmdLine), _TRUNCATE, L"\"%s\"", exePath.c_str());
            }
            STARTUPINFOW si{};
            si.cb = sizeof(si);
            PROCESS_INFORMATION pi{};
            if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
            }
            DestroyWindow(hwnd);
            return 0;
        }

        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
            if (dis && dis->CtlType == ODT_BUTTON) {
                DrawNeumorphicControl(dis);
                return TRUE;
            }
            break;
        }

        case WM_CTLCOLOREDIT: {
            HDC hdcEdit = (HDC)wParam;
            SetTextColor(hdcEdit, CTP_TEXT);
            SetBkColor(hdcEdit, CTP_CRUST);
            return (LRESULT)g_hBrCrust;
        }

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN: {
            return (LRESULT)g_hBrBase;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);

            if (id == ID_BTN_LANG) {
                ShowLanguagePopupMenu(hwnd);
                return 0;
            }

            if (id >= ID_LANG_MENU_BASE && id < ID_LANG_MENU_BASE + LANG_ID_COUNT) {
                g_lang = (AppLangId)(id - ID_LANG_MENU_BASE);
                SaveCurrentUiSettings();
                ApplyLanguageToControls();
                return 0;
            }

            if (id >= ID_SCOPE_TAB_0 && id <= ID_SCOPE_TAB_2) {
                TunnelScopeMode newScope = (TunnelScopeMode)(id - ID_SCOPE_TAB_0);
                g_engine.SetScopeMode(newScope);
                g_scrollOffsetY = 0;
                g_selectedRow = -1;
                SaveCurrentUiSettings();
                for (int i = 0; i < 3; ++i) InvalidateRect(g_hScopeTabs[i], NULL, FALSE);
                InvalidateRect(g_hBtnAddDomain, NULL, FALSE);
                InvalidateRect(g_hBtnAddExe, NULL, FALSE);
                InvalidateRect(g_hBtnAddFolder, NULL, FALSE);
                InvalidateRect(g_hBtnRemove, NULL, FALSE);
                RefreshTargetsListView();
                UpdateStatusUi();
                return 0;
            }

            if (id >= ID_STAGE_BTN_1 && id <= ID_STAGE_BTN_6) {
                int newStage = id - ID_STAGE_BTN_1 + 1;
                g_engine.SetSelectedMode(newStage);
                SaveCurrentUiSettings();
                for (int i = 0; i < 6; ++i) InvalidateRect(g_hStageBtns[i], NULL, FALSE);
                UpdateStatusUi();
                return 0;
            }

            if (id == ID_CHK_AUTOCONN) {
                g_chkStateAutoConn = !g_chkStateAutoConn;
                InvalidateRect(g_hChkAutoConn, NULL, FALSE);
                SaveCurrentUiSettings();
                return 0;
            }
            if (id == ID_CHK_WINSTART) {
                g_chkStateWinStart = !g_chkStateWinStart;
                InvalidateRect(g_hChkWinStart, NULL, FALSE);
                SaveCurrentUiSettings();
                return 0;
            }
            if (id == ID_CHK_MINSTART) {
                g_chkStateMinStart = !g_chkStateMinStart;
                InvalidateRect(g_hChkMinStart, NULL, FALSE);
                SaveCurrentUiSettings();
                return 0;
            }

            if (id == ID_BTN_STARTSTOP) {
                ToggleStartStop();
            } else if (id == ID_BTN_ADD_EXE) {
                BrowseAndAddExe(hwnd);
            } else if (id == ID_BTN_ADD_FOLDER) {
                BrowseAndAddFolder(hwnd);
            } else if (id == ID_BTN_ADD_DOMAIN) {
                wchar_t domBuf[256] = {0};
                GetWindowTextW(g_hEditDomain, domBuf, _countof(domBuf));
                if (wcslen(domBuf) > 2) {
                    bool isExcl = (g_engine.GetScopeMode() == SCOPE_ALL_EXCEPT_EXCLUDED);
                    g_engine.AddTunnelItem(ITEM_DOMAIN, domBuf, isExcl);
                    SetWindowTextW(g_hEditDomain, L"");
                    RefreshTargetsListView();
                    SaveCurrentUiSettings();
                }
            } else if (id == ID_BTN_REMOVE) {
                if (g_selectedRow >= 0 && g_selectedRow < (int)g_visibleRows.size()) {
                    int engIdx = g_visibleRows[g_selectedRow].engineIndex;
                    g_engine.RemoveTunnelItem(engIdx);
                    RefreshTargetsListView();
                    SaveCurrentUiSettings();
                }
            } else if (id == ID_TRAY_OPEN) {
                ShowWindow(hwnd, SW_RESTORE);
                SetForegroundWindow(hwnd);
            } else if (id == ID_TRAY_TOGGLE) {
                ToggleStartStop();
            } else if (id == ID_TRAY_EXIT) {
                g_engine.Stop();
                g_engine.EnableSystemSmartPac(false);
                if (g_nid.hWnd) Shell_NotifyIconW(NIM_DELETE, &g_nid);
                DestroyWindow(hwnd);
            }
            return 0;
        }

        case WM_ENGINE_LOG:
        case WM_ENGINE_STATE: {
            UpdateStatusUi();
            return 0;
        }

        case WM_TRAYICON:
            if (lParam == WM_LBUTTONDBLCLK || lParam == WM_LBUTTONUP) {
                ShowWindow(hwnd, SW_RESTORE);
                SetForegroundWindow(hwnd);
            } else if (lParam == WM_RBUTTONUP) {
                ShowTrayMenu(hwnd);
            }
            return 0;

        case WM_SIZE:
            if (wParam == SIZE_MINIMIZED) {
                SetupTrayIcon(hwnd);
                ShowWindow(hwnd, SW_HIDE);
                TrimWorkingSet();
                return 0;
            }
            break;

        case WM_CLOSE:
            if (g_engine.IsRunning()) {
                SetupTrayIcon(hwnd);
                ShowWindow(hwnd, SW_HIDE);
                TrimWorkingSet();
                return 0;
            }
            DestroyWindow(hwnd);
            return 0;

        case WM_QUERYENDSESSION:
        case WM_ENDSESSION:
            g_engine.SetCallbacks(nullptr, nullptr);
            g_engine.Stop();
            g_engine.EnableSystemSmartPac(false);
            if (g_nid.hWnd) {
                Shell_NotifyIconW(NIM_DELETE, &g_nid);
                g_nid.hWnd = NULL;
            }
            return TRUE;

        case WM_DESTROY:
            g_hMainWnd = NULL;
            g_engine.SetCallbacks(nullptr, nullptr);
            g_engine.Stop();
            g_engine.EnableSystemSmartPac(false);
            if (g_nid.hWnd) {
                Shell_NotifyIconW(NIM_DELETE, &g_nid);
                g_nid.hWnd = NULL;
            }
            CoUninitialize();
            PostQuitMessage(0);
            return 0;
    }
    if (g_uMsgTaskbarCreated != 0 && msg == g_uMsgTaskbarCreated) {
        g_nid.hWnd = NULL;
        SetupTrayIcon(hwnd);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int) {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(YuziCrashSafeFilter);
    g_uMsgTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
    if (lpCmdLine && wcsstr(lpCmdLine, L"--verify-test") != nullptr) {
        int lat = 0;
        BypassStage st = g_engine.RunAutoDiscovery(lat);
        printf("Discovered Stage: %d, Latency: %d ms\n", (int)st, lat);
        return (st >= STAGE_1_FAST_TLS_REC && st <= STAGE_6_UPSTREAM_TUNNEL && lat > 0) ? 0 : 1;
    }

    // Eğer az önce OTA güncellemesiyle yeniden başlatıldıysa (.old dosyası varsa),
    // eski sürecin penceresini kapatmasını bekle ki tek-örnek (FindWindow) kontrolüne takılmasın
    {
        std::wstring oldFile = GetCurrentExeFullPath() + L".old";
        if (GetFileAttributesW(oldFile.c_str()) != INVALID_FILE_ATTRIBUTES) {
            for (int i = 0; i < 40; ++i) {
                if (!FindWindowW(L"YuziDpiDarkLiquidGlassWnd", NULL)) break;
                Sleep(50);
            }
            Sleep(100);
            DeleteFileW(oldFile.c_str());
        }
    }

    HWND hExisting = FindWindowW(L"YuziDpiDarkLiquidGlassWnd", NULL);
    if (hExisting) {
        if (!lpCmdLine || wcsstr(lpCmdLine, L"--minimized") == nullptr) {
            PostMessageW(hExisting, WM_COMMAND, ID_TRAY_OPEN, 0);
        }
        return 0;
    }

    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);

    INITCOMMONCONTROLSEX icex{};
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    HDC screenDc = GetDC(NULL);
    if (screenDc) {
        g_dpi = GetDeviceCaps(screenDc, LOGPIXELSX);
        ReleaseDC(NULL, screenDc);
    }

    g_hFontTitle = CreateFontW(-MulDiv(15, g_dpi, 72), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_hFontSection = CreateFontW(-MulDiv(9, g_dpi, 72), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_hFontStageNum = CreateFontW(-MulDiv(14, g_dpi, 72), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_hFontNormal = CreateFontW(-MulDiv(9, g_dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_hFontBold = CreateFontW(-MulDiv(9, g_dpi, 72), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_hFontSmall = CreateFontW(-MulDiv(8, g_dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_hFontSmallB = CreateFontW(-MulDiv(8, g_dpi, 72), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_hFontBtnBig = CreateFontW(-MulDiv(10, g_dpi, 72), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    g_hBrMantle = CreateSolidBrush(CTP_MANTLE);
    g_hBrBase   = CreateSolidBrush(CTP_BASE);
    g_hBrCrust  = CreateSolidBrush(CTP_CRUST);

    g_hIconBig = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON,
        GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
    g_hIconSmall = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hIcon = g_hIconBig;
    wc.hIconSm = g_hIconSmall;
    wc.hbrBackground = g_hBrMantle;
    wc.lpszClassName = L"YuziDpiDarkLiquidGlassWnd";
    RegisterClassExW(&wc);

    RECT rcWin = { 0, 0, S(640), S(756) };
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;
    AdjustWindowRectEx(&rcWin, style, FALSE, 0);

    int winW = rcWin.right - rcWin.left;
    int winH = rcWin.bottom - rcWin.top;
    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);

    g_hMainWnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"YuziDPI",
        style,
        (scrW - winW) / 2, (scrH - winH) / 2, winW, winH,
        NULL, NULL, hInstance, NULL
    );

    if (g_hIconBig) SendMessageW(g_hMainWnd, WM_SETICON, ICON_BIG, (LPARAM)g_hIconBig);
    if (g_hIconSmall) SendMessageW(g_hMainWnd, WM_SETICON, ICON_SMALL, (LPARAM)g_hIconSmall);

    bool startHidden = (lpCmdLine && wcsstr(lpCmdLine, L"--minimized") != nullptr);

    SetupTrayIcon(g_hMainWnd);
    if (startHidden) {
        ShowWindow(g_hMainWnd, SW_HIDE);
    } else {
        ShowWindow(g_hMainWnd, SW_SHOWNORMAL);
        UpdateWindow(g_hMainWnd);
    }

    if (lpCmdLine && wcsstr(lpCmdLine, L"--test-start") != nullptr) {
        ToggleStartStop();
    }
    TrimWorkingSet();

    MSG msg{};
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (!IsDialogMessageW(g_hMainWnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    DeleteObject(g_hFontTitle);
    DeleteObject(g_hFontSection);
    DeleteObject(g_hFontStageNum);
    DeleteObject(g_hFontNormal);
    DeleteObject(g_hFontBold);
    DeleteObject(g_hFontSmall);
    DeleteObject(g_hFontSmallB);
    DeleteObject(g_hFontBtnBig);
    DeleteObject(g_hBrMantle);
    DeleteObject(g_hBrBase);
    DeleteObject(g_hBrCrust);
    if (g_gdiplusToken) {
        GdiplusShutdown(g_gdiplusToken);
    }
    return (int)msg.wParam;
}
