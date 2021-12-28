/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2013 Amaury Pouly
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef MSPACKUTIL_H
#define MSPACKUTIL_H

#include <QtCore>
#include "archiveutil.h"
#include "mspack/mspack.h"

class MsPackUtil : public ArchiveUtil
{
    Q_OBJECT

    public:
        // archive types can be ORed
        MsPackUtil(QObject* parent);
        ~MsPackUtil();
        bool open(QString& mspackfile);
        virtual bool close(void);
        virtual bool extractArchive(const QString& dest, QString file = "");
        virtual QStringList files(void);

    private:
        QString errorStringMsPack(int error) const;
        struct mscab_decompressor* m_cabd;
        struct mscabd_cabinet *m_cabinet;

};
#endif

 
