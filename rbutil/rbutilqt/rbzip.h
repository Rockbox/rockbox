/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2008 by Dominik Wenger
 *   $Id: rbzip.h 16993 2008-04-06 18:12:56Z bluebrother $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef RBZIP_H
#define RBZIP_H

#include <QtCore>
#include "zip/zip.h"

class RbZip : public QObject, public Zip
{
	Q_OBJECT
	public:
        Zip::ErrorCode createZip(QString zip,QString dir);

        virtual void progress();
        
        signals:
		void zipProgress(int, int);
    
    private:
        int m_curEntry;
        int m_numEntrys;
};

#endif

