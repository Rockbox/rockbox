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
#ifndef REGTAB_H
#define REGTAB_H

#include <QComboBox>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QSplitter>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QGroupBox>
#include <QToolButton>
#include <QMenu>
#include <QCheckBox>
#include "backend.h"
#include "settings.h"
#include "mainwindow.h"
#include "utils.h"

class RegTabPanel
{
public:
    RegTabPanel() {}
    virtual ~RegTabPanel() {}
    virtual void AllowWrite(bool en) = 0;
    virtual QWidget *GetWidget() = 0;
};

class EmptyRegTabPanel : public QWidget, public RegTabPanel
{
public:
    EmptyRegTabPanel(QWidget *parent = 0);
    void AllowWrite(bool en);
    QWidget *GetWidget();
};

class RegTab : public QSplitter, public DocumentTab
{
    Q_OBJECT
public:
    RegTab(Backend *backend, QWidget *parent = 0);
    ~RegTab();
    virtual bool Quit();
    virtual QWidget *GetWidget();

protected:
    QTreeWidgetItem *MakeNode(const soc_desc::node_inst_t& inst, const QString& s);
    soc_desc::node_inst_t NodeInst(QTreeWidgetItem *item);
    void FillSubTree(QTreeWidgetItem *item);
    void FillRegTree();
    void FillAnalyserList();
    void UpdateSocList();
    void DisplayNode(const soc_desc::node_inst_t& ref);
    void SetDataSocName(const QString& socname);
    void SetPanel(RegTabPanel *panel);
    void UpdateSocFilename();
    void UpdateTabName();
    int SetMessage(MessageWidget::MessageType type, const QString& msg);
    void HideMessage(int id);

    QComboBox *m_soc_selector;
    BackendSelector *m_backend_selector;
    Backend *m_backend;
    QTreeWidget *m_reg_tree;
    soc_desc::soc_ref_t m_cur_soc;
    QVBoxLayout *m_right_panel;
    RegTabPanel *m_right_content;
    QCheckBox *m_readonly_check;
    QLabel *m_data_soc_label;
    QPushButton *m_data_sel_reload;
    QPushButton *m_dump;
    IoBackend *m_io_backend;
    QTabWidget *m_type_selector;
    QListWidget *m_analysers_list;
    MessageWidget *m_msg;
    int m_msg_select_id;
    int m_msg_error_id;

private slots:
    void SetReadOnlyIndicator();
    void OnSocChanged(int index);
    void OnSocAdded(const SocFileRef& ref);
    void OnRegItemClicked(QTreeWidgetItem *clicked, int col);
    void OnBackendSelect(IoBackend *backend);
    void OnDataChanged();
    void OnDataSocActivated(const QString&);
    void OnAnalyserClicked(QListWidgetItem *clicked);
    void OnReadOnlyClicked(bool);
    void OnDumpRegs(bool);
    void OnBackendReload(bool);
    void OnTypeChanged(int index);
};

#endif /* REGTAB_H */