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
#ifndef REGEDIT_H
#define REGEDIT_H

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
#include <QRadioButton>
#include <QButtonGroup>
#include <QDebug>
#include <QScrollArea>
#include "backend.h"
#include "settings.h"
#include "mainwindow.h"
#include "utils.h"

class AbstractRegEditPanel
{
public:
    AbstractRegEditPanel() {}
    virtual ~AbstractRegEditPanel() {}
    virtual void OnModified() = 0;
};

class EmptyEditPanel : public QWidget, public AbstractRegEditPanel
{
    Q_OBJECT
public:
    EmptyEditPanel(QWidget *parent);

signals:
    void OnModified();

protected:
};

class SocEditPanel : public QWidget, public AbstractRegEditPanel
{
    Q_OBJECT
public:
    SocEditPanel(const soc_desc::soc_ref_t& ref, QWidget *parent = 0);

signals:
    void OnModified();

protected slots:
    void OnTextEdited();
    void OnNameEdited(const QString& text);
    void OnTitleEdited(const QString& text);
    void OnVersionEdited(const QString& text);
    void OnIsaEdited(const QString& text);

protected:
    soc_desc::soc_ref_t m_ref;
    QListWidget *m_authors_list;
    MyTextEditor *m_desc_edit;
};

class NodeEditPanel : public QWidget, public AbstractRegEditPanel
{
    Q_OBJECT
public:
    NodeEditPanel(const soc_desc::node_ref_t& ref, QWidget *parent = 0);

signals:
    void OnModified();

protected slots:
    void OnInstActivated(int row, int column);
    void OnInstSelected(int row, int col, int old_row, int old_col);
    void OnNameEdited(const QString& text);
    void OnTitleEdited(const QString& text);
    void OnDescEdited();

protected:
    void FillRow(int row, const soc_desc::instance_t& addr);
    void CreateNewRow(int row);
    void FillInstInfo(const soc_desc::instance_t& inst);
    soc_desc::instance_t *GetInstanceById(soc_id_t id);
    soc_desc::instance_t *GetInstanceByRow(int row);

    soc_desc::node_ref_t m_ref;
    QGroupBox *m_instance_info_group;
    QTableWidget *m_instances_table;
    MyTextEditor *m_desc_edit;
    QLineEdit *m_inst_name_edit;
    QLineEdit *m_inst_title_edit;
    QLineEdit *m_inst_desc_edit;
};

#if 0
class RegEditPanel : public QWidget, public AbstractRegEditPanel
{
    Q_OBJECT
public:
    RegEditPanel(const soc_desc::register_ref_t& ref, QWidget *parent = 0);

signals:
    void OnModified(bool update_tree);

protected slots:
    void OnInstActivated(int row, int column);
    void OnInstChanged(int row, int column);
    void OnNameEdited(const QString& text);
    void OnDescEdited();
    void OnSctEdited(int state);
    void OnFormulaChanged(int index);
    void OnFormulaStringChanged(const QString& text);
    void OnFormulaGenerate(bool checked);

protected:
    void UpdateWarning(int row);

    soc_desc::register_ref_t m_ref;
    QGroupBox *m_name_group;
    QLineEdit *m_name_edit;
    QGroupBox *m_instances_group;
    QTableWidget *m_instances_table;
    QGroupBox *m_desc_group;
    QGroupBox *m_flags_group;
    QCheckBox *m_sct_check;
    QFont m_reg_font;
    QGroupBox *m_formula_group;
    QButtonGroup *m_formula_radio_group;
    QLabel *m_formula_type_label;
    QComboBox *m_formula_combo;
    QLineEdit *m_formula_string_edit;
    QPushButton *m_formula_string_gen;
    Unscroll< RegSexyDisplay2 > *m_sexy_display2;
    MyTextEditor *m_desc_edit;
    QGroupBox *m_field_group;
    QTableView *m_value_table;
    RegFieldTableModel *m_value_model;
    QStyledItemDelegate *m_table_delegate;
};

class FieldEditPanel : public QWidget, public AbstractRegEditPanel
{
    Q_OBJECT
public:
    FieldEditPanel(const soc_desc::field_ref_t& ref, QWidget *parent = 0);

signals:
    void OnModified(bool mod);

protected slots:
    void OnDescEdited();
    void OnNameEdited(const QString& text);
    void OnBitRangeEdited(const QString& string);
    void OnValueActivated(int row, int column);
    void OnValueChanged(int row, int column);

protected:
    void CreateNewRow(int row);
    void FillRow(int row, const soc_desc::enum_t& val);
    void UpdateWarning(int row);
    void UpdateDelegates();

    enum
    {
        FieldValueDeleteType = QTableWidgetItem::UserType,
        FieldValueNewType,
    };

    enum
    {
        FieldValueIconColumn = 0,
        FieldValueNameColumn,
        FieldValueValueColumn,
        FieldValueDescColumn,
        FieldValueNrColumns,
    };

    soc_desc::field_ref_t m_ref;
    QGroupBox *m_name_group;
    QLineEdit *m_name_edit;
    QGroupBox *m_bitrange_group;
    QLineEdit *m_bitrange_edit;
    QGroupBox *m_desc_group;
    MyTextEditor *m_desc_edit;
    QGroupBox *m_value_group;
    QTableWidget *m_value_table;
};
#endif

class RegEdit : public QWidget, public DocumentTab
{
    Q_OBJECT
public:
    RegEdit(Backend *backend, QWidget *parent = 0);
    ~RegEdit();
    virtual bool Quit();
    virtual QWidget *GetWidget();

protected slots:
    void OnSocItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void OnSocItemActivated(QTreeWidgetItem *current, int column);
    void OnOpen();
    void OnSave();
    void OnSaveAs();
    void OnSocModified();
    void OnNew();
    void OnSocItemDelete();
    void OnSocItemNew();
    void OnSocItemCreate();
    void OnSocTreeContextMenu(QPoint point);

protected:
    void LoadSocFile(const QString& filename);
    void UpdateSocFile();
    void FillSocTree();
    void FillNodeTreeItem(QTreeWidgetItem *item);
    void SetPanel(QWidget *panel);
    void DisplaySoc(const soc_desc::soc_ref_t& ref);
    void DisplayNode(const soc_desc::node_ref_t& ref);
    void DisplayReg(const soc_desc::register_ref_t& ref);
    void DisplayField(const soc_desc::field_ref_t& ref);
    bool CloseSoc();
    bool SaveSoc();
    bool SaveSocAs();
    bool SaveSocFile(const QString& filename);
    bool GetFilename(QString& filename, bool save);
    void SetModified(bool add, bool mod);
    void FixupItem(QTreeWidgetItem *item);
    QIcon GetIconFromType(int type);
    void MakeItalic(QTreeWidgetItem *item, bool it);
    void AddDevice(QTreeWidgetItem *item);
    void AddRegister(QTreeWidgetItem *_item);
    void UpdateName(QTreeWidgetItem *current);
    void AddField(QTreeWidgetItem *_item);
    void CreateNewNodeItem(QTreeWidgetItem *parent);
    void CreateNewRegisterItem(QTreeWidgetItem *parent);
    void CreateNewFieldItem(QTreeWidgetItem *parent);
    void UpdateTabName();
    bool ValidateName(const QString& name);
    int SetMessage(MessageWidget::MessageType type, const QString& msg);
    void HideMessage(int id);

    QAction *m_delete_action;
    QAction *m_new_action;
    QAction *m_create_action;
    QTreeWidgetItem *m_action_item;
    QGroupBox *m_file_group;
    QToolButton *m_file_open;
    QToolButton *m_file_save;
    QLineEdit *m_file_edit;
    QSplitter *m_splitter;
    QTreeWidget *m_soc_tree;
    Backend *m_backend;
    bool m_modified;
    SocFile m_cur_socfile;
    QWidget *m_right_panel;
    MessageWidget *m_msg;
    QVBoxLayout *m_right_panel_layout;
    int m_msg_welcome_id;
    int m_msg_name_error_id;
};

#endif /* REGEDIT_H */ 
