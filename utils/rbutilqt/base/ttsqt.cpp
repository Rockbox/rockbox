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

#include <QtCore>
#include "ttsbase.h"
#include "ttsqt.h"
#include "encttssettings.h"
#include "rbsettings.h"

#include <QEventLoop>
#include <QFile>
#include <QDataStream>
#include <QIODevice>
#include <QtTextToSpeech/QTextToSpeech>
#include <QtMultimedia/QAudioFormat>

#include "Logger.h"

TTSQt::TTSQt(QObject *parent) : TTSBase(parent)
{
    m_tts = new QTextToSpeech(this);
}

TTSQt::~TTSQt()
{
}

TTSStatus TTSQt::voice(const QString& text, const QString& wavfile, QString* errStr)
{
    if (!m_tts) {
        if (errStr) *errStr = "TTS engine not initialized";
        return FatalError;
    }

    if (text.isEmpty()) {
        if (errStr) *errStr = "Input text is empty";
        return Warning;
    }

    QEventLoop loop;
    bool success = true;
    QString errorMsg;
    bool hasStarted = false;

    // Connect to state changes to know when the TTS engine finishes its task
    QMetaObject::Connection conn = connect(m_tts, &QTextToSpeech::stateChanged, &loop, [&](QTextToSpeech::State state) {
        if (state == QTextToSpeech::Error) {
            success = false;
            errorMsg = m_tts->errorString();
            loop.quit();
        } else if (state != QTextToSpeech::Ready) {
            // Any state other than Ready (e.g., Speaking, Synthesizing) means it has started
            hasStarted = true;
        } else if (state == QTextToSpeech::Ready && hasStarted) {
            // It has finished and returned to Ready state
            loop.quit();
        }
    });

    if (wavfile.isEmpty()) {
        m_tts->say(text);
    }
    else {
        m_audioData.clear();
        m_format = QAudioFormat();

        // Start synthesis. The functor is called with chunks of audio data.
        m_tts->synthesize(text, [this](const QAudioFormat &format, const QByteArray &bytes) {
            if (!m_format.isValid()) {
                m_format = format; // Store the format from the first chunk
            }
            m_audioData.append(bytes);
        });
    }

    // Wait for the asynchronous operation to finish.
    // We check if it finished synchronously (edge case) to avoid deadlocking the QEventLoop.
    if (m_tts->state() == QTextToSpeech::Ready && !hasStarted) {
        // Already finished or didn't start
    } else {
        loop.exec();
    }

    disconnect(conn);

    if (!success) {
        if (errStr) *errStr = errorMsg;
        return FatalError;
    }

    if (!wavfile.isEmpty()) {
        if (m_audioData.isEmpty() || !m_format.isValid()) {
            if (errStr) *errStr = "No audio data generated or invalid format";
            return Warning;
        }

        if (!writeWavFile(wavfile, m_audioData, m_format)) {
            if (errStr) *errStr = "Failed to write WAV file";
            return FatalError;
        }
    }

    return NoError;
}

bool TTSQt::start(QString *errStr)
{
    if (!m_tts) {
        m_tts = new QTextToSpeech(this);
    }
    if (m_tts->availableEngines().isEmpty()) {
        if (errStr) *errStr = "No TTS engines available on this system";
        return false;
    }

    // XXX figure out the "best" engine.  Ignore 'mock' and
    // any anything that doesn't support file output!

    if (!(m_tts->engineCapabilities() & QTextToSpeech::Capability::Synthesize)) {
        LOG_ERROR() << "QT TTS engine '" << m_tts->engine() << " does not support synthesis to file";
        return false;
    }

    LOG_INFO() << "QT TTS engine: " << m_tts->engine();

    return true;
}

bool TTSQt::stop()
{
    if (m_tts) {
        m_tts->stop();
    }
    return true;
}

QString TTSQt::voiceVendor(void)
{
    return "Qt TextToSpeech";
}

bool TTSQt::configOk()
{
    return m_tts != nullptr && m_tts->state() != QTextToSpeech::Error;
}

void TTSQt::generateSettings()
{
    // TODO
}

void TTSQt::saveSettings()
{
    // TODO
}

TTSBase::Capabilities TTSQt::capabilities()
{
    return TTSBase::CanSpeak | TTSBase::RunInParallel;
}

bool TTSQt::writeWavFile(const QString &filePath, const QByteArray &audioData, const QAudioFormat &format)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    quint32 dataSize = audioData.size();
    quint32 fileSize = 36 + dataSize;
    quint32 fmtChunkSize = 16;

    // 3 for IEEE Float, 1 for standard PCM
    quint16 audioFormat = (format.sampleFormat() == QAudioFormat::Float) ? 3 : 1;
    quint16 channels = format.channelCount();
    quint32 sampleRate = format.sampleRate();
    quint16 bitsPerSample = format.bytesPerSample() * 8;
    quint16 blockAlign = channels * format.bytesPerSample();
    quint32 byteRate = sampleRate * blockAlign;

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    // RIFF header
    stream.writeRawData("RIFF", 4);
    stream << fileSize;
    stream.writeRawData("WAVE", 4);

    // fmt chunk
    stream.writeRawData("fmt ", 4);
    stream << fmtChunkSize;
    stream << audioFormat;
    stream << channels;
    stream << sampleRate;
    stream << byteRate;
    stream << blockAlign;
    stream << bitsPerSample;

    // data chunk
    stream.writeRawData("data", 4);
    stream << dataSize;
    stream.writeRawData(audioData.constData(), dataSize);

    file.close();
    return true;
}
