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
#include <QHeaderView>
#include <QElapsedTimer>

/**
 * DiffReport
 */

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
    :QWidget(parent), RegDiffPanel(0)
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
 * SocDiffDisplayPanel
 */

SocDiffDisplayPanel::SocDiffDisplayPanel(QWidget *parent, DiffReport *report)
    :QGroupBox(parent), RegDiffPanel(report)
{
    QVBoxLayout *right_layout = new QVBoxLayout;

    m_name = new QLabel(this);
    m_name->setTextFormat(Qt::RichText);
    m_name->setText("<h1>" + QString::fromStdString(report->GetSocRef().GetSoc().name) + "</h1>");

    m_desc = new QLabel(this);
    m_name->setTextFormat(Qt::RichText);
    m_desc->setText(QString::fromStdString(report->GetSocRef().GetSoc().desc));

    right_layout->addWidget(m_name, 0);
    right_layout->addWidget(m_desc, 0);
    right_layout->addStretch(1);

    setTitle("System-on-Chip Description");
    setLayout(right_layout);
}

void SocDiffDisplayPanel::ShowDiffOnly(bool en)
{
    Q_UNUSED(en);
}

QWidget *SocDiffDisplayPanel::GetWidget()
{
    return this;
}

/**
 * DevDiffDisplayPanel
 */
DevDiffDisplayPanel::DevDiffDisplayPanel(QWidget *parent, DiffReport *report, const SocDevRef& dev_ref)
    :QGroupBox(parent), RegDiffPanel(report), m_dev(dev_ref), m_reg_font(font())
{
    QVBoxLayout *right_layout = new QVBoxLayout;
    const soc_dev_addr_t& dev_addr = m_dev.GetDevAddr();

    m_reg_font.setWeight(100);
    m_reg_font.setKerning(false);

    QString dev_name;
    dev_name.sprintf("HW_%s_BASE", dev_addr.name.c_str());

    QLabel *label_names = new QLabel("<b>" + dev_name + "</b>");
    label_names->setTextFormat(Qt::RichText);

    QLabel *label_addr = new QLabel("<b>" + QString().sprintf("0x%03x", dev_addr.addr) + "</b>");
    label_addr->setTextFormat(Qt::RichText);

    QHBoxLayout *top_layout = new QHBoxLayout;
    top_layout->addStretch();
    top_layout->addWidget(label_names);
    top_layout->addWidget(label_addr);
    top_layout->addStretch();

    m_name = new QLabel(this);
    m_name->setTextFormat(Qt::RichText);
    m_name->setText("<h1>" + QString::fromStdString(m_dev.GetDev().long_name) + "</h1>");

    m_desc = new QLabel(this);
    m_name->setTextFormat(Qt::RichText);
    m_desc->setText(QString::fromStdString(m_dev.GetDev().desc));

    right_layout->addWidget(m_name, 0);
    right_layout->addLayout(top_layout, 0);
    right_layout->addWidget(m_desc, 0);
    right_layout->addStretch(1);

    setTitle("Device Description");
    setLayout(right_layout);
}

void DevDiffDisplayPanel::ShowDiffOnly(bool en)
{
    Q_UNUSED(en);
}

QWidget *DevDiffDisplayPanel::GetWidget()
{
    return this;
}

/**
 * RegDiffDisplayPanel
 */

RegDiffDisplayPanel::RegDiffDisplayPanel(QWidget *parent, DiffReport *report, const SocRegRef& reg_ref)
    :QGroupBox(parent), RegDiffPanel(report), m_reg(reg_ref), m_reg_font(font())
{
    QVBoxLayout *right_layout = new QVBoxLayout;

    const soc_dev_addr_t& dev_addr = m_reg.GetDevAddr();
    const soc_reg_t& reg = m_reg.GetReg();
    const soc_reg_addr_t& reg_addr = m_reg.GetRegAddr();

    m_reg_font.setWeight(100);
    m_reg_font.setKerning(false);

    QString reg_name;
    reg_name.sprintf("<b>HW_%s_%s</b>", dev_addr.name.c_str(), reg_addr.name.c_str());
    QLabel *label_name = new QLabel(this);
    label_name->setTextFormat(Qt::RichText);
    label_name->setText(reg_name);

    QString str_addr = QString().sprintf("<b>0x%03x</b>", dev_addr.addr);
    QLabel *label_addr = new QLabel(this);
    label_addr->setTextFormat(Qt::RichText);
    label_addr->setText(str_addr);

    QHBoxLayout *top_layout = new QHBoxLayout;
    top_layout->addStretch();
    top_layout->addWidget(label_name);
    top_layout->addWidget(label_addr);
    top_layout->addStretch();

    m_raw_val_name = new QLabel(this);
    m_raw_val_name->setText("Raw value:");
    for(int i = 0; i < 2; i++)
    {
        m_raw_val_edit[i] = new QLineEdit(this);
        m_raw_val_edit[i]->setReadOnly(true);
        m_raw_val_edit[i]->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        m_raw_val_edit[i]->setFont(m_reg_font);
    }
    QHBoxLayout *raw_val_layout = new QHBoxLayout;
    raw_val_layout->addStretch();
    raw_val_layout->addWidget(m_raw_val_name);
    raw_val_layout->addWidget(m_raw_val_edit[0]);
    raw_val_layout->addWidget(m_raw_val_edit[1]);
    raw_val_layout->addStretch();

    m_value_table = new GrowingTableView;
    m_value_model = new RegFieldTableModel(m_value_table); // view takes ownership
    m_value_model->SetRegister(m_reg.GetReg());
    m_value_table->setModel(m_value_model);
    m_value_table->verticalHeader()->setVisible(false);
    m_value_table->resizeColumnsToContents();
    m_value_table->horizontalHeader()->setStretchLastSection(true);
    m_value_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_value_table->setAlternatingRowColors(true);

    m_sexy_display = new RegSexyDisplay(reg_ref, this);
    m_sexy_display->setFont(m_reg_font);

    m_desc = new QLabel(this);
    m_desc->setTextFormat(Qt::RichText);
    m_desc->setText(QString::fromStdString(m_reg.GetReg().desc));

    right_layout->addWidget(m_desc);
    right_layout->addLayout(top_layout);
    if(raw_val_layout)
        right_layout->addLayout(raw_val_layout);
    right_layout->addWidget(m_sexy_display);
    right_layout->addWidget(m_value_table);

    setTitle("Register Description");
    m_viewport = new QWidget;
    m_viewport->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_viewport->setLayout(right_layout);
    m_scroll = new QScrollArea;
    m_scroll->setWidget(m_viewport);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(m_scroll, 1);
    setLayout(layout);

    Reload();
}

RegDiffDisplayPanel::~RegDiffDisplayPanel()
{
}

void RegDiffDisplayPanel::ApplyTheme(const RegDiffTheme& theme)
{
    m_theme = theme;
    m_value_model->SetTheme(theme.panel);
}

void RegDiffDisplayPanel::Reload()
{
    const soc_dev_addr_t& dev_addr = m_reg.GetDevAddr();
    const soc_reg_t& reg = m_reg.GetReg();
    const soc_reg_addr_t& reg_addr = m_reg.GetRegAddr();
    soc_word_t value[2];
    bool has_value[2];
    QString name = QString("HW.%1.%2").arg(dev_addr.name.c_str()).arg(reg_addr.name.c_str());
    has_value[0] = m_report->GetValue(name, 0, value[0]);
    has_value[1] = m_report->GetValue(name, 1, value[1]);

    QVector< QVariant > values;
    for(int i = 0; i < 2; i++)
    {
        if(has_value[i])
        {
            m_raw_val_edit[i]->show();
            m_raw_val_edit[i]->setText(QString().sprintf("0x%08x", value[i]));
            values.push_back(QVariant(value[i]));
        }
        else
        {
            m_raw_val_edit[i]->setText("<Error>");
            values.push_back(QVariant());
        }
    }
    m_value_model->SetValues(values);

    m_value_table->resizeColumnsToContents();
    m_value_table->horizontalHeader()->setStretchLastSection(true);
}

QWidget *RegDiffDisplayPanel::GetWidget()
{
    return this;
}

void RegDiffDisplayPanel::ShowDiffOnly(bool en)
{
    Q_UNUSED(en);
}

/**
 * Misc
 */

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
    const RegThemeGroup *group = &theme.tree.normal;
    if(m_status.error)
        group = &theme.tree.error;
    else if(m_status.diff)
        group = &theme.tree.diff;
    setForeground(0, group->foreground);
    setBackground(0, group->background);
    setFont(0, group->font);
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
 * RegDiff
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
        QLabel *label = new QLabel(this);
        label->setTextFormat(Qt::RichText);
        label->setText(QString("<b>Input %1:</b>").arg((char)('A' + i)));
        m_backend_selector[i] = new BackendSelector(m_backend, this);
        m_backend_selector[i]->SetNothingMessage("<i>Select a data source to compare its content.</i>");
        m_data_soc_label[i] = new QLabel;
        m_data_sel_reload[i] = new QPushButton(this);
        m_data_sel_reload[i]->setIcon(QIcon::fromTheme("view-refresh"));
        m_data_sel_reload[i]->setToolTip("Reload data");
        data_sel_layout->addWidget(label);
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

    /* load theme */
    LoadTheme();

    UpdateTabName();
    m_msg_select_id = SetMessage(MessageWidget::Information,
        "Please select two data sources to compare.");
    m_msg_error_id = 0;
    connect(m_reg_tree, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
        this, SLOT(OnRegItemClicked(QTreeWidgetItem*, int)));
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

void RegDiff::LoadTheme()
{
    RegThemeGroup dflt;
    dflt.foreground = palette().brush(QPalette::ButtonText);
    dflt.background = palette().brush(QPalette::Base);
    dflt.font = font();
    /* tree */
    m_theme.tree.normal.foreground = Settings::Get()->value("regdiff/theme/tree/normal/fg",
        dflt.foreground).value<QBrush>();
    m_theme.tree.normal.background = Settings::Get()->value("regdiff/theme/tree/normal/bg",
        dflt.background).value<QBrush>();
    m_theme.tree.diff.foreground = Settings::Get()->value("regdiff/theme/tree/diff/fg",
        QBrush(QColor(255, 150, 150))).value<QBrush>();
    m_theme.tree.diff.background = Settings::Get()->value("regdiff/theme/tree/diff/bg",
        dflt.background).value<QBrush>();
    m_theme.tree.error.foreground = Settings::Get()->value("regdiff/theme/tree/error/fg",
        QBrush(QColor(150, 150, 255))).value<QBrush>();
    m_theme.tree.error.background = Settings::Get()->value("regdiff/theme/tree/error/bg",
        dflt.background).value<QBrush>();
    /* panel */
    m_theme.panel.normal.foreground = Settings::Get()->value("regdiff/theme/panel/normal/fg",
        dflt.foreground).value<QBrush>();
    m_theme.panel.normal.background = Settings::Get()->value("regdiff/theme/panel/normal/bg",
        dflt.background).value<QBrush>();
    m_theme.panel.diff.foreground = Settings::Get()->value("regdiff/theme/panel/diff/fg",
        dflt.foreground).value<QBrush>();
    m_theme.panel.diff.background = Settings::Get()->value("regdiff/theme/panel/diff/bg",
        QBrush(QColor(255, 150, 150))).value<QBrush>();
    m_theme.panel.error.foreground = Settings::Get()->value("regdiff/theme/panel/error/fg",
        dflt.foreground).value<QBrush>();
    m_theme.panel.error.background = Settings::Get()->value("regdiff/theme/panel/error/bg",
        QBrush(QColor(150, 150, 255))).value<QBrush>();

    m_theme.tree.valid = true;
    m_theme.panel.valid = true;
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
    panel->ApplyTheme(m_theme);
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
        m_msg_error_id = SetMessage(MessageWidget::Warning, "Cannot compare different SoC.");
        dont_diff = true;
    }
    /* early exit */
    if(dont_diff)
    {
        SetPanel(new EmptyRegDiffPanel);
        return;
    }
    /* Hide "Please select two SoC" message */
    HideMessage(m_msg_select_id);
    /* compute diff report */
    delete m_diff_report;
    m_diff_report = DiffReport::Compare(m_backend, m_io_backend[0], m_io_backend[1]);
    if(m_diff_report == 0)
        m_msg_error_id = SetMessage(MessageWidget::Error,
            "An error occured when computing the difference.");
    else
        HideMessage(m_msg_error_id);
    /* update interface */
    FillRegTree();
    UpdateTabName();
    OnRegItemClicked(m_reg_tree->invisibleRootItem()->child(0), 0);
}

int RegDiff::SetMessage(MessageWidget::MessageType type, const QString& msg)
{
    return m_msg->SetMessage(type, msg);
}

void RegDiff::HideMessage(int id)
{
    m_msg->HideMessage(id);
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

void RegDiff::DisplayRegister(const SocRegRef& ref)
{
    Q_UNUSED(ref);
    SetPanel(new RegDiffDisplayPanel(this, m_diff_report, ref));
}

void RegDiff::DisplayDevice(const SocDevRef& ref)
{
    Q_UNUSED(ref);
    SetPanel(new DevDiffDisplayPanel(this, m_diff_report, ref));
}

void RegDiff::DisplaySoc(const SocRef& ref)
{
    Q_UNUSED(ref);
    SetPanel(new SocDiffDisplayPanel(this, m_diff_report));
}

void RegDiff::OnRegItemClicked(QTreeWidgetItem *current, int col)
{
    Q_UNUSED(col);
    if(current == 0)
        return;
    if(current->type() == RegTreeSocType)
    {
        SocTreeItem *item = dynamic_cast< SocTreeItem * >(current);
        DisplaySoc(item->GetRef());
    }
    if(current->type() == RegTreeRegType)
    {
        RegTreeItem *item = dynamic_cast< RegTreeItem * >(current);
        DisplayRegister(item->GetRef());
    }
    else if(current->type() == RegTreeDevType)
    {
        DevTreeItem *item = dynamic_cast< DevTreeItem * >(current);
        DisplayDevice(item->GetRef());
    }
}
