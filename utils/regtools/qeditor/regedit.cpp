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
#include "regedit.h"
#include <QFileDialog>
#include <QDebug>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QStandardItemModel>

/**
 * EmptyEditPanel
 */
EmptyEditPanel::EmptyEditPanel(QWidget *parent)
    :QWidget(parent)
{
}

/**
 * SocEditPanel
 */

namespace
{

template< typename T >
void my_remove_at(std::vector< T >& v, size_t at)
{
    v.erase(v.begin() + at);
}

enum
{
    SocEditPanelDelType = QTableWidgetItem::UserType,
    SocEditPanelAddType,
};

}

SocEditPanel::SocEditPanel(const soc_desc::soc_ref_t& ref, QWidget *parent)
    :QWidget(parent), m_ref(ref)
{
    QLineEdit *name_edit = new QLineEdit(this);
    QLineEdit *title_edit = new QLineEdit(this);
    QLineEdit *isa_edit = new QLineEdit(this);
    QLineEdit *version_edit = new QLineEdit(this);

    m_authors_list = new QTableWidget(this);
    QGroupBox *authors_group = Misc::EncloseInBox("Authors", m_authors_list);

    m_desc_edit = new MyTextEditor(this);

    QGroupBox *desc_group = Misc::EncloseInBox("Description", m_desc_edit);

    QFormLayout *banner_left_layout = new QFormLayout;
    banner_left_layout->addRow("Name:", name_edit);
    banner_left_layout->addRow("Title:", title_edit);
    banner_left_layout->addRow("Instruction Set:", isa_edit);
    banner_left_layout->addRow("Version:", version_edit);

    QGroupBox *banner_left_group = new QGroupBox("Information");
    banner_left_group->setLayout(banner_left_layout);

    QHBoxLayout *banner_layout = new QHBoxLayout;
    banner_layout->addWidget(banner_left_group);
    banner_layout->addWidget(authors_group);
    banner_layout->addStretch(0);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(banner_layout);
    layout->addWidget(desc_group);
    layout->addStretch(1);

    /* fill data */
    name_edit->setText(QString::fromStdString(ref.get()->name));
    title_edit->setText(QString::fromStdString(ref.get()->title));
    isa_edit->setText(QString::fromStdString(ref.get()->isa));
    version_edit->setText(QString::fromStdString(ref.get()->version));
    m_desc_edit->SetTextHtml(QString::fromStdString(ref.get()->desc));

    m_authors_list->setColumnCount(2);
    m_authors_list->setHorizontalHeaderItem(0, new QTableWidgetItem(""));
    m_authors_list->setHorizontalHeaderItem(1, new QTableWidgetItem("Name"));
    m_authors_list->horizontalHeader()->setVisible(false);
    m_authors_list->verticalHeader()->setVisible(false);
    m_authors_list->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    std::vector< std::string >& authors = ref.get()->author;
    m_authors_list->setRowCount(authors.size() + 1);
    for(size_t i = 0; i < authors.size(); i++)
    {
        QTableWidgetItem *item = new QTableWidgetItem(
            YIconManager::Get()->GetIcon(YIconManager::ListRemove), "", SocEditPanelDelType);
        item->setToolTip("Remove this author");
        item->setFlags(Qt::ItemIsEnabled);
        m_authors_list->setItem(i, 0, item);
        item = new QTableWidgetItem(QString::fromStdString(authors[i]));
        m_authors_list->setItem(i, 1, item);
    }
    QTableWidgetItem *new_item = new QTableWidgetItem(
        YIconManager::Get()->GetIcon(YIconManager::ListAdd), "", SocEditPanelAddType);
    new_item->setFlags(Qt::ItemIsEnabled);
    m_authors_list->setItem(authors.size(), 0, new_item);
    new_item = new QTableWidgetItem("New author...", QTableWidgetItem::UserType);
    new_item->setFlags(Qt::ItemIsEnabled);
    QFont font = new_item->font();
    font.setItalic(true);
    new_item->setFont(font);
    m_authors_list->setItem(authors.size(), 1, new_item);
    m_authors_list->resizeColumnsToContents();
    m_authors_list->horizontalHeader()->setStretchLastSection(true);

    connect(name_edit, SIGNAL(textChanged(const QString&)), this,
        SLOT(OnNameEdited(const QString&)));
    connect(m_desc_edit, SIGNAL(OnTextChanged()), this, SLOT(OnTextEdited()));
    connect(title_edit, SIGNAL(textChanged(const QString&)), this,
        SLOT(OnTitleEdited(const QString&)));
    connect(version_edit, SIGNAL(textChanged(const QString&)), this,
        SLOT(OnVersionEdited(const QString&)));
    connect(isa_edit, SIGNAL(textChanged(const QString&)), this,
        SLOT(OnIsaEdited(const QString&)));
    connect(m_authors_list, SIGNAL(itemActivated(QTableWidgetItem *)), this,
        SLOT(OnAuthorActivated(QTableWidgetItem *)));
    connect(m_authors_list, SIGNAL(itemChanged(QTableWidgetItem *)), this,
        SLOT(OnAuthorChanged(QTableWidgetItem *)));

    setLayout(layout);
}

void SocEditPanel::OnNameEdited(const QString& text)
{
    m_ref.get()->name = text.toStdString();
    OnModified();
}

void SocEditPanel::OnTitleEdited(const QString& text)
{
    m_ref.get()->title = text.toStdString();
    OnModified();
}

void SocEditPanel::OnVersionEdited(const QString& text)
{
    m_ref.get()->version = text.toStdString();
    OnModified();
}

void SocEditPanel::OnIsaEdited(const QString& text)
{
    m_ref.get()->isa = text.toStdString();
    OnModified();
}

void SocEditPanel::OnTextEdited()
{
    m_ref.get()->desc = m_desc_edit->GetTextHtml().toStdString();
    OnModified();
}

void SocEditPanel::OnAuthorActivated(QTableWidgetItem *item)
{
    if(item->type() == SocEditPanelDelType)
    {
        int row = item->row();
        my_remove_at(m_ref.get()->author, row);
        m_authors_list->removeRow(row);
        OnModified();
    }
    else if(item->type() == SocEditPanelAddType)
    {
        int row = m_ref.get()->author.size();
        m_ref.get()->author.push_back("Anonymous");
        m_authors_list->insertRow(row);
        QTableWidgetItem *item = new QTableWidgetItem(
            YIconManager::Get()->GetIcon(YIconManager::ListRemove), "", SocEditPanelDelType);
        item->setToolTip("Remove this author");
        item->setFlags(Qt::ItemIsEnabled);
        m_authors_list->setItem(row, 0, item);
        item = new QTableWidgetItem(QString::fromStdString(m_ref.get()->author.back()));
        m_authors_list->setItem(row, 1, item);
        OnModified();
    }
}

void SocEditPanel::OnAuthorChanged(QTableWidgetItem *item)
{
    if((size_t)item->row() >= m_ref.get()->author.size())
        return;
    if(item->column() == 1)
        m_ref.get()->author[item->row()] = item->text().toStdString();
    OnModified();
}

/**
 * NodeInstanceEditPanel
 */

namespace
{

template< typename T >
soc_id_t GetFreshId(const std::vector< T >& list)
{
    soc_id_t id = 0;
    for(size_t i = 0; i < list.size(); i++)
        id = std::max(id, list[i].id);
    return id + 1;
}

template< typename T >
int GetIndexById(const std::vector< T >& list, soc_id_t id)
{
    for(size_t i = 0; i < list.size(); i++)
        if(list[i].id == id)
            return i;
    return -1;
}

soc_desc::instance_t *GetInstanceById(const soc_desc::node_ref_t& node, soc_id_t id)
{
    std::vector< soc_desc::instance_t >& inst_list = node.get()->instance;
    for(size_t i = 0; i < inst_list.size(); i++)
        if(inst_list[i].id == id)
            return &inst_list[i];
    return 0;
}

bool RemoveInstanceById(const soc_desc::node_ref_t& node, soc_id_t id)
{
    std::vector< soc_desc::instance_t >& inst_list = node.get()->instance;
    for(size_t i = 0; i < inst_list.size(); i++)
        if(inst_list[i].id == id)
        {
            my_remove_at(inst_list, i);
            return true;
        }
    return false;
}

enum
{
    InstTypeSingle,
    InstTypeRangeStride,
    InstTypeRangeFormula,
    InstTypeRangeList
};

enum
{
    NodeInstEditPanelDelType = QTableWidgetItem::UserType,
    NodeInstEditPanelAddType
};

}

NodeInstanceEditPanel::NodeInstanceEditPanel(const soc_desc::node_ref_t& ref,
    soc_id_t inst_id, QWidget *parent)
    :QWidget(parent), m_ref(ref), m_id(inst_id)
{
    QLineEdit *name_edit = new QLineEdit(this);
    QLineEdit *title_edit = new QLineEdit(this);
    m_type_combo = new QComboBox(this);
    QLabel *type_label = new QLabel("Type:", this);
    QFont f = type_label->font();
    f.setBold(true);
    type_label->setFont(f);

    QHBoxLayout *type_layout = new QHBoxLayout;
    type_layout->addWidget(type_label);
    type_layout->addWidget(m_type_combo);
    type_layout->addStretch(0);

    soc_desc::field_t fake_field;
    fake_field.pos = 0;
    fake_field.width = 32;

    m_single_group = new QWidget(this);
    QHBoxLayout *sg_layout = new QHBoxLayout;
    sg_layout->addWidget(new QLabel("Address:", m_single_group));
    SocFieldEditor *addr_edit = new SocFieldEditor(fake_field, m_single_group);
    sg_layout->addWidget(addr_edit);
    m_single_group->setLayout(sg_layout);

    m_range_group = new QWidget(this);
    QGridLayout *rg_layout = new QGridLayout;
    rg_layout->addWidget(new QLabel("First:", m_range_group), 0, 0);
    QSpinBox *first_spin = new QSpinBox(m_range_group);
    rg_layout->addWidget(first_spin, 0, 1);
    rg_layout->addWidget(new QLabel("Count:", m_range_group), 1, 0);
    QSpinBox *count_spin = new QSpinBox(m_range_group);
    rg_layout->addWidget(count_spin, 1, 1);
    m_range_group->setLayout(rg_layout);

    m_stride_group = new QWidget(m_range_group);
    QGridLayout *rsg_layout = new QGridLayout;
    rsg_layout->addWidget(new QLabel("Base:", m_stride_group), 0, 0);
    SocFieldEditor *base_edit = new SocFieldEditor(fake_field, m_stride_group);
    rsg_layout->addWidget(base_edit, 0, 1);
    rsg_layout->addWidget(new QLabel("Stride:", m_stride_group), 1, 0);
    SocFieldEditor *stride_edit = new SocFieldEditor(fake_field, m_stride_group);
    rsg_layout->addWidget(stride_edit, 1, 1);
    m_stride_group->setLayout(rsg_layout);

    m_formula_group = new QWidget(m_range_group);
    QGridLayout *fsg_layout = new QGridLayout;
    fsg_layout->addWidget(new QLabel("Variable:", m_formula_group), 0, 0);
    QLineEdit *variable_edit = new QLineEdit(m_formula_group);
    fsg_layout->addWidget(variable_edit, 0, 1);
    fsg_layout->addWidget(new QLabel("Formula:", m_formula_group), 1, 0);
    QLineEdit *formula_edit = new QLineEdit(m_formula_group);
    fsg_layout->addWidget(formula_edit, 1, 1);
    m_formula_group->setLayout(fsg_layout);

    QTableWidget *addr_list = new QTableWidget(m_range_group);
    m_list_group = addr_list;

    QHBoxLayout *inst_layout = new QHBoxLayout;
    inst_layout->addWidget(m_single_group);
    inst_layout->addWidget(m_range_group);
    inst_layout->addWidget(m_stride_group);
    inst_layout->addWidget(m_formula_group);
    inst_layout->addWidget(m_list_group);
    inst_layout->addStretch(0);

    QGroupBox *inst_groupbox = new QGroupBox(this);
    inst_groupbox->setLayout(inst_layout);

    MyTextEditor *desc_edit = new MyTextEditor(this);
    QVBoxLayout *ii_layout = new QVBoxLayout;

    QFormLayout *info_layout = new QFormLayout();
    info_layout->addRow("Name", name_edit);
    info_layout->addRow("Title", title_edit);

    QGroupBox *info_group = Misc::EncloseInBox("Information", info_layout);
    QGroupBox *desc_group = Misc::EncloseInBox("Description", desc_edit);
    QHBoxLayout *name_title_desc_layout = new QHBoxLayout;
    name_title_desc_layout->addWidget(info_group, 1);
    name_title_desc_layout->addWidget(desc_group, 2);

    ii_layout->addLayout(name_title_desc_layout);
    ii_layout->addLayout(type_layout);
    ii_layout->addWidget(inst_groupbox);
    ii_layout->addStretch(1);

    m_type_combo->addItem("Single", QVariant(InstTypeSingle));
    m_type_combo->addItem("Range > Stride", QVariant(InstTypeRangeStride));
    m_type_combo->addItem("Range > Formula", QVariant(InstTypeRangeFormula));
    m_type_combo->addItem("Range > List", QVariant(InstTypeRangeList));

    /* fill info */
    soc_desc::instance_t& inst = GetInstance();
    name_edit->setText(QString::fromStdString(inst.name));
    title_edit->setText(QString::fromStdString(inst.title));
    desc_edit->SetTextHtml(QString::fromStdString(inst.desc));
    addr_edit->setField(inst.addr);
    base_edit->setField(inst.range.base);
    stride_edit->setField(inst.range.stride);
    first_spin->setValue(inst.range.first);
    count_spin->setValue(inst.range.count);
    formula_edit->setText(QString::fromStdString(inst.range.formula));
    variable_edit->setText(QString::fromStdString(inst.range.variable));
    addr_list->setColumnCount(2);
    addr_list->setHorizontalHeaderItem(0, new QTableWidgetItem(""));
    addr_list->setHorizontalHeaderItem(1, new QTableWidgetItem("Address"));
    addr_list->horizontalHeader()->setVisible(false);
    addr_list->verticalHeader()->setVisible(false);
    addr_list->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    std::vector< soc_word_t >& addrs = inst.range.list;
    addr_list->setRowCount(addrs.size() + 1);
    for(size_t i = 0; i < addrs.size(); i++)
    {
        QTableWidgetItem *item = new QTableWidgetItem(
            YIconManager::Get()->GetIcon(YIconManager::ListRemove), "", NodeInstEditPanelDelType);
        item->setToolTip("Remove this address");
        item->setFlags(Qt::ItemIsEnabled);
        addr_list->setItem(i, 0, item);
        item = new QTableWidgetItem();
        item->setData(Qt::EditRole, QVariant::fromValue(addrs[i]));
        addr_list->setItem(i, 1, item);
    }
    QTableWidgetItem *new_item = new QTableWidgetItem(
        YIconManager::Get()->GetIcon(YIconManager::ListAdd), "", NodeInstEditPanelAddType);
    new_item->setFlags(Qt::ItemIsEnabled);
    addr_list->setItem(addrs.size(), 0, new_item);
    new_item = new QTableWidgetItem("New address...", QTableWidgetItem::UserType);
    new_item->setFlags(Qt::ItemIsEnabled);
    QFont font = new_item->font();
    font.setItalic(true);
    new_item->setFont(font);
    addr_list->setItem(addrs.size(), 1, new_item);
    addr_list->resizeColumnsToContents();
    addr_list->horizontalHeader()->setStretchLastSection(true);
    m_table_delegate = new SocFieldItemDelegate(this);
    m_table_delegate->setItemEditorFactory(new QItemEditorFactory);
    m_table_edit_factory = new SocFieldEditorCreator;
    m_table_delegate->itemEditorFactory()->registerEditor(QVariant::UInt, m_table_edit_factory);
    addr_list->setItemDelegate(m_table_delegate);

    setLayout(ii_layout);

    connect(name_edit, SIGNAL(textChanged(const QString&)), this,
        SLOT(OnNameEdited(const QString&)));
    connect(desc_edit, SIGNAL(OnTextChanged(const QString&)), this,
        SLOT(OnDescEdited(const QString&)));
    connect(title_edit, SIGNAL(textChanged(const QString&)), this,
        SLOT(OnTitleEdited(const QString&)));
    connect(addr_edit, SIGNAL(editingFinished(uint)), this, SLOT(OnAddrChanged(uint)));
    connect(base_edit, SIGNAL(editingFinished(uint)), this, SLOT(OnBaseChanged(uint)));
    connect(stride_edit, SIGNAL(editingFinished(uint)), this, SLOT(OnStrideChanged(uint)));
    connect(first_spin, SIGNAL(valueChanged(int)), this, SLOT(OnFirstChanged(int)));
    connect(count_spin, SIGNAL(valueChanged(int)), this, SLOT(OnCountChanged(int)));
    connect(formula_edit, SIGNAL(textChanged(const QString&)), this,
        SLOT(OnFormulaChanged(const QString&)));
    connect(variable_edit, SIGNAL(textChanged(const QString&)), this,
        SLOT(OnVariableChanged(const QString&)));
    connect(m_type_combo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(OnTypeChanged(int)));
    connect(addr_list, SIGNAL(itemActivated(QTableWidgetItem *)), this,
        SLOT(OnAddressActivated(QTableWidgetItem *)));
    connect(addr_list, SIGNAL(itemChanged(QTableWidgetItem *)), this,
        SLOT(OnAddressChanged(QTableWidgetItem *)));

    /* fill info */
    int combo_type;
    if(inst.type == soc_desc::instance_t::RANGE)
    {
        if(inst.range.type == soc_desc::range_t::STRIDE)
            combo_type = InstTypeRangeStride;
        else if(inst.range.type == soc_desc::range_t::FORMULA)
            combo_type = InstTypeRangeFormula;
        else /* LIST */
            combo_type = InstTypeRangeList;
    }
    else
        combo_type = InstTypeSingle;
    m_type_combo->setCurrentIndex(m_type_combo->findData(QVariant(combo_type)));
    UpdateType(combo_type);
}

soc_desc::instance_t& NodeInstanceEditPanel::GetInstance()
{
    return *GetInstanceById(m_ref, m_id);
}

void NodeInstanceEditPanel::OnNameEdited(const QString& text)
{
    GetInstance().name = text.toStdString();
    OnModified();
}

void NodeInstanceEditPanel::OnTitleEdited(const QString& text)
{
    GetInstance().title = text.toStdString();
    OnModified();
}

void NodeInstanceEditPanel::OnDescEdited(const QString& text)
{
    GetInstance().desc = text.toStdString();
    OnModified();
}

void NodeInstanceEditPanel::OnAddrChanged(uint addr)
{
    GetInstance().addr = addr;
    OnModified();
}

void NodeInstanceEditPanel::OnBaseChanged(uint base)
{
    GetInstance().range.base = base;
    OnModified();
}

void NodeInstanceEditPanel::OnStrideChanged(uint stride)
{
    GetInstance().range.stride = stride;
    OnModified();
}

void NodeInstanceEditPanel::OnFirstChanged(int first)
{
    GetInstance().range.first = first;
    OnModified();
}

void NodeInstanceEditPanel::OnCountChanged(int count)
{
    GetInstance().range.count = count;
    OnModified();
}

void NodeInstanceEditPanel::OnFormulaChanged(const QString& formula)
{
    GetInstance().range.formula = formula.toStdString();
    OnModified();
}

void NodeInstanceEditPanel::OnVariableChanged(const QString& variable)
{
    GetInstance().range.variable = variable.toStdString();
    OnModified();
}

void NodeInstanceEditPanel::OnAddressActivated(QTableWidgetItem *item)
{
    QTableWidget *table = item->tableWidget();
    soc_desc::instance_t& inst = GetInstance();
    if(item->type() == NodeInstEditPanelDelType)
    {
        int row = item->row();
        my_remove_at(inst.range.list, row);
        table->removeRow(row);
        OnModified();
    }
    else if(item->type() == NodeInstEditPanelAddType)
    {
        int row = inst.range.list.size();
        soc_word_t new_addr = 0;
        GetInstance().range.list.push_back(new_addr);
        table->insertRow(row);
        QTableWidgetItem *item = new QTableWidgetItem(
            YIconManager::Get()->GetIcon(YIconManager::ListRemove), "", NodeInstEditPanelDelType);
        item->setToolTip("Remove this address");
        item->setFlags(Qt::ItemIsEnabled);
        table->setItem(row, 0, item);
        item = new QTableWidgetItem();
        item->setData(Qt::EditRole, QVariant(new_addr));
        table->setItem(row, 1, item);
        OnModified();
    }
}

void NodeInstanceEditPanel::OnAddressChanged(QTableWidgetItem *item)
{
    soc_desc::instance_t& inst = GetInstance();
    if((size_t)item->row() >= inst.range.list.size())
        return;
    soc_word_t& addr = inst.range.list[item->row()];
    if(item->column() == 1)
        addr = item->data(Qt::EditRole).value< soc_word_t >();
    OnModified();
}

void NodeInstanceEditPanel::UpdateType(int type)
{
    m_single_group->hide();
    m_range_group->hide();
    m_stride_group->hide();
    m_formula_group->hide();
    m_list_group->hide();

    switch(type)
    {
        case InstTypeSingle:
            m_single_group->show();
            GetInstance().type = soc_desc::instance_t::SINGLE;
            break;
        case InstTypeRangeStride:
            m_range_group->show();
            m_stride_group->show();
            GetInstance().type = soc_desc::instance_t::RANGE;
            GetInstance().range.type = soc_desc::range_t::STRIDE;
            break;
        case InstTypeRangeFormula:
            m_range_group->show();
            m_formula_group->show();
            GetInstance().type = soc_desc::instance_t::RANGE;
            GetInstance().range.type = soc_desc::range_t::FORMULA;
            break;
        case InstTypeRangeList:
            m_range_group->show();
            m_formula_group->hide();
            m_list_group->show();
            GetInstance().type = soc_desc::instance_t::RANGE;
            GetInstance().range.type = soc_desc::range_t::LIST;
            break;
        default:
            break;
    }
}

void NodeInstanceEditPanel::OnTypeChanged(int index)
{
    if(index == -1)
        return;
    UpdateType(m_type_combo->itemData(index).toInt());
    OnModified();
}

soc_id_t NodeInstanceEditPanel::GetId()
{
    return m_id;
}

/**
 * NodeEditPanel
 */

NodeEditPanel::NodeEditPanel(const soc_desc::node_ref_t& ref, QWidget *parent)
    :QWidget(parent), m_ref(ref)
{
    /* top layout: name, title then desc */
    QLineEdit *name_edit = new QLineEdit(this);
    name_edit->setText(QString::fromStdString(ref.get()->name));

    QLineEdit *title_edit = new QLineEdit(this);
    title_edit->setText(QString::fromStdString(ref.get()->title));

    QFormLayout *info_layout = new QFormLayout();
    info_layout->addRow("Name", name_edit);
    info_layout->addRow("Title", title_edit);

    QGroupBox *info_group = Misc::EncloseInBox("Information", info_layout);

    m_desc_edit = new MyTextEditor(this);
    m_desc_edit->SetTextHtml(QString::fromStdString(ref.get()->desc));
    QGroupBox *desc_group = Misc::EncloseInBox("Description", m_desc_edit);

    QHBoxLayout *name_title_desc_layout = new QHBoxLayout;
    name_title_desc_layout->addWidget(info_group, 1);
    name_title_desc_layout->addWidget(desc_group, 2);

    /* instance tab */
    m_instances_tab = new YTabWidget(0, this);
    m_instances_tab->setTabOpenable(true);
    std::vector< soc_desc::instance_t >& inst_list = m_ref.get()->instance;
    m_instances_tab->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    m_instances_tab->setTabsClosable(true);
    QGroupBox *instance_tab_group = Misc::EncloseInBox("Instances", m_instances_tab);
    for(size_t i = 0; i < inst_list.size(); i++)
    {
        NodeInstanceEditPanel *p = new NodeInstanceEditPanel(m_ref, inst_list[i].id, this);
        connect(p, SIGNAL(OnModified()), this, SLOT(OnInstModified()));
        m_instances_tab->addTab(p, QString::fromStdString(inst_list[i].name));
    }

    /* boring */
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(name_title_desc_layout);
    layout->addWidget(instance_tab_group);
    layout->addStretch(1);

    setLayout(layout);

    connect(name_edit, SIGNAL(textChanged(const QString&)), this,
        SLOT(OnNameEdited(const QString&)));
    connect(m_desc_edit, SIGNAL(OnTextChanged()), this, SLOT(OnDescEdited()));
    connect(title_edit, SIGNAL(textChanged(const QString&)), this,
        SLOT(OnTitleEdited(const QString&)));
    connect(m_instances_tab, SIGNAL(tabCloseRequested(int)), this,
        SLOT(OnInstRemove(int)));
    connect(m_instances_tab, SIGNAL(tabOpenRequested()), this, SLOT(OnInstCreate()));
}

void NodeEditPanel::OnNameEdited(const QString& text)
{
    m_ref.get()->name = text.toStdString();
    OnModified();
}

void NodeEditPanel::OnTitleEdited(const QString& text)
{
    m_ref.get()->title = text.toStdString();
    OnModified();
}

void NodeEditPanel::OnDescEdited()
{
    m_ref.get()->desc = m_desc_edit->GetTextHtml().toStdString();
    OnModified();
}

void NodeEditPanel::OnInstRemove(int index)
{
    NodeInstanceEditPanel *panel =
        dynamic_cast< NodeInstanceEditPanel * >(m_instances_tab->widget(index));
    RemoveInstanceById(m_ref, panel->GetId());
    m_instances_tab->removeTab(index);
    delete panel;
    OnModified();
}

void NodeEditPanel::OnInstModified()
{
    int index = m_instances_tab->currentIndex();
    NodeInstanceEditPanel *panel =
        dynamic_cast< NodeInstanceEditPanel * >(m_instances_tab->widget(index));
    m_instances_tab->setTabText(index, QString::fromStdString(panel->GetInstance().name));
    OnModified();
}

QString NodeEditPanel::GuessName()
{
    /* try to find instances named Xddd where X is the node name (case insensitive)
     * and d are digits. */
    int max_nr_digits = -1;
    int max_value = -1;
    QString node_name = QString::fromStdString(m_ref.get()->name);
    std::vector< soc_desc::instance_t >& inst_list = m_ref.get()->instance;
    for(size_t i = 0; i < inst_list.size(); i++)
    {
        QString inst_name = QString::fromStdString(inst_list[i].name);
        /* ignore name if it doesn't start like the node name */
        if(!inst_name.startsWith(node_name, Qt::CaseInsensitive))
            continue;
        /* check if the suffix is a digit */
        QString suffix = inst_name.mid(node_name.size());
        if(suffix.size() == 0)
        {
            max_nr_digits = qMax(max_nr_digits, 0);
            continue;
        }
        bool ok;
        int value = suffix.toUInt(&ok);
        if(!ok)
            continue;
        max_value = qMax(max_value, value);
        max_nr_digits = qMax(max_nr_digits, suffix.size());
    }
    /* if no match, use node name */
    if(max_nr_digits == -1)
        return node_name;
    /* match of size 0, add "1" at the end */
    if(max_nr_digits == 0)
        return node_name + "1";
    /* otherwise, pick next value */
    return QString("%1%2").arg(node_name)
        .arg(max_value + 1, max_nr_digits, 10, QChar('0'));
}

void NodeEditPanel::OnInstCreate()
{
    std::vector< soc_desc::instance_t >& inst_list = m_ref.get()->instance;
    soc_desc::instance_t inst;
    inst.id = GetFreshId(inst_list);
    inst.name = GuessName().toStdString();
    inst.type = soc_desc::instance_t::SINGLE;
    inst.addr = 0;
    inst.range.type = soc_desc::range_t::STRIDE;
    inst.range.first = 0;
    inst.range.count = 0;
    inst.range.stride = 0;
    inst_list.push_back(inst);
    NodeInstanceEditPanel *p = new NodeInstanceEditPanel(m_ref, inst.id, this);
    connect(p, SIGNAL(OnModified()), this, SLOT(OnInstModified()));
    int idx = m_instances_tab->addTab(p, QString::fromStdString(inst.name));
    m_instances_tab->setCurrentIndex(idx);
    OnModified();
}

/**
 * RegFieldEditPanel
 */

namespace
{

enum
{
    RegFieldEditPanelDelType = QTableWidgetItem::UserType,
    RegFieldEditPanelAddType,
};

}

RegFieldEditPanel::RegFieldEditPanel(const soc_desc::field_ref_t& ref, QWidget *parent)
    :QWidget(parent), m_ref(ref)
{
    m_name_edit = new QLineEdit(this);
    m_range_edit = new QLineEdit(this);
    m_range_validator = new SocBitRangeValidator(this);
    m_range_validator->setWidth(m_ref.reg().get()->width);
    m_range_edit->setValidator(m_range_validator);
    m_desc_edit = new MyTextEditor(this);
    QHBoxLayout *namepos_layout = new QHBoxLayout;
    namepos_layout->addWidget(new QLabel("Name:"));
    namepos_layout->addWidget(m_name_edit);
    namepos_layout->addWidget(new QLabel("Range:"));
    namepos_layout->addWidget(m_range_edit);
    QVBoxLayout *nameposdesc_layout = new QVBoxLayout;
    nameposdesc_layout->addWidget(Misc::EncloseInBox("Information", namepos_layout));
    nameposdesc_layout->addWidget(Misc::EncloseInBox("Description", m_desc_edit));
    nameposdesc_layout->addStretch(0);

    m_enum_table = new QTableWidget(this);
    m_enum_table->setColumnCount(4);
    m_enum_table->setHorizontalHeaderItem(0, new QTableWidgetItem(""));
    m_enum_table->setHorizontalHeaderItem(1, new QTableWidgetItem("Name"));
    m_enum_table->setHorizontalHeaderItem(2, new QTableWidgetItem("Value"));
    m_enum_table->setHorizontalHeaderItem(3, new QTableWidgetItem("Description"));
    m_enum_table->verticalHeader()->setVisible(false);
    m_enum_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_enum_delegate = new SocFieldItemDelegate(this);
    m_enum_delegate->setItemEditorFactory(new QItemEditorFactory);
    m_enum_editor = new SocFieldEditorCreator;
    m_enum_delegate->itemEditorFactory()->registerEditor(QVariant::UInt, m_enum_editor);
    m_enum_table->setItemDelegate(m_enum_delegate);

    QHBoxLayout *field_layout = new QHBoxLayout;
    field_layout->addLayout(nameposdesc_layout);
    field_layout->addWidget(Misc::EncloseInBox("Special Values", m_enum_table), 1);

    setLayout(field_layout);

    const soc_desc::field_t& field = *m_ref.get();
    m_name_edit->setText(QString::fromStdString(field.name));
    m_range_edit->setText(m_range_validator->generate(
        field.pos + field.width - 1, field.pos));
    m_desc_edit->SetTextHtml(QString::fromStdString(field.desc));
    m_enum_delegate->setWidth(field.width);
    m_enum_editor->setWidth(field.width);
    m_enum_table->setRowCount(field.enum_.size() + 1);
    for(size_t i = 0; i  < field.enum_.size(); i++)
    {
        QTableWidgetItem *item = new QTableWidgetItem(
            YIconManager::Get()->GetIcon(YIconManager::ListRemove), "", RegFieldEditPanelDelType);
        item->setToolTip("Remove this enum");
        item->setFlags(Qt::ItemIsEnabled);
        m_enum_table->setItem(i, 0, item);
        item = new QTableWidgetItem(QString::fromStdString(field.enum_[i].name));
        m_enum_table->setItem(i, 1, item);
        item = new QTableWidgetItem();
        item->setData(Qt::EditRole, QVariant(field.enum_[i].value));
        m_enum_table->setItem(i, 2, item);
        item = new QTableWidgetItem(QString::fromStdString(field.enum_[i].desc));
        m_enum_table->setItem(i, 3, item);
    }
    QTableWidgetItem *new_item = new QTableWidgetItem(
        YIconManager::Get()->GetIcon(YIconManager::ListAdd), "", RegFieldEditPanelAddType);
    new_item->setFlags(Qt::ItemIsEnabled);
    m_enum_table->setItem(field.enum_.size(), 0, new_item);
    new_item = new QTableWidgetItem("New field...");
    new_item->setFlags(Qt::ItemIsEnabled);
    QFont font = new_item->font();
    font.setItalic(true);
    new_item->setFont(font);
    m_enum_table->setItem(field.enum_.size(), 1, new_item);
    new_item = new QTableWidgetItem();
    new_item->setFlags(Qt::ItemIsEnabled);
    m_enum_table->setItem(field.enum_.size(), 2, new_item);
    new_item = new QTableWidgetItem();
    new_item->setFlags(Qt::ItemIsEnabled);
    m_enum_table->setItem(field.enum_.size(), 3, new_item);
    m_enum_table->resizeColumnsToContents();
    m_enum_table->horizontalHeader()->setStretchLastSection(true);

    connect(m_name_edit, SIGNAL(textChanged(const QString&)), this,
        SLOT(OnFieldNameChanged(const QString&)));
    connect(m_range_edit, SIGNAL(textChanged(const QString&)), this,
        SLOT(OnFieldRangeChanged(const QString&)));
    connect(m_desc_edit, SIGNAL(OnTextChanged(const QString&)), this,
        SLOT(OnFieldDescChanged(const QString&)));
    connect(m_enum_table, SIGNAL(itemActivated(QTableWidgetItem *)), this,
        SLOT(OnFieldValueActivated(QTableWidgetItem *)));
    connect(m_enum_table, SIGNAL(itemChanged(QTableWidgetItem *)), this,
        SLOT(OnFieldValueChanged(QTableWidgetItem *)));
}

void RegFieldEditPanel::UpdateWidth()
{
    m_range_validator->setWidth(m_ref.reg().get()->width);
}

void RegFieldEditPanel::UpdateRange()
{
    soc_desc::field_t *field = m_ref.get();
    m_range_edit->setText(m_range_validator->generate(
        field->pos + field->width - 1, field->pos));
}

void RegFieldEditPanel::OnFieldValueActivated(QTableWidgetItem *item)
{
    if(item->type() == RegFieldEditPanelDelType)
    {
        int row = item->row();
        my_remove_at(m_ref.get()->enum_, row);
        m_enum_table->removeRow(row);
        OnModified();
    }
    else if(item->type() == RegFieldEditPanelAddType)
    {
        soc_desc::field_t& field = *m_ref.get();
        int row = field.enum_.size();
        soc_desc::enum_t new_enum;
        new_enum.id = GetFreshId(field.enum_);
        new_enum.name = "UNNAMED";
        new_enum.value = 0;
        field.enum_.push_back(new_enum);
        m_enum_table->insertRow(row);
        QTableWidgetItem *item = new QTableWidgetItem(
            YIconManager::Get()->GetIcon(YIconManager::ListRemove), "", RegFieldEditPanelDelType);
        item->setToolTip("Remove this enum");
        item->setFlags(Qt::ItemIsEnabled);
        m_enum_table->setItem(row, 0, item);
        item = new QTableWidgetItem(QString::fromStdString(new_enum.name));
        m_enum_table->setItem(row, 1, item);
        item = new QTableWidgetItem();
        item->setData(Qt::EditRole, QVariant(new_enum.value));
        m_enum_table->setItem(row, 2, item);
        item = new QTableWidgetItem(QString::fromStdString(new_enum.desc));
        m_enum_table->setItem(row, 3, item);
        OnModified();
    }
}

void RegFieldEditPanel::OnFieldValueChanged(QTableWidgetItem *item)
{
    soc_desc::field_t& field = *m_ref.get();
    if((size_t)item->row() >= field.enum_.size())
        return;
    soc_desc::enum_t& enum_ = field.enum_[item->row()];
    if(item->column() == 1)
        enum_.name = item->text().toStdString();
    else if(item->column() == 2)
        enum_.value = item->data(Qt::EditRole).value< soc_word_t >();
    else if(item->column() == 3)
        enum_.desc = item->text().toStdString();
    OnModified();
}

void RegFieldEditPanel::OnFieldNameChanged(const QString& name)
{
    m_ref.get()->name = name.toStdString();
    OnModified();
}

void RegFieldEditPanel::OnFieldRangeChanged(const QString& range)
{
    soc_desc::field_t *field = m_ref.get();
    int last, first;
    if(m_range_validator->parse(range, last, first) != QValidator::Acceptable)
        return;
    field->pos = first;
    field->width = last - first + 1;
    m_enum_delegate->setWidth(field->width);
    m_enum_editor->setWidth(field->width);
    OnModified();
}

void RegFieldEditPanel::OnFieldDescChanged(const QString& desc)
{
    m_ref.get()->desc = desc.toStdString();
    OnModified();
}

soc_desc::field_ref_t RegFieldEditPanel::GetField()
{
    return m_ref;
}

/**
 * RegEditPanel
 */

namespace
{

enum
{
    RegVariantEditPanelDelType = QTableWidgetItem::UserType,
    RegVariantEditPanelAddType,
};

}

RegEditPanel::RegEditPanel(const soc_desc::register_ref_t& ref, QWidget *parent)
    :QWidget(parent), m_ref(ref), m_reg_font(font())
{
    m_reg_font.setWeight(100);
    m_reg_font.setKerning(false);

    m_value_model = new RegFieldTableModel(this); // view takes ownership
    m_value_model->SetRegister(*ref.get());
    m_value_model->SetReadOnly(false);

    m_sexy_display2 = new Unscroll<YRegDisplay>(this);
    m_sexy_display2->setFont(m_reg_font);
    m_sexy_display2->setModel(m_value_model);
    m_sexy_display2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_sexy_display2->setContextMenuPolicy(Qt::CustomContextMenu);
    m_sexy_display2->setWidth(m_ref.get()->width);

    m_view_tab = new QTabWidget(this);
    m_view_tab->setTabPosition(QTabWidget::West);

    /* field tab */
    m_fields_tab = new YTabWidget(0, this);
    m_fields_tab->setTabOpenable(true);
    std::vector< soc_desc::field_ref_t > field_list = m_ref.fields();
    m_fields_tab->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    m_fields_tab->setTabsClosable(true);
    m_fields_tab->setElideMode(Qt::ElideRight);
    for(size_t i = 0; i < field_list.size(); i++)
    {
        RegFieldEditPanel *p = new RegFieldEditPanel(field_list[i], this);
        connect(p, SIGNAL(OnModified()), this, SLOT(OnFieldModified()));
        m_fields_tab->addTab(p, QString::fromStdString(field_list[i].get()->name));
    }

    m_reg_size_group = new QButtonGroup(this);
    QRadioButton *reg_size_32 = new QRadioButton("32-bit");
    QRadioButton *reg_size_16 = new QRadioButton("16-bit");
    QRadioButton *reg_size_8 = new QRadioButton("8-bit");
    m_reg_size_group->addButton(reg_size_32, 32);
    m_reg_size_group->addButton(reg_size_16, 16);
    m_reg_size_group->addButton(reg_size_8, 8);
    if(m_reg_size_group->button(m_ref.get()->width))
        m_reg_size_group->button(m_ref.get()->width)->click();
    QVBoxLayout *width_group_layout = new QVBoxLayout;
    width_group_layout->addWidget(reg_size_32);
    width_group_layout->addWidget(reg_size_16);
    width_group_layout->addWidget(reg_size_8);
    QGroupBox *width_group = new QGroupBox("Width");
    width_group->setLayout(width_group_layout);

    m_reg_access_group = new QButtonGroup(this);
    QRadioButton *reg_dflt = new QRadioButton("Unspecified");
    QRadioButton *reg_rw = new QRadioButton("Read-Write");
    QRadioButton *reg_ro = new QRadioButton("Read-Only");
    QRadioButton *reg_wo = new QRadioButton("Write-Only");
    m_reg_access_group->addButton(reg_dflt, soc_desc::UNSPECIFIED);
    m_reg_access_group->addButton(reg_rw, soc_desc::READ_WRITE);
    m_reg_access_group->addButton(reg_ro, soc_desc::READ_ONLY);
    m_reg_access_group->addButton(reg_wo, soc_desc::WRITE_ONLY);
    if(m_reg_access_group->button(m_ref.get()->access))
        m_reg_access_group->button(m_ref.get()->access)->click();
    QVBoxLayout *access_group_layout = new QVBoxLayout;
    access_group_layout->addWidget(reg_dflt);
    access_group_layout->addWidget(reg_rw);
    access_group_layout->addWidget(reg_ro);
    access_group_layout->addWidget(reg_wo);
    QGroupBox *access_group = new QGroupBox("Access");
    access_group->setLayout(access_group_layout);

    QVBoxLayout *width_access_layout = new QVBoxLayout;
    width_access_layout->addWidget(width_group);
    width_access_layout->addWidget(access_group);
    width_access_layout->addStretch(0);

    m_variant_table = new QTableWidget;
    m_variant_table->setColumnCount(4);
    m_variant_table->setHorizontalHeaderItem(0, new QTableWidgetItem(""));
    m_variant_table->setHorizontalHeaderItem(1, new QTableWidgetItem("Type"));
    m_variant_table->setHorizontalHeaderItem(2, new QTableWidgetItem("Offset"));
    m_variant_table->setHorizontalHeaderItem(3, new QTableWidgetItem("Access"));
    m_variant_table->verticalHeader()->setVisible(false);
    m_variant_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_variant_delegate = new SocFieldItemDelegate(this);
    m_variant_delegate->setItemEditorFactory(new QItemEditorFactory);
    m_variant_delegate->itemEditorFactory()->registerEditor(QVariant::UInt,
        new SocFieldEditorCreator);
    m_variant_table->setItemDelegate(m_variant_delegate);
    m_access_delegate = new SocAccessItemDelegate("Unspecified", this);
    m_access_delegate->setItemEditorFactory(new QItemEditorFactory);
    // FIXME see QTBUG-30392
    m_access_delegate->itemEditorFactory()->registerEditor(
        (QVariant::Type)qMetaTypeId< soc_desc::access_t >(),
        new SocAccessEditorCreator);
    m_variant_table->setItemDelegateForColumn(3, m_access_delegate);

    std::vector< soc_desc::variant_ref_t > variants = m_ref.variants();
    m_variant_table->setRowCount(variants.size() + 1);
    for(size_t i = 0; i  < variants.size(); i++)
    {
        QTableWidgetItem *item = new QTableWidgetItem(
            YIconManager::Get()->GetIcon(YIconManager::ListRemove), "", RegVariantEditPanelDelType);
        item->setToolTip("Remove this variant");
        item->setFlags(Qt::ItemIsEnabled);
        m_variant_table->setItem(i, 0, item);
        item = new QTableWidgetItem(QString::fromStdString(variants[i].get()->type));
        m_variant_table->setItem(i, 1, item);
        item = new QTableWidgetItem();
        item->setData(Qt::EditRole, QVariant(variants[i].get()->offset));
        m_variant_table->setItem(i, 2, item);
        item = new QTableWidgetItem();
        item->setData(Qt::EditRole, QVariant::fromValue(variants[i].get()->access));
        m_variant_table->setItem(i, 3, item);
    }
    QTableWidgetItem *new_item = new QTableWidgetItem(
        YIconManager::Get()->GetIcon(YIconManager::ListAdd), "", RegVariantEditPanelAddType);
    new_item->setFlags(Qt::ItemIsEnabled);
    m_variant_table->setItem(variants.size(), 0, new_item);
    new_item = new QTableWidgetItem("New variant...");
    new_item->setFlags(Qt::ItemIsEnabled);
    QFont font = new_item->font();
    font.setItalic(true);
    new_item->setFont(font);
    m_variant_table->setItem(variants.size(), 1, new_item);
    new_item = new QTableWidgetItem();
    new_item->setFlags(Qt::ItemIsEnabled);
    m_variant_table->setItem(variants.size(), 2, new_item);
    new_item = new QTableWidgetItem();
    new_item->setFlags(Qt::ItemIsEnabled);
    m_variant_table->resizeColumnsToContents();
    m_variant_table->horizontalHeader()->setStretchLastSection(true);

    m_desc_edit = new MyTextEditor(this);
    m_desc_edit->SetTextHtml(QString::fromStdString(ref.get()->desc));

    QHBoxLayout *top_info_layout = new QHBoxLayout;
    top_info_layout->addLayout(width_access_layout);
    top_info_layout->addWidget(Misc::EncloseInBox("Variants", m_variant_table));
    top_info_layout->addWidget(Misc::EncloseInBox("Description", m_desc_edit));

    QWidget *reg_info = new QWidget(this);
    QVBoxLayout *reg_info_layout = new QVBoxLayout;
    reg_info_layout->addLayout(top_info_layout);
    reg_info_layout->addStretch(0);
    reg_info->setLayout(reg_info_layout);

    m_view_tab->addTab(reg_info, "Information");
    m_view_tab->addTab(m_fields_tab, "Fields");

    QVBoxLayout *main_layout = new QVBoxLayout;
    main_layout->addWidget(m_sexy_display2, 1);
    main_layout->addWidget(m_view_tab, 2);

    m_delete_action = new QAction("&Delete", this);
    m_delete_action->setIcon(YIconManager::Get()->GetIcon(YIconManager::ListRemove));
    m_new_action = new QAction("&New field", this);
    m_new_action->setIcon(YIconManager::Get()->GetIcon(YIconManager::ListAdd));

    setLayout(main_layout);

    OnRegFieldActivated(QModelIndex());
    UpdateWidthRestrictions();

    connect(m_sexy_display2, SIGNAL(clicked(const QModelIndex&)), this,
        SLOT(OnRegFieldActivated(const QModelIndex&)));
    connect(m_sexy_display2, SIGNAL(customContextMenuRequested(QPoint)), this,
        SLOT(OnRegDisplayContextMenu(QPoint)));
    connect(m_reg_size_group, SIGNAL(buttonClicked(int)), this, SLOT(OnWidthChanged(int)));
    connect(m_reg_access_group, SIGNAL(buttonClicked(int)), this, SLOT(OnAccessChanged(int)));
    connect(m_delete_action, SIGNAL(triggered()), this, SLOT(OnRegFieldDelete()));
    connect(m_new_action, SIGNAL(triggered()), this, SLOT(OnRegFieldNew()));
    connect(m_variant_table, SIGNAL(itemActivated(QTableWidgetItem *)), this,
        SLOT(OnVariantActivated(QTableWidgetItem *)));
    connect(m_variant_table, SIGNAL(itemChanged(QTableWidgetItem *)), this,
        SLOT(OnVariantValueChanged(QTableWidgetItem *)));
    connect(m_desc_edit, SIGNAL(OnTextChanged()), this, SLOT(OnDescEdited()));
    connect(m_fields_tab, SIGNAL(tabCloseRequested(int)), this, SLOT(OnFieldRemove(int)));
    connect(m_fields_tab, SIGNAL(tabOpenRequested()), this, SLOT(OnFieldCreate()));
    connect(m_value_model, SIGNAL(OnBitrangeModified(int)), this, SLOT(OnBitrangeModified(int)));
}

void RegEditPanel::UpdateWidthRestrictions()
{
    /* only allow width large enough to fit all fields */
    size_t max_bit = 0;
    std::vector< soc_desc::field_ref_t > field_list = m_ref.fields();
    for(size_t i = 0; i < field_list.size(); i++)
    {
        soc_desc::field_t& f = *field_list[i].get();
        max_bit = std::max(max_bit, f.pos + f.width - 1);
    }
    /* update buttons */
    m_reg_size_group->button(8)->setEnabled(max_bit < 8);
    m_reg_size_group->button(16)->setEnabled(max_bit < 16);
    m_reg_size_group->button(32)->setEnabled(max_bit < 32);
}

int RegEditPanel::IndexById(soc_id_t id)
{
    for(int i = 0; i < m_fields_tab->count(); i++)
    {
        RegFieldEditPanel *p = dynamic_cast< RegFieldEditPanel * >(m_fields_tab->widget(i));
        if(p->GetField().get()->id == id)
            return i;
    }
    return -1;
}

void RegEditPanel::OnWidthChanged(int w)
{
    m_ref.get()->width = w;
    m_sexy_display2->setWidth(w);
    for(int i = 0; i < m_fields_tab->count(); i++)
        dynamic_cast< RegFieldEditPanel * >(m_fields_tab->widget(i))->UpdateWidth();
    OnModified();
}

void RegEditPanel::OnBitrangeModified(int index)
{
    soc_desc::register_t reg = m_value_model->GetRegister();
    m_ref.get()->field[index].pos = reg.field[index].pos;
    m_ref.get()->field[index].width = reg.field[index].width;
    for(int i = 0; i < m_fields_tab->count(); i++)
        dynamic_cast< RegFieldEditPanel * >(m_fields_tab->widget(i))->UpdateRange();
    OnModified();
}

void RegEditPanel::OnAccessChanged(int acc)
{
    m_ref.get()->access = (soc_desc::access_t)acc;
    OnModified();
}

void RegEditPanel::OnDescEdited()
{
    m_ref.get()->desc = m_desc_edit->GetTextHtml().toStdString();
    OnModified();
}

void RegEditPanel::OnVariantActivated(QTableWidgetItem *item)
{
    if(item->type() == RegVariantEditPanelDelType)
    {
        int row = item->row();
        my_remove_at(m_ref.get()->variant, row);
        m_variant_table->removeRow(row);
        OnModified();
    }
    else if(item->type() == RegVariantEditPanelAddType)
    {
        int row = m_ref.get()->variant.size();
        soc_desc::variant_t& variant = *m_ref.create_variant().get();
        variant.type = "untyped";
        variant.offset = 0;
        variant.access = soc_desc::UNSPECIFIED;
        m_variant_table->insertRow(row);
        QTableWidgetItem *item = new QTableWidgetItem(
            YIconManager::Get()->GetIcon(YIconManager::ListRemove), "", RegVariantEditPanelDelType);
        item->setToolTip("Remove this variant");
        item->setFlags(Qt::ItemIsEnabled);
        m_variant_table->setItem(row, 0, item);
        item = new QTableWidgetItem(QString::fromStdString(variant.type));
        m_variant_table->setItem(row, 1, item);
        item = new QTableWidgetItem();
        item->setData(Qt::EditRole, QVariant(variant.offset));
        m_variant_table->setItem(row, 2, item);
        item = new QTableWidgetItem();
        item->setData(Qt::EditRole, QVariant::fromValue(variant.access));
        m_variant_table->setItem(row, 3, item);
        OnModified();
    }
}

void RegEditPanel::OnVariantValueChanged(QTableWidgetItem *item)
{
    soc_desc::register_t& reg = *m_ref.get();
    if((size_t)item->row() >= reg.variant.size())
        return;
    soc_desc::variant_t& var = reg.variant[item->row()];
    if(item->column() == 1)
        var.type = item->text().toStdString();
    else if(item->column() == 2)
        var.offset = item->data(Qt::EditRole).value< soc_word_t >();
    else if(item->column() == 3)
        var.access = item->data(Qt::EditRole).value< soc_desc::access_t >();
    OnModified();
}

void RegEditPanel::OnRegFieldActivated(const QModelIndex& index)
{
    if(!index.isValid())
        return;
    m_fields_tab->setCurrentIndex(IndexById(m_ref.get()->field[index.row()].id));
    m_view_tab->setCurrentIndex(1);
}

void RegEditPanel::OnFieldModified()
{
    int idx = m_fields_tab->currentIndex();
    RegFieldEditPanel *p = dynamic_cast< RegFieldEditPanel * >(m_fields_tab->widget(idx));
    m_fields_tab->setTabText(idx, QString::fromStdString(p->GetField().get()->name));
    DoModify();
}

void RegEditPanel::DoModify()
{
    m_value_model->UpdateRegister(*m_ref.get());
    UpdateWidthRestrictions();
    OnModified();
}

void RegEditPanel::OnRegFieldDelete()
{
    QModelIndex current = m_sexy_display2->currentIndex();
    if(!current.isValid())
        return;
    QMessageBox msgbox(QMessageBox::Question, "Delete field ?",
        "Are you sure you want to delete this field ?",
        QMessageBox::Yes | QMessageBox::No, this);
    msgbox.setDefaultButton(QMessageBox::No);
    int ret = msgbox.exec();
    if(ret != QMessageBox::Yes)
        return;
    m_fields_tab->removeTab(IndexById(m_ref.get()->field[current.row()].id));
    my_remove_at(m_ref.get()->field, current.row());
    DoModify();
    OnRegFieldActivated(QModelIndex());
}

void RegEditPanel::OnFieldRemove(int index)
{
    Q_UNUSED(index);
}

int RegEditPanel::FindFreeBit(int preferred)
{
    int nr_bits = m_ref.get()->width;
    soc_word_t free_mask = (nr_bits == 32) ? 0xffffffff : (1 << nr_bits) - 1;
    soc_desc::register_t& reg = *m_ref.get();
    for(size_t i = 0; i < reg.field.size(); i++)
        free_mask &= ~reg.field[i].bitmask();
    /* any space ? */
    if(free_mask == 0)
        return -1;
    int closest_bit = -1;
    int closest_dist = nr_bits;
    for(int bit = 0; bit < nr_bits; bit++)
    {
        if(!(free_mask & (1 << bit)))
            continue;
        if(abs(bit - preferred) < closest_dist)
        {
            closest_bit = bit;
            closest_dist = abs(bit - preferred);
        }
    }
    return closest_bit;
}

void RegEditPanel::OnRegFieldNew()
{
    int bit_col = m_sexy_display2->bitColumnAt(m_menu_point);
    /* we need to make sure the created field does not overlap something */
    bit_col = FindFreeBit(bit_col);
    if(bit_col == -1)
        return; /* impossible to find a free position */
    soc_desc::field_ref_t ref = m_ref.create_field();
    soc_desc::field_t& field = *ref.get();
    field.pos = bit_col;
    field.width = 1;
    field.name = "UNNAMED";

    RegFieldEditPanel *p = new RegFieldEditPanel(ref, this);
    connect(p, SIGNAL(OnModified()), this, SLOT(OnFieldModified()));
    m_fields_tab->addTab(p, QString::fromStdString(field.name));

    DoModify();
}

void RegEditPanel::OnFieldCreate()
{
    OnRegFieldNew();
}

void RegEditPanel::OnRegDisplayContextMenu(QPoint point)
{
    m_menu_point = point;
    QMenu *menu = new QMenu(this);
    QModelIndex item = m_sexy_display2->indexAt(point);
    menu->addAction(m_new_action);
    if(item.isValid())
        menu->addAction(m_delete_action);
    menu->popup(m_sexy_display2->viewport()->mapToGlobal(point));
}

namespace
{

enum
{
    SocTreeSocType = QTreeWidgetItem::UserType, // SocRefRole -> node_ref_t to root
    SocTreeNodeType, // SocRefRole -> node_ref_t
    SocTreeRegType, // SocRefRole -> register_ref_t
};

enum
{
    SocRefRole = Qt::UserRole,
};

template<typename T>
T SocTreeItemVal(QTreeWidgetItem *item)
{
    return item->data(0, SocRefRole).value<T>();
}

template<typename T>
QTreeWidgetItem *MakeSocTreeItem(int type, const T& val)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(type);
    item->setData(0, SocRefRole, QVariant::fromValue(val));
    return item;
}

}

/**
 * RegEdit
 */
RegEdit::RegEdit(Backend *backend, QWidget *parent)
    :QWidget(parent), m_backend(backend)
{
    QVBoxLayout *m_vert_layout = new QVBoxLayout();
    QLabel *file_static = new QLabel("File:");
    m_file_edit = new QLineEdit(this);
    m_file_edit->setReadOnly(true);
    m_file_open = new QToolButton(this);
    m_file_open->setText("Open");
    m_file_open->setIcon(YIconManager::Get()->GetIcon(YIconManager::DocumentOpen));
    m_file_open->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    QMenu *file_open_menu = new QMenu(this);
    QAction *new_act = file_open_menu->addAction(YIconManager::Get()->GetIcon(YIconManager::DocumentNew),
        "New...");
    m_file_open->setPopupMode(QToolButton::MenuButtonPopup);
    m_file_open->setMenu(file_open_menu);

    m_file_save = new QToolButton(this);
    m_file_save->setText("Save");
    m_file_save->setIcon(YIconManager::Get()->GetIcon(YIconManager::DocumentSave));
    m_file_save->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_file_save->setPopupMode(QToolButton::MenuButtonPopup);
    QMenu *file_save_menu = new QMenu(this);
    QAction *saveas_act = file_save_menu->addAction(YIconManager::Get()->GetIcon(YIconManager::DocumentSaveAs),
        "Save as...");
    m_file_save->setMenu(file_save_menu);

    QHBoxLayout *file_group_layout = new QHBoxLayout();
    file_group_layout->addWidget(m_file_open);
    file_group_layout->addWidget(m_file_save);
    file_group_layout->addWidget(file_static);
    file_group_layout->addWidget(m_file_edit);

    m_splitter = new QSplitter(this);
    m_soc_tree = new QTreeWidget(this);
    m_soc_tree->setColumnCount(1);
    m_soc_tree->setHeaderLabel(QString("Name"));
    m_soc_tree->setContextMenuPolicy(Qt::CustomContextMenu);

    m_delete_action = new QAction("&Delete", this);
    m_delete_action->setIcon(YIconManager::Get()->GetIcon(YIconManager::ListRemove));
    m_new_action = new QAction("&New", this);
    m_new_action->setIcon(YIconManager::Get()->GetIcon(YIconManager::ListAdd));
    m_create_action = new QAction("&Create register", this);
    m_create_action->setIcon(YIconManager::Get()->GetIcon(YIconManager::FolderNew));

    m_splitter->addWidget(m_soc_tree);
    m_splitter->setStretchFactor(0, 0);

    m_msg = new MessageWidget(this);
    QWidget *splitter_right = new QWidget(this);
    m_right_panel_layout = new QVBoxLayout;
    m_right_panel_layout->addWidget(m_msg, 0);
    splitter_right->setLayout(m_right_panel_layout);
    m_splitter->addWidget(splitter_right);
    m_splitter->setStretchFactor(1, 2);

    m_msg_welcome_id = SetMessage(MessageWidget::Information,
        "Open a description file to edit, or create a new file from scratch.");
    m_msg_name_error_id = 0;

    m_vert_layout->addLayout(file_group_layout);
    m_vert_layout->addWidget(m_splitter, 1);

    setLayout(m_vert_layout);

    SetModified(false, false);
    m_right_panel = 0;
    SetPanel(new EmptyEditPanel(this));
    UpdateTabName();

    connect(m_soc_tree, SIGNAL(customContextMenuRequested(QPoint)), this,
        SLOT(OnSocTreeContextMenu(QPoint)));
    connect(m_delete_action, SIGNAL(triggered()), this, SLOT(OnSocItemDelete()));
    connect(m_new_action, SIGNAL(triggered()), this, SLOT(OnSocItemNew()));
    connect(m_create_action, SIGNAL(triggered()), this, SLOT(OnSocItemCreate()));
    connect(m_file_open, SIGNAL(clicked()), this, SLOT(OnOpen()));
    connect(m_file_save, SIGNAL(clicked()), this, SLOT(OnSave()));
    connect(new_act, SIGNAL(triggered()), this, SLOT(OnNew()));
    connect(saveas_act, SIGNAL(triggered()), this, SLOT(OnSaveAs()));
    connect(m_soc_tree, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
        this, SLOT(OnSocItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
}

QWidget *RegEdit::GetWidget()
{
    return this;
}

RegEdit::~RegEdit()
{
}

int RegEdit::SetMessage(MessageWidget::MessageType type, const QString& msg)
{
    return m_msg->SetMessage(type, msg);
}

void RegEdit::HideMessage(int id)
{
    m_msg->HideMessage(id);
}

void RegEdit::OnSave()
{
    SaveSoc();
}

void RegEdit::OnSaveAs()
{
    SaveSocAs();
}

bool RegEdit::CloseSoc()
{
    if(!m_modified)
        return true;
    QMessageBox msgbox(QMessageBox::Question, "Save changes ?", 
        "The description has been modified. Do you want to save your changes ?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, this);
    msgbox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgbox.exec();
    if(ret == QMessageBox::Discard)
        return true;
    if(ret == QMessageBox::Cancel)
        return false;
    return SaveSoc();
}

bool RegEdit::SaveSoc()
{
    if(m_file_edit->text().size() == 0)
        return SaveSocAs();
    else
        return SaveSocFile(m_file_edit->text());
}

bool RegEdit::GetFilename(QString& filename, bool save)
{
    QFileDialog *fd = new QFileDialog(this);
    if(save)
        fd->setAcceptMode(QFileDialog::AcceptSave);
    QStringList filters;
    filters << "Description files (*.xml)";
    filters << "All files (*)";
    fd->setNameFilters(filters);
    fd->setDirectory(Settings::Get()->value("regedit/loaddescdir", QDir::currentPath()).toString());
    if(fd->exec())
    {
        QStringList filenames = fd->selectedFiles();
        filename = filenames[0];
        Settings::Get()->setValue("regedit/loaddescdir", fd->directory().absolutePath());
        return true;
    }
    else
        return false;
}

bool RegEdit::SaveSocAs()
{
    QString filename;
    if(!GetFilename(filename, true))
        return false;
    m_file_edit->setText(filename);
    return SaveSocFile(filename);
}

void RegEdit::OnOpen()
{
    if(!CloseSoc())
        return;
    QString filename;
    if(!GetFilename(filename, false))
        return;
    LoadSocFile(filename);
}

void RegEdit::OnNew()
{
    if(!CloseSoc())
        return;
    m_cur_socfile = SocFile();
    m_file_edit->setText("");
    SetModified(false, false);
    UpdateSocFile();
}

bool RegEdit::SaveSocFile(const QString& filename)
{
    soc_desc::error_context_t ctx;
    soc_desc::normalize(m_cur_socfile.GetSoc());
    if(!soc_desc::produce_xml(filename.toStdString(), m_cur_socfile.GetSoc(), ctx))
    {
        QMessageBox::warning(this, "The description was not saved",
            "There was an error when saving the file");
        return false;
    }
    UpdateSocFile();
    SetModified(false, false);
    return true;
}

void RegEdit::UpdateTabName()
{
    QFileInfo info(m_cur_socfile.GetFilename());
    if(info.exists())
        SetTabName(info.fileName());
    else
        SetTabName("Register Editor");
}

void RegEdit::LoadSocFile(const QString& filename)
{
    m_cur_socfile = SocFile(filename);
    if(!m_cur_socfile.IsValid())
    {
        QMessageBox::warning(this, "The description was not loaded",
            "There was an error when loading the file");
        return;
    }
    m_file_edit->setText(filename);
    SetModified(false, false);
    UpdateSocFile();
    UpdateTabName();
    m_msg_welcome_id = SetMessage(MessageWidget::Information,
        "Select items to edit in tree, or right-click on them to see available actions.");
}

void RegEdit::FillNodeTreeItem(QTreeWidgetItem *item)
{
    soc_desc::node_ref_t node = SocTreeItemVal< soc_desc::node_ref_t >(item);
    soc_desc::register_ref_t reg = node.reg();
    /* put register if there, otherwise offer to create one */
    if(reg.valid() && reg.node() == node)
    {
        QTreeWidgetItem *reg_item = MakeSocTreeItem(SocTreeRegType, reg);
        FixupItem(reg_item);
        item->addChild(reg_item);
    }
    std::vector< soc_desc::node_ref_t > list = node.children();
    for(size_t i = 0; i < list.size(); i++)
    {
        QTreeWidgetItem *node_item = MakeSocTreeItem(SocTreeNodeType, list[i]);
        FixupItem(node_item);
        FillNodeTreeItem(node_item);
        item->addChild(node_item);
    }
}

void RegEdit::FillSocTree()
{
    soc_desc::soc_ref_t ref = m_cur_socfile.GetSocRef();
    QTreeWidgetItem *root_item = MakeSocTreeItem(SocTreeSocType, ref.root());
    FixupItem(root_item);
    FillNodeTreeItem(root_item);
    m_soc_tree->addTopLevelItem(root_item);
    root_item->setExpanded(true);
}

void RegEdit::MakeItalic(QTreeWidgetItem *item, bool it)
{
    QFont font = item->font(0);
    font.setItalic(it);
    item->setFont(0, font);
}

QIcon RegEdit::GetIconFromType(int type)
{
    switch(type)
    {
        case SocTreeSocType: return YIconManager::Get()->GetIcon(YIconManager::Computer);
        case SocTreeNodeType: return YIconManager::Get()->GetIcon(YIconManager::Cpu);
        case SocTreeRegType: return style()->standardIcon(QStyle::SP_ArrowRight);
        default: return QIcon();
    }
}

bool RegEdit::ValidateName(const QString& name)
{
    if(name.size() == 0)
        return false;
    for(int i = 0; i < name.size(); i++)
        if(!name[i].isLetterOrNumber() && name[i] != QChar('_'))
            return false;
    return true;
}

void RegEdit::FixupItem(QTreeWidgetItem *item)
{
    UpdateName(item);
    if(!ValidateName(item->text(0)))
    {
        item->setIcon(0, YIconManager::Get()->GetIcon(YIconManager::DialogError));
        if(item->text(0).size() == 0)
        {
            MakeItalic(item, true);
            item->setText(0, "Unnamed");
        }
        m_msg_name_error_id = SetMessage(MessageWidget::Error,
            "The item name is invalid. It must be non-empty and consists only "
            "of alphanumerical or underscore characters");
    }
    else
    {
        HideMessage(m_msg_name_error_id);
        item->setIcon(0, GetIconFromType(item->type()));
        MakeItalic(item, false);
    }
}

void RegEdit::UpdateSocFile()
{
    m_soc_tree->clear();
    FillSocTree();
    SetPanel(new EmptyEditPanel(this));
}

void RegEdit::SetPanel(QWidget *panel)
{
    delete m_right_panel;
    m_right_panel = panel;
    connect(m_right_panel, SIGNAL(OnModified()), this,
        SLOT(OnSocModified()));
    m_right_panel_layout->addWidget(m_right_panel, 1);
}

void RegEdit::SetModified(bool add, bool mod)
{
    m_modified = add ? (m_modified || mod) : mod;
    OnModified(mod);
}

void RegEdit::OnSocItemNew()
{
    QTreeWidgetItem *current = m_soc_tree->currentItem();
    if(current == 0)
        return;
    soc_desc::node_ref_t node = SocTreeItemVal< soc_desc::node_ref_t >(current);
    node = node.create();
    node.get()->name = "unnamed";
    QTreeWidgetItem *node_item = MakeSocTreeItem(SocTreeNodeType, node);
    FixupItem(node_item);
    current->addChild(node_item);
    m_soc_tree->setCurrentItem(node_item);
}

void RegEdit::OnSocItemCreate()
{
    QTreeWidgetItem *current = m_soc_tree->currentItem();
    if(current == 0)
        return;
    soc_desc::register_t reg;
    reg.width = 32;
    reg.access = soc_desc::UNSPECIFIED;
    soc_desc::node_ref_t node = SocTreeItemVal< soc_desc::node_ref_t >(current);
    node.get()->register_.push_back(reg);
    QTreeWidgetItem *reg_item = MakeSocTreeItem(SocTreeRegType, node.reg());
    FixupItem(reg_item);
    current->insertChild(0, reg_item);
    m_soc_tree->setCurrentItem(reg_item);
}

void RegEdit::OnSocItemDelete()
{
    QTreeWidgetItem *current = m_soc_tree->currentItem();
    if(current == 0)
        return;
    QMessageBox msgbox(QMessageBox::Question, "Delete item ?",
        "Are you sure you want to delete this item ?",
        QMessageBox::Yes | QMessageBox::No, this);
    msgbox.setDefaultButton(QMessageBox::No);
    int ret = msgbox.exec();
    if(ret != QMessageBox::Yes)
        return;
    if(current->type() == SocTreeSocType)
    {
        SocTreeItemVal< soc_desc::node_ref_t >(current).remove();
        current->takeChildren();
        OnSocModified();
    }
    else if(current->type() == SocTreeNodeType)
    {
        SocTreeItemVal< soc_desc::node_ref_t >(current).remove();
        current->parent()->removeChild(current);
        OnSocModified();
    }
    else if(current->type() == SocTreeRegType)
    {
        SocTreeItemVal< soc_desc::register_ref_t >(current).remove();
        current->parent()->removeChild(current);
        OnSocModified();
    }
}

void RegEdit::OnSocModified()
{
    // we might need to update the name in the tree
    FixupItem(m_soc_tree->currentItem());
    SetModified(true, true);
}

void RegEdit::DisplaySoc(const soc_desc::soc_ref_t& ref)
{
    SetPanel(new SocEditPanel(ref, this));
}

void RegEdit::DisplayNode(const soc_desc::node_ref_t& ref)
{
    SetPanel(new NodeEditPanel(ref, this));
}

void RegEdit::DisplayReg(const soc_desc::register_ref_t& ref)
{
    SetPanel(new RegEditPanel(ref, this));
}

void RegEdit::UpdateName(QTreeWidgetItem *current)
{
    if(current == 0)
        return;

    if(current->type() == SocTreeSocType)
    {
        current->setText(0, QString::fromStdString(
            SocTreeItemVal< soc_desc::node_ref_t >(current).soc().get()->name));
    }
    else if(current->type() == SocTreeNodeType)
    {
        current->setText(0, QString::fromStdString(
            SocTreeItemVal< soc_desc::node_ref_t >(current).get()->name));
    }
    else if(current->type() == SocTreeRegType)
    {
        current->setText(0, "register");
    }
}

void RegEdit::OnSocTreeContextMenu(QPoint point)
{
    HideMessage(m_msg_welcome_id);
    QTreeWidgetItem *item = m_soc_tree->itemAt(point);
    if(item == 0)
        return;
    /* customise messages with item */
    m_action_item = item;
    QMenu *menu = new QMenu(this);
    switch(item->type())
    {
        case SocTreeSocType:
            m_new_action->setText("New node...");
            m_delete_action->setText("Delete all nodes...");
            menu->addAction(m_new_action);
            menu->addAction(m_delete_action);
            break;
        case SocTreeNodeType:
        {
            m_new_action->setText("New node...");
            m_delete_action->setText("Delete node...");
            soc_desc::node_ref_t node = SocTreeItemVal< soc_desc::node_ref_t >(item);
            if(node.reg().node() != node)
                menu->addAction(m_create_action);
            menu->addAction(m_new_action);
            menu->addAction(m_delete_action);
            break;
        }
        case SocTreeRegType:
            m_delete_action->setText("Delete register...");
            menu->addAction(m_new_action);
            menu->addAction(m_delete_action);
            break;
    }
    menu->popup(m_soc_tree->viewport()->mapToGlobal(point));
}

void RegEdit::OnSocItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    Q_UNUSED(previous);
    HideMessage(m_msg_welcome_id);
    if(current == 0)
        return;
    if(current->type() == SocTreeSocType)
        DisplaySoc(SocTreeItemVal< soc_desc::node_ref_t >(current).soc());
    else if(current->type() == SocTreeNodeType)
        DisplayNode(SocTreeItemVal< soc_desc::node_ref_t >(current));
    else if(current->type() == SocTreeRegType)
        DisplayReg(SocTreeItemVal< soc_desc::register_ref_t >(current));
}

bool RegEdit::Quit()
{
    return CloseSoc();
}