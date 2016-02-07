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
#include <QSpinBox>
#include <QFormLayout>
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
    void OnAuthorActivated(QTableWidgetItem *);
    void OnAuthorChanged(QTableWidgetItem *);

protected:
    soc_desc::soc_ref_t m_ref;
    QTableWidget *m_authors_list;
    MyTextEditor *m_desc_edit;
};

class NodeInstanceEditPanel : public QWidget
{
    Q_OBJECT
public:
    NodeInstanceEditPanel(const soc_desc::node_ref_t& ref, soc_id_t inst_id,
        QWidget *parent = 0);
    soc_id_t GetId();
    soc_desc::instance_t& GetInstance();

signals:
    void OnModified();

protected slots:
    void OnNameEdited(const QString& text);
    void OnTitleEdited(const QString& text);
    void OnDescEdited(const QString& text);
    void OnTypeChanged(int index);
    void OnAddrChanged(uint addr);
    void OnBaseChanged(uint base);
    void OnStrideChanged(uint stride);
    void OnFirstChanged(int first);
    void OnCountChanged(int count);
    void OnFormulaChanged(const QString& formula);
    void OnVariableChanged(const QString& variable);
    void OnAddressActivated(QTableWidgetItem *);
    void OnAddressChanged(QTableWidgetItem *);

protected:
    void UpdateType(int type);

    soc_desc::node_ref_t m_ref;
    soc_id_t m_id;
    QComboBox *m_type_combo;
    QWidget *m_single_group;
    QWidget *m_range_group;
    QWidget *m_formula_group;
    QWidget *m_stride_group;
    QWidget *m_list_group;
    QStyledItemDelegate *m_table_delegate;
    SocFieldEditorCreator *m_table_edit_factory;
};

class NodeEditPanel : public QWidget, public AbstractRegEditPanel
{
    Q_OBJECT
public:
    NodeEditPanel(const soc_desc::node_ref_t& ref, QWidget *parent = 0);

signals:
    void OnModified();

protected slots:
    void OnNameEdited(const QString& text);
    void OnTitleEdited(const QString& text);
    void OnDescEdited();
    void OnInstRemove(int index);
    void OnInstCreate();
    void OnInstModified();

protected:
    soc_desc::instance_t *GetInstanceById(soc_id_t id);
    soc_desc::instance_t *GetInstanceByRow(int row);
    QString GuessName();

    soc_desc::node_ref_t m_ref;
    MyTextEditor *m_desc_edit;
    YTabWidget *m_instances_tab;
};

class RegFieldEditPanel : public QWidget
{
    Q_OBJECT
public:
    RegFieldEditPanel(const soc_desc::field_ref_t& ref, QWidget *parent = 0);
    soc_desc::field_ref_t GetField();
    void UpdateWidth();
    void UpdateRange();

signals:
    void OnModified();

protected slots:
    void OnFieldNameChanged(const QString& name);
    void OnFieldRangeChanged(const QString& range);
    void OnFieldDescChanged(const QString& name);
    void OnFieldValueActivated(QTableWidgetItem *item);
    void OnFieldValueChanged(QTableWidgetItem *item);

protected:

    soc_desc::field_ref_t m_ref;
    QLineEdit *m_name_edit;
    QLineEdit *m_range_edit;
    SocBitRangeValidator *m_range_validator;
    MyTextEditor *m_desc_edit;
    QTableWidget *m_enum_table;
    SocFieldItemDelegate *m_enum_delegate;
    SocFieldEditorCreator *m_enum_editor;
};

class RegEditPanel : public QWidget, public AbstractRegEditPanel
{
    Q_OBJECT
public:
    RegEditPanel(const soc_desc::register_ref_t& ref, QWidget *parent = 0);

signals:
    void OnModified();

protected slots:
    void OnRegFieldActivated(const QModelIndex& index);
    void OnRegDisplayContextMenu(QPoint point);
    void OnRegFieldDelete();
    void OnRegFieldNew();
    void OnWidthChanged(int size);
    void OnAccessChanged(int access);
    void OnFieldModified();
    void OnDescEdited();
    void OnVariantActivated(QTableWidgetItem *item);
    void OnVariantValueChanged(QTableWidgetItem *item);
    void OnFieldRemove(int index);
    void OnFieldCreate();
    void OnBitrangeModified(int index);

protected:
    void DoModify();
    int FindFreeBit(int preferred);
    int IndexById(soc_id_t id); // tab index
    void UpdateWidthRestrictions();

    MyTextEditor *m_desc_edit;
    soc_desc::register_ref_t m_ref;
    QFont m_reg_font;
    Unscroll< YRegDisplay > *m_sexy_display2;
    RegFieldTableModel *m_value_model;
    YTabWidget *m_fields_tab;
    QTabWidget *m_view_tab;
    QTableWidget *m_variant_table;
    QAction *m_new_action;
    QAction *m_delete_action;
    QPoint m_menu_point;
    SocFieldItemDelegate *m_variant_delegate;
    SocAccessItemDelegate *m_access_delegate;
    SocFieldEditorCreator *m_variant_editor;
    QButtonGroup *m_reg_size_group;
    QButtonGroup *m_reg_access_group;
};

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
