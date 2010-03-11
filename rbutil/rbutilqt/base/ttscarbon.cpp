/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2010 by Dominik Riebeling
 *   $Id$
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QtCore>
#include "ttsbase.h"
#include "ttscarbon.h"
#include "encttssettings.h"
#include "rbsettings.h"

#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <unistd.h>
#include <sys/stat.h>
#include <inttypes.h>

TTSCarbon::TTSCarbon(QObject* parent) : TTSBase(parent)
{
}


bool TTSCarbon::configOk()
{
    return true;
}


bool TTSCarbon::start(QString *errStr)
{
    (void)errStr;
    VoiceSpec vspec;
    VoiceSpec* vspecref;
    VoiceDescription vdesc;
    OSErr error;
    QString selectedVoice
            = RbSettings::subValue("carbon", RbSettings::TtsVoice).toString();
    SInt16 numVoices;
    SInt16 voiceIndex;
    error = CountVoices(&numVoices);
    for(voiceIndex = 1; voiceIndex < numVoices; ++voiceIndex) {
        error = GetIndVoice(voiceIndex, &vspec);
        error = GetVoiceDescription(&vspec, &vdesc, sizeof(vdesc));
        // name is pascal string, i.e. the first byte is the length.
        QString name = QString::fromLocal8Bit((const char*)&vdesc.name[1],
                                              vdesc.name[0]);
        if(name == selectedVoice) {
            vspecref = &vspec;
            if(vdesc.script != -1)
                m_voiceScript = (CFStringBuiltInEncodings)vdesc.script;
            else
                m_voiceScript = (CFStringBuiltInEncodings)vdesc.reserved[0];
            break;
        }
    }
    if(voiceIndex == numVoices) {
        // voice not found. Add user notification here and proceed with
        // system default voice.
        qDebug() << "selected voice not found, using system default!";
        vspecref = NULL;
        GetVoiceDescription(&vspec, &vdesc, sizeof(vdesc));
        if(vdesc.script != -1)
            m_voiceScript = (CFStringBuiltInEncodings)vdesc.script;
        else
            m_voiceScript = (CFStringBuiltInEncodings)vdesc.reserved[0];
    }

    error = NewSpeechChannel(vspecref, &m_channel);
    //SetSpeechInfo(channel, soSpeechDoneCallBack, speechDone);
    Fixed rate = (Fixed)(0x10000 * RbSettings::subValue("carbon",
                                    RbSettings::TtsSpeed).toInt());
    if(rate != 0)
        SetSpeechRate(m_channel, rate);
    return (error == 0) ? true : false;
}


bool TTSCarbon::stop(void)
{
    DisposeSpeechChannel(m_channel);
    return true;
}


void TTSCarbon::generateSettings(void)
{
    QStringList voiceNames;
    QString systemVoice;
    SInt16 numVoices;
    OSErr error;
    VoiceSpec vspec;
    VoiceDescription vdesc;

    // get system voice
    error = GetVoiceDescription(NULL, &vdesc, sizeof(vdesc));
    systemVoice
            = QString::fromLocal8Bit((const char*)&vdesc.name[1], vdesc.name[0]);
    // get list of all voices
    CountVoices(&numVoices);
    for(SInt16 i = 1; i < numVoices; ++i) {
        error = GetIndVoice(i, &vspec);
        error = GetVoiceDescription(&vspec, &vdesc, sizeof(vdesc));
        // name is pascal string, i.e. the first byte is the length.
        QString name
            = QString::fromLocal8Bit((const char*)&vdesc.name[1], vdesc.name[0]);
        voiceNames.append(name.trimmed());
    }
    // voice
    EncTtsSetting* setting;
    QString voice
        = RbSettings::subValue("carbon", RbSettings::TtsVoice).toString();
    if(voice.isEmpty())
        voice = systemVoice;
    setting = new EncTtsSetting(this, EncTtsSetting::eSTRINGLIST,
        tr("Voice:"), voice, voiceNames, EncTtsSetting::eNOBTN);
    insertSetting(ConfigVoice, setting);

    // speed
    int speed = RbSettings::subValue("carbon", RbSettings::TtsSpeed).toInt();
    setting = new EncTtsSetting(this, EncTtsSetting::eINT,
                                tr("Speed (words/min):"), speed, 80, 500,
                                EncTtsSetting::eNOBTN);
    insertSetting(ConfigSpeed, setting);
}


void TTSCarbon::saveSettings(void)
{
    // save settings in user config
    RbSettings::setSubValue("carbon", RbSettings::TtsVoice,
                            getSetting(ConfigVoice)->current().toString());
    RbSettings::setSubValue("carbon", RbSettings::TtsSpeed,
                            getSetting(ConfigSpeed)->current().toInt());
    RbSettings::sync();
}


/** @brief create wav file from text using the selected TTS voice.
  */
TTSStatus TTSCarbon::voice(QString text, QString wavfile, QString* errStr)
{
    TTSStatus status = NoError;
    OSErr error;

    QString aifffile = wavfile + ".aiff";
    // FIXME: find out why we need to do this.
    // Create a local copy of the temporary file filename.
    // Not doing so causes weird issues (path contains trailing spaces)
    unsigned int len = aifffile.size() + 1;
    char* tmpfile = (char*)malloc(len * sizeof(char));
    strncpy(tmpfile, aifffile.toLocal8Bit().constData(), len);
    CFStringRef tmpfileref = CFStringCreateWithCString(kCFAllocatorDefault,
                                tmpfile, kCFStringEncodingUTF8);
    CFURLRef urlref = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                tmpfileref, kCFURLPOSIXPathStyle, false);
    SetSpeechInfo(m_channel, soOutputToFileWithCFURL, urlref);

    // speak it.
    // Convert the string to the encoding requested by the voice. Do this
    // via CFString, as this allows to directly use the destination encoding
    // as CFString uses the same values as the voice.

    // allocate enough space to allow storing the string in a 2 byte encoding
    unsigned int textlen = 2 * text.length() + 1;
    char* textbuf = (char*)calloc(textlen, sizeof(char));
    char* utf8data = (char*)text.toUtf8().constData();
    int utf8bytes = text.toUtf8().size();
    CFStringRef cfstring = CFStringCreateWithBytes(kCFAllocatorDefault,
                            (UInt8*)utf8data, utf8bytes,
                            kCFStringEncodingUTF8, (Boolean)false);
    CFIndex usedBuf = 0;
    CFRange range;
    range.location = 0; // character in string to start.
    range.length = text.length(); // number of _characters_ in string
    // FIXME: check if converting between encodings was lossless.
    CFStringGetBytes(cfstring, range, m_voiceScript, ' ',
                     false, (UInt8*)textbuf, textlen, &usedBuf);

    error = SpeakText(m_channel, textbuf, (unsigned long)usedBuf);
    while(SpeechBusy()) {
        // FIXME: add small delay here to make calls less frequent
        QCoreApplication::processEvents();
    }
    if(error != 0) {
        *errStr = tr("Could not voice string");
        status = FatalError;
    }
    free(textbuf);
    CFRelease(cfstring);

    // convert the temporary aiff file to wav
    if(status == NoError
        && convertAiffToWav(tmpfile, wavfile.toLocal8Bit().constData()) != 0) {
        *errStr = tr("Could not convert intermediate file");
        status = FatalError;
    }
    // remove temporary aiff file
    unlink(tmpfile);
    free(tmpfile);

    return status;
}


unsigned long TTSCarbon::be2u32(unsigned char* buf)
{
    return (buf[0]&0xff)<<24 | (buf[1]&0xff)<<16 | (buf[2]&0xff)<<8 | (buf[3]&0xff);
}


unsigned long TTSCarbon::be2u16(unsigned char* buf)
{
    return buf[1]&0xff | (buf[0]&0xff)<<8;
}


unsigned char* TTSCarbon::u32tobuf(unsigned char* buf, uint32_t val)
{
    buf[0] =  val      & 0xff;
    buf[1] = (val>> 8) & 0xff;
    buf[2] = (val>>16) & 0xff;
    buf[3] = (val>>24) & 0xff;
    return buf;
}


unsigned char* TTSCarbon::u16tobuf(unsigned char* buf, uint16_t val)
{
    buf[0] =  val      & 0xff;
    buf[1] = (val>> 8) & 0xff;
    return buf;
}


/** @brief convert 80 bit extended ("long double") to int.
 *  This is simplified to handle the usual audio sample rates. Everything else
 *  might break. If the value isn't supported it will return 0.
 *  Conversion taken from Rockbox aiff codec.
 */
unsigned int TTSCarbon::extended2int(unsigned char* buf)
{
    unsigned int result = 0;
    /* value negative? */
    if(buf[0] & 0x80)
        return 0;
    /* check exponent. Int can handle up to 2^31. */
    int exponent = buf[0] << 8 | buf[1];
    if(exponent < 0x4000 || exponent > (0x4000 + 30))
        return 0;
    result = ((buf[2]<<24) | (buf[3]<<16) | (buf[4]<<8) | buf[5]) + 1;
    result >>= (16 + 14 - buf[1]);
    return result;
}


/** @brief Convert aiff file to wav. Returns 0 on success.
  */
int TTSCarbon::convertAiffToWav(const char* aiff, const char* wav)
{
    struct commchunk {
        unsigned long chunksize;
        unsigned short channels;
        unsigned long frames;
        unsigned short size;
        int rate;
    };

    struct ssndchunk {
        unsigned long chunksize;
        unsigned long offset;
        unsigned long blocksize;
    };

    FILE* in;
    FILE* out;
    unsigned char obuf[4];
    unsigned char* buf;
    /* minimum file size for a valid aiff file is 46 bytes:
     * - FORM chunk: 12 bytes
     * - COMM chunk: 18 bytes
     * - SSND chunk: 16 bytes (with no actual data)
     */
    struct stat filestat;
    stat(aiff, &filestat);
    if(filestat.st_size < 46)
        return -1;
    /* read input file into memory */
    buf = (unsigned char*)malloc(filestat.st_size * sizeof(unsigned char));
    if(!buf) /* error out if malloc() failed */
        return -1;
    in = fopen(aiff, "rb");
    if(fread(buf, 1, filestat.st_size, in) < filestat.st_size) {
        printf("could not read file: not enought bytes read\n");
        fclose(in);
        return -1;
    }
    fclose(in);

    /* check input file format */
    if(memcmp(buf, "FORM", 4) | memcmp(&buf[8], "AIFF", 4)) {
        printf("No valid AIFF header found.\n");
        free(buf);
        return -1;
    }
    /* read COMM chunk */
    unsigned char* commstart = &buf[12];
    struct commchunk comm;
    if(memcmp(commstart, "COMM", 4)) {
        printf("COMM chunk not at beginning.\n");
        free(buf);
        return -1;
    }
    comm.chunksize = be2u32(&commstart[4]);
    comm.channels  = be2u16(&commstart[8]);
    comm.frames    = be2u32(&commstart[10]);
    comm.size      = be2u16(&commstart[14]);
    comm.rate      = extended2int(&commstart[16]);

    /* find SSND as next chunk */
    unsigned char* ssndstart = commstart + 8 + comm.chunksize;
    while(memcmp(ssndstart, "SSND", 4) && ssndstart < (buf + filestat.st_size)) {
        printf("Skipping chunk.\n");
        ssndstart += be2u32(&ssndstart[4]) + 8;
    }
    if(ssndstart > (buf + filestat.st_size)) {
        free(buf);
        return -1;
    }

    struct ssndchunk ssnd;
    ssnd.chunksize = be2u32(&ssndstart[4]);
        ssnd.offset    = be2u32(&ssndstart[8]);
        ssnd.blocksize = be2u32(&ssndstart[12]);

    /* Calculate the total length of the resulting RIFF chunk.
     * The length is given by frames * samples * bytes/sample.
     * We need to add:
     * - 16 bytes: fmt chunk header
     * -  8 bytes: data chunk header
     * -  4 bytes: wave chunk identifier
     */
    out = fopen(wav, "wb+");

    /* write the wav header */
    unsigned short blocksize = comm.channels * (comm.size >> 3);
    unsigned long rifflen = blocksize * comm.frames + 28;
    fwrite("RIFF", 1, 4, out);
    fwrite(u32tobuf(obuf, rifflen), 1, 4, out);
    fwrite("WAVE", 1, 4, out);

    /* write the fmt chunk and chunk size (always 16) */
    /* write fmt chunk header:
     * header, size (always 0x10, format code (always 0x0001)
     */
    fwrite("fmt \x10\x00\x00\x00\x01\x00", 1, 10, out);
    /* number of channels (2 bytes) */
    fwrite(u16tobuf(obuf, comm.channels), 1, 2, out);
    /* sampling rate (4 bytes) */
    fwrite(u32tobuf(obuf, comm.rate), 1, 4, out);

    /* data rate, i.e. bytes/sec */
    fwrite(u32tobuf(obuf, comm.rate * blocksize), 1, 4, out);

    /* data block size */
    fwrite(u16tobuf(obuf, blocksize), 1, 2, out);

    /* bits per sample */
    fwrite(u16tobuf(obuf, comm.size), 1, 2, out);

    /* write the data chunk */
    /* chunk id */
    fwrite("data", 1, 4, out);
    /* chunk size: 4 bytes. */
    unsigned long cs = blocksize * comm.frames;
    fwrite(u32tobuf(obuf, cs), 1, 4, out);

    /* write data */
    unsigned char* data = ssndstart;
    unsigned long pos = ssnd.chunksize;
    /* byteswap if samples are 16 bit */
    if(comm.size == 16) {
        while(pos) {
            obuf[1] = *data++ & 0xff;
            obuf[0] = *data++ & 0xff;
            fwrite(obuf, 1, 2, out);
            pos -= 2;
        }
    }
    /* 8 bit samples have need no conversion so we can bulk copy.
     * Everything that is not 16 bit is considered 8. */
    else {
        fwrite(data, 1, pos, out);
    }
    /* number of bytes has to be even, even if chunksize is not. */
    if(cs % 2) {
        fwrite(obuf, 1, 1, out);
    }

    fclose(out);
    free(buf);
    return 0;
}

