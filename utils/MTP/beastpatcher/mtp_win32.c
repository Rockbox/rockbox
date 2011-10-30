/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * $Id$
 *
 * Copyright (c) 2009, Dave Chapman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <wchar.h>
#include <windows.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <tchar.h>

#include "mtp_common.h"

#include "../MTP_DLL/MTP_DLL.h"


static int filesize(const char* filename);


int mtp_init(struct mtp_info_t* mtp_info)
{
   /* Fill the info struct with zeros - mainly for the strings */
    memset(mtp_info, 0, sizeof(struct mtp_info_t));

    return 0;

}

int mtp_finished(struct mtp_info_t* mtp_info)
{
    (void)mtp_info;

    return 0;
}

int mtp_scan(struct mtp_info_t* mtp_info)
{
    wchar_t name[256];
    wchar_t manufacturer[256];
    DWORD version;
    int num = 0;

    num = mtp_description(name, manufacturer, &version);

    wcstombs(mtp_info->manufacturer, manufacturer, 200);
    wcstombs(mtp_info->modelname, name, 200);

    sprintf(mtp_info->version, "%x", (unsigned int)version);
    return (num > 0) ? num : -1;

}

static void callback(unsigned int progress, unsigned int max)
{
    int percent = (progress * 100) / max;

    printf("[INFO] Progress: %u of %u (%d%%)\r", progress, max, percent);
    fflush(stdout);
}


int mtp_send_firmware(struct mtp_info_t* mtp_info, unsigned char* fwbuf,
                      int fwsize)
{
    HANDLE hTempFile;
    DWORD dwRetVal;
    DWORD dwBytesWritten;
    UINT uRetVal;
    TCHAR szTempName[1024];
    TCHAR lpPathBuffer[1024];
    BOOL fSuccess;
    wchar_t *tmp;
    int ret;

    (void)mtp_info;

    /* Get the path for temporary files */
    dwRetVal = GetTempPath(sizeof(lpPathBuffer), lpPathBuffer);
    if (dwRetVal > sizeof(lpPathBuffer) || (dwRetVal == 0))
    {
        fprintf(stderr, "[ERR]  GetTempPath failed (%d)\n", (int)GetLastError());
        return -1;
    }

    /* Create the temporary file */
    uRetVal = GetTempFileName(lpPathBuffer, TEXT("NKBIN"), 0, szTempName);
    if (uRetVal == 0)
    {
        fprintf(stderr, "[ERR]  GetTempFileName failed (%d)\n", (int)GetLastError());
        return -1;
    }

    /* Now create the file */
    hTempFile = CreateFile((LPTSTR) szTempName, // file name
                           GENERIC_READ | GENERIC_WRITE, // open r-w
                           0,                    // do not share
                           NULL,                 // default security
                           CREATE_ALWAYS,        // overwrite existing
                           FILE_ATTRIBUTE_NORMAL,// normal file
                           NULL);                // no template
    if (hTempFile == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "[ERR]  Could not create %s\n", szTempName);
        return -1;
    }

    fSuccess = WriteFile(hTempFile, fwbuf, fwsize, &dwBytesWritten, NULL);
    if (!fSuccess)
    {
        fprintf(stderr, "[ERR]  WriteFile failed (%d)\n", (int)GetLastError());
        return -1;
    }

    fSuccess = CloseHandle (hTempFile);
    if (!fSuccess)
    {
       fprintf(stderr, "[ERR]  CloseHandle failed (%d)\n", (int)GetLastError());
       return -1;
    }

    tmp = (LPWSTR)malloc(_tcslen(szTempName)*2+1);
    mbstowcs(tmp, (char*)szTempName, _tcslen(szTempName)*2+1);

    fprintf(stderr, "[INFO] Sending firmware...\n");
    if (mtp_sendnk(tmp, fwsize, &callback))
    {
        fprintf(stderr, "\n");
        fprintf(stderr, "[INFO] Firmware sent successfully\n");
        ret = 0;
    }
    else
    {
        fprintf(stderr, "\n");
        fprintf(stderr, "[ERR]  Error occured during sending.\n");
        ret = -1;
    }
    free(tmp);

    if (!DeleteFile(szTempName))
        fprintf(stderr,"[WARN] Could not remove temporary file %s\n",szTempName);

    return ret;
}


int mtp_send_file(struct mtp_info_t* mtp_info, const char* filename)
{
    wchar_t *fn;

    fn = (LPWSTR)malloc(strlen(filename)*2+1);
    mbstowcs(fn, filename, strlen(filename)*2+1);

    if (mtp_init(mtp_info) < 0) {
        fprintf(stderr,"[ERR]  Can not init MTP\n");
        return 1;
    }
    /* Scan for attached MTP devices. */
    if (mtp_scan(mtp_info) < 0)
    {
        fprintf(stderr,"[ERR]  No devices found\n");
        return 1;
    }

    fprintf(stderr, "[INFO] Sending firmware...\n");
    if (mtp_sendnk(fn, filesize(filename), &callback))
    {
        /* keep progress on screen */
        printf("\n");
        fprintf(stderr, "[INFO] Firmware sent successfully\n");
        return 0;
    }
    else
    {
        fprintf(stderr, "[ERR]  Error occured during sending.\n");
        return -1;
    }
    mtp_finished(mtp_info);
}


static int filesize(const char* filename)
{
    struct _stat sb;
    int res;

    res = _stat(filename, &sb);
    if(res == -1) {
        fprintf(stderr, "Error getting filesize!\n");
        return -1;
    }
    return sb.st_size;
}

/* Retrieve version of WMP as described in
 * http://msdn.microsoft.com/en-us/library/dd562731%28VS.85%29.aspx
 * Since we're after MTP support checking for the WMP6 key is not necessary.
 */
int mtp_wmp_version(void)
{
    DWORD ret;
    HKEY hk;
    DWORD type;
    DWORD enable = -1;
    DWORD enablelen = sizeof(DWORD);
    char buf[32];
    DWORD buflen = sizeof(buf);
    int version = 0;

    ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\Active Setup\\Installed Components\\{6BF52A52-394A-11d3-B153-00C04F79FAA6}",
        0, KEY_QUERY_VALUE, &hk);

    if(ret != ERROR_SUCCESS) {
        return 0;
    }
    type = REG_DWORD;
    ret = RegQueryValueExA(hk, "IsInstalled", NULL, &type, (LPBYTE)&enable, &enablelen);
    if(ret != ERROR_SUCCESS) {
        RegCloseKey(hk);
        return 0;
    }
    if(enable) {
        type = REG_SZ;
        ret = RegQueryValueExA(hk, "Version", NULL, &type, (LPBYTE)buf, &buflen);
    }
    if(ret == ERROR_SUCCESS) {
        /* get major version from registry value */
        buf[31] = '\0';
        version = atoi(buf);
    }
    RegCloseKey(hk);

    return version;
}

