/* ============================================================
   gui.c  --  MODULE 4: Native Win32 GUI (login + dashboards)

   Build (MinGW64, from VS Code terminal):
     gcc gui.c student.c fileio.c auth.c -o StudentRecordSystem.exe ^
         -lcomctl32 -mwindows

   (see build.bat / tasks.json included in this project)
   ============================================================ */
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

/* ---------------- control IDs ---------------- */
#define IDC_RADIO_TEACHER   101
#define IDC_RADIO_STUDENT   102
#define IDC_EDIT_USER       103
#define IDC_EDIT_PASS       104
#define IDC_BTN_LOGIN       105
#define IDC_STATIC_STATUS   106
#define IDC_STATIC_USERLBL  107
#define IDC_STATIC_PASSLBL  108

#define IDC_LISTVIEW        201
#define IDC_BTN_ADD         202
#define IDC_BTN_SEARCH      203
#define IDC_BTN_SORT        204
#define IDC_BTN_SAVE        205
#define IDC_BTN_LOGOUT      206
#define IDC_STATIC_WELCOME  207
#define IDC_BTN_DELETE      208
#define IDC_BTN_EXPORT      209

#define IDC_ADD_ROLL        301
#define IDC_ADD_NAME        302
#define IDC_ADD_M1          303
#define IDC_ADD_M2          304
#define IDC_ADD_M3          305
#define IDC_ADD_OK          306
#define IDC_ADD_CANCEL      307
#define IDC_ADD_STATUS      308

#define IDC_SEARCH_ROLL     401
#define IDC_SEARCH_GO       402
#define IDC_SEARCH_CLOSE    403
#define IDC_SEARCH_RESULT   404

/* ---------------- colour palette ---------------- */
#define COLOR_ACCENT       RGB(31,78,121)     /* deep blue  - header / primary buttons */
#define COLOR_ACCENT_DK    RGB(20,55,90)
#define COLOR_BG           RGB(240,244,248)   /* light body background */
#define COLOR_SUCCESS      RGB(46,125,50)     /* green - Save / Add-confirm */
#define COLOR_SUCCESS_DK   RGB(30,90,35)
#define COLOR_DANGER       RGB(178,34,34)     /* red - Delete */
#define COLOR_DANGER_DK    RGB(130,20,20)
#define COLOR_NEUTRAL      RGB(96,106,116)    /* gray - Logout / Cancel / Close */
#define COLOR_NEUTRAL_DK   RGB(70,78,86)
#define COLOR_WHITE        RGB(255,255,255)
#define HEADER_H            60

/* ---------------- application state ----------------
   (file-scope globals here are normal/expected for a Win32 GUI
   app's window-procedure state; the assignment's "no globals"
   rule applies specifically to calculateTotalAverage() in
   student.c, which is fully pointer-based.) */
static Student g_students[MAX_STUDENTS];
static int     g_count       = 0;
static int     g_role        = 0;      /* 1 = teacher, 2 = student */
static int     g_studentIdx  = -1;     /* record index for student role */
static HINSTANCE g_hInst;
static HWND    g_hLogin, g_hMain;
static BOOL    g_quitting    = FALSE;
static HBRUSH  g_hBgBrush;
static HICON   g_hAppIcon;

/* forward declarations */
LRESULT CALLBACK LoginWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK AddWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SearchWndProc(HWND, UINT, WPARAM, LPARAM);
void CreateMainDashboard(void);
void RefreshListView(HWND hList);
void ShowLoginAgain(void);
void ExportToCSV(HWND hwnd);
static void DrawFlatButton(LPDRAWITEMSTRUCT dis, COLORREF bgNormal, COLORREF bgPressed, COLORREF textColor);
static void DrawLogoEmblem(HDC hdc, int x, int y, int size);
static void PaintHeader(HWND hwnd, const char *title);

/* ===================================================================
   WinMain -- entry point
   =================================================================== */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmd, int nCmdShow)
{
    WNDCLASSEXA wc;
    INITCOMMONCONTROLSEX icc;
    MSG msg;

    (void)hPrev; (void)lpCmd;
    g_hInst = hInstance;

    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icc);

    /* Load persisted data at startup. A missing file on first run
       is handled gracefully inside loadStudentsFromFile(). */
    loadStudentsFromFile(g_students, &g_count, DATA_FILE);

    /* app icon + shared background brush, used by every window class */
    g_hAppIcon = (HICON)LoadImageA(NULL, "app.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
    g_hBgBrush = CreateSolidBrush(COLOR_BG);

    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize        = sizeof(wc);
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = g_hBgBrush;
    wc.hIcon         = g_hAppIcon ? g_hAppIcon : LoadIcon(NULL, IDI_APPLICATION);

    wc.lpfnWndProc   = LoginWndProc;
    wc.lpszClassName = "LoginWndClass";
    RegisterClassExA(&wc);

    wc.lpfnWndProc   = MainWndProc;
    wc.lpszClassName = "MainWndClass";
    RegisterClassExA(&wc);

    wc.lpfnWndProc   = AddWndProc;
    wc.lpszClassName = "AddWndClass";
    RegisterClassExA(&wc);

    wc.lpfnWndProc   = SearchWndProc;
    wc.lpszClassName = "SearchWndClass";
    RegisterClassExA(&wc);

    g_hLogin = CreateWindowExA(0, "LoginWndClass",
        "Student Record System - Login",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 360,
        NULL, NULL, hInstance, NULL);

    ShowWindow(g_hLogin, nCmdShow);
    UpdateWindow(g_hLogin);

    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

/* ===================================================================
   Shared drawing helpers
   =================================================================== */

/* Flat, coloured push-button drawn manually (BS_OWNERDRAW). */
static void DrawFlatButton(LPDRAWITEMSTRUCT dis, COLORREF bgNormal, COLORREF bgPressed, COLORREF textColor)
{
    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;
    char text[64];
    HBRUSH hBrush;
    HFONT hFont, hOldFont;
    COLORREF bg = (dis->itemState & ODS_SELECTED) ? bgPressed : bgNormal;

    GetWindowTextA(dis->hwndItem, text, sizeof(text));

    hBrush = CreateSolidBrush(bg);
    FillRect(hdc, &rc, hBrush);
    DeleteObject(hBrush);

    hFont = CreateFontA(16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
    hOldFont = (HFONT)SelectObject(hdc, hFont);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textColor);
    DrawTextA(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);

    if (dis->itemState & ODS_FOCUS) {
        RECT focusRc = rc;
        InflateRect(&focusRc, -3, -3);
        DrawFocusRect(hdc, &focusRc);
    }
}

/* Small graduation-cap emblem, drawn purely with GDI (no image file
   needed) -- used as the app "logo" in the header banner. */
static void DrawLogoEmblem(HDC hdc, int x, int y, int size)
{
    POINT cap[4];
    HBRUSH hWhite = (HBRUSH)GetStockObject(WHITE_BRUSH);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hWhite);
    HPEN hPen = CreatePen(PS_SOLID, 1, COLOR_WHITE);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    cap[0].x = x + size / 2; cap[0].y = y;
    cap[1].x = x + size;     cap[1].y = y + size / 3;
    cap[2].x = x + size / 2; cap[2].y = y + (2 * size) / 3;
    cap[3].x = x;            cap[3].y = y + size / 3;
    Polygon(hdc, cap, 4);

    Rectangle(hdc, x + size / 3, y + size / 3 + 2, x + (2 * size) / 3, y + size / 2 + 4);

    MoveToEx(hdc, x + size / 2, y + size / 3, NULL);
    LineTo(hdc, x + size / 2 + size / 6, y + size);
    Ellipse(hdc, x + size / 2 + size / 6 - 2, y + size - 4, x + size / 2 + size / 6 + 2, y + size);

    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

/* Fills the top HEADER_H strip of a window with the accent colour,
   draws the logo emblem and a bold title -- used by both the login
   window and the main dashboard. */
static void PaintHeader(HWND hwnd, const char *title)
{
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rc, header;
    HBRUSH hBrush;
    HFONT hFont, hOldFont;

    hdc = BeginPaint(hwnd, &ps);
    GetClientRect(hwnd, &rc);
    header = rc;
    header.bottom = HEADER_H;

    hBrush = CreateSolidBrush(COLOR_ACCENT);
    FillRect(hdc, &header, hBrush);
    DeleteObject(hBrush);

    DrawLogoEmblem(hdc, 14, 11, 36);

    hFont = CreateFontA(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
    hOldFont = (HFONT)SelectObject(hdc, hFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, COLOR_WHITE);
    TextOutA(hdc, 62, 18, title, (int)strlen(title));
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);

    EndPaint(hwnd, &ps);
}

/* ===================================================================
   LOGIN WINDOW
   =================================================================== */
LRESULT CALLBACK LoginWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND hRadioTeacher, hRadioStudent, hEditUser, hEditPass;
    static HWND hStatus;
    int top = HEADER_H + 15;

    switch (msg) {
    case WM_CREATE:
        if (g_hAppIcon) {
            SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)g_hAppIcon);
            SendMessageA(hwnd, WM_SETICON, ICON_BIG,   (LPARAM)g_hAppIcon);
        }

        CreateWindowA("STATIC", "Login as:", WS_CHILD | WS_VISIBLE,
            20, top, 100, 20, hwnd, NULL, g_hInst, NULL);

        hRadioTeacher = CreateWindowA("BUTTON", "Teacher",
            WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
            120, top, 100, 20, hwnd, (HMENU)IDC_RADIO_TEACHER, g_hInst, NULL);
        hRadioStudent = CreateWindowA("BUTTON", "Student",
            WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
            230, top, 100, 20, hwnd, (HMENU)IDC_RADIO_STUDENT, g_hInst, NULL);
        SendMessageA(hRadioTeacher, BM_SETCHECK, BST_CHECKED, 0);

        CreateWindowA("STATIC", "Username / Roll No:", WS_CHILD | WS_VISIBLE,
            20, top + 40, 180, 20, hwnd, (HMENU)IDC_STATIC_USERLBL, g_hInst, NULL);
        hEditUser = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER,
            20, top + 63, 340, 26, hwnd, (HMENU)IDC_EDIT_USER, g_hInst, NULL);

        CreateWindowA("STATIC", "Password (teacher only):", WS_CHILD | WS_VISIBLE,
            20, top + 97, 200, 20, hwnd, (HMENU)IDC_STATIC_PASSLBL, g_hInst, NULL);
        hEditPass = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD,
            20, top + 120, 340, 26, hwnd, (HMENU)IDC_EDIT_PASS, g_hInst, NULL);

        CreateWindowA("BUTTON", "Login", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            140, top + 158, 100, 34, hwnd, (HMENU)IDC_BTN_LOGIN, g_hInst, NULL);

        hStatus = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE,
            20, top + 200, 350, 20, hwnd, (HMENU)IDC_STATIC_STATUS, g_hInst, NULL);
        return 0;

    case WM_PAINT:
        PaintHeader(hwnd, "Student Record System");
        return 0;

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        SetTextColor(hdcStatic, RGB(30, 30, 30));
        return (LRESULT)g_hBgBrush;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        if (dis->CtlID == IDC_BTN_LOGIN) {
            DrawFlatButton(dis, COLOR_ACCENT, COLOR_ACCENT_DK, COLOR_WHITE);
        }
        return TRUE;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BTN_LOGIN) {
            char userBuf[NAME_LEN], passBuf[NAME_LEN];
            BOOL isTeacher = (SendMessageA(hRadioTeacher, BM_GETCHECK, 0, 0) == BST_CHECKED);

            GetWindowTextA(hEditUser, userBuf, NAME_LEN);
            GetWindowTextA(hEditPass, passBuf, NAME_LEN);

            if (isTeacher) {
                if (teacherLogin(userBuf, passBuf)) {
                    g_role = 1;
                    g_studentIdx = -1;
                    ShowWindow(hwnd, SW_HIDE);
                    CreateMainDashboard();
                } else {
                    SetWindowTextA(hStatus, "Invalid teacher username or password.");
                }
            } else {
                int roll = atoi(userBuf);
                int idx;
                if (userBuf[0] == '\0') {
                    SetWindowTextA(hStatus, "Please enter your roll number.");
                } else if (studentLogin(g_students, g_count, roll, &idx)) {
                    g_role = 2;
                    g_studentIdx = idx;
                    ShowWindow(hwnd, SW_HIDE);
                    CreateMainDashboard();
                } else {
                    SetWindowTextA(hStatus, "Roll number not found.");
                }
            }
            SetWindowTextA(hEditPass, "");
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void ShowLoginAgain(void)
{
    HWND hEditUser = GetDlgItem(g_hLogin, IDC_EDIT_USER);
    HWND hEditPass = GetDlgItem(g_hLogin, IDC_EDIT_PASS);
    HWND hStatus   = GetDlgItem(g_hLogin, IDC_STATIC_STATUS);
    SetWindowTextA(hEditUser, "");
    SetWindowTextA(hEditPass, "");
    SetWindowTextA(hStatus, "");
    ShowWindow(g_hLogin, SW_SHOW);
    SetForegroundWindow(g_hLogin);
}

/* ===================================================================
   MAIN DASHBOARD (teacher: full CRUD, student: view-only own record)
   =================================================================== */
void CreateMainDashboard(void)
{
    const char *title = (g_role == 1) ? "Teacher Dashboard - Student Record System"
                                       : "Student Dashboard - Student Record System";
    g_hMain = CreateWindowExA(0, "MainWndClass", title,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 780, 600,
        NULL, NULL, g_hInst, NULL);
    ShowWindow(g_hMain, SW_SHOW);
    UpdateWindow(g_hMain);
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND hList;
    int top = HEADER_H;

    switch (msg) {
    case WM_CREATE: {
        char welcomeBuf[128];

        if (g_hAppIcon) {
            SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)g_hAppIcon);
            SendMessageA(hwnd, WM_SETICON, ICON_BIG,   (LPARAM)g_hAppIcon);
        }

        if (g_role == 1) {
            /* ---- Teacher: ListView with all records + CRUD buttons ---- */
            LVCOLUMNA col;
            sprintf(welcomeBuf, "Logged in as Teacher (admin) -- %d record(s)", g_count);
            CreateWindowA("STATIC", welcomeBuf, WS_CHILD | WS_VISIBLE,
                15, top + 12, 500, 20, hwnd, (HMENU)IDC_STATIC_WELCOME, g_hInst, NULL);

            hList = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "",
                WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
                15, top + 42, 735, 330, hwnd, (HMENU)IDC_LISTVIEW, g_hInst, NULL);
            ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

            ZeroMemory(&col, sizeof(col));
            col.mask = LVCF_TEXT | LVCF_WIDTH;
            col.pszText = "Roll";    col.cx = 60;  ListView_InsertColumn(hList, 0, &col);
            col.pszText = "Name";    col.cx = 190; ListView_InsertColumn(hList, 1, &col);
            col.pszText = "Marks1";  col.cx = 75;  ListView_InsertColumn(hList, 2, &col);
            col.pszText = "Marks2";  col.cx = 75;  ListView_InsertColumn(hList, 3, &col);
            col.pszText = "Marks3";  col.cx = 75;  ListView_InsertColumn(hList, 4, &col);
            col.pszText = "Total";   col.cx = 85;  ListView_InsertColumn(hList, 5, &col);
            col.pszText = "Average"; col.cx = 95;  ListView_InsertColumn(hList, 6, &col);

            RefreshListView(hList);

            /* row 1 */
            CreateWindowA("BUTTON", "Add Student", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                15, top + 388, 130, 34, hwnd, (HMENU)IDC_BTN_ADD, g_hInst, NULL);
            CreateWindowA("BUTTON", "Search by Roll", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                155, top + 388, 130, 34, hwnd, (HMENU)IDC_BTN_SEARCH, g_hInst, NULL);
            CreateWindowA("BUTTON", "Sort (Avg desc)", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                295, top + 388, 130, 34, hwnd, (HMENU)IDC_BTN_SORT, g_hInst, NULL);
            CreateWindowA("BUTTON", "Save to File", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                435, top + 388, 130, 34, hwnd, (HMENU)IDC_BTN_SAVE, g_hInst, NULL);

            /* row 2 */
            CreateWindowA("BUTTON", "Delete Selected", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                15, top + 430, 150, 34, hwnd, (HMENU)IDC_BTN_DELETE, g_hInst, NULL);
            CreateWindowA("BUTTON", "Export to CSV", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                175, top + 430, 150, 34, hwnd, (HMENU)IDC_BTN_EXPORT, g_hInst, NULL);
            CreateWindowA("BUTTON", "Logout", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                600, top + 430, 150, 34, hwnd, (HMENU)IDC_BTN_LOGOUT, g_hInst, NULL);
        } else {
            /* ---- Student: view-only, own record only ---- */
            Student *s = &g_students[g_studentIdx];
            char buf[256];
            int y = top + 20;

            sprintf(welcomeBuf, "Logged in as Student -- Roll No %d", s->roll);
            CreateWindowA("STATIC", welcomeBuf, WS_CHILD | WS_VISIBLE,
                15, y, 400, 20, hwnd, NULL, g_hInst, NULL);
            y += 40;

            sprintf(buf, "Name: %s", s->name);
            CreateWindowA("STATIC", buf, WS_CHILD | WS_VISIBLE, 15, y, 400, 22, hwnd, NULL, g_hInst, NULL); y += 30;
            sprintf(buf, "Subject 1 Marks: %.2f", s->marks[0]);
            CreateWindowA("STATIC", buf, WS_CHILD | WS_VISIBLE, 15, y, 400, 22, hwnd, NULL, g_hInst, NULL); y += 30;
            sprintf(buf, "Subject 2 Marks: %.2f", s->marks[1]);
            CreateWindowA("STATIC", buf, WS_CHILD | WS_VISIBLE, 15, y, 400, 22, hwnd, NULL, g_hInst, NULL); y += 30;
            sprintf(buf, "Subject 3 Marks: %.2f", s->marks[2]);
            CreateWindowA("STATIC", buf, WS_CHILD | WS_VISIBLE, 15, y, 400, 22, hwnd, NULL, g_hInst, NULL); y += 30;
            sprintf(buf, "Total: %.2f", s->total);
            CreateWindowA("STATIC", buf, WS_CHILD | WS_VISIBLE, 15, y, 400, 22, hwnd, NULL, g_hInst, NULL); y += 30;
            sprintf(buf, "Average: %.2f", s->average);
            CreateWindowA("STATIC", buf, WS_CHILD | WS_VISIBLE, 15, y, 400, 22, hwnd, NULL, g_hInst, NULL); y += 45;

            CreateWindowA("BUTTON", "Logout", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                15, y, 130, 34, hwnd, (HMENU)IDC_BTN_LOGOUT, g_hInst, NULL);
        }
        return 0;
    }

    case WM_PAINT:
        PaintHeader(hwnd, (g_role == 1) ? "Teacher Dashboard" : "Student Dashboard");
        return 0;

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        SetTextColor(hdcStatic, RGB(30, 30, 30));
        return (LRESULT)g_hBgBrush;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        switch (dis->CtlID) {
        case IDC_BTN_ADD:
        case IDC_BTN_SEARCH:
        case IDC_BTN_SORT:
            DrawFlatButton(dis, COLOR_ACCENT, COLOR_ACCENT_DK, COLOR_WHITE);
            break;
        case IDC_BTN_SAVE:
        case IDC_BTN_EXPORT:
            DrawFlatButton(dis, COLOR_SUCCESS, COLOR_SUCCESS_DK, COLOR_WHITE);
            break;
        case IDC_BTN_DELETE:
            DrawFlatButton(dis, COLOR_DANGER, COLOR_DANGER_DK, COLOR_WHITE);
            break;
        case IDC_BTN_LOGOUT:
            DrawFlatButton(dis, COLOR_NEUTRAL, COLOR_NEUTRAL_DK, COLOR_WHITE);
            break;
        }
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BTN_ADD: {
            HWND hAdd = CreateWindowExA(WS_EX_DLGMODALFRAME, "AddWndClass",
                "Add New Student",
                WS_POPUP | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT, CW_USEDEFAULT, 340, 300,
                hwnd, NULL, g_hInst, NULL);
            EnableWindow(hwnd, FALSE);
            ShowWindow(hAdd, SW_SHOW);
            return 0;
        }
        case IDC_BTN_SEARCH: {
            HWND hSearch = CreateWindowExA(WS_EX_DLGMODALFRAME, "SearchWndClass",
                "Search Student by Roll Number",
                WS_POPUP | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT, CW_USEDEFAULT, 380, 260,
                hwnd, NULL, g_hInst, NULL);
            EnableWindow(hwnd, FALSE);
            ShowWindow(hSearch, SW_SHOW);
            return 0;
        }
        case IDC_BTN_SORT:
            sortStudentsDescByAverage(g_students, g_count);
            RefreshListView(GetDlgItem(hwnd, IDC_LISTVIEW));
            return 0;

        case IDC_BTN_SAVE:
            if (saveStudentsToFile(g_students, g_count, DATA_FILE)) {
                MessageBoxA(hwnd, "Records saved to file successfully.", "Save", MB_OK | MB_ICONINFORMATION);
            } else {
                MessageBoxA(hwnd, "Could not write to file (disk full or permission denied).", "Save Failed", MB_OK | MB_ICONERROR);
            }
            return 0;

        case IDC_BTN_DELETE: {
            HWND hLV = GetDlgItem(hwnd, IDC_LISTVIEW);
            int sel = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);
            if (sel == -1) {
                MessageBoxA(hwnd, "Select a row in the table first.", "Delete Student", MB_OK | MB_ICONWARNING);
            } else {
                int roll = g_students[sel].roll;
                char q[64];
                sprintf(q, "Delete student with roll number %d?", roll);
                if (MessageBoxA(hwnd, q, "Confirm Delete", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                    deleteStudentByRoll(g_students, &g_count, roll);
                    RefreshListView(hLV);
                    saveStudentsToFile(g_students, g_count, DATA_FILE);
                }
            }
            return 0;
        }

        case IDC_BTN_EXPORT:
            ExportToCSV(hwnd);
            return 0;

        case IDC_BTN_LOGOUT:
            saveStudentsToFile(g_students, g_count, DATA_FILE); /* persist before leaving */
            DestroyWindow(hwnd);
            ShowLoginAgain();
            return 0;
        }
        return 0;

    case WM_CLOSE:
        saveStudentsToFile(g_students, g_count, DATA_FILE);
        g_quitting = TRUE;
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        if (g_quitting) {
            PostQuitMessage(0);
        }
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void RefreshListView(HWND hList)
{
    int i;
    char buf[64];
    LVITEMA item;

    ListView_DeleteAllItems(hList);

    for (i = 0; i < g_count; i++) {
        ZeroMemory(&item, sizeof(item));
        item.mask = LVIF_TEXT;
        item.iItem = i;

        sprintf(buf, "%d", g_students[i].roll);
        item.pszText = buf;
        item.iSubItem = 0;
        ListView_InsertItem(hList, &item);

        ListView_SetItemText(hList, i, 1, g_students[i].name);

        sprintf(buf, "%.2f", g_students[i].marks[0]);
        ListView_SetItemText(hList, i, 2, buf);
        sprintf(buf, "%.2f", g_students[i].marks[1]);
        ListView_SetItemText(hList, i, 3, buf);
        sprintf(buf, "%.2f", g_students[i].marks[2]);
        ListView_SetItemText(hList, i, 4, buf);
        sprintf(buf, "%.2f", g_students[i].total);
        ListView_SetItemText(hList, i, 5, buf);
        sprintf(buf, "%.2f", g_students[i].average);
        ListView_SetItemText(hList, i, 6, buf);
    }
}

/* Bonus feature: export all records to a CSV file next to the .exe,
   openable directly in Excel/Sheets. */
void ExportToCSV(HWND hwnd)
{
    FILE *fp = fopen("students_export.csv", "w");
    int i;

    if (fp == NULL) {
        MessageBoxA(hwnd, "Could not create students_export.csv (check folder permissions).",
            "Export Failed", MB_OK | MB_ICONERROR);
        return;
    }

    fprintf(fp, "Roll,Name,Marks1,Marks2,Marks3,Total,Average\n");
    for (i = 0; i < g_count; i++) {
        fprintf(fp, "%d,%s,%.2f,%.2f,%.2f,%.2f,%.2f\n",
            g_students[i].roll, g_students[i].name,
            g_students[i].marks[0], g_students[i].marks[1], g_students[i].marks[2],
            g_students[i].total, g_students[i].average);
    }
    fclose(fp);

    MessageBoxA(hwnd, "Exported to students_export.csv (same folder as the .exe).",
        "Export Complete", MB_OK | MB_ICONINFORMATION);
}

/* ===================================================================
   ADD STUDENT popup window
   =================================================================== */
LRESULT CALLBACK AddWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND hRoll, hName, hM1, hM2, hM3, hStatus;

    switch (msg) {
    case WM_CREATE:
        CreateWindowA("STATIC", "Roll Number:", WS_CHILD | WS_VISIBLE, 15, 15, 120, 20, hwnd, NULL, g_hInst, NULL);
        hRoll = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
            150, 12, 160, 24, hwnd, (HMENU)IDC_ADD_ROLL, g_hInst, NULL);

        CreateWindowA("STATIC", "Name:", WS_CHILD | WS_VISIBLE, 15, 48, 120, 20, hwnd, NULL, g_hInst, NULL);
        hName = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER,
            150, 45, 160, 24, hwnd, (HMENU)IDC_ADD_NAME, g_hInst, NULL);

        CreateWindowA("STATIC", "Marks - Subject 1:", WS_CHILD | WS_VISIBLE, 15, 81, 130, 20, hwnd, NULL, g_hInst, NULL);
        hM1 = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER,
            150, 78, 160, 24, hwnd, (HMENU)IDC_ADD_M1, g_hInst, NULL);

        CreateWindowA("STATIC", "Marks - Subject 2:", WS_CHILD | WS_VISIBLE, 15, 114, 130, 20, hwnd, NULL, g_hInst, NULL);
        hM2 = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER,
            150, 111, 160, 24, hwnd, (HMENU)IDC_ADD_M2, g_hInst, NULL);

        CreateWindowA("STATIC", "Marks - Subject 3:", WS_CHILD | WS_VISIBLE, 15, 147, 130, 20, hwnd, NULL, g_hInst, NULL);
        hM3 = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER,
            150, 144, 160, 24, hwnd, (HMENU)IDC_ADD_M3, g_hInst, NULL);

        CreateWindowA("BUTTON", "Add", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            50, 190, 100, 32, hwnd, (HMENU)IDC_ADD_OK, g_hInst, NULL);
        CreateWindowA("BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            180, 190, 100, 32, hwnd, (HMENU)IDC_ADD_CANCEL, g_hInst, NULL);

        hStatus = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE,
            15, 230, 300, 40, hwnd, (HMENU)IDC_ADD_STATUS, g_hInst, NULL);
        return 0;

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        SetTextColor(hdcStatic, RGB(30, 30, 30));
        return (LRESULT)g_hBgBrush;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        if (dis->CtlID == IDC_ADD_OK) {
            DrawFlatButton(dis, COLOR_SUCCESS, COLOR_SUCCESS_DK, COLOR_WHITE);
        } else if (dis->CtlID == IDC_ADD_CANCEL) {
            DrawFlatButton(dis, COLOR_NEUTRAL, COLOR_NEUTRAL_DK, COLOR_WHITE);
        }
        return TRUE;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_ADD_OK) {
            char rollBuf[16], nameBuf[NAME_LEN], m1Buf[16], m2Buf[16], m3Buf[16];
            Student ns;
            int result;

            GetWindowTextA(hRoll, rollBuf, sizeof(rollBuf));
            GetWindowTextA(hName, nameBuf, sizeof(nameBuf));
            GetWindowTextA(hM1, m1Buf, sizeof(m1Buf));
            GetWindowTextA(hM2, m2Buf, sizeof(m2Buf));
            GetWindowTextA(hM3, m3Buf, sizeof(m3Buf));

            if (rollBuf[0] == '\0' || nameBuf[0] == '\0') {
                SetWindowTextA(hStatus, "Roll number and name are required.");
                return 0;
            }

            ns.roll = atoi(rollBuf);
            strncpy(ns.name, nameBuf, NAME_LEN - 1);
            ns.name[NAME_LEN - 1] = '\0';
            ns.marks[0] = (float)atof(m1Buf);
            ns.marks[1] = (float)atof(m2Buf);
            ns.marks[2] = (float)atof(m3Buf);

            result = addStudent(g_students, &g_count, ns);

            if (result == -1) {
                SetWindowTextA(hStatus, "Cannot add: maximum record limit reached.");
            } else if (result == -2) {
                SetWindowTextA(hStatus, "Cannot add: that roll number already exists.");
            } else {
                /* success -- close popup, refresh parent list, re-enable parent */
                HWND hParent = GetParent(hwnd);
                EnableWindow(hParent, TRUE);
                RefreshListView(GetDlgItem(hParent, IDC_LISTVIEW));
                SetForegroundWindow(hParent);
                DestroyWindow(hwnd);
            }
            return 0;
        }
        if (LOWORD(wParam) == IDC_ADD_CANCEL) {
            HWND hParent = GetParent(hwnd);
            EnableWindow(hParent, TRUE);
            SetForegroundWindow(hParent);
            DestroyWindow(hwnd);
            return 0;
        }
        return 0;

    case WM_DESTROY:
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/* ===================================================================
   SEARCH BY ROLL popup window
   =================================================================== */
LRESULT CALLBACK SearchWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND hRoll, hResult;

    switch (msg) {
    case WM_CREATE:
        CreateWindowA("STATIC", "Roll Number:", WS_CHILD | WS_VISIBLE, 15, 15, 100, 20, hwnd, NULL, g_hInst, NULL);
        hRoll = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
            120, 12, 150, 24, hwnd, (HMENU)IDC_SEARCH_ROLL, g_hInst, NULL);
        CreateWindowA("BUTTON", "Search", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            280, 10, 80, 28, hwnd, (HMENU)IDC_SEARCH_GO, g_hInst, NULL);

        hResult = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE,
            15, 55, 340, 150, hwnd, (HMENU)IDC_SEARCH_RESULT, g_hInst, NULL);

        CreateWindowA("BUTTON", "Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            135, 210, 100, 32, hwnd, (HMENU)IDC_SEARCH_CLOSE, g_hInst, NULL);
        return 0;

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        SetTextColor(hdcStatic, RGB(30, 30, 30));
        return (LRESULT)g_hBgBrush;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        if (dis->CtlID == IDC_SEARCH_GO) {
            DrawFlatButton(dis, COLOR_ACCENT, COLOR_ACCENT_DK, COLOR_WHITE);
        } else if (dis->CtlID == IDC_SEARCH_CLOSE) {
            DrawFlatButton(dis, COLOR_NEUTRAL, COLOR_NEUTRAL_DK, COLOR_WHITE);
        }
        return TRUE;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_SEARCH_GO) {
            char rollBuf[16], out[300];
            int roll, idx;

            GetWindowTextA(hRoll, rollBuf, sizeof(rollBuf));
            roll = atoi(rollBuf);
            idx = searchStudentByRoll(g_students, g_count, roll);

            if (idx == -1) {
                sprintf(out, "No student found with roll number %d.", roll);
            } else {
                Student *s = &g_students[idx];
                sprintf(out,
                    "Found!\r\nName: %s\r\nMarks: %.2f, %.2f, %.2f\r\nTotal: %.2f\r\nAverage: %.2f",
                    s->name, s->marks[0], s->marks[1], s->marks[2], s->total, s->average);
            }
            SetWindowTextA(hResult, out);
            return 0;
        }
        if (LOWORD(wParam) == IDC_SEARCH_CLOSE) {
            HWND hParent = GetParent(hwnd);
            EnableWindow(hParent, TRUE);
            SetForegroundWindow(hParent);
            DestroyWindow(hwnd);
            return 0;
        }
        return 0;

    case WM_DESTROY:
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}
