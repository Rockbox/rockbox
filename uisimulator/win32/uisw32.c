/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Felix Arends
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <windows.h>
#include <process.h>
#include <stdlib.h>
#include <fcntl.h>
#include "uisw32.h"
#include "resource.h"
#include "button.h"
#include "thread.h"
#include "thread-win32.h"
#include "kernel.h"

#ifndef LR_VGACOLOR             /* Should be under MINGW32 builds? */
#define LR_VGACOLOR LR_COLOR
#endif

// extern functions
extern void                 app_main (void *); // mod entry point
extern void					new_key(int key);

void button_event(int key, bool pressed);

// variables
HWND                                hGUIWnd; // the GUI window handle
unsigned int                        uThreadID; // id of mod thread
PBYTE                               lpKeys;
bool                                bActive; // window active?
HANDLE                              hGUIThread; // thread for GUI
bool                                bIsWinNT; // Windows NT derivate?

bool lcd_display_redraw=true; // Used for player simulator
char having_new_lcd=true; // Used for player simulator

// GUIWndProc
// window proc for GUI simulator
LRESULT CALLBACK GUIWndProc (
                             HWND hWnd,
                             UINT uMsg,
                             WPARAM wParam,
                             LPARAM lParam
                            )
{
    static HBITMAP hBkgnd;
    static HDC hMemDc;

    static LARGE_INTEGER    persec, tick1, ticknow;

    switch (uMsg)
    {
    case WM_TIMER:
        QueryPerformanceCounter(&ticknow);
        current_tick = ((ticknow.QuadPart-tick1.QuadPart)*HZ)/persec.QuadPart;
        return TRUE;
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
            bActive = true;
        else
            bActive = false;
        return TRUE;
    case WM_CREATE:
        QueryPerformanceFrequency(&persec);
        QueryPerformanceCounter(&tick1);
        SetTimer (hWnd, TIMER_EVENT, 1, NULL);
        
        // load background image
        hBkgnd = (HBITMAP)LoadImage (GetModuleHandle (NULL),
                  MAKEINTRESOURCE(IDB_UI), IMAGE_BITMAP, 0, 0, LR_VGACOLOR);
        hMemDc = CreateCompatibleDC (GetDC (hWnd));
        SelectObject (hMemDc, hBkgnd);
        return TRUE;
    case WM_SIZING:
        {
            LPRECT r = (LPRECT)lParam;
            char s[256];
            int v;
            int h_add = GetSystemMetrics (SM_CXSIZEFRAME) * 2 + 4;
            int v_add = GetSystemMetrics (SM_CYSIZEFRAME) * 2 
                        + GetSystemMetrics (SM_CYCAPTION) + 4;

            switch (wParam)
            {
            case WMSZ_BOTTOM:
                v = (r->bottom - r->top) / (UI_HEIGHT / 5);
                r->bottom = r->top + v * UI_HEIGHT / 5 + v_add;
                r->right = r->left + v * UI_WIDTH / 5 + h_add;
                break;
            case WMSZ_RIGHT:
                v = (r->right - r->left) / (UI_WIDTH / 5);
                r->bottom = r->top + v * UI_HEIGHT / 5 + v_add;
                r->right = r->left + v * UI_WIDTH / 5 + h_add;
                break;
            case WMSZ_TOP:
                v = (r->bottom - r->top) / (UI_HEIGHT / 5);
                r->top = r->bottom - v * UI_HEIGHT / 5 - v_add;
                r->right = r->left + v * UI_WIDTH / 5 + h_add;
                break;
            case WMSZ_LEFT:
                v = (r->right - r->left) / (UI_WIDTH / 5);
                r->bottom = r->top + v * UI_HEIGHT / 5 + v_add;
                r->left = r->right - v * UI_WIDTH / 5 - h_add;
                break;
            case WMSZ_BOTTOMRIGHT:
                v = ((r->right - r->left) * UI_HEIGHT
                     +(r->bottom - r->top) * UI_WIDTH)
                    / (2 * UI_WIDTH * UI_HEIGHT / 5);
                r->bottom = r->top + v * UI_HEIGHT / 5 + v_add;
                r->right = r->left + v * UI_WIDTH / 5 + h_add;
                break;
            case WMSZ_BOTTOMLEFT:
                v = ((r->right - r->left) * UI_HEIGHT
                     +(r->bottom - r->top) * UI_WIDTH)
                    / (2 * UI_WIDTH * UI_HEIGHT / 5);
                r->bottom = r->top + v * UI_HEIGHT / 5 + v_add;
                r->left = r->right - v * UI_WIDTH / 5 - h_add;
                break;
            case WMSZ_TOPRIGHT:
                v = ((r->right - r->left) * UI_HEIGHT
                     +(r->bottom - r->top) * UI_WIDTH)
                    / (2 * UI_WIDTH * UI_HEIGHT / 5);
                r->top = r->bottom - v * UI_HEIGHT / 5 - v_add;
                r->right = r->left + v * UI_WIDTH / 5 + h_add;
                break;
            case WMSZ_TOPLEFT:
                v = ((r->right - r->left) * UI_HEIGHT
                     +(r->bottom - r->top) * UI_WIDTH)
                    / (2 * UI_WIDTH * UI_HEIGHT / 5);
                r->top = r->bottom - v * UI_HEIGHT / 5 - v_add;
                r->left = r->right - v * UI_WIDTH / 5 - h_add;
                break;
            }

            wsprintf (s, UI_TITLE " @%d%%",
                (r->right - r->left - h_add + 1) * 100 / UI_WIDTH);
            SetWindowText (hWnd, s);

            return TRUE;
        }
    case WM_ERASEBKGND:
        {
            HDC hDc = (HDC) wParam;
            RECT r;

            GetClientRect (hWnd, &r);
            // blit background image to screen
            SetStretchBltMode(hDc, bIsWinNT ? HALFTONE : COLORONCOLOR);
            StretchBlt (hDc, 0, 0, r.right, r.bottom,
                hMemDc, 0, 0, UI_WIDTH, UI_HEIGHT, SRCCOPY);
            return TRUE;
        }
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            RECT r;
            HDC hDc = BeginPaint (hWnd, &ps);

            GetClientRect (hWnd, &r);
            // draw lcd screen
            SetStretchBltMode(hDc, bIsWinNT ? HALFTONE : COLORONCOLOR);
            StretchDIBits (hDc,
                           UI_LCD_POSX * r.right / UI_WIDTH,
                           UI_LCD_POSY * r.bottom / UI_HEIGHT,
                           UI_LCD_WIDTH * r.right / UI_WIDTH,
                           UI_LCD_HEIGHT * r.bottom / UI_HEIGHT,
                           0, 0, LCD_WIDTH, LCD_HEIGHT,
                           bitmap, (BITMAPINFO *) &bmi, DIB_RGB_COLORS,
                           SRCCOPY);
            
            EndPaint (hWnd, &ps);
            return TRUE;
        }
    case WM_CLOSE:
        // close simulator
        KillTimer (hWnd, TIMER_EVENT);
        hGUIWnd = NULL;
        PostQuitMessage (0);
        break;
    case WM_DESTROY:
        // close simulator
        hGUIWnd = NULL;
        PostQuitMessage (0);
        break;
    case WM_KEYDOWN:
        button_event(wParam, true);
        break;
    case WM_KEYUP:
        button_event(wParam, false);
        break;
    }

    return DefWindowProc (hWnd, uMsg, wParam, lParam);
}

// GUIStartup
// register window class, show window, init GUI
BOOL GUIStartup ()
{
    WNDCLASS wc;

    // create window class
    ZeroMemory (&wc, sizeof(wc));
    wc.hbrBackground = GetSysColorBrush (COLOR_WINDOW);
    wc.hCursor = LoadCursor (NULL, IDC_ARROW);
    wc.hInstance = GetModuleHandle (NULL);
    wc.lpfnWndProc = GUIWndProc;
    wc.lpszClassName = "RockBoxUISimulator";
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (RegisterClass (&wc) == 0)
        return FALSE;

    // create window
    hGUIWnd = CreateWindowEx (
        WS_EX_OVERLAPPEDWINDOW,
        "RockBoxUISimulator", UI_TITLE,
        WS_VISIBLE | WS_SYSMENU | WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        UI_WIDTH + GetSystemMetrics (SM_CXSIZEFRAME) * 2 +4,
        UI_HEIGHT + GetSystemMetrics (SM_CYSIZEFRAME) * 2 +
        GetSystemMetrics (SM_CYCAPTION) +4,
        NULL, NULL, GetModuleHandle (NULL), NULL);

    if (hGUIWnd == NULL)
        return FALSE;

    return TRUE;
}

// GUIDown
// destroy window, unregister window class
int GUIDown ()
{
    int i;

    DestroyWindow (hGUIWnd);
    CloseHandle (hGUIThread);
    for (i = 0; i < nThreads; i++)
    {
        ResumeThread (lpThreads[i]);
        WaitForSingleObject (lpThreads[i], 1);
        CloseHandle (lpThreads[i]);
    }
    return 0;
}

// GUIMessageLoop
// standard message loop for GUI window
void GUIMessageLoop ()
{
    MSG msg;
    while (GetMessage (&msg, NULL, 0, 0))
    {
        TranslateMessage (&msg);
        DispatchMessage (&msg);
        if (msg.message == TM_YIELD)
        {
            SuspendThread (lpThreads[nPos]);
            if (++nPos >= nThreads)
                nPos = 0;
            ResumeThread (lpThreads[nPos]);
        }
    }
}


// WinMain
// program entry point
int WINAPI WinMain (
                    HINSTANCE hInstance, // current instance
                    HINSTANCE hPrevInstance, // previous instance
                    LPSTR lpCmd, // command line
                    int nShowCmd // show command
                    )
{
    DWORD           dwThreadID;

    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmd;
    (void)nShowCmd;

    /* default file mode should be O_BINARY to be consistent with rockbox */
    _fmode = _O_BINARY;
    
    bIsWinNT = ((GetVersion() & 0x80000000) == 0);

    if (!GUIStartup ())
        return 0;

    hGUIThread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE)app_main,
        NULL, 0, &dwThreadID);

    if (hGUIThread == NULL)
        return MessageBox (NULL, "Error creating gui thread!", "Error", MB_OK);

    GUIMessageLoop ();

    return GUIDown ();
}
