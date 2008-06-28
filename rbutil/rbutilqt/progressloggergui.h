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

#include <QtGui>

#include "progressloggerinterface.h"
#include "ui_progressloggerfrm.h"

class ProgressLoggerGui :public ProgressloggerInterface
{
    Q_OBJECT
public:
    ProgressLoggerGui(QWidget * parent);

    virtual void setProgressValue(int value);
    virtual void setProgressMax(int max);
    virtual int getProgressMax();
    virtual void setProgressVisible(bool);

signals:
    virtual void aborted();
    virtual void closed();

public slots:
    virtual void addItem(const QString &text);  //! add a string to the progress list
    virtual void addItem(const QString &text, int flag);  //! add a string to the list
    virtual void setProgress(int, int); //! set progress bar

    virtual void abort();
    virtual void undoAbort();
    virtual void close();
    virtual void show();

private:
    Ui::ProgressLoggerFrm dp;
    QDialog *downloadProgress;

};

#endif

