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
#ifndef _STDANALYSER_H_
#define _STDANALYSER_H_

#include "analyser.h"

#include <QGroupBox>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QToolBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include "analyser.h"

class AnalyserEx : public Analyser
{
public:
    AnalyserEx(const soc_desc::soc_ref_t& soc, IoBackend *backend);
protected:
    bool ReadRegister(const QString& path, soc_word_t& val);
    bool ReadRegisterOld(const QString& dev, const QString& reg, soc_word_t& val);
    bool ReadField(const QString& path, const QString& field, soc_word_t& val);
    bool ReadFieldOld(const QString& dev, const QString& reg, const QString& field,
        soc_word_t& val);

    BackendHelper m_helper;
};

/**
 * Clock analyser
 */

class ClockAnalyser : public AnalyserEx
{
public:
    ClockAnalyser(const soc_desc::soc_ref_t& soc, IoBackend *backend);
    virtual ~ClockAnalyser();
    virtual QWidget *GetWidget();
    static bool SupportSoc(const QString& soc_name);

private:
    QString GetFreq(unsigned freq);

    enum
    {
        DISABLED = 0,
        INVALID = -1,
        FROM_PARENT = -2,
    };

    QTreeWidgetItem *AddClock(QTreeWidgetItem *parent, const QString& name, int freq, int mul = 1, int div = 1);
    int GetClockFreq(QTreeWidgetItem *item);
    void FillTree();
    void FillTreeIMX233();
    void FillTreeRK27XX();
    void FillTreeATJ213X();
    void FillTreeJZ4760B();

private:
    QGroupBox *m_group;
    QTreeWidget *m_tree_widget;
};

/**
 * EMI analyser
 */
class EmiAnalyser : public QObject, public AnalyserEx
{
    Q_OBJECT
public:
    EmiAnalyser(const soc_desc::soc_ref_t& soc, IoBackend *backend);
    virtual ~EmiAnalyser();
    virtual QWidget *GetWidget();

    static bool SupportSoc(const QString& soc_name);

private slots:
    void OnChangeDisplayMode(int index);

private:
    enum DisplayMode
    {
        DisplayCycles,
        DisplayRawHex,
        DisplayTime,
    };

    enum
    {
        NONE = -999999,
        INVALID = -1000000
    };

    void NewGroup(const QString& name);
    void AddLine(const QString& name, int value, const QString& unit, const QString& comment = "");
    void AddCycleLine(const QString& name, unsigned raw_val, float val, int digits, const QString& comment = "");
    void FillTable();

private:
    QGroupBox *m_group;
    QComboBox *m_display_selector;
    QToolBox *m_panel;
    DisplayMode m_display_mode;
    unsigned m_emi_freq;
    QLineEdit *m_emi_freq_label;
};

/**
 * PINCTRL analyzer
 */
class PinAnalyser : public AnalyserEx
{
public:
    PinAnalyser(const soc_desc::soc_ref_t& soc, IoBackend *backend);
    virtual ~PinAnalyser();
    virtual QWidget *GetWidget();

    static bool SupportSoc(const QString& soc_name);

private:
    void FillList();

private:
    QGroupBox *m_group;
    QLineEdit *m_package_edit;
    QToolBox *m_panel;
};

#endif /* _STDANALYSER_H_ */
