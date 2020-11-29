
#include "playerbuildinfo.h"
#include "rbsettings.h"
#include "rockboxinfo.h"

// not used by the test, just to make things compile.
QVariant RbSettings::value(RbSettings::UserSettings setting)
{
    (void)setting;
    return QVariant();
}


// not used in the test. If used the test will crash!
PlayerBuildInfo* PlayerBuildInfo::instance()
{
    return nullptr;
}

QVariant PlayerBuildInfo::value(PlayerBuildInfo::DeviceInfo item, QString target)
{
    (void)item;
    (void)target;
    return QVariant();
}

RockboxInfo::RockboxInfo(QString, QString)
{
}


