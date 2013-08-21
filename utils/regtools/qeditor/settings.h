#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <QSettings>
#include <QString>
#include <QDir>

class Settings
{
public:
    Settings();
    ~Settings();

    QSettings *GetSettings();
    static QSettings *Get();
private:
    QSettings *m_settings;
    static Settings g_settings;
};

#endif /* _SETTINGS_H_ */