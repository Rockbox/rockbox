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
    virtual void OnModified(bool mod) = 0;
};

class EmptyEditPanel : public QWidget, public AbstractRegEditPanel
{
    Q_OBJECT
public:
    EmptyEditPanel(QWidget *parent);

signals:
    void OnModified(bool mod);

protected:
};

class SocEditPanel : public QWidget, public AbstractRegEditPanel
{
    Q_OBJECT
public:
    SocEditPanel(SocRef ref, QWidget *parent = 0);

signals:
    void OnModified(bool mod);

protected slots:
    void OnTextEdited();
    void OnNameEdited(const QString& text);

protected:
    SocRef m_ref;
    QGroupBox *m_name_group;
    QLineEdit *m_name_edit;
    QGroupBox *m_desc_group;
    MyTextEditor *m_desc_edit;
};

class DevEditPanel : public QWidget, public AbstractRegEditPanel
{
    Q_OBJECT
public:
    DevEditPanel(SocDevRef ref, QWidget *parent = 0);

signals:
    void OnModified(bool mod);

protected slots:
    void OnInstActivated(int row, int column);
    void OnInstChanged(int row, int column);
    void OnNameEdited(const QString& text);
    void OnLongNameEdited(const QString& text);
    void OnVersionEdited(const QString& text);
    void OnDescEdited();

protected:
    void FillRow(int row, const soc_dev_addr_t& addr);
    void CreateNewRow(int row);

    enum
    {
        DevInstDeleteType = QTableWidgetItem::UserType,
        DevInstNewType
    };

    enum
    {
        DevInstIconColumn = 0,
        DevInstNameColumn = 1,
        DevInstAddrColumn = 2,
    };

    SocDevRef m_ref;
    QGroupBox *m_name_group;
    QLineEdit *m_name_edit;
    QGroupBox *m_long_name_group;
    QLineEdit *m_long_name_edit;
    QGroupBox *m_version_group;
    QLineEdit *m_version_edit;
    QGroupBox *m_instances_group;
    QTableWidget *m_instances_table;
    QGroupBox *m_desc_group;
    MyTextEditor *m_desc_edit;
};

class RegEditPanel : public QWidget, public AbstractRegEditPanel
{
    Q_OBJECT
public:
    RegEditPanel(SocRegRef ref, QWidget *parent = 0);

signals:
    void OnModified(bool mod);

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
    void CreateNewAddrRow(int row);
    void FillRow(int row, const soc_reg_addr_t& addr);
    void UpdateFormula();
    void UpdateWarning(int row);

    enum
    {
        RegInstDeleteType = QTableWidgetItem::UserType,
        RegInstNewType
    };

    enum
    {
        RegInstIconColumn = 0,
        RegInstNameColumn,
        RegInstAddrColumn,
        RegInstNrColumns,
    };

    SocRegRef m_ref;
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
    RegSexyDisplay *m_sexy_display;
    MyTextEditor *m_desc_edit;
    QGroupBox *m_field_group;
    QTableWidget *m_field_table;
};

class FieldEditPanel : public QWidget, public AbstractRegEditPanel
{
    Q_OBJECT
public:
    FieldEditPanel(SocFieldRef ref, QWidget *parent = 0);

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
    void FillRow(int row, const soc_reg_field_value_t& val);
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

    SocFieldRef m_ref;
    QGroupBox *m_name_group;
    QLineEdit *m_name_edit;
    QGroupBox *m_bitrange_group;
    QLineEdit *m_bitrange_edit;
    QGroupBox *m_desc_group;
    MyTextEditor *m_desc_edit;
    QGroupBox *m_value_group;
    QTableWidget *m_value_table;
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
    void OnSocItemActivated(QTreeWidgetItem *current, int column);
    void OnOpen();
    void OnSave();
    void OnSaveAs();
    void OnSocModified(bool modified);
    void OnNew();
    void OnSocItemDelete();

protected:
    void LoadSocFile(const QString& filename);
    void UpdateSocFile();
    void FillSocTree();
    void FillSocTreeItem(QTreeWidgetItem *_item);
    void FillDevTreeItem(QTreeWidgetItem *_item);
    void FillRegTreeItem(QTreeWidgetItem *_item);
    void SetPanel(QWidget *panel);
    void DisplaySoc(SocRef ref);
    void DisplayDev(SocDevRef ref);
    void DisplayReg(SocRegRef ref);
    void DisplayField(SocFieldRef ref);
    bool CloseSoc();
    bool SaveSoc();
    bool SaveSocAs();
    bool SaveSocFile(const QString& filename);
    bool GetFilename(QString& filename, bool save);
    void SetModified(bool add, bool mod);
    void FixupEmptyItem(QTreeWidgetItem *item);
    void MakeItalic(QTreeWidgetItem *item, bool it);
    void AddDevice(QTreeWidgetItem *item);
    void AddRegister(QTreeWidgetItem *_item);
    void UpdateName(QTreeWidgetItem *current);
    void AddField(QTreeWidgetItem *_item);
    void CreateNewDeviceItem(QTreeWidgetItem *parent);
    void CreateNewRegisterItem(QTreeWidgetItem *parent);
    void CreateNewFieldItem(QTreeWidgetItem *parent);
    void UpdateTabName();

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
};

#endif /* REGEDIT_H */ 
