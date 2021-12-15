/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef ENCODERRBSPEEX_H
#define ENCODERRBSPEEX_H

#include <QtCore>
#include "encoderbase.h"

class EncoderRbSpeex : public EncoderBase
{
    enum ESettings
    {
        eVOLUME,
        eQUALITY,
        eCOMPLEXITY,
        eNARROWBAND
    };

    Q_OBJECT
    public:
        EncoderRbSpeex(QObject *parent = nullptr);
        bool encode(QString input,QString output);
        bool start();
        bool stop() {return true;}

        // for settings view
        bool configOk();
        void generateSettings();
        void saveSettings();

    private:
        void loadSettings(void);
        float quality;
        float volume;
        int complexity;
        bool narrowband;

        float defaultQuality;
        float defaultVolume;
        int defaultComplexity;
        bool defaultBand;
};

#endif

