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

DiffReport::DiffReport()
{
    for(int i = 0; i < 2; i++)
        m_backend[i] = new RamIoBackend("");
}

DiffReport::~DiffReport()
{
    for(int i = 0; i < 2; i++)
        delete m_backend[i];
}

bool DiffReport::GetStatus(const QString& path, Status& status)
{
    if(m_status.find(path) == m_status.end())
        return false;
    status = m_status[path];
    return true;
}

bool DiffReport::GetValue(const QString& path, int index, soc_word_t& value)
{
    if(index < 0 || index >= 2)
        return false;
    return m_backend[index]->ReadRegister(path, value);
}

bool DiffReport::FindSoc(Backend *b, const QString& soc_name, SocRef& ref)
{
    QList< SocRef > list = b->GetSocList();
    for(int i = 0; i < list.size(); i++)
        if(QString::fromStdString(list[i].GetSoc().name) == soc_name)
        {
            ref = list[i];
            return true;
        }
    return false;
}

void DiffReport::UpdateStatus(Status& status, const Status& update_with)
{
    if(update_with.error)
        status.error = true;
    if(update_with.diff)
        status.diff = true;
}

DiffReport::Status DiffReport::BuildReport(SocRegRef reg)
{
    QString name = QString::fromStdString("HW." + reg.GetDevAddr().name + "." +
        reg.GetRegAddr().name);
    Status st;
    soc_word_t v1, v2;
    if(m_backend[0]->ReadRegister(name, v1) && m_backend[1]->ReadRegister(name, v2))
        st.diff = (v1 != v2);
    else
        st.error = true;
    m_status[name] = st;
    return st;
}

DiffReport::Status DiffReport::BuildReport(SocDevRef dev)
{
    QString name = QString::fromStdString("HW." + dev.GetDevAddr().name);
    Status st;
    for(size_t i = 0; i < dev.GetDev().reg.size(); i++)
        for(size_t j = 0; j < dev.GetDev().reg[i].addr.size(); j++)
        {
            Status up = BuildReport(SocRegRef(dev, i, j));
            UpdateStatus(st, up);
        }
    m_status[name] = st;
    return st;
}

void DiffReport::BuildReport()
{
    Status st;
    for(size_t i = 0; i < m_soc.GetSoc().dev.size(); i++)
        for(size_t j = 0; j < m_soc.GetSoc().dev[i].addr.size(); j++)
        {
            Status up = BuildReport(SocDevRef(m_soc, i, j));
            UpdateStatus(st, up);
        }
    m_status["HW"] = st;
}

DiffReport *DiffReport::Compare(Backend *b, IoBackend *iob1, IoBackend *iob2)
{
    /* check the soc is the same and description is available */
    if(iob1->GetSocName() != iob2->GetSocName())
        return 0;
    SocRef ref;
    if(!FindSoc(b, iob1->GetSocName(), ref))
        return 0;
    DiffReport *report = new DiffReport;
    report->m_soc = ref;
    /* dump content of all registers */
    for(int i = 0; i < 2; i++)
    {
        report->m_backend[i]->DeleteAll();
        BackendHelper bh(i == 0 ? iob1 : iob2, report->m_soc);
        bh.DumpAllRegisters(report->m_backend[i]);
    }
    /* do recursive diff */
    report->BuildReport();
    return report;
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

namespace
{

enum
{
    RegTreeDevType = QTreeWidgetItem::UserType,
    RegTreeRegType,
    RegTreeSocType
};

/**
 * DiffTreeItem
 */

class DiffTreeItem : public QTreeWidgetItem
{
public:
    DiffTreeItem(const QString& string, DiffReport::Status st, int type):
        QTreeWidgetItem(QStringList(string), type), m_status(st) {}
    virtual void ApplyTheme(const RegDiffTheme& theme);
protected:
    DiffReport::Status m_status;
};

void DiffTreeItem::ApplyTheme(const RegDiffTheme& theme)
{
    if(m_status.error)
        setForeground(0, theme.error_brush);
    else if(m_status.diff)
        setForeground(0, theme.diff_brush);
    else
        setForeground(0, theme.normal_brush);
}

/**
 * SocTreeItem
 */

class SocTreeItem : public DiffTreeItem
{
public:
    SocTreeItem(const QString& string, DiffReport::Status st, const SocRef& ref)
        :DiffTreeItem(string, st, RegTreeSocType), m_ref(ref) {}

    const SocRef& GetRef() { return m_ref; }
private:
    SocRef m_ref;
};

/**
 * DevTreeItem
 */

class DevTreeItem : public DiffTreeItem
{
public:
    DevTreeItem(const QString& string, DiffReport::Status st, const SocDevRef& ref)
        :DiffTreeItem(string, st, RegTreeDevType), m_ref(ref) {}

    const SocDevRef& GetRef() { return m_ref; }
private:
    SocDevRef m_ref;
};

/**
 * RegTreeItem
 */

class RegTreeItem : public DiffTreeItem
{
public:
    RegTreeItem(const QString& string, DiffReport::Status st, const SocRegRef& ref)
        :DiffTreeItem(string, st, RegTreeRegType), m_ref(ref) {}

    const SocRegRef& GetRef() { return m_ref; }
private:
    SocRegRef m_ref;
};

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
    m_diff_report = 0;

    m_theme.normal_brush = palette().brush(QPalette::ButtonText);
    /* load theme */
    m_theme.diff_brush = Settings::Get()->value("regdiff/diff_brush",
        QBrush(QColor(0, 0, 255))).value<QBrush>();
    m_theme.error_brush = Settings::Get()->value("regdiff/error_brush",
        QBrush(QColor(255, 0, 0))).value<QBrush>();

    UpdateTabName();
    SetMessage(MessageWidget::Information, "Please select two data sources to compare.");
}

QWidget *RegDiff::GetWidget()
{
    return this;
}

RegDiff::~RegDiff()
{
    delete m_diff_report;
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
    m_reg_tree->clear();
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
    /* compute diff report */
    delete m_diff_report;
    m_diff_report = DiffReport::Compare(m_backend, m_io_backend[0], m_io_backend[1]);
    if(m_diff_report == 0)
        SetMessage(MessageWidget::Error, "An error occured when computing the difference.");
    /* update interface */
    FillRegTree();
    UpdateTabName();
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
        if(m_backend_selector[i] == sender() || m_data_sel_reload[i] == sender())
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
}

void RegDiff::FillDevSubTree(QTreeWidgetItem *_item)
{
    DevTreeItem *item = dynamic_cast< DevTreeItem* >(_item);
    const soc_dev_t& dev = item->GetRef().GetDev();
    for(size_t i = 0; i < dev.reg.size(); i++)
    {
        const soc_reg_t& reg = dev.reg[i];
        for(size_t j = 0; j < reg.addr.size(); j++)
        {
            QString name = QString::fromStdString("HW." + item->GetRef().GetDevAddr().name +
                "." + reg.addr[j].name);
            DiffReport::Status st;
            if(!m_diff_report->GetStatus(name, st))
                st.error = true;
            RegTreeItem *reg_item = new RegTreeItem(reg.addr[j].name.c_str(), st,
                SocRegRef(item->GetRef(), i, j));
            reg_item->ApplyTheme(m_theme);
            item->addChild(reg_item);
        }
    }
}

void RegDiff::FillSocSubTree(QTreeWidgetItem *_item)
{
    SocTreeItem *item = dynamic_cast< SocTreeItem* >(_item);
    const soc_t& soc = item->GetRef().GetSoc();
    for(size_t i = 0; i < soc.dev.size(); i++)
    {
        const soc_dev_t& dev = soc.dev[i];
        for(size_t j = 0; j < dev.addr.size(); j++)
        {
            QString name = QString::fromStdString("HW." + dev.addr[j].name);
            DiffReport::Status st;
            if(!m_diff_report->GetStatus(name, st))
                st.error = true;
            DevTreeItem *dev_item = new DevTreeItem(dev.addr[j].name.c_str(), st,
                SocDevRef(m_diff_report->GetSocRef(), i, j));
            dev_item->ApplyTheme(m_theme);
            FillDevSubTree(dev_item);
            item->addChild(dev_item);
        }
    }
}

void RegDiff::FillRegTree()
{
    if(!m_diff_report)
        return;
    DiffReport::Status st;
    if(!m_diff_report->GetStatus("HW", st))
        st.error = true;
    SocTreeItem *soc_item = new SocTreeItem(
        m_diff_report->GetSocRef().GetSoc().name.c_str(), st, m_diff_report->GetSocRef());
    soc_item->ApplyTheme(m_theme);
    FillSocSubTree(soc_item);
    m_reg_tree->addTopLevelItem(soc_item);
    m_reg_tree->expandItem(soc_item);
}
