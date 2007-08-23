/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: autodetection.cpp 14027 2007-07-27 17:42:49Z domonoky $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "autodetection.h"

Autodetection::Autodetection(QObject* parent): QObject(parent)
{

}

bool Autodetection::detect()
{
    m_device = "";
    m_mountpoint = "";
    
    // Try detection via rockbox.info
    QStringList mountpoints = getMountpoints();

    for(int i=0; i< mountpoints.size();i++)
    {
        QDir dir(mountpoints.at(i));
        if(dir.exists())
        {
            QFile file(mountpoints.at(i) + "/.rockbox/rockbox-info.txt");
            if(file.exists())
            {
                file.open(QIODevice::ReadOnly | QIODevice::Text);
                QString line = file.readLine();
                if(line.startsWith("Target: "))
                {
                    line.remove("Target: ");
                    m_device = line;
                    m_mountpoint = mountpoints.at(i);
                    return true;
                }
            }
        }
    }
    int n;
    
    //try ipodpatcher
    struct ipod_t ipod;
    n = ipod_scan(&ipod);
    if(n == 1) {
        qDebug() << "Ipod found:" << ipod.modelstr << "at" << ipod.diskname;
        m_device = ipod.targetname;
        m_mountpoint = resolveMountPoint(ipod.diskname);
        return true;
    }
    
    //try sansapatcher
    struct sansa_t sansa;
    n = sansa_scan(&sansa);
    if(n == 1) {
        qDebug() << "Sansa found:" << "sansae200" << "at" << sansa.diskname;
        m_device = "sansae200";
        m_mountpoint = resolveMountPoint(sansa.diskname);
        return true;
    }
    
    return false;
}


QStringList Autodetection::getMountpoints()
{
#if defined(Q_OS_WIN32)
    QStringList tempList;
    QFileInfoList list = QDir::drives();
    for(int i=0; i<list.size();i++)
    {
        tempList << list.at(i).absolutePath();
    }
    return tempList;
    
#elif defined(Q_OS_MACX)
    QDir dir("/Volumes");
    return dir.entryList(); 
#elif defined(Q_OS_LINUX)
    QStringList tempList;

    FILE *fp = fopen( "/proc/mounts", "r" );
    if( !fp ) return tempList;
    char *dev, *dir;
    while( fscanf( fp, "%as %as %*s %*s %*s %*s", &dev, &dir ) != EOF )
    {
        tempList << dir;
        free( dev );
        free( dir );
    }
    fclose( fp );

    return tempList;
#else
#error Unknown Plattform
#endif
}

QString Autodetection::resolveMountPoint(QString device)
{
    qDebug() << "Autodetection::resolveMountPoint(QString)" << device;
#if defined(Q_OS_LINUX)
    FILE *fp = fopen( "/proc/mounts", "r" );
    if( !fp ) return QString("");
    char *dev, *dir;
    while( fscanf( fp, "%as %as %*s %*s %*s %*s", &dev, &dir ) != EOF )
    {
        if( QString(dev).startsWith(device) )
        {
            QString directory = dir;
            free( dev );
            free( dir );
            fclose(fp);
            return directory;
        }
        free( dev );
        free( dir );
    }
    fclose( fp );
    
#endif
    return QString("");

}
