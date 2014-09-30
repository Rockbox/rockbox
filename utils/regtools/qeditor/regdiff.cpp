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
#include "regdiff.h"

namespace
{

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
    virtual ~DiffReport();
    SocRef GetSocRef() { return m_soc; }
    /** Diff report naming convention: name based access are of the form:
     * HW[.dev[.reg]]
     * where <dev> is the device name (including index like APPUART1)
     * and <reg> is the register name (including index like PRIORITY29) */
    bool GetStatus(const QString& path, Status& status);
    /** Fetch cached value of a register, return false if value couldn't be read */
    bool GetValue(const QString& path, int index, soc_word_t& value);

    static DiffReport Compare(Backend *b, IoBackend *iob1, IoBackend *iob2);
protected:
    static bool FindSoc(Backend *b, const QString& soc_name, SocRef& ref);

    QMap< QString, soc_word_t > m_value[2];
    QMap< QString, Status > m_status;
    SocRef m_soc;
};

DiffReport::DiffReport()
{
}

DiffReport::~DiffReport()
{
}

bool DiffReport::GetStatus(const QString& path, Status& status)
{
    if(m_status.find(path) == m_status.end())
        return false;
    status = m_status[name];
    return true;
}

bool DiffReport::GetValue(const QString& path, int index, soc_word_t& value)
{
    if(index < 0 || index >= 2)
        return false;
    if(m_value[index].find(path) == m_value[index].end())
        return false;
    value = m_value[index][path];
    return true;
}

static bool FindSoc(Backend *b, const QString& soc_name, SocRef& ref)
{
    QList< SocRef > list = b->GetSocList();
    for(int i = 0; i < socs.size(); i++)
        if(list[i].GetSoc().name == soc_name)
        {
            ref = list[i];
            return true;
        }
    return false;
}

DiffReport DiffReport::Compare(Backend *b, IoBackend *iob1, IoBackend *iob2);
{
    DiffReport report;
    /* check the soc is the same and description is available */
    if(iob1->GetSocName() != iob2->GetSocName())
        return report;
    if(!FindSoc(b, iob1->GetSocName(), report.m_soc))
        return report;
    /* do recursive diff */
}

}

/**
 * EmptyRegDiffPanel
 */
EmptyRegDiffPanel::EmptyRegDiffPanel(QWidget *parent)
    :QWidget(parent)
{
    QVBoxLayout *l = new QVBoxLayout;
    l->addStretch();
    setLayout(l);
}

QWidget *EmptyRegDiffPanel::GetWidget()
{
    return this;
}

/**
 * RegTab
 */

RegDiff::RegDiff(Backend *backend, QWidget *parent)
    :QSplitter(parent), m_backend(backend)
{
    m_reg_tree = new QTreeWidget();
    m_reg_tree->setColumnCount(1);
    m_reg_tree->setHeaderLabel(QString("Name"));
    this->addWidget(m_reg_tree);
    this->setStretchFactor(0, 1);

    m_right_panel = new QVBoxLayout;
    QGroupBox *data_sel_group = new QGroupBox(QString("Data selection"));
    QVBoxLayout *data_sel_group_layout = new QVBoxLayout(this);
    data_sel_group->setLayout(data_sel_group_layout);
    for(int i = 0; i < 2; i++)
    {
        QHBoxLayout *data_sel_layout = new QHBoxLayout;
        m_backend_selector[i] = new BackendSelector(m_backend, this);
        m_backend_selector[i]->SetNothingMessage("<i>Select a data source to compare its content.</i>");
        m_data_soc_label[i] = new QLabel;
        m_data_sel_reload[i] = new QPushButton(this);
        m_data_sel_reload[i]->setIcon(QIcon::fromTheme("view-refresh"));
        m_data_sel_reload[i]->setToolTip("Reload data");
        data_sel_layout->addWidget(m_backend_selector[i]);
        data_sel_layout->addWidget(m_data_soc_label[i]);
        data_sel_layout->addWidget(m_data_sel_reload[i]);
        data_sel_group_layout->addLayout(data_sel_layout);
        m_data_soc_label[i]->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

        m_io_backend[i] = m_backend_selector[i]->GetBackend();

        SetDataSocName(i, "");
        connect(m_backend_selector[i], SIGNAL(OnSelect(IoBackend *)),
            this, SLOT(OnBackendSelect(IoBackend *)));
        connect(m_data_sel_reload[i], SIGNAL(clicked(bool)), this, 
            SLOT(OnBackendReload(bool)));
    }
    m_msg = new MessageWidget(this);
    m_right_panel->addWidget(data_sel_group, 0);
    m_right_panel->addWidget(m_msg, 0);
    m_right_content = 0;
    SetPanel(new EmptyRegDiffPanel);
    QWidget *w = new QWidget;
    w->setLayout(m_right_panel);
    this->addWidget(w);
    this->setStretchFactor(1, 2);

    UpdateTabName();
    SetMessage(MessageWidget::Information, "Please select two data sources to compare.");
}

QWidget *RegDiff::GetWidget()
{
    return this;
}

RegDiff::~RegDiff()
{
    /* backend will be deleted by backend selector */
}

bool RegDiff::Quit()
{
    return true;
}

void RegDiff::UpdateTabName()
{
    SetTabName("Register Diff");
}

void RegDiff::SetPanel(RegDiffPanel *panel)
{
    delete m_right_content;
    m_right_content = panel;
    m_right_panel->addWidget(m_right_content->GetWidget(), 1);
}

void RegDiff::OnDataChanged()
{
    /* ignore the case of an empty backend */
    bool dont_diff = false;
    for(int i = 0; i < 2; i++)
        if(!m_io_backend[i]->IsValid())
            dont_diff = true;
    /* check the two backends use the same soc */
    if(!dont_diff && m_io_backend[0]->GetSocName() != m_io_backend[1]->GetSocName())
    {
        SetMessage(MessageWidget::Warning, "Cannot compare different SoC.");
        dont_diff = true;
    }
    /* early exit */
    if(dont_diff)
    {
        SetPanel(new EmptyRegDiffPanel);
        return;
    }
    SetMessage(MessageWidget::Positive, "TODO.");
}

void RegDiff::SetMessage(MessageWidget::MessageType type, const QString& msg)
{
    m_msg->SetMessage(type, msg);
}

void RegDiff::SetDataSocName(int i, const QString& socname)
{
    if(socname.size() != 0)
    {
        m_data_soc_label[i]->setText("<a href=\"" + socname + "\">" + socname + "</a>");
        m_data_soc_label[i]->setTextFormat(Qt::RichText);
        m_data_soc_label[i]->show();
    }
    else
    {
        m_data_soc_label[i]->setText("");
        m_data_soc_label[i]->hide();
    }
}

int RegDiff::GetSender()
{
    for(int i = 0; i < 2; i++)
        if(m_backend_selector[i] == sender())
            return i;
    return -1;
}

void RegDiff::OnBackendReload(bool c)
{
    Q_UNUSED(c);
    int i = GetSender();
    if(i != -1)
    {
        m_io_backend[i]->Reload();
        OnDataChanged();
    }
}

void RegDiff::OnBackendSelect(IoBackend *backend)
{
    int i = GetSender();
    if(i == -1)
        return;
    m_io_backend[i] = backend;
    SetDataSocName(i, m_io_backend[i]->GetSocName());
    OnDataChanged();
    UpdateTabName();
}
