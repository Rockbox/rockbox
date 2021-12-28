/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
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
#ifndef PROGRESSLOGGERGUI_H
#define PROGRESSLOGGERGUI_H

#include <QWidget>

#include "progressloglevels.h"
#include "ui_progressloggerfrm.h"

class ProgressLoggerGui :public QObject
{
    Q_OBJECT
public:
    ProgressLoggerGui(QWidget * parent);

    void setProgressValue(int value);
    void setProgressMax(int max);
    int getProgressMax();
    void setProgressVisible(bool);

signals:
    void aborted();
    void closed();

public slots:
    void addItem(const QString &text, int flag);  //! add a string to the list
    void setProgress(int, int); //! set progress bar

    void close();
    void show();
    void setRunning();
    void setFinished();

    void saveErrorLog();
private:
    Ui::ProgressLoggerFrm dp;
    QDialog *downloadProgress;

};

#endif

