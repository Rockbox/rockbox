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
SocEditPanel::SocEditPanel(SocRef ref, QWidget *parent)
    :QWidget(parent), m_ref(ref)
{
    m_name_group = new QGroupBox("Name", this);
    m_name_edit = new QLineEdit(this);
    m_name_edit->setText(QString::fromStdString(ref.GetSoc().name));
    QVBoxLayout *name_group_layout = new QVBoxLayout;
    name_group_layout->addWidget(m_name_edit);
    m_name_group->setLayout(name_group_layout);

    m_desc_group = new QGroupBox("Description", this);
    QHBoxLayout *group_layout = new QHBoxLayout;
    m_desc_edit = new MyTextEditor(this);
    m_desc_edit->SetTextHtml(QString::fromStdString(ref.GetSoc().desc));
    group_layout->addWidget(m_desc_edit);
    m_desc_group->setLayout(group_layout);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(m_name_group);
    layout->addWidget(m_desc_group);
    layout->addStretch(1);

    connect(m_name_edit, SIGNAL(textChanged(const QString&)), this, SLOT(OnNameEdited(const QString&)));
    connect(m_desc_edit, SIGNAL(OnTextChanged()), this, SLOT(OnTextEdited()));

    setLayout(layout);
}

void SocEditPanel::OnNameEdited(const QString& text)
{
    m_ref.GetSoc().name = text.toStdString();
    OnModified(m_name_edit->isModified());
}

void SocEditPanel::OnTextEdited()
{
    m_ref.GetSoc().desc = m_desc_edit->GetTextHtml().toStdString();
    OnModified(m_desc_edit->IsModified());
}

/**
 * DevEditPanel
 */
DevEditPanel::DevEditPanel(SocDevRef ref, QWidget *parent)
    :QWidget(parent), m_ref(ref)
{
    m_name_group = new QGroupBox("Name", this);
    m_name_edit = new QLineEdit(this);
    m_name_edit->setText(QString::fromStdString(ref.GetDev().name));
    QVBoxLayout *name_group_layout = new QVBoxLayout;
    name_group_layout->addWidget(m_name_edit);
    m_name_group->setLayout(name_group_layout);

    m_long_name_group = new QGroupBox("Long Name", this);
    m_long_name_edit = new QLineEdit(this);
    m_long_name_edit->setText(QString::fromStdString(ref.GetDev().long_name));
    QVBoxLayout *long_name_group_layout = new QVBoxLayout;
    long_name_group_layout->addWidget(m_long_name_edit);
    m_long_name_group->setLayout(long_name_group_layout);

    m_version_group = new QGroupBox("Version", this);
    m_version_edit = new QLineEdit(this);
    m_version_edit->setText(QString::fromStdString(ref.GetDev().version));
    QVBoxLayout *version_group_layout = new QVBoxLayout;
    version_group_layout->addWidget(m_version_edit);
    m_version_group->setLayout(version_group_layout);

    QVBoxLayout *name_ver_layout = new QVBoxLayout;
    name_ver_layout->addWidget(m_name_group);
    name_ver_layout->addWidget(m_long_name_group);
    name_ver_layout->addWidget(m_version_group);
    name_ver_layout->addStretch();

    m_instances_table = new QTableWidget(this);
    m_instances_table->setRowCount(ref.GetDev().addr.size() + 1);
    m_instances_table->setColumnCount(3);
    for(size_t row = 0; row < ref.GetDev().addr.size(); row++)
        FillRow(row, ref.GetDev().addr[row]);
    CreateNewRow(ref.GetDev().addr.size());
    m_instances_table->setHorizontalHeaderItem(0, new QTableWidgetItem(""));
    m_instances_table->setHorizontalHeaderItem(1, new QTableWidgetItem("Name"));
    m_instances_table->setHorizontalHeaderItem(2, new QTableWidgetItem("Address"));
    m_instances_table->verticalHeader()->setVisible(false);
    m_instances_table->resizeColumnsToContents();
    m_instances_table->horizontalHeader()->setStretchLastSection(true);
    m_instances_table->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_instances_group = new QGroupBox("Instances", this);
    QHBoxLayout *instances_group_layout = new QHBoxLayout;
    instances_group_layout->addWidget(m_instances_table);
    m_instances_group->setLayout(instances_group_layout);

    QHBoxLayout *top_layout = new QHBoxLayout;
    top_layout->addWidget(m_instances_group);
    top_layout->addLayout(name_ver_layout);
    top_layout->addStretch();

    m_desc_group = new QGroupBox("Description", this);
    QHBoxLayout *group_layout = new QHBoxLayout;
    m_desc_edit = new MyTextEditor(this);
    m_desc_edit->SetTextHtml(QString::fromStdString(ref.GetDev().desc));
    group_layout->addWidget(m_desc_edit);
    m_desc_group->setLayout(group_layout);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(top_layout, 0);
    layout->addWidget(m_desc_group, 1);

    setLayout(layout);

    SocFieldItemDelegate *m_table_delegate = new SocFieldItemDelegate(this);
    QItemEditorFactory *m_table_edit_factory = new QItemEditorFactory();
    SocFieldEditorCreator *m_table_edit_creator = new SocFieldEditorCreator();
    m_table_edit_factory->registerEditor(QVariant::UInt, m_table_edit_creator);
    m_table_delegate->setItemEditorFactory(m_table_edit_factory);
    m_instances_table->setItemDelegate(m_table_delegate);

    connect(m_instances_table, SIGNAL(cellActivated(int,int)), this, SLOT(OnInstActivated(int,int)));
    connect(m_instances_table, SIGNAL(cellChanged(int,int)), this, SLOT(OnInstChanged(int,int)));
    connect(m_name_edit, SIGNAL(textChanged(const QString&)), this, SLOT(OnNameEdited(const QString&)));
    connect(m_long_name_edit, SIGNAL(textChanged(const QString&)), this, SLOT(OnLongNameEdited(const QString&)));
    connect(m_version_edit, SIGNAL(textChanged(const QString&)), this, SLOT(OnVersionEdited(const QString&)));
    connect(m_desc_edit, SIGNAL(OnTextChanged()), this, SLOT(OnDescEdited()));
}

void DevEditPanel::OnNameEdited(const QString& text)
{
    m_ref.GetDev().name = text.toStdString();
    OnModified(m_name_edit->isModified());
}

void DevEditPanel::OnLongNameEdited(const QString& text)
{
    m_ref.GetDev().long_name = text.toStdString();
    OnModified(m_long_name_edit->isModified());
}

void DevEditPanel::OnVersionEdited(const QString& text)
{
    m_ref.GetDev().version = text.toStdString();
    OnModified(m_version_edit->isModified());
}

void DevEditPanel::OnDescEdited()
{
    m_ref.GetDev().desc = m_desc_edit->GetTextHtml().toStdString();
    OnModified(m_desc_edit->IsModified());
}

void DevEditPanel::CreateNewRow(int row)
{
    QTableWidgetItem *item = new QTableWidgetItem(QIcon::fromTheme("list-add"), "", DevInstNewType);
    item->setToolTip("New?");
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    m_instances_table->setItem(row, DevInstIconColumn, item);
    item = new QTableWidgetItem("New instance...");
    QFont font = item->font();
    font.setItalic(true);
    item->setFont(font);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    m_instances_table->setItem(row, DevInstNameColumn, item);
    item = new QTableWidgetItem("");
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    m_instances_table->setItem(row, DevInstAddrColumn, item);
}

void DevEditPanel::FillRow(int row, const soc_dev_addr_t& addr)
{
    QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(addr.name));
    item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    m_instances_table->setItem(row, DevInstNameColumn, item);
    item = new QTableWidgetItem();
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    item->setData(Qt::DisplayRole, QVariant(addr.addr));
    m_instances_table->setItem(row, DevInstAddrColumn, item);
    item = new QTableWidgetItem(QIcon::fromTheme("list-remove"), "", DevInstDeleteType);
    item->setToolTip("Remove?");
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    m_instances_table->setItem(row, DevInstIconColumn, item);
}

void DevEditPanel::OnInstActivated(int row, int column)
{
    if(column != 0)
        return;
    int type = m_instances_table->item(row, column)->type();
    if(type == DevInstDeleteType)
    {
        m_ref.GetDev().addr.erase(m_ref.GetDev().addr.begin() + row);
        m_instances_table->removeRow(row);
        OnModified(true);
    }
    else if(type == DevInstNewType)
    {
        m_instances_table->insertRow(row);
        soc_dev_addr_t addr;
        addr.name = QString("UNNAMED_%1").arg(row).toStdString();
        addr.addr = 0;
        m_ref.GetDev().addr.push_back(addr);
        FillRow(row, addr);
    }
}

void DevEditPanel::OnInstChanged(int row, int column)
{
    /* ignore extra row for addition */
    if(row >= (int)m_ref.GetDev().addr.size())
        return;
    QTableWidgetItem *item = m_instances_table->item(row, column);
    if(column == DevInstNameColumn)
    {
        m_ref.GetDev().addr[row].name =  item->text().toStdString();
        OnModified(true);
    }
    else if(column == DevInstAddrColumn)
    {
        m_ref.GetDev().addr[row].addr = item->data(Qt::DisplayRole).toUInt();
        OnModified(true);
    }
}

/**
 * RegEditPanel
 */

RegEditPanel::RegEditPanel(SocRegRef ref, QWidget *parent)
    :QWidget(parent), m_ref(ref), m_reg_font(font())
{
    m_reg_font.setWeight(100);
    m_reg_font.setKerning(false);

    m_name_group = new QGroupBox("Name", this);
    m_name_edit = new QLineEdit(this);
    m_name_edit->setText(QString::fromStdString(ref.GetReg().name));
    QVBoxLayout *name_group_layout = new QVBoxLayout;
    name_group_layout->addWidget(m_name_edit);
    m_name_group->setLayout(name_group_layout);

    m_instances_table = new QTableWidget(this);
    m_instances_table->setRowCount(ref.GetReg().addr.size() + 1);
    m_instances_table->setColumnCount(RegInstNrColumns);
    for(size_t row = 0; row < ref.GetReg().addr.size(); row++)
        FillRow(row, ref.GetReg().addr[row]);
    CreateNewAddrRow(ref.GetReg().addr.size());
    m_instances_table->setHorizontalHeaderItem(RegInstIconColumn, new QTableWidgetItem(""));
    m_instances_table->setHorizontalHeaderItem(RegInstNameColumn, new QTableWidgetItem("Name"));
    m_instances_table->setHorizontalHeaderItem(RegInstAddrColumn, new QTableWidgetItem("Address"));
    m_instances_table->verticalHeader()->setVisible(false);
    m_instances_table->resizeColumnsToContents();
    m_instances_table->horizontalHeader()->setStretchLastSection(true);
    m_instances_table->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_instances_group = new QGroupBox("Instances", this);
    QHBoxLayout *instances_group_layout = new QHBoxLayout;
    instances_group_layout->addWidget(m_instances_table);
    m_instances_group->setLayout(instances_group_layout);

    m_desc_group = new QGroupBox("Description", this);
    QHBoxLayout *group_layout = new QHBoxLayout;
    m_desc_edit = new MyTextEditor(this);
    m_desc_edit->SetTextHtml(QString::fromStdString(ref.GetReg().desc));
    group_layout->addWidget(m_desc_edit);
    m_desc_group->setLayout(group_layout);

    bool has_sct = m_ref.GetReg().flags & REG_HAS_SCT;
    m_sct_check = new QCheckBox("Set/Clear/Toggle", this);
    m_sct_check->setCheckState(has_sct ? Qt::Checked : Qt::Unchecked);
    QHBoxLayout *flags_layout = new QHBoxLayout;
    flags_layout->addWidget(m_sct_check);
    flags_layout->addStretch();
    m_flags_group = new QGroupBox("Flags", this);
    m_flags_group->setLayout(flags_layout);

    m_formula_combo = new QComboBox(this);
    m_formula_combo->addItem("None", QVariant(REG_FORMULA_NONE));
    m_formula_combo->addItem("String", QVariant(REG_FORMULA_STRING));
    m_formula_combo->setCurrentIndex(m_formula_combo->findData(QVariant(m_ref.GetReg().formula.type)));
    m_formula_type_label = new QLabel("Type:", this);
    QHBoxLayout *formula_top_layout = new QHBoxLayout;
    formula_top_layout->addWidget(m_formula_type_label);
    formula_top_layout->addWidget(m_formula_combo);
    m_formula_string_edit = new QLineEdit(QString::fromStdString(ref.GetReg().formula.string), this);
    QVBoxLayout *formula_layout = new QVBoxLayout;
    formula_layout->addLayout(formula_top_layout);
    formula_layout->addWidget(m_formula_string_edit);
    m_formula_string_gen = new QPushButton("Generate", this);
    formula_layout->addWidget(m_formula_string_gen);
    m_formula_group = new QGroupBox("Formula", this);
    m_formula_group->setLayout(formula_layout);

    QVBoxLayout *name_layout = new QVBoxLayout;
    name_layout->addWidget(m_name_group);
    name_layout->addWidget(m_flags_group);
    name_layout->addWidget(m_formula_group);
    name_layout->addStretch();

    QHBoxLayout *top_layout = new QHBoxLayout;
    top_layout->addWidget(m_instances_group);
    top_layout->addLayout(name_layout);
    top_layout->addWidget(m_desc_group, 1);

    m_sexy_display = new RegSexyDisplay(m_ref, this);
    m_sexy_display->setFont(m_reg_font);

    m_field_table = new QTableWidget;
    m_field_table->setRowCount(m_ref.GetReg().field.size());
    m_field_table->setColumnCount(4);
    for(size_t row = 0; row < m_ref.GetReg().field.size(); row++)
    {
        const soc_reg_field_t& field = m_ref.GetReg().field[row];
        QString bits_str;
        if(field.first_bit == field.last_bit)
            bits_str.sprintf("%d", field.first_bit);
        else
            bits_str.sprintf("%d:%d", field.last_bit, field.first_bit);
        QTableWidgetItem *item = new QTableWidgetItem(bits_str);
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        m_field_table->setItem(row, 1, item);
        item = new QTableWidgetItem(QString(field.name.c_str()));
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        m_field_table->setItem(row, 2, item);
        item = new QTableWidgetItem(QString(field.desc.c_str()));
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        m_field_table->setItem(row, 3, item);
        UpdateWarning(row);
    }
    m_field_table->setHorizontalHeaderItem(0, new QTableWidgetItem(""));
    m_field_table->setHorizontalHeaderItem(1, new QTableWidgetItem("Bits"));
    m_field_table->setHorizontalHeaderItem(2, new QTableWidgetItem("Name"));
    m_field_table->setHorizontalHeaderItem(3, new QTableWidgetItem("Description"));
    m_field_table->verticalHeader()->setVisible(false);
    m_field_table->resizeColumnsToContents();
    m_field_table->horizontalHeader()->setStretchLastSection(true);
    QHBoxLayout *field_layout = new QHBoxLayout;
    field_layout->addWidget(m_field_table);
    m_field_group = new QGroupBox("Flags", this);
    m_field_group->setLayout(field_layout);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(top_layout, 0);
    layout->addWidget(m_sexy_display, 0);
    layout->addWidget(m_field_group);

    UpdateFormula();

    setLayout(layout);

    SocFieldItemDelegate *m_table_delegate = new SocFieldItemDelegate(this);
    QItemEditorFactory *m_table_edit_factory = new QItemEditorFactory();
    SocFieldEditorCreator *m_table_edit_creator = new SocFieldEditorCreator();
    m_table_edit_factory->registerEditor(QVariant::UInt, m_table_edit_creator);
    m_table_delegate->setItemEditorFactory(m_table_edit_factory);
    m_instances_table->setItemDelegate(m_table_delegate);

    connect(m_instances_table, SIGNAL(cellActivated(int,int)), this, SLOT(OnInstActivated(int,int)));
    connect(m_instances_table, SIGNAL(cellChanged(int,int)), this, SLOT(OnInstChanged(int,int)));
    connect(m_name_edit, SIGNAL(textChanged(const QString&)), this, SLOT(OnNameEdited(const QString&)));
    connect(m_desc_edit, SIGNAL(OnTextChanged()), this, SLOT(OnDescEdited()));
    connect(m_sct_check, SIGNAL(stateChanged(int)), this, SLOT(OnSctEdited(int)));
    connect(m_formula_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(OnFormulaChanged(int)));
    connect(m_formula_string_edit, SIGNAL(textChanged(const QString&)), this, 
        SLOT(OnFormulaStringChanged(const QString&)));
    connect(m_formula_string_gen, SIGNAL(clicked(bool)), this, SLOT(OnFormulaGenerate(bool)));
}

void RegEditPanel::UpdateWarning(int row)
{
    Q_UNUSED(row);
}

void RegEditPanel::OnFormulaStringChanged(const QString& text)
{
    m_ref.GetReg().formula.string = text.toStdString();
    OnModified(true);
}

void RegEditPanel::OnFormulaGenerate(bool checked)
{
    Q_UNUSED(checked);
    bool ok;
    int count = QInputDialog::getInt(this, "Instance generator", "Number of instances",
        0, 0, 100, 1, &ok);
    if(!ok)
        return;
    std::string name(m_ref.GetReg().name);
    size_t pos = name.find('n');
    if(pos == std::string::npos)
    {
        name.push_back('n');
        pos = name.size() - 1;
    }
    std::map< std::string, soc_word_t > map;
    std::vector< std::pair< std::string, soc_word_t > > list;
    std::string formula = m_ref.GetReg().formula.string;
    for(int n = 0; n < count; n++)
    {
        map["n"] = n;
        std::string err;
        soc_word_t res;
        if(!soc_desc_evaluate_formula(formula, map, res, err))
        {
            qDebug() << "Cannot evaluator " << QString::fromStdString(formula) 
                << "for n=" << n << ": " << QString::fromStdString(err);
            return;
        }
        std::string regname = name;
        std::string strn = QString("%1").arg(n).toStdString();
        regname.replace(pos, 1, strn);
        list.push_back(std::make_pair(regname, res));
    }
    // everything went good, commit result
    while(m_instances_table->rowCount() > 1)
        m_instances_table->removeRow(0);
    m_ref.GetReg().addr.resize(list.size());
    for(size_t i = 0; i < list.size(); i++)
    {
        m_instances_table->insertRow(i);
        m_ref.GetReg().addr[i].name = list[i].first;
        m_ref.GetReg().addr[i].addr = list[i].second;
        FillRow(i, m_ref.GetReg().addr[i]);
    }
}

void RegEditPanel::OnFormulaChanged(int index)
{
    if(index == -1)
        return;
    m_ref.GetReg().formula.type = static_cast< soc_reg_formula_type_t >(m_formula_combo->itemData(index).toInt());
    UpdateFormula();
    OnModified(true);
}

void RegEditPanel::UpdateFormula()
{
    m_formula_string_edit->hide();
    m_formula_string_gen->hide();
    switch(m_ref.GetReg().formula.type)
    {
        case REG_FORMULA_STRING:
            m_formula_string_edit->show();
            m_formula_string_gen->show();
            break;
        case REG_FORMULA_NONE:
        default:
            break;
    }
}

void RegEditPanel::OnSctEdited(int state)
{
    if(state == Qt::Checked)
        m_ref.GetReg().flags |= REG_HAS_SCT;
    else
        m_ref.GetReg().flags &= ~REG_HAS_SCT;
    OnModified(true);
}

void RegEditPanel::FillRow(int row, const soc_reg_addr_t& addr)
{
    QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(addr.name));
    item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    m_instances_table->setItem(row, RegInstNameColumn, item);
    item = new QTableWidgetItem();
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    item->setData(Qt::DisplayRole, QVariant(addr.addr));
    m_instances_table->setItem(row, RegInstAddrColumn, item);
    item = new QTableWidgetItem(QIcon::fromTheme("list-remove"), "", RegInstDeleteType);
    item->setToolTip("Remove?");
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    m_instances_table->setItem(row, RegInstIconColumn, item);
}

void RegEditPanel::CreateNewAddrRow(int row)
{
    QTableWidgetItem *item = new QTableWidgetItem(QIcon::fromTheme("list-add"), "", RegInstNewType);
    item->setToolTip("New?");
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    m_instances_table->setItem(row, RegInstIconColumn, item);
    item = new QTableWidgetItem("New instance...");
    QFont font = item->font();
    font.setItalic(true);
    item->setFont(font);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    m_instances_table->setItem(row, RegInstNameColumn, item);
    item = new QTableWidgetItem("");
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    m_instances_table->setItem(row, RegInstAddrColumn, item);
}

void RegEditPanel::OnNameEdited(const QString& text)
{
    m_ref.GetReg().name = text.toStdString();
    OnModified(m_name_edit->isModified());
}

void RegEditPanel::OnDescEdited()
{
    m_ref.GetReg().desc = m_desc_edit->GetTextHtml().toStdString();
    OnModified(m_desc_edit->IsModified());
}

void RegEditPanel::OnInstActivated(int row, int column)
{
    if(column != 0)
        return;
    int type = m_instances_table->item(row, column)->type();
    if(type == RegInstDeleteType)
    {
        m_ref.GetReg().addr.erase(m_ref.GetReg().addr.begin() + row);
        m_instances_table->removeRow(row);
        OnModified(true);
    }
    else if(type == RegInstNewType)
    {
        m_instances_table->insertRow(row);
        soc_reg_addr_t addr;
        addr.name = QString("UNNAMED_%1").arg(row).toStdString();
        addr.addr = 0;
        m_ref.GetReg().addr.push_back(addr);
        FillRow(row, addr);
    }
}

void RegEditPanel::OnInstChanged(int row, int column)
{
    /* ignore extra row for addition */
    if(row >= (int)m_ref.GetReg().addr.size())
        return;
    QTableWidgetItem *item = m_instances_table->item(row, column);
    if(column == RegInstNameColumn)
    {
        m_ref.GetReg().addr[row].name = item->text().toStdString();
        OnModified(true);
    }
    else if(column == RegInstAddrColumn)
    {
        m_ref.GetReg().addr[row].addr = item->data(Qt::DisplayRole).toUInt();
        OnModified(true);
    }
}

/**
 * FieldEditPanel
 */
FieldEditPanel::FieldEditPanel(SocFieldRef ref, QWidget *parent)
    :QWidget(parent), m_ref(ref)
{
    m_name_group = new QGroupBox("Name", this);
    m_name_edit = new QLineEdit(this);
    m_name_edit->setText(QString::fromStdString(ref.GetField().name));
    QVBoxLayout *name_group_layout = new QVBoxLayout;
    name_group_layout->addWidget(m_name_edit);
    m_name_group->setLayout(name_group_layout);

    m_bitrange_group = new QGroupBox("Bit Range", this);
    m_bitrange_edit = new QLineEdit(this);
    const soc_reg_field_t& field = ref.GetField();
    QString bits_str;
    if(field.first_bit == field.last_bit)
        bits_str.sprintf("%d", field.first_bit);
    else
        bits_str.sprintf("%d:%d", field.last_bit, field.first_bit);
    m_bitrange_edit->setText(bits_str);
    m_bitrange_edit->setValidator(new SocBitRangeValidator(m_bitrange_edit));
    QVBoxLayout *bitrange_group_layout = new QVBoxLayout;
    bitrange_group_layout->addWidget(m_bitrange_edit);
    m_bitrange_group->setLayout(bitrange_group_layout);

    m_desc_group = new QGroupBox("Description", this);
    QHBoxLayout *group_layout = new QHBoxLayout;
    m_desc_edit = new MyTextEditor(this);
    m_desc_edit->SetTextHtml(QString::fromStdString(ref.GetField().desc));
    group_layout->addWidget(m_desc_edit);
    m_desc_group->setLayout(group_layout);

    m_value_group = new QGroupBox("Values", this);
    QHBoxLayout *value_layout = new QHBoxLayout;
    m_value_table = new QTableWidget(this);
    m_value_table->setRowCount(ref.GetField().value.size() + 1);
    m_value_table->setColumnCount(FieldValueNrColumns);
    for(size_t row = 0; row < ref.GetField().value.size(); row++)
        FillRow(row, ref.GetField().value[row]);
    CreateNewRow(ref.GetField().value.size());
    m_value_table->setHorizontalHeaderItem(FieldValueIconColumn, new QTableWidgetItem(""));
    m_value_table->setHorizontalHeaderItem(FieldValueNameColumn, new QTableWidgetItem("Name"));
    m_value_table->setHorizontalHeaderItem(FieldValueValueColumn, new QTableWidgetItem("Value"));
    m_value_table->setHorizontalHeaderItem(FieldValueDescColumn, new QTableWidgetItem("Description"));
    m_value_table->verticalHeader()->setVisible(false);
    m_value_table->horizontalHeader()->setStretchLastSection(true);
    value_layout->addWidget(m_value_table);
    m_value_group->setLayout(value_layout);

    QHBoxLayout *line_layout = new QHBoxLayout;
    line_layout->addWidget(m_name_group);
    line_layout->addWidget(m_bitrange_group);
    line_layout->addStretch();

    QVBoxLayout *left_layout = new QVBoxLayout;
    left_layout->addLayout(line_layout);
    left_layout->addWidget(m_desc_group);
    left_layout->addWidget(m_value_group, 1);

    UpdateDelegates();

    connect(m_name_edit, SIGNAL(textChanged(const QString&)), this, SLOT(OnNameEdited(const QString&)));
    connect(m_desc_edit, SIGNAL(OnTextChanged()), this, SLOT(OnDescEdited()));
    connect(m_value_table, SIGNAL(cellActivated(int,int)), this, SLOT(OnValueActivated(int,int)));
    connect(m_value_table, SIGNAL(cellChanged(int,int)), this, SLOT(OnValueChanged(int,int)));
    connect(m_bitrange_edit, SIGNAL(textChanged(const QString&)), this, SLOT(OnBitRangeEdited(const QString&)));

    setLayout(left_layout);
}

void FieldEditPanel::UpdateDelegates()
{
    SocFieldItemDelegate *m_table_delegate = new SocFieldItemDelegate(m_ref.GetField(), this);
    QItemEditorFactory *m_table_edit_factory = new QItemEditorFactory();
    SocFieldEditorCreator *m_table_edit_creator = new SocFieldEditorCreator(m_ref.GetField());
    m_table_edit_factory->registerEditor(QVariant::UInt, m_table_edit_creator);
    m_table_delegate->setItemEditorFactory(m_table_edit_factory);
    m_value_table->setItemDelegate(m_table_delegate);
    m_value_table->resizeColumnsToContents();
}

void FieldEditPanel::UpdateWarning(int row)
{
    soc_word_t val = m_ref.GetField().value[row].value;
    soc_word_t max = m_ref.GetField().bitmask() >> m_ref.GetField().first_bit;
    QTableWidgetItem *item = m_value_table->item(row, FieldValueValueColumn);
    if(val > max)
    {
        item->setIcon(QIcon::fromTheme("dialog-warning"));
        item->setToolTip("Value is too big for the field");
    }
    else
    {
        item->setIcon(QIcon());
        item->setToolTip("");
    }
}

void FieldEditPanel::FillRow(int row, const soc_reg_field_value_t& val)
{
    QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(val.name));
    item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    m_value_table->setItem(row, FieldValueNameColumn, item);
    item = new QTableWidgetItem();
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    item->setData(Qt::DisplayRole, QVariant(val.value));
    m_value_table->setItem(row, FieldValueValueColumn, item);
    item = new QTableWidgetItem(QString::fromStdString(val.desc));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    m_value_table->setItem(row, FieldValueDescColumn, item);
    item = new QTableWidgetItem(QIcon::fromTheme("list-remove"), "", FieldValueDeleteType);
    item->setToolTip("Remove?");
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    m_value_table->setItem(row, FieldValueIconColumn, item);
    UpdateWarning(row);
}

void FieldEditPanel::CreateNewRow(int row)
{
    QTableWidgetItem *item = new QTableWidgetItem(QIcon::fromTheme("list-add"), "", FieldValueNewType);
    item->setToolTip("New?");
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    m_value_table->setItem(row, FieldValueIconColumn, item);
    item = new QTableWidgetItem("New value...");
    QFont font = item->font();
    font.setItalic(true);
    item->setFont(font);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    m_value_table->setItem(row, FieldValueNameColumn, item);
}

void FieldEditPanel::OnBitRangeEdited(const QString& input)
{
    const SocBitRangeValidator *validator = 
        dynamic_cast< const SocBitRangeValidator *>(m_bitrange_edit->validator());
    int first, last;
    QValidator::State state = validator->parse(input, last, first);
    if(state != QValidator::Acceptable)
        return;
    m_ref.GetField().first_bit = first;
    m_ref.GetField().last_bit = last;
    // update all warning signs
    for(size_t row = 0; row < m_ref.GetField().value.size(); row++)
        UpdateWarning(row);
    // also updates delegates because they now have the wrong view of the field
    UpdateDelegates();
    OnModified(true);
}

void FieldEditPanel::OnNameEdited(const QString& text)
{
    m_ref.GetField().name = text.toStdString();
    OnModified(m_name_edit->isModified());
}

void FieldEditPanel::OnDescEdited()
{
    m_ref.GetField().desc = m_desc_edit->GetTextHtml().toStdString();
    OnModified(m_desc_edit->IsModified());
}

void FieldEditPanel::OnValueActivated(int row, int column)
{
    if(column != 0)
        return;
    int type = m_value_table->item(row, column)->type();
    if(type == FieldValueDeleteType)
    {
        m_ref.GetField().value.erase(m_ref.GetField().value.begin() + row);
        m_value_table->removeRow(row);
        OnModified(true);
    }
    else if(type == FieldValueNewType)
    {
        m_value_table->insertRow(row);
        soc_reg_field_value_t val;
        val.name = QString("UNNAMED_%1").arg(row).toStdString();
        val.value = 0;
        m_ref.GetField().value.push_back(val);
        FillRow(row, val);
    }
}

void FieldEditPanel::OnValueChanged(int row, int column)
{
    /* ignore extra row for addition */
    if(row >= (int)m_ref.GetField().value.size())
        return;
    QTableWidgetItem *item = m_value_table->item(row, column);
    if(column == FieldValueNameColumn)
        m_ref.GetField().value[row].name = item->text().toStdString();
    else if(column == FieldValueValueColumn)
    {
        soc_word_t& fval = m_ref.GetField().value[row].value;
        soc_word_t new_val = item->data(Qt::DisplayRole).toUInt();
        /* avoid infinite recursion by calling UpdateWarning() when
         * only the icon changes which would trigger this callback again */
        if(fval != new_val)
        {
            fval = new_val;
            UpdateWarning(row);
        }
    }
    else if(column == FieldValueDescColumn)
        m_ref.GetField().value[row].desc = item->text().toStdString();
    OnModified(true);
}

namespace
{

enum
{
    SocTreeSocType = QTreeWidgetItem::UserType,
    SocTreeDevType,
    SocTreeRegType,
    SocTreeFieldType,
    SocTreeNewDevType,
    SocTreeNewRegType,
    SocTreeNewFieldType,
};

/**
 * SocTreeItem
 */

class SocTreeItem : public QTreeWidgetItem
{
public:
    SocTreeItem(const QString& string, const SocRef& ref)
        :QTreeWidgetItem(QStringList(string), SocTreeSocType), m_ref(ref) {}

    const SocRef& GetRef() { return m_ref; }
private:
    SocRef m_ref;
};

/**
 * NewDevTreeItem
 */

class NewDevTreeItem : public QTreeWidgetItem
{
public:
    NewDevTreeItem(const QString& string, const SocRef& ref)
        :QTreeWidgetItem(QStringList(string), SocTreeNewDevType), m_ref(ref) {}

    const SocRef& GetRef() { return m_ref; }
private:
    SocRef m_ref;
};

/**
 * DevTreeItem
 */

class DevTreeItem : public QTreeWidgetItem
{
public:
    DevTreeItem(const QString& string, const SocDevRef& ref)
        :QTreeWidgetItem(QStringList(string), SocTreeDevType), m_ref(ref) {}

    const SocDevRef& GetRef() { return m_ref; }
private:
    SocDevRef m_ref;
};

/**
 * NewRegTreeItem
 */

class NewRegTreeItem : public QTreeWidgetItem
{
public:
    NewRegTreeItem(const QString& string, const SocDevRef& ref)
        :QTreeWidgetItem(QStringList(string), SocTreeNewRegType), m_ref(ref) {}

    const SocDevRef& GetRef() { return m_ref; }
private:
    SocDevRef m_ref;
};

/**
 * RegTreeItem
 */

class RegTreeItem : public QTreeWidgetItem
{
public:
    RegTreeItem(const QString& string, const SocRegRef& ref)
        :QTreeWidgetItem(QStringList(string), SocTreeRegType), m_ref(ref) {}

    const SocRegRef& GetRef() { return m_ref; }
private:
    SocRegRef m_ref;
};

/**
 * NewFieldTreeItem
 */

class NewFieldTreeItem : public QTreeWidgetItem
{
public:
    NewFieldTreeItem(const QString& string, const SocRegRef& ref)
        :QTreeWidgetItem(QStringList(string), SocTreeNewFieldType), m_ref(ref) {}

    const SocRegRef& GetRef() { return m_ref; }
private:
    SocRegRef m_ref;
};

/**
 * FieldTreeItem
 */

class FieldTreeItem : public QTreeWidgetItem
{
public:
    FieldTreeItem(const QString& string, const SocFieldRef& ref)
        :QTreeWidgetItem(QStringList(string), SocTreeFieldType), m_ref(ref) {}

    const SocFieldRef& GetRef() { return m_ref; }
private:
    SocFieldRef m_ref;
};

}

/**
 * RegEdit
 */
RegEdit::RegEdit(Backend *backend, QWidget *parent)
    :QWidget(parent), m_backend(backend)
{
    QVBoxLayout *m_vert_layout = new QVBoxLayout();
    m_file_group = new QGroupBox("File selection", this);
    QHBoxLayout *m_file_group_layout = new QHBoxLayout();
    m_file_edit = new QLineEdit(this);
    m_file_edit->setReadOnly(true);
    m_file_open = new QToolButton(this);
    m_file_open->setText("Open");
    m_file_open->setIcon(QIcon::fromTheme("document-open"));
    m_file_open->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    QMenu *file_open_menu = new QMenu(this);
    QAction *new_act = file_open_menu->addAction(QIcon::fromTheme("document-new"), "New...");
    m_file_open->setMenu(file_open_menu);

    m_file_save = new QToolButton(this);
    m_file_save->setText("Save");
    m_file_save->setIcon(QIcon::fromTheme("document-save"));
    m_file_save->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    QMenu *file_save_menu = new QMenu(this);
    QAction *saveas_act = file_save_menu->addAction(QIcon::fromTheme("document-save-as"), "Save as...");
    m_file_save->setMenu(file_save_menu);
    m_file_group_layout->addWidget(m_file_open);
    m_file_group_layout->addWidget(m_file_save);
    m_file_group_layout->addWidget(m_file_edit);

    m_splitter = new QSplitter(this);
    m_soc_tree = new QTreeWidget(this);
    m_soc_tree->setColumnCount(1);
    m_soc_tree->setHeaderLabel(QString("Name"));
    m_soc_tree->setContextMenuPolicy(Qt::ActionsContextMenu);
    QAction *soc_tree_delete_action = new QAction("&Delete", this);
    soc_tree_delete_action->setIcon(QIcon::fromTheme("list-remove"));
    connect(soc_tree_delete_action, SIGNAL(triggered()), this, SLOT(OnSocItemDelete()));
    m_soc_tree->addAction(soc_tree_delete_action);
    m_splitter->addWidget(m_soc_tree);
    m_splitter->setStretchFactor(0, 0);

    m_file_group->setLayout(m_file_group_layout);
    m_vert_layout->addWidget(m_file_group);
    m_vert_layout->addWidget(m_splitter, 1);

    setLayout(m_vert_layout);

    SetModified(false, false);
    m_right_panel = 0;
    SetPanel(new EmptyEditPanel(this));
    UpdateTabName();

    connect(m_file_open, SIGNAL(clicked()), this, SLOT(OnOpen()));
    connect(m_file_save, SIGNAL(clicked()), this, SLOT(OnSave()));
    connect(new_act, SIGNAL(triggered()), this, SLOT(OnNew()));
    connect(saveas_act, SIGNAL(triggered()), this, SLOT(OnSaveAs()));
    connect(m_soc_tree, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
        this, SLOT(OnSocItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
    connect(m_soc_tree, SIGNAL(itemActivated(QTreeWidgetItem*, int)),
        this, SLOT(OnSocItemActivated(QTreeWidgetItem*, int)));
}

QWidget *RegEdit::GetWidget()
{
    return this;
}

RegEdit::~RegEdit()
{
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
    fd->setFilter("Description files (*.xml);;All files (*)");
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
    soc_desc_normalize(m_cur_socfile.GetSoc());
    if(!soc_desc_produce_xml(filename.toStdString(), m_cur_socfile.GetSoc()))
    {
        QMessageBox::warning(this, "The description was not saved",
            "There was an error when saving the file");
        return false;
    }
    m_soc_tree->clear();
    FillSocTree();
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
}

void RegEdit::CreateNewFieldItem(QTreeWidgetItem *_parent)
{
    RegTreeItem *parent = dynamic_cast< RegTreeItem* >(_parent);
    NewFieldTreeItem *newdev_item = new NewFieldTreeItem("New field...", parent->GetRef());
    MakeItalic(newdev_item, true);
    newdev_item->setIcon(0, QIcon::fromTheme("list-add"));
    parent->addChild(newdev_item);
}

void RegEdit::FillRegTreeItem(QTreeWidgetItem *_item)
{
    RegTreeItem *item = dynamic_cast< RegTreeItem* >(_item);
    const soc_reg_t& reg = item->GetRef().GetReg();
    for(size_t i = 0; i < reg.field.size(); i++)
    {
        const soc_reg_field_t& field = reg.field[i];
        FieldTreeItem *field_item = new FieldTreeItem(QString::fromStdString(field.name),
            SocFieldRef(item->GetRef(), i));
        FixupEmptyItem(field_item);
        item->addChild(field_item);
    }
    CreateNewFieldItem(item);
}

void RegEdit::CreateNewRegisterItem(QTreeWidgetItem *_parent)
{
    DevTreeItem *parent = dynamic_cast< DevTreeItem* >(_parent);
    NewRegTreeItem *newdev_item = new NewRegTreeItem("New register...", parent->GetRef());
    MakeItalic(newdev_item, true);
    newdev_item->setIcon(0, QIcon::fromTheme("list-add"));
    parent->addChild(newdev_item);
}

void RegEdit::FillDevTreeItem(QTreeWidgetItem *_item)
{
    DevTreeItem *item = dynamic_cast< DevTreeItem* >(_item);
    const soc_dev_t& dev = item->GetRef().GetDev();
    for(size_t i = 0; i < dev.reg.size(); i++)
    {
        const soc_reg_t& reg = dev.reg[i];
        RegTreeItem *reg_item = new RegTreeItem(QString::fromStdString(reg.name),
            SocRegRef(item->GetRef(), i, -1));
        FixupEmptyItem(reg_item);
        FillRegTreeItem(reg_item);
        item->addChild(reg_item);
    }
    CreateNewRegisterItem(item);
}

void RegEdit::CreateNewDeviceItem(QTreeWidgetItem *_parent)
{
    SocTreeItem *parent = dynamic_cast< SocTreeItem* >(_parent);
    NewDevTreeItem *newdev_item = new NewDevTreeItem("New device...", parent->GetRef());
    MakeItalic(newdev_item, true);
    newdev_item->setIcon(0, QIcon::fromTheme("list-add"));
    parent->addChild(newdev_item);
}

void RegEdit::FillSocTreeItem(QTreeWidgetItem *_item)
{
    SocTreeItem *item = dynamic_cast< SocTreeItem* >(_item);
    const soc_t& soc = item->GetRef().GetSoc();
    for(size_t i = 0; i < soc.dev.size(); i++)
    {
        const soc_dev_t& reg = soc.dev[i];
        DevTreeItem *dev_item = new DevTreeItem(QString::fromStdString(reg.name),
            SocDevRef(item->GetRef(), i, -1));
        FixupEmptyItem(dev_item);
        FillDevTreeItem(dev_item);
        item->addChild(dev_item);
    }
    CreateNewDeviceItem(item);
}

void RegEdit::FillSocTree()
{
    SocRef ref = m_cur_socfile.GetSocRef();
    SocTreeItem *soc_item = new SocTreeItem(
        QString::fromStdString(ref.GetSoc().name), ref);
    FixupEmptyItem(soc_item);
    FillSocTreeItem(soc_item);
    m_soc_tree->addTopLevelItem(soc_item);
    soc_item->setExpanded(true);
}

void RegEdit::MakeItalic(QTreeWidgetItem *item, bool it)
{
    QFont font = item->font(0);
    font.setItalic(it);
    item->setFont(0, font);
}

void RegEdit::FixupEmptyItem(QTreeWidgetItem *item)
{
    if(item->text(0).size() == 0)
    {
        item->setIcon(0, QIcon::fromTheme("dialog-error"));
        MakeItalic(item, true);
        item->setText(0, "Unnamed");
    }
    else
    {
        item->setIcon(0, QIcon::fromTheme("cpu"));
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
    connect(m_right_panel, SIGNAL(OnModified(bool)), this, SLOT(OnSocModified(bool)));
    m_splitter->addWidget(m_right_panel);
    m_splitter->setStretchFactor(1, 2);
}

void RegEdit::SetModified(bool add, bool mod)
{
    m_modified = add ? (m_modified || mod) : mod;
    OnModified(mod);
}

namespace
{

template< typename T >
void my_remove_at(std::vector< T >& v, size_t at)
{
    v.erase(v.begin() + at);
}

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
        SocTreeItem *item = dynamic_cast< SocTreeItem * >(current);
        item->GetRef().GetSoc().dev.clear();
        item->takeChildren();
        FillSocTreeItem(item);
        m_soc_tree->expandItem(item);
    }
    else if(current->type() == SocTreeDevType)
    {
        DevTreeItem *item = dynamic_cast< DevTreeItem * >(current);
        my_remove_at(item->GetRef().GetSoc().dev, item->GetRef().GetDevIndex());
        QTreeWidgetItem *parent = item->parent();
        parent->takeChildren();
        FillSocTreeItem(parent);
        m_soc_tree->expandItem(parent);
    }
    else if(current->type() == SocTreeRegType)
    {
        RegTreeItem *item = dynamic_cast< RegTreeItem * >(current);
        my_remove_at(item->GetRef().GetDev().reg, item->GetRef().GetRegIndex());
        QTreeWidgetItem *parent = item->parent();
        parent->takeChildren();
        FillDevTreeItem(parent);
        m_soc_tree->expandItem(parent);
    }
    else if(current->type() == SocTreeFieldType)
    {
        FieldTreeItem *item = dynamic_cast< FieldTreeItem * >(current);
        my_remove_at(item->GetRef().GetReg().field, item->GetRef().GetFieldIndex());
        QTreeWidgetItem *parent = item->parent();
        parent->takeChildren();
        FillRegTreeItem(parent);
        m_soc_tree->expandItem(parent);
    }
}

void RegEdit::OnSocModified(bool modified)
{
    // we might need to update the name in the tree
    UpdateName(m_soc_tree->currentItem());
    if(modified)
        SetModified(true, true);
}

void RegEdit::DisplaySoc(SocRef ref)
{
    SetPanel(new SocEditPanel(ref, this));
}

void RegEdit::DisplayDev(SocDevRef ref)
{
    SetPanel(new DevEditPanel(ref, this));
}

void RegEdit::DisplayReg(SocRegRef ref)
{
    SetPanel(new RegEditPanel(ref, this));
}

void RegEdit::DisplayField(SocFieldRef ref)
{
    SetPanel(new FieldEditPanel(ref, this));
}

void RegEdit::UpdateName(QTreeWidgetItem *current)
{
    if(current == 0)
        return;
    if(current->type() == SocTreeSocType)
    {
        SocTreeItem *item = dynamic_cast< SocTreeItem * >(current);
        item->setText(0, QString::fromStdString(item->GetRef().GetSoc().name));
    }
    else if(current->type() == SocTreeDevType)
    {
        DevTreeItem *item = dynamic_cast< DevTreeItem * >(current);
        item->setText(0, QString::fromStdString(item->GetRef().GetDev().name));
    }
    else if(current->type() == SocTreeRegType)
    {
        RegTreeItem *item = dynamic_cast< RegTreeItem * >(current);
        item->setText(0, QString::fromStdString(item->GetRef().GetReg().name));
    }
    else if(current->type() == SocTreeFieldType)
    {
        FieldTreeItem *item = dynamic_cast< FieldTreeItem * >(current);
        item->setText(0, QString::fromStdString(item->GetRef().GetField().name));
    }
    FixupEmptyItem(current);
}

void RegEdit::OnSocItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    Q_UNUSED(previous);
    if(current == 0)
        return;
    if(current->type() == SocTreeSocType)
    {
        SocTreeItem *item = dynamic_cast< SocTreeItem * >(current);
        DisplaySoc(item->GetRef());
    }
    else if(current->type() == SocTreeDevType)
    {
        DevTreeItem *item = dynamic_cast< DevTreeItem * >(current);
        DisplayDev(item->GetRef());
    }
    else if(current->type() == SocTreeRegType)
    {
        RegTreeItem *item = dynamic_cast< RegTreeItem * >(current);
        DisplayReg(item->GetRef());
    }
    else if(current->type() == SocTreeFieldType)
    {
        FieldTreeItem *item = dynamic_cast< FieldTreeItem * >(current);
        DisplayField(item->GetRef());
    }
}

void RegEdit::OnSocItemActivated(QTreeWidgetItem *current, int column)
{
    Q_UNUSED(column);
    if(current == 0)
        return;
    if(current->type() == SocTreeNewDevType)
        AddDevice(current);
    else if(current->type() == SocTreeNewRegType)
        AddRegister(current);
    else if(current->type() == SocTreeNewFieldType)
        AddField(current);
}

void RegEdit::AddDevice(QTreeWidgetItem *_item)
{
    NewDevTreeItem *item = dynamic_cast< NewDevTreeItem * >(_item);
    item->GetRef().GetSoc().dev.push_back(soc_dev_t());
    DevTreeItem *dev_item = new DevTreeItem("",
        SocDevRef(item->GetRef(), item->GetRef().GetSoc().dev.size() - 1, -1));
    FixupEmptyItem(dev_item);
    item->parent()->insertChild(item->parent()->indexOfChild(item), dev_item);
    CreateNewRegisterItem(dev_item);
    m_soc_tree->setCurrentItem(dev_item);
    OnModified(true);
}

void RegEdit::AddRegister(QTreeWidgetItem *_item)
{
    NewRegTreeItem *item = dynamic_cast< NewRegTreeItem * >(_item);
    item->GetRef().GetDev().reg.push_back(soc_reg_t());
    RegTreeItem *reg_item = new RegTreeItem("",
        SocRegRef(item->GetRef(), item->GetRef().GetDev().reg.size() - 1, -1));
    FixupEmptyItem(reg_item);
    item->parent()->insertChild(item->parent()->indexOfChild(item), reg_item);
    CreateNewFieldItem(reg_item);
    m_soc_tree->setCurrentItem(reg_item);
    OnModified(true);
}

void RegEdit::AddField(QTreeWidgetItem *_item)
{
    NewFieldTreeItem *item = dynamic_cast< NewFieldTreeItem * >(_item);
    item->GetRef().GetReg().field.push_back(soc_reg_field_t());
    FieldTreeItem *field_item = new FieldTreeItem("",
        SocFieldRef(item->GetRef(), item->GetRef().GetReg().field.size() - 1));
    FixupEmptyItem(field_item);
    item->parent()->insertChild(item->parent()->indexOfChild(item), field_item);
    m_soc_tree->setCurrentItem(field_item);
    OnModified(true);
}

bool RegEdit::Quit()
{
    return CloseSoc();
}