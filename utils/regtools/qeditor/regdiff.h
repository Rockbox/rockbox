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
#include <QScrollArea>

#include "backend.h"
#include "settings.h"
#include "mainwindow.h"
#include "utils.h"

/**
 * DiffReport
 *
 * NOTE: a diff report contains both the recursive status of the comparison
 * and the cached values for each register
 */
class DiffReport
{
public:
    struct Status
    {
        Status():diff(false),error(false) {}
        bool diff; /** true if there are some differences */
        bool error; /** true if there are some errors */
    };

    DiffReport();
    ~DiffReport();
    SocRef GetSocRef() { return m_soc; }
    /** Diff report naming convention: name based access are of the form:
     * HW[.dev[.reg]]
     * where <dev> is the device name (including index like APPUART1)
     * and <reg> is the register name (including index like PRIORITY29) */
    bool GetStatus(const QString& path, Status& status);
    /** Fetch cached value of a register, return false if value couldn't be read */
    bool GetValue(const QString& path, int index, soc_word_t& value);
    /* NOTE: return 0 on error */
    static DiffReport *Compare(Backend *b, IoBackend *iob1, IoBackend *iob2);
private:
    DiffReport(const DiffReport& o);
    DiffReport& operator=(const DiffReport& o);

protected:
    static bool FindSoc(Backend *b, const QString& soc_name, SocRef& ref);
    void BuildReport();
    Status BuildReport(SocRegRef dev);
    Status BuildReport(SocDevRef dev);
    static void UpdateStatus(Status& status, const Status& update_with);

    RamIoBackend *m_backend[2];
    QMap< QString, Status > m_status;
    SocRef m_soc;
};

struct RegDiffTheme
{
    QBrush normal_brush;
    QBrush diff_brush;
    QBrush error_brush;
};

class RegDiffPanel
{
public:
    RegDiffPanel(DiffReport *report):m_report(report) {}
    virtual ~RegDiffPanel() {}
    virtual QWidget *GetWidget() = 0;
    virtual void ShowDiffOnly(bool diff_only) = 0;

protected:
    DiffReport *m_report;
};

class EmptyRegDiffPanel : public QWidget, public RegDiffPanel
{
public:
    EmptyRegDiffPanel(QWidget *parent = 0);
    QWidget *GetWidget();
    virtual void ShowDiffOnly(bool diff_only) { Q_UNUSED(diff_only); }
};

class SocDiffDisplayPanel : public QGroupBox, public RegDiffPanel
{
    Q_OBJECT
public:
    SocDiffDisplayPanel(QWidget *parent, DiffReport *report);
    virtual QWidget *GetWidget();
    virtual void ShowDiffOnly(bool diff_only);

protected:

    QLabel *m_name;
    QLabel *m_desc;
};

class DevDiffDisplayPanel : public QGroupBox, public RegDiffPanel
{
    Q_OBJECT
public:
    DevDiffDisplayPanel(QWidget *parent, DiffReport *report, const SocDevRef& reg);
    virtual void ShowDiffOnly(bool diff_only);
    virtual QWidget *GetWidget();

protected:

    const SocDevRef& m_dev;
    QFont m_reg_font;
    QLabel *m_name;
    QLabel *m_desc;
};

class RegDiffDisplayPanel : public QGroupBox, public RegDiffPanel
{
    Q_OBJECT
public:
    RegDiffDisplayPanel(QWidget *parent, DiffReport *report, const SocRegRef& reg);
    ~RegDiffDisplayPanel();
    virtual void ShowDiffOnly(bool diff_only);
    virtual QWidget *GetWidget();

protected:
    void Reload();

    enum
    {
        FieldBitsColumn = 0,
        FieldNameColumn = 1,
        FieldValueColumn = 2,
        FieldMeaningColumn = 3,
        FieldValue2Column = 4,
        FieldMeaning2Column = 5,
        FieldDescColumn = 6,
        FieldColumnCount,
    };

    const SocRegRef& m_reg;
    QLineEdit *m_raw_val_edit[2];
    RegSexyDisplay *m_sexy_display;
    GrowingTableWidget *m_value_table;
    QStyledItemDelegate *m_table_delegate;
    QLabel *m_raw_val_name;
    QFont m_reg_font;
    QLabel *m_desc;
    QWidget *m_viewport;
    QScrollArea *m_scroll;

private slots:
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
    DiffReport *m_diff_report;
    RegDiffPanel *m_right_content;
    QVBoxLayout *m_right_panel;
    MessageWidget *m_msg;
    RegDiffTheme m_theme;
    int m_msg_select_id;
    int m_msg_error_id;

    int GetSender();
    void SetPanel(RegDiffPanel *panel);
    void OnDataChanged();
    void SetDataSocName(int i, const QString& socname);
    void UpdateTabName();
    int SetMessage(MessageWidget::MessageType type, const QString& msg);
    void HideMessage(int id);
    void FillDevSubTree(QTreeWidgetItem *item);
    void FillSocSubTree(QTreeWidgetItem *item);
    void FillRegTree();
    void DisplayRegister(const SocRegRef& ref);
    void DisplayDevice(const SocDevRef& ref);
    void DisplaySoc(const SocRef& ref);

private slots:
    void OnBackendSelect(IoBackend *backend);
    void OnBackendReload(bool c);
    void OnRegItemClicked(QTreeWidgetItem *item, int col);
};

#endif /* REGDIFF_H */