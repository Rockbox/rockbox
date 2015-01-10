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
#ifndef REGDISPLAYPANEL_H
#define REGDISPLAYPANEL_H

#include <QVBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QTableWidget>
#include <QStyledItemDelegate>
#include <QItemEditorCreatorBase>
#include <QTextEdit>
#include <QScrollArea>
#include <soc_desc.hpp>
#include "backend.h"
#include "settings.h"
#include "utils.h"
#include "regtab.h"

class SocDisplayPanel : public QGroupBox, public RegTabPanel
{
    Q_OBJECT
public:
    SocDisplayPanel(QWidget *parent, IoBackend *io_backend,
        const soc_desc::soc_ref_t& reg);
    void Reload();
    void AllowWrite(bool en);
    QWidget *GetWidget();
    bool Quit();

protected:

    soc_desc::soc_ref_t m_soc;
    QLabel *m_name;
    QLabel *m_desc;
};

class NodeDisplayPanel : public QGroupBox, public RegTabPanel
{
    Q_OBJECT
public:
    NodeDisplayPanel(QWidget *parent, IoBackend *io_backend,
        const soc_desc::node_inst_t& reg);
    void Reload();
    void AllowWrite(bool en);
    QWidget *GetWidget();
    bool Quit();

protected:

    soc_desc::node_inst_t m_node;
    QLabel *m_name;
    QLabel *m_node_desc;
    QLabel *m_inst_desc;
};

class RegDisplayPanel : public QGroupBox, public RegTabPanel
{
    Q_OBJECT
public:
    RegDisplayPanel(QWidget *parent, IoBackend *io_backend,
        const soc_desc::node_inst_t& reg);
    ~RegDisplayPanel();
    void AllowWrite(bool en);
    void Reload();
    QWidget *GetWidget();
    bool Quit();

protected:
    IoBackend::WriteMode EditModeToWriteMode(RegLineEdit::EditMode mode);

    enum
    {
        FieldBitsColumn = 0,
        FieldNameColumn = 1,
        FieldValueColumn = 2,
        FieldMeaningColumn = 3,
        FieldDescColumn = 4,
    };

    IoBackend *m_io_backend;
    soc_desc::node_inst_t m_node;
    bool m_allow_write;
    RegLineEdit *m_raw_val_edit;
    Unscroll< YRegDisplay > *m_sexy_display2;
    GrowingTableView *m_value_table;
    RegFieldTableModel *m_value_model;
    QStyledItemDelegate *m_table_delegate;
    QItemEditorFactory *m_table_edit_factory;
    QLabel *m_raw_val_name;
    QFont m_reg_font;
    QLabel *m_name;
    QLabel *m_node_desc;
    QLabel *m_inst_desc;
    QLabel *m_desc;
    QWidget *m_viewport;
    QScrollArea *m_scroll;

private slots:
    void OnRawRegValueReturnPressed();
    void OnRegValueChanged(int index);
    void OnRegFieldActivated(const QModelIndex& index);
};

#endif /* REGDISPLAYPANEL_H */ 
