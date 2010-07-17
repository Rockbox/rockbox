/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Robert Bieber
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

#ifndef NEWPROJECTDIALOG_H
#define NEWPROJECTDIALOG_H

#include <QDialog>

namespace Ui {
    class NewProjectDialog;
}

class NewProjectDialog : public QDialog {
    Q_OBJECT
public:
    struct NewProjectInfo
    {
        QString name;
        QString path;
        QString target;
        bool sbs;
        bool wps;
        bool fms;
        bool rsbs;
        bool rwps;
        bool rfms;

        NewProjectInfo()
        {
            name = "";
            path = "";
            target = "";
            sbs = true;
            wps = true;
            fms = false;
            rsbs = false;
            rwps = false;
            rfms = false;
        }

        NewProjectInfo(const NewProjectInfo& other)
        {
            operator=(other);
        }

        const NewProjectInfo& operator=(const NewProjectInfo& other)
        {
            name = other.name;
            path = other.path;
            target = other.target;
            sbs = other.sbs;
            wps = other.wps;
            fms = other.fms;
            rsbs = other.rsbs;
            rwps = other.rwps;
            rfms = other.rfms;

            return *this;
        }
    };

    NewProjectDialog(QWidget *parent = 0);
    virtual ~NewProjectDialog();

    NewProjectInfo results(){ return status; }

public slots:
    void accept();
    void reject();

private slots:
    void browse();
    void targetChange(int target);

private:
    Ui::NewProjectDialog *ui;

    NewProjectInfo status;
};

#endif // NEWPROJECTDIALOG_H
