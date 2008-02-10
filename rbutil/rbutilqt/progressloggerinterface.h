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

#ifndef PROGRESSLOGGERINTERFACE_H
#define PROGRESSLOGGERINTERFACE_H

#include <QtCore>

enum {
    LOGNOICON, LOGOK, LOGINFO, LOGWARNING, LOGERROR
};



class ProgressloggerInterface : public QObject
{
   Q_OBJECT

public:
    ProgressloggerInterface(QObject* parent) : QObject(parent) {}
    virtual void addItem(const QString &text)=0;  //adds a string to the list
    virtual void addItem(const QString &text, int flag)=0;  //adds a string to the list, with icon

    virtual void setProgressValue(int value)=0;
    virtual void setProgressMax(int max)=0;
    virtual int getProgressMax()=0;

signals:
    virtual void aborted()=0;


public slots:
    virtual void abort()=0;
    virtual void undoAbort()=0;
    virtual void close()=0;
    virtual void show()=0;

private:

};

#endif

