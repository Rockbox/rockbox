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

#include <QtCore>
#include "encoderlame.h"
#include "rbsettings.h"
#include "lame/lame.h"

/** Resolve a symbol from loaded library.
 */
#define SYMBOLRESOLVE(symbol, type) \
    do { m_##symbol = (type)lib->resolve(#symbol); \
        if(!m_##symbol) return; \
        qDebug() << "[EncoderLame] Resolved symbol " #symbol; } \
    while(0)

EncoderLame::EncoderLame(QObject *parent) : EncoderBase(parent)
{
    m_symbolsResolved = false;
    lib = new QLibrary("libmp3lame", this);

    SYMBOLRESOLVE(get_lame_short_version, const char* (*)());
    SYMBOLRESOLVE(lame_set_out_samplerate, int (*)(lame_global_flags*, int));
    SYMBOLRESOLVE(lame_set_in_samplerate, int (*)(lame_global_flags*, int));
    SYMBOLRESOLVE(lame_set_num_channels, int (*)(lame_global_flags*, int));
    SYMBOLRESOLVE(lame_set_scale, int (*)(lame_global_flags*, float));
    SYMBOLRESOLVE(lame_set_mode, int (*)(lame_global_flags*, MPEG_mode));
    SYMBOLRESOLVE(lame_set_VBR, int (*)(lame_global_flags*, vbr_mode));
    SYMBOLRESOLVE(lame_set_VBR_quality, int (*)(lame_global_flags*, float));
    SYMBOLRESOLVE(lame_set_VBR_max_bitrate_kbps, int (*)(lame_global_flags*, int));
    SYMBOLRESOLVE(lame_set_bWriteVbrTag, int (*)(lame_global_flags*, int));
    SYMBOLRESOLVE(lame_init, lame_global_flags* (*)());
    SYMBOLRESOLVE(lame_init_params, int (*)(lame_global_flags*));
    SYMBOLRESOLVE(lame_encode_buffer, int (*)(lame_global_flags*, short int*, short int*, int, unsigned char*, int));
    SYMBOLRESOLVE(lame_encode_flush, int (*)(lame_global_flags*, unsigned char*, int));
    SYMBOLRESOLVE(lame_close, int (*)(lame_global_flags*));

    qDebug() << "[EncoderLame] libmp3lame loaded:" << lib->isLoaded();

    m_encoderVolume = RbSettings::subValue("lame", RbSettings::EncoderVolume).toDouble();
    m_encoderQuality = RbSettings::subValue("lame", RbSettings::EncoderQuality).toDouble();
    m_symbolsResolved = true;
}

void EncoderLame::generateSettings()
{
    // no settings for now.
    // show lame version.
    if(m_symbolsResolved) {
        double quality = RbSettings::subValue("lame",
                    RbSettings::EncoderQuality).toDouble();
        // default quality is 0.999.
        if(quality < 0) {
            quality = 0.99;
        }
        insertSetting(LAMEVERSION, new EncTtsSetting(this, EncTtsSetting::eREADONLYSTRING,
                    tr("LAME"), QString(m_get_lame_short_version())));
        insertSetting(VOLUME, new EncTtsSetting(this, EncTtsSetting::eDOUBLE,
                    tr("Volume"),
                    RbSettings::subValue("lame", RbSettings::EncoderVolume).toDouble(),
                    0.0, 1.0));
        insertSetting(QUALITY, new EncTtsSetting(this, EncTtsSetting::eDOUBLE,
                    tr("Quality"), quality, 0.0, 1.0));
    }
    else {
        insertSetting(LAMEVERSION, new EncTtsSetting(this, EncTtsSetting::eREADONLYSTRING,
                    tr("LAME"), tr("Could not find libmp3lame!")));
    }
}

void EncoderLame::saveSettings()
{
    if(m_symbolsResolved) {
        RbSettings::setSubValue("lame", RbSettings::EncoderVolume,
                getSetting(VOLUME)->current().toDouble());
        RbSettings::setSubValue("lame", RbSettings::EncoderQuality,
                getSetting(QUALITY)->current().toDouble());
        m_encoderVolume =
            RbSettings::subValue("lame", RbSettings::EncoderVolume).toDouble();
        m_encoderQuality =
            RbSettings::subValue("lame", RbSettings::EncoderQuality).toDouble();
    }
}

bool EncoderLame::start()
{
    if(!m_symbolsResolved) {
        return false;
    }
    // try to get config from settings
    return true;
}

bool EncoderLame::encode(QString input,QString output)
{
    qDebug() << "[EncoderLame] Encoding" << QDir::cleanPath(input);
    if(!m_symbolsResolved) {
        qDebug() << "[EncoderLame] Symbols not successfully resolved, cannot run!";
        return false;
    }

    QFile fin(input);
    QFile fout(output);
    // initialize encoder
    lame_global_flags *gfp;
    unsigned char header[12];
    unsigned char chunkheader[8];
    unsigned int datalength = 0;
    unsigned int channels = 0;
    unsigned int samplerate = 0;
    unsigned int samplesize = 0;
    int num_samples = 0;
    int ret;
    unsigned char* mp3buf;
    int mp3buflen;
    short int* wavbuf;
    int wavbuflen;


    gfp = m_lame_init();
    m_lame_set_out_samplerate(gfp, 12000);      // resample to 12kHz
    // scale input volume
    m_lame_set_scale(gfp, m_encoderVolume);
    m_lame_set_mode(gfp, MONO);                 // mono output mode
    m_lame_set_VBR(gfp, vbr_default);           // enable default VBR mode
    // VBR quality
    m_lame_set_VBR_quality(gfp, m_encoderQuality);
    m_lame_set_VBR_max_bitrate_kbps(gfp, 64);   // maximum bitrate 64kbps
    m_lame_set_bWriteVbrTag(gfp, 0);            // disable LAME tag.

    if(!fin.open(QIODevice::ReadOnly)) {
        qDebug() << "[EncoderLame] Could not open input file" << input;
            return false;
    }

    // read RIFF header
    fin.read((char*)header, 12);
    if(memcmp("RIFF", header, 4) != 0) {
        qDebug() << "[EncoderLame] RIFF header not found!"
                 << header[0] << header[1] << header[2] << header[3];
        fin.close();
        return false;
    }
    if(memcmp("WAVE", &header[8], 4) != 0) {
        qDebug() << "[EncoderLame] WAVE FOURCC not found!"
                 << header[8] << header[9] << header[10] << header[11];
        fin.close();
        return false;
    }

    // search for fmt chunk
    do {
        // read fmt
        fin.read((char*)chunkheader, 8);
        int chunkdatalen = chunkheader[4] | chunkheader[5]<<8
                           | chunkheader[6]<<16 | chunkheader[7]<<24;
        if(memcmp("fmt ", chunkheader, 4) == 0) {
            // fmt found, read rest of chunk.
            // NOTE: This code ignores the format tag value.
            // Ideally this should be checked as well. However, rbspeex doesn't
            // check the format tag either when reading wave files, so if
            // problems arise we should notice pretty soon. Furthermore, the
            // input format used should be known. In case some TTS uses a
            // different wave encoding some time this needs to get adjusted.
            if(chunkdatalen < 16) {
                qDebug() << "fmt chunk too small!";
            }
            else {
                unsigned char *buf = new unsigned char[chunkdatalen];
                fin.read((char*)buf, chunkdatalen);
                channels = buf[2] | buf[3]<<8;
                samplerate = buf[4] | buf[5]<<8 | buf[6]<<16 | buf[7]<<24;
                samplesize = buf[14] | buf[15]<<8;
                delete[] buf;
            }
        }
        // read data
        else if(memcmp("data", chunkheader, 4) == 0) {
            datalength = chunkdatalen;
            break;
        }
        else {
            // unknown chunk, just skip its data.
            qDebug() << "[EncoderLame] unknown chunk, skipping."
                     << chunkheader[0] << chunkheader[1]
                     << chunkheader[2] << chunkheader[3];
            fin.seek(fin.pos() + chunkdatalen);
        }
    } while(!fin.atEnd());

    // check format
    if(channels == 0 || samplerate == 0 || samplesize == 0 || datalength == 0) {
        qDebug() << "[EncoderLame] invalid format. Channels:" << channels
                 << "Samplerate:" << samplerate << "Samplesize:" << samplesize
                 << "Data chunk length:" << datalength;
        fin.close();
        return false;
    }
    num_samples = (datalength / channels / (samplesize/8));

    // set input format values
    m_lame_set_in_samplerate(gfp, samplerate);
    m_lame_set_num_channels(gfp, channels);

    // initialize encoder.
    ret = m_lame_init_params(gfp);
    if(ret != 0) {
        qDebug() << "[EncoderLame] lame_init_params() failed with" << ret;
        fin.close();
        return false;
    }

    // we're dealing with rather small files here (100kB-ish), so don't care
    // about the possible output size and simply allocate the same number of
    // bytes the input file has. This wastes space but should be ok.
    // Put an upper limit of 8MiB.
    if(datalength > 8*1024*1024) {
        qDebug() << "[EncoderLame] Input file too large:" << datalength;
        fin.close();
        return false;
    }
    mp3buflen = datalength;
    wavbuflen = datalength;
    mp3buf = new unsigned char[mp3buflen];
    wavbuf = new short int[wavbuflen];
#if defined(Q_OS_MACX)
    // handle byte order -- the host might not be LE.
    if(samplesize == 8) {
        // no need to convert.
        fin.read((char*)wavbuf, wavbuflen);
    }
    else if(samplesize == 16) {
        // read LE 16bit words. Since the input format is either mono or
        // interleaved there's no need to care for that.
        unsigned int pos = 0;
        char word[2];
        while(pos < datalength) {
            fin.read(word, 2);
            wavbuf[pos++] = (word[0]&0xff) | ((word[1]<<8)&0xff00);
        }
    }
    else {
        qDebug() << "[EncoderLame] Unknown samplesize:" << samplesize;
        fin.close();
        delete[] mp3buf;
        delete[] wavbuf;
        return false;
    }
#else
    // all systems but OS X are considered LE.
    fin.read((char*)wavbuf, wavbuflen);
#endif
    fin.close();
    // encode data.
    fout.open(QIODevice::ReadWrite);
    ret = m_lame_encode_buffer(gfp, wavbuf, wavbuf, num_samples, mp3buf, mp3buflen);
    if(ret < 0) {
        qDebug() << "[EncoderLame] Error during encoding:" << ret;
    }
    if(fout.write((char*)mp3buf, ret) != (unsigned int)ret) {
        qDebug() << "[EncoderLame] Writing mp3 data failed!" << ret;
        fout.close();
        delete[] mp3buf;
        delete[] wavbuf;
        return false;
    }
    // flush remaining data
    ret = m_lame_encode_flush(gfp, mp3buf, mp3buflen);
    if(fout.write((char*)mp3buf, ret) != (unsigned int)ret) {
        qDebug() << "[EncoderLame] Writing final mp3 data failed!";
        fout.close();
        delete[] mp3buf;
        delete[] wavbuf;
        return false;
    }
    // shut down encoder and clean up.
    m_lame_close(gfp);
    fout.close();
    delete[] mp3buf;
    delete[] wavbuf;

    return true;
}

/** Check if the current configuration is usable.
 *  Since we're loading a library dynamically in the constructor test if that
 *  succeeded. Otherwise the "configuration" is not usable, even though the
 *  problem is not necessarily related to configuration values set by the user.
 */
bool EncoderLame::configOk()
{
    return (lib->isLoaded() && m_symbolsResolved);
}

