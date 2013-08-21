#include <QCoreApplication>
#include <QDebug>
#include "settings.h"

Settings::Settings()
{
    
}

Settings::~Settings()
{
    if(m_settings)
        delete m_settings;
}

QSettings *Settings::GetSettings()
{
    if(!m_settings)
    {
        QDir dir(QCoreApplication::applicationDirPath());
        QString filename = dir.filePath(QCoreApplication::organizationDomain() + ".ini");
        m_settings = new QSettings(filename, QSettings::IniFormat);
    }
    return m_settings;
}

QSettings *Settings::Get()
{
    return g_settings.GetSettings();
}

Settings Settings::g_settings;