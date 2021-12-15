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

#include <QtCore>
#include "encoderrbspeex.h"
#include "rbsettings.h"
#include "rbspeex.h"
#include "Logger.h"

EncoderRbSpeex::EncoderRbSpeex(QObject *parent) : EncoderBase(parent)
{

}

void EncoderRbSpeex::generateSettings()
{
    loadSettings();
    insertSetting(eVOLUME, new EncTtsSetting(this, EncTtsSetting::eDOUBLE,
        tr("Volume:"), volume, 0.0, 2.0));
    insertSetting(eQUALITY, new EncTtsSetting(this, EncTtsSetting::eDOUBLE,
        tr("Quality:"), quality, 0, 10.0));
    insertSetting(eCOMPLEXITY, new EncTtsSetting(this, EncTtsSetting::eINT,
        tr("Complexity:"), complexity, 0, 10));
    insertSetting(eNARROWBAND,new EncTtsSetting(this, EncTtsSetting::eBOOL,
        tr("Use Narrowband:"), narrowband));
}

void EncoderRbSpeex::saveSettings()
{
    //save settings in user config
    RbSettings::setSubValue("rbspeex",RbSettings::EncoderVolume,
                            getSetting(eVOLUME)->current().toDouble());
    RbSettings::setSubValue("rbspeex",RbSettings::EncoderQuality,
                            getSetting(eQUALITY)->current().toDouble());
    RbSettings::setSubValue("rbspeex",RbSettings::EncoderComplexity,
                            getSetting(eCOMPLEXITY)->current().toInt());
    RbSettings::setSubValue("rbspeex",RbSettings::EncoderNarrowBand,
                            getSetting(eNARROWBAND)->current().toBool());

    RbSettings::sync();
}


void EncoderRbSpeex::loadSettings(void)
{
    // try to get config from settings
    quality = RbSettings::subValue("rbspeex", RbSettings::EncoderQuality).toDouble();
    if(quality < 0) {
        quality = 8.0;
    }
    complexity = RbSettings::subValue("rbspeex", RbSettings::EncoderComplexity).toInt();
    volume = RbSettings::subValue("rbspeex", RbSettings::EncoderVolume).toDouble();
    narrowband = RbSettings::subValue("rbspeex", RbSettings::EncoderNarrowBand).toBool();
}


bool EncoderRbSpeex::start()
{

    // make sure configuration parameters are set.
    loadSettings();
    return true;
}

bool EncoderRbSpeex::encode(QString input,QString output)
{
    LOG_INFO() << "Encoding " << input << " to "<< output;
    char errstr[512];

    FILE *fin,*fout;
    if ((fin = fopen(input.toLocal8Bit(), "rb")) == nullptr) {
        LOG_ERROR() << "Error: could not open input file\n";
        return false;
    }
    if ((fout = fopen(output.toLocal8Bit(), "wb")) == nullptr) {
        LOG_ERROR() << "Error: could not open output file\n";
        fclose(fin);
        return false;
    }

    int ret = encode_file(fin, fout, quality, complexity, narrowband, volume,
                      errstr, sizeof(errstr));
    fclose(fout);
    fclose(fin);

    if (!ret) {
        /* Attempt to delete unfinished output */
        LOG_ERROR() << "Error:" << errstr;
        QFile(output).remove();
        return false;
    }
    return true;
}

bool EncoderRbSpeex::configOk()
{
    // check config. Make sure current settings are loaded.
    loadSettings();
    if(volume <= 0 || quality <= 0 || complexity <= 0)
        return false;
    else
        return true;
}

