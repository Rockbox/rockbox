/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id$
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
 
#ifndef UNINSTALL_H
#define UNINSTALL_H
 


#include <QtGui>


#include "progressloggerinterface.h"

 
class Uninstaller : public QObject
{ 
    Q_OBJECT
public:
    Uninstaller(QObject* parent,QString mountpoint) ;
    ~Uninstaller(){}
    
    void deleteAll(ProgressloggerInterface* dp);
    void uninstall(ProgressloggerInterface* dp);
      
    bool uninstallPossible();
    
    QStringList getAllSections();

    void setSections(QStringList sections) {uninstallSections = sections;}
    
    
private slots:


private:
    bool recRmdir( QString &dirName );
    
    QString m_mountpoint;
    
    QStringList uninstallSections;

    ProgressloggerInterface* m_dp;
};
 
 

#endif

