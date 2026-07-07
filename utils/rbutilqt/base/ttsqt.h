/***************************************************************************
*             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2026 by Vencislav Atanasov
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef TTSQT_H
#define TTSQT_H

#include <QtCore>
#include "ttsbase.h"

#include <QtTextToSpeech/QTextToSpeech>
#include <QtMultimedia/QAudioFormat>

class TTSQt : public TTSBase
{
    Q_OBJECT
public:
    explicit TTSQt(QObject *parent = nullptr);
    ~TTSQt() override;

    TTSStatus voice(const QString& text, const QString& wavfile, QString* errStr) override;
    bool start(QString *errStr) override;
    bool stop() override;

    QString voiceVendor(void) override;
    bool configOk() override;
    void generateSettings() override;
    void saveSettings() override;
    Capabilities capabilities() override;

private:
    QTextToSpeech *m_tts = nullptr;
    QByteArray m_audioData;
    QAudioFormat m_format;

    // Helper to write raw PCM data to a WAV file
    bool writeWavFile(const QString &filePath, const QByteArray &audioData, const QAudioFormat &format);
};

#endif // TTSQT_H
