// gui.cpp - Win32 GUI frontend for si-temperature library
// Cross-Unit Temperature Difference Calculator
// Compile with C++20: g++ gui.cpp -o temperature.exe -mwindows -std=c++20

#ifndef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef WINVER
#define WINVER 0x0601
#endif
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>
#include "temperature.hpp"

// ============================================================================
// Forward declarations
// ============================================================================
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK NumberEditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void Calculate(HWND hwnd);
void ShowStartupText(HWND hwnd);

// ============================================================================
// Constants
// ============================================================================
constexpr int CLIENT_W = 280;
constexpr int CLIENT_H = 395;
constexpr int MARGIN = 4;
constexpr int ROW_H = 24;
constexpr int UNIT_BTN_W = 60;
constexpr int ENTRY_W = CLIENT_W - MARGIN * 2 - MARGIN - UNIT_BTN_W;  // 204
constexpr int GAP = 4;

const wchar_t* CLASS_NAME = L"TemperatureCalculator";
const char UNIT_CHARS[3] = {'C', 'F', 'K'};
const wchar_t* UNIT_TEXTS[3] = {L"\u2103", L"\u2109", L"K"};

// ============================================================================
// Application state
// ============================================================================
struct AppData {
    HWND hEntry1, hEntry2;
    HWND hUnitBtn1, hUnitBtn2;
    HWND hCalcBtn;
    HWND hRadioZh, hRadioEn;
    HWND hCheckAscii;
    HWND hResult;

    int  unitIdx1;       // 0=C, 1=F, 2=K
    int  unitIdx2;
    bool calculated;     // whether user has pressed Calculate

    // Subclassing
    WNDPROC origEntryProc1;
    WNDPROC origEntryProc2;

    // Fonts
    HFONT hFontResult;   // Consolas 10
    HFONT hFontUI;       // default UI

    // For validation revert
    std::wstring lastValid1;
    std::wstring lastValid2;

    // Re-entrancy guard for EN_UPDATE
    bool updating1;
    bool updating2;
};

// Single-window global state
AppData g_app;

// ============================================================================
// Helpers
// ============================================================================

/** Return the button text for a unit, respecting ASCII mode. */
static std::wstring UnitBtnText(int unitIdx, bool asciiOnly)
{
    if (asciiOnly) {
        wchar_t buf[8];
        swprintf(buf, 8, L"_%c", UNIT_CHARS[unitIdx]);
        return buf;
    }
    return UNIT_TEXTS[unitIdx];
}

/** Return the degree symbol string for a unit in the given mode. */
static std::wstring Sym(int unitIdx, bool asciiOnly)
{
    if (asciiOnly) {
        wchar_t buf[8];
        swprintf(buf, 8, L"_%c", UNIT_CHARS[unitIdx]);
        return buf;
    }
    return UNIT_TEXTS[unitIdx];
}

/** Format a temperature line: "XX.X sym(tag)" */
static std::wstring FormatTempLine(double value, int unitIdx, bool asciiOnly, const wchar_t* tag)
{
    wchar_t buf[64];
    std::wstring sym = Sym(unitIdx, asciiOnly);
    swprintf(buf, 64, L"%.1f %s%s", value, sym.c_str(), tag);
    return buf;
}

/** Format a difference value: "XX.X sym" */
static std::wstring FormatDiffVal(double value, int unitIdx, bool asciiOnly)
{
    wchar_t buf[32];
    std::wstring sym = Sym(unitIdx, asciiOnly);
    swprintf(buf, 32, L"%.1f %s", value, sym.c_str());
    return buf;
}

/** Check if string is a valid number (digits, optional '.' and leading '-').
 *  Allows "-" alone as a transient state (user hasn't typed the digit yet). */
static bool IsValidNumberText(const std::wstring& s)
{
    if (s.empty() || s == L"-") return true;         // empty or pending minus
    size_t start = 0;
    if (s[0] == L'-') start = 1;
    if (start >= s.length()) return false;
    bool dotSeen = false;
    for (size_t i = start; i < s.length(); ++i) {
        if (s[i] == L'.') {
            if (dotSeen) return false;               // multiple dots
            dotSeen = true;
            if (i == start) return false;            // starts with "."
            if (i == s.length() - 1) return false;   // ends with "."
        } else if (s[i] < L'0' || s[i] > L'9') {
            return false;                            // invalid char
        }
    }
    return true;
}

// ============================================================================
// Window procedure
// ============================================================================
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        HINSTANCE hinst = ((LPCREATESTRUCT)lParam)->hInstance;
        ZeroMemory(&g_app, sizeof(g_app));

        // -- Create fonts --
        g_app.hFontUI = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        g_app.hFontResult = CreateFont(
            -MulDiv(10, GetDeviceCaps(GetDC(hwnd), LOGPIXELSY), 72),
            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Consolas");

        int y = MARGIN + 4;

        // Row 0: Entry1 + UnitBtn1
        g_app.hEntry1 = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
            MARGIN, y, ENTRY_W, ROW_H,
            hwnd, NULL, hinst, NULL);
        SendMessage(g_app.hEntry1, WM_SETFONT, (WPARAM)g_app.hFontUI, TRUE);

        // Row 0: Entry1 + UnitBtn1
        g_app.hUnitBtn1 = CreateWindow(L"BUTTON", L"\u2103",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            MARGIN + ENTRY_W + GAP, y, UNIT_BTN_W, ROW_H,
            hwnd, (HMENU)100, hinst, NULL);
        SendMessage(g_app.hUnitBtn1, WM_SETFONT, (WPARAM)g_app.hFontUI, TRUE);

        y += ROW_H + GAP;

        // Row 1: Entry2 + UnitBtn2
        g_app.hEntry2 = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
            MARGIN, y, ENTRY_W, ROW_H,
            hwnd, NULL, hinst, NULL);
        SendMessage(g_app.hEntry2, WM_SETFONT, (WPARAM)g_app.hFontUI, TRUE);

        g_app.hUnitBtn2 = CreateWindow(L"BUTTON", L"\u2103",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            MARGIN + ENTRY_W + GAP, y, UNIT_BTN_W, ROW_H,
            hwnd, (HMENU)200, hinst, NULL);
        SendMessage(g_app.hUnitBtn2, WM_SETFONT, (WPARAM)g_app.hFontUI, TRUE);

        y += ROW_H + GAP + 4;

        // Row 2: Calculate button
        g_app.hCalcBtn = CreateWindow(L"BUTTON", L"\u8fd0\u7b97",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            MARGIN, y, CLIENT_W - MARGIN * 2, ROW_H + 4,
            hwnd, (HMENU)300, hinst, NULL);
        SendMessage(g_app.hCalcBtn, WM_SETFONT, (WPARAM)g_app.hFontUI, TRUE);

        y += ROW_H + 4 + GAP;

        // Row 3: Radio/Check options
        g_app.hRadioZh = CreateWindow(L"BUTTON", L"\u4e2d\u6587",
            WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
            MARGIN, y, 50, ROW_H,
            hwnd, (HMENU)400, hinst, NULL);
        SendMessage(g_app.hRadioZh, WM_SETFONT, (WPARAM)g_app.hFontUI, TRUE);
        SendMessage(g_app.hRadioZh, BM_SETCHECK, BST_CHECKED, 0);

        g_app.hRadioEn = CreateWindow(L"BUTTON", L"EN",
            WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
            MARGIN + 54, y, 40, ROW_H,
            hwnd, (HMENU)401, hinst, NULL);
        SendMessage(g_app.hRadioEn, WM_SETFONT, (WPARAM)g_app.hFontUI, TRUE);

        g_app.hCheckAscii = CreateWindow(L"BUTTON", L"ASCII ONLY",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            MARGIN + 100, y, 90, ROW_H,
            hwnd, (HMENU)402, hinst, NULL);
        SendMessage(g_app.hCheckAscii, WM_SETFONT, (WPARAM)g_app.hFontUI, TRUE);

        y += ROW_H + GAP;

        // Separator (SS_ETCHEDHORIZ = 0x0010)
        CreateWindow(L"STATIC", L"",
            WS_CHILD | WS_VISIBLE | 0x0010,
            MARGIN, y, CLIENT_W - MARGIN * 2, 2,
            hwnd, NULL, hinst, NULL);

        y += 4;

        // Row 5: Result text area (fills remaining space)
        int resultH = CLIENT_H - y - MARGIN;
        if (resultH < 50) resultH = 80;
        g_app.hResult = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY |
            ES_LEFT | WS_VSCROLL,
            MARGIN, y, CLIENT_W - MARGIN * 2, resultH,
            hwnd, NULL, hinst, NULL);
        SendMessage(g_app.hResult, WM_SETFONT, (WPARAM)g_app.hFontResult, TRUE);

        // -- Subclass entry controls for number validation --
        g_app.origEntryProc1 = (WNDPROC)SetWindowLongPtr(g_app.hEntry1,
            GWLP_WNDPROC, (LONG_PTR)NumberEditProc);
        g_app.origEntryProc2 = (WNDPROC)SetWindowLongPtr(g_app.hEntry2,
            GWLP_WNDPROC, (LONG_PTR)NumberEditProc);

        // -- Initial state --
        g_app.unitIdx1 = 0;
        g_app.unitIdx2 = 0;
        g_app.calculated = false;
        g_app.updating1 = false;
        g_app.updating2 = false;

        // -- Show startup text --
        ShowStartupText(hwnd);

        // -- Set focus to first entry --
        SetFocus(g_app.hEntry1);

        return 0;
    }

    case WM_COMMAND:
    {
        WORD id = LOWORD(wParam);
        WORD code = HIWORD(wParam);

        // --- Unit button 1 ---
        if (id == 100 && code == BN_CLICKED) {
            g_app.unitIdx1 = (g_app.unitIdx1 + 1) % 3;
            bool ascii = (SendMessage(g_app.hCheckAscii, BM_GETCHECK, 0, 0) == BST_CHECKED);
            std::wstring txt = UnitBtnText(g_app.unitIdx1, ascii);
            SetWindowText(g_app.hUnitBtn1, txt.c_str());
            return 0;
        }
        // --- Unit button 2 ---
        if (id == 200 && code == BN_CLICKED) {
            g_app.unitIdx2 = (g_app.unitIdx2 + 1) % 3;
            bool ascii = (SendMessage(g_app.hCheckAscii, BM_GETCHECK, 0, 0) == BST_CHECKED);
            std::wstring txt = UnitBtnText(g_app.unitIdx2, ascii);
            SetWindowText(g_app.hUnitBtn2, txt.c_str());
            return 0;
        }
        // --- Calculate button ---
        if (id == 300 && code == BN_CLICKED) {
            Calculate(hwnd);
            return 0;
        }
        // --- Radio / Checkbox ---
        if ((id >= 400 && id <= 402) && code == BN_CLICKED) {
            // After changing lang/ascii, revert to startup text
            g_app.calculated = false;
            ShowStartupText(hwnd);
            // Update calc button text based on language
            bool isZh = (SendMessage(g_app.hRadioZh, BM_GETCHECK, 0, 0) == BST_CHECKED);
            SetWindowText(g_app.hCalcBtn, isZh ? L"\u8fd0\u7b97" : L"Calculate");
            // Update unit button text based on ASCII mode
            bool ascii = (SendMessage(g_app.hCheckAscii, BM_GETCHECK, 0, 0) == BST_CHECKED);
            std::wstring txt1 = UnitBtnText(g_app.unitIdx1, ascii);
            std::wstring txt2 = UnitBtnText(g_app.unitIdx2, ascii);
            SetWindowText(g_app.hUnitBtn1, txt1.c_str());
            SetWindowText(g_app.hUnitBtn2, txt2.c_str());
            return 0;
        }
        // --- EN_UPDATE on entry controls (validate on change) ---
        if (code == EN_UPDATE) {
            HWND hEntry = (HWND)lParam;
            if (hEntry == g_app.hEntry1 || hEntry == g_app.hEntry2) {
                bool is1 = (hEntry == g_app.hEntry1);
                bool& guard = is1 ? g_app.updating1 : g_app.updating2;
                if (guard) return 0;  // avoid re-entrancy

                int len = GetWindowTextLengthW(hEntry);
                std::wstring text(len + 1, L'\0');
                GetWindowTextW(hEntry, &text[0], len + 1);
                text.resize(len);

                std::wstring& lastValid = is1 ? g_app.lastValid1 : g_app.lastValid2;
                if (!IsValidNumberText(text)) {
                    // Revert to last valid
                    guard = true;
                    SetWindowTextW(hEntry, lastValid.c_str());
                    guard = false;
                } else {
                    lastValid = text;
                }
                return 0;
            }
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    case WM_CTLCOLORSTATIC:
    {
        HWND hCtrl = (HWND)lParam;
        if (hCtrl == g_app.hResult) {
            SetBkColor((HDC)wParam, RGB(255, 255, 255));
            return (LRESULT)GetStockObject(WHITE_BRUSH);
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    case WM_DESTROY:
    {
        if (g_app.hFontResult) DeleteObject(g_app.hFontResult);
        PostQuitMessage(0);
        return 0;
    }

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

// ============================================================================
// Subclassed edit proc — number-only input
// ============================================================================
LRESULT CALLBACK NumberEditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Determine which original proc to use
    WNDPROC orig = (hwnd == g_app.hEntry1) ? g_app.origEntryProc1 : g_app.origEntryProc2;

    if (uMsg == WM_CHAR) {
        // Allow control keys
        if (wParam == VK_BACK || wParam == VK_RETURN) {
            if (wParam == VK_RETURN) {
                // Move focus: entry1 -> entry2 -> calculate
                if (hwnd == g_app.hEntry1) {
                    SetFocus(g_app.hEntry2);
                } else {
                    // Pressing Enter on entry2 triggers calculation
                    HWND hParent = GetParent(hwnd);
                    if (hParent) Calculate(hParent);
                }
                return 0;
            }
            return CallWindowProc(orig, hwnd, uMsg, wParam, lParam);
        }
        // Only digits, '.', '-'
        if (wParam == L'.' || wParam == L'-' || (wParam >= L'0' && wParam <= L'9')) {
            return CallWindowProc(orig, hwnd, uMsg, wParam, lParam);
        }
        return 0;  // block
    }

    return CallWindowProc(orig, hwnd, uMsg, wParam, lParam);
}

// ============================================================================
// Calculate
// ============================================================================
void Calculate(HWND hwnd)
{
    // Read values
    int len1 = GetWindowTextLengthW(g_app.hEntry1);
    int len2 = GetWindowTextLengthW(g_app.hEntry2);
    if (len1 == 0 || len2 == 0) {
        SetWindowTextW(g_app.hResult, L"Please enter both temperatures");
        return;
    }

    std::wstring s1(len1 + 1, L'\0');
    std::wstring s2(len2 + 1, L'\0');
    GetWindowTextW(g_app.hEntry1, &s1[0], len1 + 1);
    GetWindowTextW(g_app.hEntry2, &s2[0], len2 + 1);
    s1.resize(len1);
    s2.resize(len2);

    // Parse
    wchar_t* end1 = nullptr;
    wchar_t* end2 = nullptr;
    double v1 = wcstod(s1.c_str(), &end1);
    double v2 = wcstod(s2.c_str(), &end2);
    if (*end1 != L'\0' || *end2 != L'\0') {
        SetWindowTextW(g_app.hResult, L"Invalid number");
        return;
    }

    // Build Temperature objects
    Temperature t1, t2;
    switch (g_app.unitIdx1) {
        case 0: t1 = Temperature::fromCelsius(v1);     break;
        case 1: t1 = Temperature::fromFahrenheit(v1);  break;
        case 2: t1 = Temperature::fromKelvin(v1);      break;
    }
    switch (g_app.unitIdx2) {
        case 0: t2 = Temperature::fromCelsius(v2);     break;
        case 1: t2 = Temperature::fromFahrenheit(v2);  break;
        case 2: t2 = Temperature::fromKelvin(v2);      break;
    }

    double c1 = t1.toCelsius();
    double c2 = t2.toCelsius();

    bool isZh = (SendMessage(g_app.hRadioZh, BM_GETCHECK, 0, 0) == BST_CHECKED);
    bool ascii = (SendMessage(g_app.hCheckAscii, BM_GETCHECK, 0, 0) == BST_CHECKED);

    const double eps = 1e-10;
    std::wstring tag1, tag2;
    if (std::abs(c1 - c2) < eps) {
        tag1 = L"";
        tag2 = L"";
    } else if (c1 > c2) {
        tag1 = isZh ? L"\uff08\u66f4\u5927\uff09" : L" (Bigger)";
        tag2 = L"";
    } else {
        tag1 = L"";
        tag2 = isZh ? L"\uff08\u66f4\u5927\uff09" : L" (Bigger)";
    }

    std::wstring line1 = FormatTempLine(v1, g_app.unitIdx1, ascii, tag1.c_str());
    std::wstring line2 = FormatTempLine(v2, g_app.unitIdx2, ascii, tag2.c_str());

    // Difference
    double diffC = std::abs(c1 - c2);
    double diffF = diffC * 9.0 / 5.0;
    double diffK = diffC;

    std::wstring diffLabel;
    if (isZh) {
        diffLabel = L"\u5dee\u503c\uff1a";
    } else if (ascii) {
        diffLabel = L"Diff:";
    } else {
        diffLabel = L"Difference:";
    }

    std::wstring diffLine =
        FormatDiffVal(diffC, 0, ascii) + L" / " +
        FormatDiffVal(diffF, 1, ascii) + L" / " +
        FormatDiffVal(diffK, 2, ascii);

    std::wstring output = line1 + L"\r\n" + line2 + L"\r\n\r\n" +
                          diffLabel + L"\r\n" + diffLine;

    SetWindowTextW(g_app.hResult, output.c_str());
    g_app.calculated = true;
}

// ============================================================================
// Startup text
// ============================================================================
void ShowStartupText(HWND hwnd)
{
    bool isZh = (SendMessage(g_app.hRadioZh, BM_GETCHECK, 0, 0) == BST_CHECKED);
    bool ascii = (SendMessage(g_app.hCheckAscii, BM_GETCHECK, 0, 0) == BST_CHECKED);

    std::wstring text;
    if (isZh) {
        text = L"\u8de8\u5355\u4f4d\u6e29\u5dee\u8ba1\u7b97\u5668 demo\r\n\r\n"
               L"\u5728\u4e0a\u65b9\u8f93\u5165\u4e24\u4e2a\u6e29\u5ea6\uff0c\r\n"
               L"\u6309\u4e0b\u6309\u94ae\u5207\u6362\u5355\u4f4d\u3002\r\n\r\n"
               L"\u6309\u4e0b\u8fd0\u7b97\u540e\uff0c\r\n"
               L"\u5c06\u5728\u6b64\u5c55\u793a\u54ea\u4e2a\u66f4\u5927\uff0c\r\n"
               L"\u4ee5\u53ca\u4e09\u4e2a\u5355\u4f4d\u7684\u6e29\u5dee\u3002";
    } else {
        text = L"Cross-Unit Temperature Difference Demo\r\n\r\n"
               L"Enter two temperatures above.\r\n"
               L"Click the unit button to switch.\r\n\r\n"
               L"Press Calculate to see\r\n"
               L"which one is bigger and the\r\n"
               L"difference in all three units.";
    }

    SetWindowTextW(g_app.hResult, text.c_str());
}

// ============================================================================
// Entry point
// ============================================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    // Register window class
    WNDCLASSEX wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;
    if (!RegisterClassEx(&wc)) return 1;

    // Calculate window rect with title bar and borders
    RECT rect = {0, 0, CLIENT_W, CLIENT_H};
    DWORD winStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&rect, winStyle, FALSE);

    int winW = rect.right - rect.left;
    int winH = rect.bottom - rect.top;

    // Center on screen
    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);
    int x = std::max(0, (scrW - winW) / 2);
    int y = std::max(0, (scrH - winH) / 2);

    // Create window
    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"\u8de8\u5355\u4f4d\u6e29\u5dee\u8ba1\u7b97\u5668",
                               winStyle, x, y, winW, winH,
                               NULL, NULL, hInstance, NULL);
    if (!hwnd) return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
