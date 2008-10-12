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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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

