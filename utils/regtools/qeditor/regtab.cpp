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
#include "regtab.h"

#include <QSizePolicy>
#include <QStringBuilder>
#include <QFileDialog>
#include <QDebug>
#include <QStyle>
#include <QMessageBox>
#include "backend.h"
#include "analyser.h"
#include "regdisplaypanel.h"

namespace
{

enum
{
    RegTreeDevType = QTreeWidgetItem::UserType,
    RegTreeRegType,
    RegTreeSocType
};

class SocTreeItem : public QTreeWidgetItem
{
public:
    SocTreeItem(const QString& string, const SocRef& ref)
        :QTreeWidgetItem(QStringList(string), RegTreeSocType), m_ref(ref) {}

    const SocRef& GetRef() { return m_ref; }
private:
    SocRef m_ref;
};

class DevTreeItem : public QTreeWidgetItem
{
public:
    DevTreeItem(const QString& string, const SocDevRef& ref)
        :QTreeWidgetItem(QStringList(string), RegTreeDevType), m_ref(ref) {}

    const SocDevRef& GetRef() { return m_ref; }
private:
    SocDevRef m_ref;
};

class RegTreeItem : public QTreeWidgetItem
{
public:
    RegTreeItem(const QString& string, const SocRegRef& ref)
        :QTreeWidgetItem(QStringList(string), RegTreeRegType), m_ref(ref) {}

    const SocRegRef& GetRef() { return m_ref; }
private:
    SocRegRef m_ref;
};

}

/**
 * EmptyRegTabPanel
 */
EmptyRegTabPanel::EmptyRegTabPanel(QWidget *parent)
    :QWidget(parent)
{
    QVBoxLayout *l = new QVBoxLayout;
    l->addStretch();
    setLayout(l);
}

void EmptyRegTabPanel::AllowWrite(bool en)
{
    Q_UNUSED(en);
}

QWidget *EmptyRegTabPanel::GetWidget()
{
    return this;
}


/**
 * RegTab
 */

RegTab::RegTab(Backend *backend, QWidget *parent)
    :QSplitter(parent), m_backend(backend)
{
    QWidget *left = new QWidget;
    this->addWidget(left);
    this->setStretchFactor(0, 1);
    QVBoxLayout *left_layout = new QVBoxLayout;
    left->setLayout(left_layout);

    QGroupBox *top_group = new QGroupBox("SOC selection");
    QVBoxLayout *top_group_layout = new QVBoxLayout;
    m_soc_selector = new QComboBox;
    top_group_layout->addWidget(m_soc_selector);
    top_group->setLayout(top_group_layout);

    m_reg_tree = new QTreeWidget();
    m_reg_tree->setColumnCount(1);
    m_reg_tree->setHeaderLabel(QString("Name"));

    m_analysers_list = new QListWidget;

    m_type_selector = new QTabWidget;
    m_type_selector->addTab(m_reg_tree, "Registers");
    m_type_selector->addTab(m_analysers_list, "Analyzers");
    m_type_selector->setTabPosition(QTabWidget::West);

    left_layout->addWidget(top_group);
    left_layout->addWidget(m_type_selector);

    m_right_panel = new QVBoxLayout;
    QGroupBox *data_sel_group = new QGroupBox("Data selection");
    QHBoxLayout *data_sel_layout = new QHBoxLayout;
    m_backend_selector = new BackendSelector(m_backend, this);
    m_backend_selector->SetNothingMessage("<i>Select a data source to analyse its content.</i>");
    m_readonly_check = new QCheckBox("Read-only");
    m_readonly_check->setCheckState(Qt::Checked);
    m_data_soc_label = new QLabel;
    m_dump = new QPushButton("Dump", this);
    m_dump->setIcon(QIcon::fromTheme("system-run"));
    m_data_sel_reload = new QPushButton(this);
    m_data_sel_reload->setIcon(QIcon::fromTheme("view-refresh"));
    m_data_sel_reload->setToolTip("Reload data");
    data_sel_layout->addWidget(m_backend_selector);
    data_sel_layout->addWidget(m_readonly_check);
    data_sel_layout->addWidget(m_data_soc_label);
    data_sel_layout->addWidget(m_dump);
    data_sel_layout->addWidget(m_data_sel_reload);
    data_sel_group->setLayout(data_sel_layout);
    m_data_soc_label->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    m_msg = new MessageWidget(this);

    m_right_panel->addWidget(data_sel_group, 0);
    m_right_panel->addWidget(m_msg, 0);
    m_right_content = 0;
    SetPanel(new EmptyRegTabPanel);
    QWidget *w = new QWidget;
    w->setLayout(m_right_panel);
    this->addWidget(w);
    this->setStretchFactor(1, 2);

    m_io_backend = m_backend_selector->GetBackend();

    connect(m_soc_selector, SIGNAL(currentIndexChanged(int)),
        this, SLOT(OnSocChanged(int)));
    connect(m_backend, SIGNAL(OnSocListChanged()), this, SLOT(OnSocListChanged()));
    connect(m_reg_tree, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
        this, SLOT(OnRegItemClicked(QTreeWidgetItem*, int)));
    connect(m_data_soc_label, SIGNAL(linkActivated(const QString&)), this,
        SLOT(OnDataSocActivated(const QString&)));
    connect(m_analysers_list, SIGNAL(itemClicked(QListWidgetItem *)),
        this, SLOT(OnAnalyserClicked(QListWidgetItem *)));
    connect(m_backend_selector, SIGNAL(OnSelect(IoBackend *)),
        this, SLOT(OnBackendSelect(IoBackend *)));
    connect(m_readonly_check, SIGNAL(clicked(bool)), this, SLOT(OnReadOnlyClicked(bool)));
    connect(m_dump, SIGNAL(clicked(bool)), this, SLOT(OnDumpRegs(bool)));
    connect(m_data_sel_reload, SIGNAL(clicked(bool)), this, SLOT(OnBackendReload(bool)));
    connect(m_type_selector, SIGNAL(currentChanged(int)), this, SLOT(OnTypeChanged(int)));

    m_msg_select_id = SetMessage(MessageWidget::Information,
        "You can browse the registers. Select a data source to analyse the values.");
    m_msg_error_id = 0;

    OnSocListChanged();
    SetDataSocName("");
    UpdateTabName();
}

QWidget *RegTab::GetWidget()
{
    return this;
}

RegTab::~RegTab()
{
    /* backend will be deleted by backend selector */
}

bool RegTab::Quit()
{
    return true;
}

void RegTab::SetDataSocName(const QString& socname)
{
    if(socname.size() != 0)
    {
        m_data_soc_label->setText("<a href=\"" + socname + "\">" + socname + "</a>");
        m_data_soc_label->setTextFormat(Qt::RichText);
        m_data_soc_label->show();
    }
    else
    {
        m_data_soc_label->setText("");
        m_data_soc_label->hide();
    }
}

void RegTab::OnDataSocActivated(const QString& str)
{
    int index = m_soc_selector->findText(str);
    if(index != -1)
        m_soc_selector->setCurrentIndex(index);
    else if(str.size() > 0)
    {
        m_msg_error_id = SetMessage(MessageWidget::Error,
            "Description file for this SoC is not available.");
        SetPanel(new EmptyRegTabPanel);
    }
}

void RegTab::UpdateTabName()
{
    /* do it the ugly way: try to cast to the different possible backends */
    FileIoBackend *file = dynamic_cast< FileIoBackend* >(m_io_backend);
#ifdef HAVE_HWSTUB
    HWStubIoBackend *hwstub = dynamic_cast< HWStubIoBackend* >(m_io_backend);
#endif
    if(file)
    {
        QFileInfo info(file->GetFileName());
        SetTabName(info.fileName());
    }
#ifdef HAVE_HWSTUB
    else if(hwstub)
    {
        HWStubDevice *dev = hwstub->GetDevice();
        SetTabName(QString("HWStub %1.%2").arg(dev->GetBusNumber())
            .arg(dev->GetDevAddress()));
    }
#endif
    else
    {
        SetTabName("Register Tab");
    }
}

void RegTab::OnBackendSelect(IoBackend *backend)
{
    /* Hide "Please select two SoC" and errors message */
    HideMessage(m_msg_select_id);
    HideMessage(m_msg_error_id);
    m_io_backend = backend;
    SetReadOnlyIndicator();
    SetDataSocName(m_io_backend->GetSocName());
    OnDataSocActivated(m_io_backend->GetSocName());
    OnDataChanged();
    UpdateTabName();
}

void RegTab::SetReadOnlyIndicator()
{
    if(m_io_backend->IsReadOnly())
        m_readonly_check->setCheckState(Qt::Checked);
}

void RegTab::OnDataChanged()
{
    OnRegItemClicked(m_reg_tree->currentItem(), 0);
}

void RegTab::OnRegItemClicked(QTreeWidgetItem *current, int col)
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

void RegTab::OnAnalyserClicked(QListWidgetItem *current)
{
    if(current == 0)
        return;
    AnalyserFactory *ana = AnalyserFactory::GetAnalyserByName(current->text());
    SetPanel(ana->Create(m_cur_soc, m_io_backend));
}

void RegTab::DisplayRegister(const SocRegRef& ref)
{
    SetPanel(new RegDisplayPanel(this, m_io_backend, ref));
}

void RegTab::DisplayDevice(const SocDevRef& ref)
{
    SetPanel(new DevDisplayPanel(this, ref));
}

void RegTab::DisplaySoc(const SocRef& ref)
{
    SetPanel(new SocDisplayPanel(this, ref));
}

int RegTab::SetMessage(MessageWidget::MessageType type, const QString& msg)
{
    return m_msg->SetMessage(type, msg);
}

void RegTab::HideMessage(int id)
{
    m_msg->HideMessage(id);
}

void RegTab::SetPanel(RegTabPanel *panel)
{
    delete m_right_content;
    m_right_content = panel;
    m_right_content->AllowWrite(m_readonly_check->checkState() == Qt::Unchecked);
    m_right_panel->addWidget(m_right_content->GetWidget(), 1);
}

void RegTab::OnSocListChanged()
{
    m_soc_selector->clear();
    QList< SocRef > socs = m_backend->GetSocList();
    for(int i = 0; i < socs.size(); i++)
    {
        QVariant v;
        v.setValue(socs[i]);
        m_soc_selector->addItem(QString::fromStdString(socs[i].GetSoc().name), v);
    }
}

void RegTab::FillDevSubTree(QTreeWidgetItem *_item)
{
    DevTreeItem *item = dynamic_cast< DevTreeItem* >(_item);
    const soc_dev_t& dev = item->GetRef().GetDev();
    for(size_t i = 0; i < dev.reg.size(); i++)
    {
        const soc_reg_t& reg = dev.reg[i];
        for(size_t j = 0; j < reg.addr.size(); j++)
        {
            RegTreeItem *reg_item = new RegTreeItem(reg.addr[j].name.c_str(),
                SocRegRef(item->GetRef(), i, j));
            item->addChild(reg_item);
        }
    }
}

void RegTab::FillSocSubTree(QTreeWidgetItem *_item)
{
    SocTreeItem *item = dynamic_cast< SocTreeItem* >(_item);
    const soc_t& soc = item->GetRef().GetSoc();
    for(size_t i = 0; i < soc.dev.size(); i++)
    {
        const soc_dev_t& dev = soc.dev[i];
        for(size_t j = 0; j < dev.addr.size(); j++)
        {
            DevTreeItem *dev_item = new DevTreeItem(dev.addr[j].name.c_str(),
                SocDevRef(m_cur_soc, i, j));
            FillDevSubTree(dev_item);
            item->addChild(dev_item);
        }
    }
}

void RegTab::FillRegTree()
{
    SocTreeItem *soc_item = new SocTreeItem(m_cur_soc.GetSoc().name.c_str(),
        m_cur_soc);
    FillSocSubTree(soc_item);
    m_reg_tree->addTopLevelItem(soc_item);
    m_reg_tree->expandItem(soc_item);
}

void RegTab::FillAnalyserList()
{
    m_analysers_list->clear();
    m_analysers_list->addItems(AnalyserFactory::GetAnalysersForSoc(m_cur_soc.GetSoc().name.c_str()));
}

void RegTab::OnSocChanged(int index)
{
    if(index == -1)
        return;
    m_reg_tree->clear();
    m_cur_soc = m_soc_selector->itemData(index).value< SocRef >();
    FillRegTree();
    FillAnalyserList();
}

void RegTab::OnReadOnlyClicked(bool checked)
{
    if(m_io_backend->IsReadOnly())
        return SetReadOnlyIndicator();
    m_right_content->AllowWrite(!checked);
}

void RegTab::OnDumpRegs(bool c)
{
    Q_UNUSED(c);
    QFileDialog *fd = new QFileDialog(this);
    fd->setAcceptMode(QFileDialog::AcceptSave);
    fd->setFilter("Textual files (*.txt);;All files (*)");
    fd->setDirectory(Settings::Get()->value("regtab/loaddatadir", QDir::currentPath()).toString());
    if(!fd->exec())
        return;
    QStringList filenames = fd->selectedFiles();
    Settings::Get()->setValue("regtab/loaddatadir", fd->directory().absolutePath());
    BackendHelper bh(m_io_backend, m_cur_soc);
    if(!bh.DumpAllRegisters(filenames[0]))
    {
        QMessageBox::warning(this, "The register dump was not saved",
            "There was an error when dumping the registers");
    }
}

void RegTab::OnBackendReload(bool c)
{
    Q_UNUSED(c);
    m_io_backend->Reload();
    OnDataChanged();
}

void RegTab::OnTypeChanged(int index)
{
    if(index == -1)
        return;
    if(index == 0) /* registers */
        OnRegItemClicked(m_reg_tree->currentItem(), 0);
    else if(index == 1) /* analysers */
        OnAnalyserClicked(m_analysers_list->currentItem());
}
