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

#ifndef BROWSEOF_H
#define BROWSEOF_H

#include <QtGui>
#include "ui_browseoffrm.h"

class BrowseOF : public QDialog
{
    Q_OBJECT

    public:
        BrowseOF(QWidget *parent = 0);
        void setFile(QString file);
        QString getFile();
        

    private slots:
        void onBrowse();
    
    private:
        Ui::BrowseOFFrm ui;

    private slots:
        void accept(void);
};

#endif
