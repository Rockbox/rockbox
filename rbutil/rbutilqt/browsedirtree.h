/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Riebeling
 *   $Id$
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef BROWSEDIRTREE_H
#define BROWSEDIRTREE_H

#include <QtGui>
#include "ui_browsedirtreefrm.h"

class BrowseDirtree : public QDialog
{
    Q_OBJECT

    public:
        BrowseDirtree(QWidget *parent = 0);
        void setFilter(const QDir::Filters&);
        void setDir(const QDir&);
        void setDir(const QString&);
        QString getSelected();
        void setRoot(const QString&);

    signals:
        void itemChanged(QString);

    private:
        Ui::BrowseDirtreeFrm ui;
        QDirModel model;

    private slots:
        void accept(void);
};

#endif
