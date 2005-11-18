/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Thom Johansen
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* TODO: integrate the iriver.c and mkboot stuff better, they're pretty much
 * intended to be called from a command line tool, and i haven't changed that.
 */

#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <windows.h>
#include "iriver.h"
#include "md5.h"
#include "resource.h"

#define WINDOW_WIDTH 280 
#define WINDOW_HEIGHT 130 

#define IDM_RESTORE 1000
#define IDM_EXIT 1010

#define LABEL_FILENAME 0
#define EDIT_FILENAME 1
#define BUTTON_BROWSE 2
#define BUTTON_PATCH 3

#define CTL_NUM 4

TCHAR appsversion[] = TEXT("Rockbox firmware patcher V" APPSVERSION);

struct sumpairs {
    char *unpatched;
    char *patched;
};

/* precalculated checksums for H110/H115 */
static struct sumpairs h100pairs[] = {
#include "h100sums.h"
};

/* precalculated checksums for H120/H140 */
static struct sumpairs h120pairs[] = {
#include "h120sums.h"
};

/* precalculated checksums for H320/H340 */
static struct sumpairs h300pairs[] = {
#include "h300sums.h"
};

HICON rbicon;
HFONT deffont;
HWND controls[CTL_NUM];

/* begin mkboot.c excerpt */

unsigned char image[0x400000 + 0x220 + 0x400000/0x200];

int mkboot(TCHAR *infile, TCHAR *outfile, unsigned char *bldata, int bllen,
           int origin)
{
    FILE *f;
    int i;
    int len;
    int actual_length, total_length, binary_length, num_chksums;

    memset(image, 0xff, sizeof(image));

    /* First, read the iriver original firmware into the image */
    f = _tfopen(infile, TEXT("rb"));
    if(!f) {
        perror(infile);
        return 0;
    }

    i = fread(image, 1, 16, f);
    if(i < 16) {
        perror(infile);
        return 0;
    }

    /* This is the length of the binary image without the scrambling
       overhead (but including the ESTFBINR header) */
    binary_length = image[4] + (image[5] << 8) +
        (image[6] << 16) + (image[7] << 24);

    /* Read the rest of the binary data, but not the checksum block */
    len = binary_length+0x200-16;
    i = fread(image+16, 1, len, f);
    if(i < len) {
        perror(infile);
        return 0;
    }
    
    fclose(f);

    memcpy(image + 0x220 + origin, bldata, bllen);
    
    f = _tfopen(outfile, TEXT("wb"));
    if(!f) {
        perror(outfile);
        return 0;
    }

    /* Patch the reset vector to start the boot loader */
    image[0x220 + 4] = image[origin + 0x220 + 4];
    image[0x220 + 5] = image[origin + 0x220 + 5];
    image[0x220 + 6] = image[origin + 0x220 + 6];
    image[0x220 + 7] = image[origin + 0x220 + 7];

    /* This is the actual length of the binary, excluding all headers */
    actual_length = origin + bllen;

    /* Patch the ESTFBINR header */
    image[0x20c] = (actual_length >> 24) & 0xff;
    image[0x20d] = (actual_length >> 16) & 0xff;
    image[0x20e] = (actual_length >> 8) & 0xff;
    image[0x20f] = actual_length & 0xff;
    
    image[0x21c] = (actual_length >> 24) & 0xff;
    image[0x21d] = (actual_length >> 16) & 0xff;
    image[0x21e] = (actual_length >> 8) & 0xff;
    image[0x21f] = actual_length & 0xff;

    /* This is the length of the binary, including the ESTFBINR header and
       rounded up to the nearest 0x200 boundary */
    binary_length = (actual_length + 0x20 + 0x1ff) & 0xfffffe00;

    /* The number of checksums, i.e number of 0x200 byte blocks */
    num_chksums = binary_length / 0x200;

    /* The total file length, including all headers and checksums */
    total_length = binary_length + num_chksums + 0x200;

    /* Patch the scrambler header with the new length info */
    image[0] = total_length & 0xff;
    image[1] = (total_length >> 8) & 0xff;
    image[2] = (total_length >> 16) & 0xff;
    image[3] = (total_length >> 24) & 0xff;
    
    image[4] = binary_length & 0xff;
    image[5] = (binary_length >> 8) & 0xff;
    image[6] = (binary_length >> 16) & 0xff;
    image[7] = (binary_length >> 24) & 0xff;
    
    image[8] = num_chksums & 0xff;
    image[9] = (num_chksums >> 8) & 0xff;
    image[10] = (num_chksums >> 16) & 0xff;
    image[11] = (num_chksums >> 24) & 0xff;
    
    i = fwrite(image, 1, total_length, f);
    if(i < total_length) {
        perror(outfile);
        return 0;
    }

    fclose(f);
    
    return 1;
}

/* end mkboot.c excerpt */

int intable(char *md5, struct sumpairs *table, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (strncmp(md5, table[i].unpatched, 32) == 0) {
            return i;
        }
    }
    return -1;
}

int FileMD5(TCHAR *name, char *md5)
{
    int i, read;
    md5_context ctx;
    unsigned char md5sum[16];
    unsigned char block[32768];
    FILE *file;
    
    file = _tfopen(name, TEXT("rb"));
    if (!file) {
        MessageBox(NULL,
                   TEXT("Could not open patched firmware for checksum check"),
                   TEXT("Error"), MB_ICONERROR);
        return 0;
    }
    md5_starts(&ctx);
    while ((read = fread(block, 1, sizeof(block), file)) > 0) {
        md5_update(&ctx, block, read);
    }
    fclose(file);
    md5_finish(&ctx, md5sum);
    for (i = 0; i < 16; ++i)
        sprintf(md5 + 2*i, "%02x", md5sum[i]);
    return 1;
}

int PatchFirmware(int series, int table_entry)
{
    TCHAR fn[MAX_PATH];
    TCHAR name1[MAX_PATH], name2[MAX_PATH], name3[MAX_PATH];
    HRSRC res;
    HGLOBAL resload;
    unsigned char *bootloader;
    unsigned char md5sum_str[256];
    DWORD blsize;
    int i;
    struct sumpairs *sums;
    int origin;
    
    /* get pointer to the correct bootloader.bin */
    switch(series) {
        case 100:
            res = FindResource(NULL, MAKEINTRESOURCE(IDI_BOOTLOADERH100), TEXT("BIN"));
            sums = &h100pairs[0];
            origin = 0x1f0000;
            break;
        case 120:
            res = FindResource(NULL, MAKEINTRESOURCE(IDI_BOOTLOADERH120), TEXT("BIN"));
            sums = &h120pairs[0];
            origin = 0x1f0000;
            break;
        case 300:
            res = FindResource(NULL, MAKEINTRESOURCE(IDI_BOOTLOADERH300), TEXT("BIN"));
            sums = &h300pairs[0];
            origin = 0x3f0000;
            break;
    }
    resload = LoadResource(NULL, res);
    bootloader = (unsigned char *)LockResource(resload);
    blsize = SizeofResource(NULL, res);
    
    /* get filename from edit box */
    GetWindowText(controls[EDIT_FILENAME], fn, MAX_PATH);
    /* store temp files in temp directory */
    GetTempPath(MAX_PATH, name1);
    GetTempPath(MAX_PATH, name2);
    GetTempPath(MAX_PATH, name3);
    _tcscat(name1, TEXT("firmware.bin")); /* descrambled file */
    _tcscat(name2, TEXT("new.bin"));      /* patched file */
    _tcscat(name3, TEXT("new.hex"));      /* patched and scrambled file */
    if (iriver_decode(fn, name1, FALSE, STRIP_NONE) == -1) {
        MessageBox(NULL, TEXT("Error in descramble"),
                   TEXT("Error"), MB_ICONERROR);
        goto error;
    }
    if (!mkboot(name1, name2, bootloader, blsize, origin)) {
        MessageBox(NULL, TEXT("Error in patching"),
                   TEXT("Error"), MB_ICONERROR);
        goto error;
    }
    if (iriver_encode(name2, name3, FALSE) == -1) {
        MessageBox(NULL, TEXT("Error in scramble"),
                   TEXT("Error"), MB_ICONERROR);
        goto error;
    }
    /* now md5sum it */
    if (!FileMD5(name3, md5sum_str)) {
        MessageBox(NULL, TEXT("Error in checksumming"),
                   TEXT("Error"), MB_ICONERROR);
        goto error;
    }
    if (strncmp(sums[table_entry].patched, md5sum_str, 32) == 0) {
        /* delete temp files */
        DeleteFile(name1);
        DeleteFile(name2);
        /* all is fine, rename the patched file to original name of the firmware */
        if (DeleteFile(fn) && MoveFile(name3, fn)) {
            return 1;
        }
        else {
            DeleteFile(name3); /* Deleting a perfectly good firmware here really */
            MessageBox(NULL,
                       TEXT("Couldn't modify existing file.\n")
                       TEXT("Check if file is write protected, then try again."),
                       TEXT("Error"), MB_ICONERROR);
            return 0;
        }
    }
    MessageBox(NULL,
               TEXT("Checksum doesn't match known good patched firmware.\n")
               TEXT("Download another firmware image, then try again."),
               TEXT("Error"), MB_ICONERROR);
error:
    /* delete all temp files, don't care if some aren't created yet */
    DeleteFile(name1);
    DeleteFile(name2);
    DeleteFile(name3);
    return 0;
}

int FileDialog(TCHAR *fn)
{
    OPENFILENAME ofn;
    TCHAR filename[MAX_PATH];

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = filename;
    ofn.lpstrFile[0] = '\0'; // no default filename
    ofn.nMaxFile = sizeof(filename);
    ofn.lpstrFilter = TEXT("Firmware\0*.HEX\0");
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    
    if (GetOpenFileName(&ofn) == TRUE) {
        _tcscpy(fn, filename);
        return 1;
    }
    return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   int i, series, table_entry;
   unsigned char md5sum_str[256];
   switch (msg) {
   TCHAR fn[MAX_PATH];
   case WM_CREATE:
       /* text label */
       controls[LABEL_FILENAME] = 
           CreateWindowEx(0, TEXT("STATIC"), TEXT("Firmware file name:"),
                          WS_CHILD | WS_VISIBLE, 10, 14,
                          100, 32, hwnd, 0, 0, 0);
       /* text field for inputing file name */
       controls[EDIT_FILENAME] = 
           CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), TEXT(""),
                          WS_CHILD | WS_TABSTOP | WS_VISIBLE | ES_AUTOHSCROLL,
                          10, 35, 180, 20, hwnd, 0, 0, 0);
       /* browse button */
       controls[BUTTON_BROWSE] =
           CreateWindowEx(0, TEXT("BUTTON"), TEXT("Browse"),
                          WS_CHILD | WS_TABSTOP | WS_VISIBLE, 200, 32, 70, 25,
                          hwnd, 0, 0, 0);        
       /* patch button */
       controls[BUTTON_PATCH] =
           CreateWindowEx(0, TEXT("BUTTON"), TEXT("Patch"),
                          WS_CHILD | WS_TABSTOP | WS_VISIBLE, 90, 70, 90, 25,
                          hwnd, 0, 0, 0);
       /* set default font on all controls, will be ugly if we don't do this */
       deffont = GetStockObject(DEFAULT_GUI_FONT);
       for (i = 0; i < CTL_NUM; ++i) 
           SendMessage(controls[i], WM_SETFONT, (WPARAM)deffont,
                       MAKELPARAM(FALSE, 0));
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        break;
    case WM_COMMAND:
        /* user pressed browse button */
        if (((HWND)lParam == controls[BUTTON_BROWSE])) {
            TCHAR buf[MAX_PATH];
            if (FileDialog(buf))
                SetWindowText(controls[EDIT_FILENAME], buf);
        }
        /* user pressed patch button */
        if (((HWND)lParam == controls[BUTTON_PATCH])) {
            GetWindowText(controls[EDIT_FILENAME], fn, MAX_PATH);
            if (!FileMD5(fn, md5sum_str)) {
                MessageBox(NULL, TEXT("Couldn't open firmware"), TEXT("Fail"), MB_OK);
            }
            else {
                /* Check firmware against md5sums in h120sums and h100sums */
                series = 0;
                table_entry = intable(md5sum_str, &h120pairs[0],
                                      sizeof(h120pairs)/sizeof(struct sumpairs));
                if (table_entry >= 0) {
                    series = 120;
                }
                else {
                    table_entry = intable(md5sum_str, &h100pairs[0],
                                          sizeof(h100pairs)/sizeof(struct sumpairs));
                    if (table_entry >= 0) {
                        series = 100;
                    }
                    else {
                        table_entry =
                            intable(md5sum_str, &h300pairs[0],
                                    sizeof(h300pairs)/sizeof(struct sumpairs));
                        if (table_entry >= 0)
                            series = 300;
                    }
                }
                if (series == 0) {
                    MessageBox(NULL, TEXT("Unrecognised firmware"), TEXT("Fail"), MB_OK);
                }
                else if (PatchFirmware(series, table_entry))
                    MessageBox(NULL, TEXT("Firmware patched successfully"),
                               TEXT("Success"), MB_OK);
            }
        }
        break;
    case WM_USER:
        /* command line driven patch button */
        SetWindowText(controls[EDIT_FILENAME], (LPCTSTR)wParam);
        SendMessage(hwnd, WM_COMMAND, 0, (LPARAM)(controls[BUTTON_PATCH]) );
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void getargs(LPTSTR p, int * argc, LPCTSTR * argv, int MAXARGS)
{
    int quote=FALSE,whitespace=FALSE;
    LPCTSTR tok=p;
    *argc=0;
    while(*argc<MAXARGS)
    {
        if((*p==TEXT(' ') || *p==TEXT('\0')) && !quote)
        {
            if(!whitespace)
            {
                whitespace=TRUE;
                *argv++ = tok;
                (*argc)++;
            };
            if(*p==TEXT('\0'))
                break;  // end of cmd line
            *p=TEXT('\0');
            p++;
        }
        else
        {
            if(whitespace)
                tok=p;
            whitespace=FALSE;
            if(*p==TEXT('\"'))
            {
                *p=TEXT(' ');
                if(!quote)
                {
                    p++;
                    tok=p;
                } 
                quote = !quote;
            }
            else p++;
        }
    }
}

#define MAXARGC 4

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance,
                   LPSTR command_line, int command_show) 
{
    HWND window;
    WNDCLASSEX wc;
    MSG msg;
    int argc;
    LPCTSTR argv[MAXARGC] = { NULL };
    LPTSTR cmdline = GetCommandLine();

    getargs(cmdline, &argc, argv, MAXARGC);
    if (argc > 1)
        command_show = SW_HIDE;
    
    rbicon = LoadIcon(instance, MAKEINTRESOURCE(IDI_RBICON));
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = instance;
    wc.hIcon = rbicon;
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszClassName = TEXT("patcher");
    RegisterClassEx(&wc);

    window = CreateWindowEx(0, TEXT("patcher"), appsversion,
                            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
                            WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT,
                            WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0, instance, NULL);
    if (!window) return 0;

    ShowWindow(window, command_show);

    if (argc > 1) {
        SendMessage(window, WM_USER, (WPARAM)(argv[1]), 0);
        SendMessage(window, WM_CLOSE, 0, 0);
    }

    while (GetMessage(&msg, 0, 0, 0) > 0) {
        if (!IsDialogMessage(window, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return 0;
}

