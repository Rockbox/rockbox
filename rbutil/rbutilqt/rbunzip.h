/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2008 by Dominik Riebeling
 *   $Id$
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef RBUNZIP_H
#define RBUNZIP_H

#include <QtCore>
#include "zip/unzip.h"
#include "zip/zip.h"

class RbUnZip : public QObject, public UnZip
{
	Q_OBJECT
	public:
        UnZip::ErrorCode extractArchive(const QString&);

	signals:
		void unzipProgress(int, int);

    public slots:
        void abortUnzip(void);

    private:
        bool m_abortunzip;
};

#endif

