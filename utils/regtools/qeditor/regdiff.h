/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Amaury Pouly
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
#ifndef REGDIFF_H
#define REGDIFF_H

#include <QSplitter>
#include <QTreeWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

#include "backend.h"
#include "settings.h"
#include "mainwindow.h"
#include "utils.h"

class RegDiffPanel
{
public:
    RegDiffPanel() {}
    virtual ~RegDiffPanel() {}
    virtual QWidget *GetWidget() = 0;
};

class EmptyRegDiffPanel : public QWidget, public RegDiffPanel
{
public:
    EmptyRegDiffPanel(QWidget *parent = 0);
    QWidget *GetWidget();
};

class RegDiff : public QSplitter, public DocumentTab
{
    Q_OBJECT
public:
    RegDiff(Backend *backend, QWidget *parent = 0);
    ~RegDiff();
    virtual bool Quit();
    virtual QWidget *GetWidget();

protected:
    Backend *m_backend;
    QTreeWidget *m_reg_tree;
    BackendSelector *m_backend_selector[2];
    QLabel *m_data_soc_label[2];
    QPushButton *m_data_sel_reload[2];
    IoBackend *m_io_backend[2];
    RegDiffPanel *m_right_content;
    QVBoxLayout *m_right_panel;
    MessageWidget *m_msg;

    int GetSender();
    void SetPanel(RegDiffPanel *panel);
    void OnDataChanged();
    void SetDataSocName(int i, const QString& socname);
    void UpdateTabName();
    void SetMessage(MessageWidget::MessageType type, const QString& msg);

private slots:
    void OnBackendSelect(IoBackend *backend);
    void OnBackendReload(bool c);
};

#endif /* REGDIFF_H */