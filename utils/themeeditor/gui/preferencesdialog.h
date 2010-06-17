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

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QPushButton>

namespace Ui {
    class PreferencesDialog;
}

class PreferencesDialog : public QDialog {
    Q_OBJECT
public:
    PreferencesDialog(QWidget *parent = 0);
    virtual ~PreferencesDialog();

    static void setButtonColor(QPushButton* button, QColor color)
    {
        QString style = "* { background:" + color.name() + "}";
        button->setStyleSheet(style);
    }

public slots:
    void accept();
    void reject();

private slots:
    void colorClicked();

private:
    Ui::PreferencesDialog *ui;

    void loadSettings();
    void loadColors();
    void loadFont();
    void saveSettings();
    void saveColors();
    void saveFont();

    void setupUI();

    QColor fgColor;
    QColor bgColor;
    QColor errorColor;
    QColor commentColor;
    QColor escapedColor;
    QColor tagColor;
    QColor conditionalColor;
};

#endif // PREFERENCESDIALOG_H
