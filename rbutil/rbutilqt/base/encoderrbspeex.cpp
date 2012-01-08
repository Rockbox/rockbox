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

EncoderRbSpeex::EncoderRbSpeex(QObject *parent) : EncoderBase(parent)
{

}

void EncoderRbSpeex::generateSettings()
{
    insertSetting(eVOLUME,new EncTtsSetting(this,EncTtsSetting::eDOUBLE,
        tr("Volume:"),RbSettings::subValue("rbspeex",RbSettings::EncoderVolume),1.0,10.0));
    insertSetting(eQUALITY,new EncTtsSetting(this,EncTtsSetting::eDOUBLE,
        tr("Quality:"),RbSettings::subValue("rbspeex",RbSettings::EncoderQuality),0,10.0));
    insertSetting(eCOMPLEXITY,new EncTtsSetting(this,EncTtsSetting::eINT,
        tr("Complexity:"),RbSettings::subValue("rbspeex",RbSettings::EncoderComplexity),0,10));
    insertSetting(eNARROWBAND,new EncTtsSetting(this,EncTtsSetting::eBOOL,
        tr("Use Narrowband:"),RbSettings::subValue("rbspeex",RbSettings::EncoderNarrowBand)));
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

bool EncoderRbSpeex::start()
{

    // try to get config from settings
    quality = RbSettings::subValue("rbspeex", RbSettings::EncoderQuality).toDouble();
    complexity = RbSettings::subValue("rbspeex", RbSettings::EncoderComplexity).toInt();
    volume = RbSettings::subValue("rbspeex", RbSettings::EncoderVolume).toDouble();
    narrowband = RbSettings::subValue("rbspeex", RbSettings::EncoderNarrowBand).toBool();


    return true;
}

bool EncoderRbSpeex::encode(QString input,QString output)
{
    qDebug() << "[RbSpeex] Encoding " << input << " to "<< output;
    char errstr[512];

    FILE *fin,*fout;
    if ((fin = fopen(input.toLocal8Bit(), "rb")) == NULL) {
        qDebug() << "[RbSpeex] Error: could not open input file\n";
        return false;
    }
    if ((fout = fopen(output.toLocal8Bit(), "wb")) == NULL) {
        qDebug() << "[RbSpeex] Error: could not open output file\n";
        fclose(fin);
        return false;
    }

    int ret = encode_file(fin, fout, quality, complexity, narrowband, volume,
                      errstr, sizeof(errstr));
    fclose(fout);
    fclose(fin);

    if (!ret) {
        /* Attempt to delete unfinished output */
        qDebug() << "[RbSpeex] Error:" << errstr;
        QFile(output).remove();
        return false;
    }
    return true;
}

bool EncoderRbSpeex::configOk()
{
    bool result=true;
    // check config

    if(RbSettings::subValue("rbspeex", RbSettings::EncoderVolume).toDouble() <= 0)
        result =false;

    if(RbSettings::subValue("rbspeex", RbSettings::EncoderQuality).toDouble() <= 0)
        result =false;

    if(RbSettings::subValue("rbspeex", RbSettings::EncoderComplexity).toInt() <= 0)
        result =false;

    return result;
}

