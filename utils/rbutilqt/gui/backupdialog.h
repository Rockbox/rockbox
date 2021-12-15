/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2012 by Dominik Riebeling
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef BACKUPDIALOG_H
#define BACKUPDIALOG_H

#include <QDialog>
#include "ui_backupdialogfrm.h"
#include "progressloggergui.h"

class BackupSizeThread;

class BackupDialog : public QDialog
{
    Q_OBJECT
    public:
        BackupDialog(QWidget* parent = nullptr);

    private slots:
        void changeBackupPath(void);
        void updateSizeInfo(void);
        void backup(void);

    private:
        Ui::BackupDialog ui;
        QString m_backupName;
        QString m_mountpoint;
        BackupSizeThread *m_thread;
        ProgressLoggerGui *m_logger;
};

#endif

