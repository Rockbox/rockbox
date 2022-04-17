/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2022 Dominik Riebeling
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

// Stubs for TalkGenerator unit test.

#include "rbsettings.h"
#include "ttsbase.h"
#include "encoderbase.h"
#include "playerbuildinfo.h"

extern "C" int wavtrim(char* filename, int maxsilence, char* errstring, int errsize)
{
    (void)filename;
    (void)maxsilence;
    (void)errstring;
    (void)errsize;
    return 0;
}

static QMap<RbSettings::UserSettings, QVariant> stubUserSettings;

QVariant RbSettings::value(UserSettings setting)
{
    switch (setting)
    {
        case RbSettings::Tts:
            return QString("espeak");
        default:
            return QVariant();
    }
}

class TTSFakeEspeak : public TTSBase
{
    Q_OBJECT
public:
    TTSFakeEspeak(QObject *parent): TTSBase(parent) {}
    virtual bool start(QString *errStr) { (void)errStr; return true; }
    virtual bool stop() { return true; }
    virtual TTSStatus voice(const QString& text, const QString& wavfile, QString *errStr)
    { (void)text; (void)wavfile; (void)errStr; return NoError; }
    virtual QString voiceVendor() { return QString("DummyVendor"); }
    virtual bool configOk() { return true; }
    virtual void generateSettings() {}
    virtual void saveSettings() {}
    virtual Capabilities capabilities() { return None; }
};

TTSBase::TTSBase(QObject* parent) : EncTtsSettingInterface(parent)
{
}

TTSStatus TTSBase::voice(const QString& /*text*/, const QString& /*wavfile*/, QString* /*errStr*/)
{
    return NoError;
}

TTSBase* TTSBase::getTTS(QObject* parent, QString /*ttsName*/)
{
    return new TTSFakeEspeak(parent);
}

EncoderBase* EncoderBase::getEncoder(QObject*, QString)
{
    return nullptr;
}

QVariant PlayerBuildInfo::value(PlayerBuildInfo::DeviceInfo /*item*/, QString /*target*/)
{
    return QVariant();
}

PlayerBuildInfo* PlayerBuildInfo::infoInstance = nullptr;

PlayerBuildInfo::PlayerBuildInfo()
{
}

PlayerBuildInfo* PlayerBuildInfo::instance()
{
    if (infoInstance == nullptr) {
        infoInstance = new PlayerBuildInfo();
    }
    return infoInstance;
}

#include "stubs-talkgenerator.moc"
