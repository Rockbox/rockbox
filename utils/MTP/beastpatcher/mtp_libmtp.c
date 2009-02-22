/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * 
 * $Id:$
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

#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <inttypes.h>
#include "libmtp.h"
#include "mtp_common.h"

int mtp_init(struct mtp_info_t* mtp_info)
{
    /* Fill the info struct with zeros - mainly for the strings */
    memset(mtp_info, 0, sizeof(struct mtp_info_t));

    LIBMTP_Init();

    return 0;

}

int mtp_finished(struct mtp_info_t* mtp_info)
{
    LIBMTP_Release_Device(mtp_info->device);

    return 0;
}

int mtp_scan(struct mtp_info_t* mtp_info)
{
    char* str;

    mtp_info->device = LIBMTP_Get_First_Device();

    if (mtp_info->device == NULL)
    {
        return -1;
    } 
    else 
    {
        /* NOTE: These strings are filled with zeros in mtp_init() */

        if ((str = LIBMTP_Get_Manufacturername(mtp_info->device)))
        {
            strncpy(mtp_info->manufacturer, str, sizeof(mtp_info->manufacturer)-1);
        }

        if ((str = LIBMTP_Get_Modelname(mtp_info->device)))
        {
            strncpy(mtp_info->modelname, str, sizeof(mtp_info->modelname)-1);
        }

        if ((str = LIBMTP_Get_Deviceversion(mtp_info->device)))
        {
            strncpy(mtp_info->version, str, sizeof(mtp_info->version)-1);
        }

        return 0;
    }
}

static int progress(uint64_t const sent, uint64_t const total,
                    void const *const data)
{
    (void)data;

    int percent = (sent * 100) / total;
#ifdef __WIN32__
    printf("Progress: %I64u of %I64u (%d%%)\r", sent, total, percent);
#else
    printf("Progress: %"PRIu64" of %"PRIu64" (%d%%)\r", sent, total, percent);
#endif
    fflush(stdout);
    return 0;
}


int mtp_send_firmware(struct mtp_info_t* mtp_info, unsigned char* fwbuf,
                      int fwsize)
{
    LIBMTP_file_t *genfile;
    int ret;
    size_t n;
    FILE* fwfile;

    /* Open a temporary file - this will be automatically deleted when closed */
    fwfile = tmpfile();

    if (fwfile == NULL)
    {
        fprintf(stderr,"[ERR]  Could not create temporary file.\n");
        return -1;
    }

    n = fwrite(fwbuf, 1, fwsize, fwfile);
    if ((int)n < fwsize)
    {
        fprintf(stderr,"[ERR]  Could not write to temporary file - n = %d.\n",(int)n);
        fclose(fwfile);
        return -1;
    }

    /* Reset file pointer */
    fseek(fwfile, SEEK_SET, 0);

    /* Prepare for uploading firmware */
    genfile = LIBMTP_new_file_t();
    genfile->filetype = LIBMTP_FILETYPE_FIRMWARE;
    genfile->filename = strdup("nk.bin");
    genfile->filesize = fwsize;

#ifdef OLDMTP
    ret = LIBMTP_Send_File_From_File_Descriptor(mtp_info->device, 
            fileno(fwfile), genfile, progress, NULL, 0);
#else
    ret = LIBMTP_Send_File_From_File_Descriptor(mtp_info->device, 
            fileno(fwfile), genfile, progress, NULL);
#endif

    /* Keep the progress line onscreen */
    printf("\n");

    /* NOTE: According to the docs, a value of ret != 0 means error, but libMTP
       seems to return that even when successful.  So we can't check the return
       code.
    */

    /* Cleanup */
    LIBMTP_destroy_file_t(genfile);

    /* Close the temporary file - this also deletes it. */
    fclose(fwfile);

    return 0;
}
