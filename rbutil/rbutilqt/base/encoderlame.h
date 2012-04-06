/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2012 Dominik Riebeling
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef ENCODERLAME_H
#define ENCODERLAME_H

#include <QtCore>
#include "encoderbase.h"
#include "lame/lame.h"

class EncoderLame : public EncoderBase
{
    enum ESettings
    {
        LAMEVERSION,
        VOLUME,
        QUALITY,
    };

    Q_OBJECT
    public:
        EncoderLame(QObject *parent = NULL);
        bool encode(QString input,QString output);
        bool start();
        bool stop() {return true;}

        // for settings view
        bool configOk();
        void generateSettings();
        void saveSettings();

    private:
        QLibrary *lib;
        const char*(*m_get_lame_short_version)(void);
        int (*m_lame_set_out_samplerate)(lame_global_flags*, int);
        int (*m_lame_set_in_samplerate)(lame_global_flags*, int);
        int (*m_lame_set_num_channels)(lame_global_flags*, int);
        int (*m_lame_set_scale)(lame_global_flags*, float);
        int (*m_lame_set_mode)(lame_global_flags*, MPEG_mode);
        int (*m_lame_set_VBR)(lame_global_flags*, vbr_mode);
        int (*m_lame_set_VBR_quality)(lame_global_flags*, float);
        int (*m_lame_set_VBR_max_bitrate_kbps)(lame_global_flags*, int);
        int (*m_lame_set_bWriteVbrTag)(lame_global_flags*, int);
        lame_global_flags*(*m_lame_init)(void);
        int (*m_lame_init_params)(lame_global_flags*);
        int (*m_lame_encode_buffer)(lame_global_flags*, short int[], short
                int[], int, unsigned char*, int);
        int (*m_lame_encode_flush)(lame_global_flags*, unsigned char*, int);
        int (*m_lame_close)(lame_global_flags*);

        bool m_symbolsResolved;
        double m_encoderVolume;
        double m_encoderQuality;
};

#endif

