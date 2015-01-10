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
SocEditPanel::SocEditPanel(const soc_desc::soc_ref_t& ref, QWidget *parent)
    :QWidget(parent), m_ref(ref)
{
    QLineEdit *name_edit = new QLineEdit(this);
    name_edit->setText(QString::fromStdString(ref.get()->name));
    QGroupBox *name_group = Misc::EncloseInBox("Name", name_edit);

    QLineEdit *title_edit = new QLineEdit(this);
    title_edit->setText(QString::fromStdString(ref.get()->title));
    QGroupBox *title_group = Misc::EncloseInBox("Title", title_edit);

    QLineEdit *isa_edit = new QLineEdit(this);
    isa_edit->setText(QString::fromStdString(ref.get()->isa));
    QGroupBox *isa_group = Misc::EncloseInBox("Instruction Set", isa_edit);

    QLineEdit *version_edit = new QLineEdit(this);
    version_edit->setText(QString::fromStdString(ref.get()->version));
    QGroupBox *version_group = Misc::EncloseInBox("Version", version_edit);

    m_authors_list = new QListWidget(this);
    std::vector< std::string >& authors = ref.get()->author;
    for(size_t i = 0; i < authors.size(); i++)
        m_authors_list->addItem(QString::fromStdString(authors[i]));
    QGroupBox *authors_group = Misc::EncloseInBox("Authors", m_authors_list);

    m_desc_edit = new MyTextEditor(this);
    m_desc_edit->SetTextHtml(QString::fromStdString(ref.get()->desc));
    QGroupBox *desc_group = Misc::EncloseInBox("Description", m_desc_edit);

    QVBoxLayout *banner_left_layout = new QVBoxLayout;
    banner_left_layout->addWidget(name_group);
    banner_left_layout->addWidget(title_group);
    banner_left_layout->addWidget(isa_group);
    banner_left_layout->addWidget(version_group);
    banner_left_layout->addStretch(1);
    QHBoxLayout *banner_layout = new QHBoxLayout;
    banner_layout->addLayout(banner_left_layout);
    banner_layout->addWidget(authors_group);
    banner_left_layout->addStretch(1);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(banner_layout);
    layout->addWidget(desc_group);
    layout->addStretch(1);

    connect(name_edit, SIGNAL(textChanged(const QString&)), this,
        SLOT(OnNameEdited(const QString&)));
    connect(m_desc_edit, SIGNAL(OnTextChanged()), this, SLOT(OnTextEdited()));
    connect(title_edit, SIGNAL(textChanged(const QString&)), this,
        SLOT(OnTitleEdited(const QString&)));
    connect(version_edit, SIGNAL(textChanged(const QString&)), this,
        SLOT(OnVersionEdited(const QString&)));
    connect(isa_edit, SIGNAL(textChanged(const QString&)), this,
        SLOT(OnIsaEdited(const QString&)));

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

/**
 * NodeEditPanel
 */
namespace
{

enum
{
    NodeInstDeleteType = QTableWidgetItem::UserType,
    NodeInstNewType
};

enum
{
    NodeInstIconColumn = 0,
    NodeInstNameColumn = 1,
    NodeInstInfoColumn = 2,
};

enum
{
    NodeInstRefRole = Qt::UserRole, // soc_id_t
};

template< typename T >
soc_id_t GetFreshId(const std::vector< T >& list)
{
    soc_id_t id = 0;
    for(size_t i = 0; i < list.size(); i++)
        id = std::max(id, list[i].id);
    return id + 1;
}

}

NodeEditPanel::NodeEditPanel(const soc_desc::node_ref_t& ref, QWidget *parent)
    :QWidget(parent), m_ref(ref)
{
    QLineEdit *name_edit = new QLineEdit(this);
    name_edit->setText(QString::fromStdString(ref.get()->name));
    QGroupBox *name_group = Misc::EncloseInBox("Name", name_edit);

    QLineEdit *title_edit = new QLineEdit(this);
    title_edit->setText(QString::fromStdString(ref.get()->title));
    QGroupBox *title_group = Misc::EncloseInBox("Title", title_edit);

    m_desc_edit = new MyTextEditor(this);
    m_desc_edit->SetTextHtml(QString::fromStdString(ref.get()->desc));
    QGroupBox *desc_group = Misc::EncloseInBox("Description", m_desc_edit);

    QHBoxLayout *name_title_layout = new QHBoxLayout;
    name_title_layout->addWidget(name_group, 0);
    name_title_layout->addWidget(title_group, 1);

    m_instances_table = new QTableWidget(this);
    std::vector< soc_desc::instance_t >& inst_list = m_ref.get()->instance;
    m_instances_table->setRowCount(inst_list.size() + 1);
    m_instances_table->setColumnCount(3);
    for(size_t row = 0; row < inst_list.size(); row++)
        FillRow(row, inst_list[row]);
    CreateNewRow(inst_list.size());
    m_instances_table->setHorizontalHeaderItem(NodeInstIconColumn,
        new QTableWidgetItem(""));
    m_instances_table->setHorizontalHeaderItem(NodeInstNameColumn,
        new QTableWidgetItem("Name"));
    m_instances_table->setHorizontalHeaderItem(NodeInstInfoColumn,
        new QTableWidgetItem("Info"));
    m_instances_table->verticalHeader()->setVisible(false);
    m_instances_table->resizeColumnsToContents();
    m_instances_table->horizontalHeader()->setStretchLastSection(true);
    m_instances_table->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    QGroupBox *instance_table_group = Misc::EncloseInBox("Instances", m_instances_table);

    m_inst_name_edit = new QLineEdit(this);
    m_inst_title_edit = new QLineEdit(this);
    QHBoxLayout *iint_layout = new QHBoxLayout;
    iint_layout->addWidget(Misc::EncloseInBox("Name", m_inst_name_edit), 0);
    iint_layout->addWidget(Misc::EncloseInBox("Title", m_inst_title_edit), 1);
    m_inst_desc_edit = new QLineEdit(this);
    QVBoxLayout *ii_layout = new QVBoxLayout;
    ii_layout->addLayout(iint_layout);
    ii_layout->addWidget(Misc::EncloseInBox("Description", m_inst_desc_edit));
    ii_layout->addStretch(1);
    QWidget *instance_info = new QWidget(this);
    instance_info->setLayout(ii_layout);
    m_instance_info_group = Misc::EncloseInBox("Instance Information", instance_info);

    QHBoxLayout *instances_layout = new QHBoxLayout;
    instances_layout->addWidget(instance_table_group);
    instances_layout->addWidget(m_instance_info_group, 1);
    instances_layout->addStretch(0);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(name_title_layout);
    layout->addWidget(desc_group);
    layout->addLayout(instances_layout);
    layout->addStretch(1);

    m_instance_info_group->hide();

    setLayout(layout);

    connect(name_edit, SIGNAL(textChanged(const QString&)), this,
        SLOT(OnNameEdited(const QString&)));
    connect(m_desc_edit, SIGNAL(OnTextChanged()), this, SLOT(OnDescEdited()));
    connect(title_edit, SIGNAL(textChanged(const QString&)), this,
        SLOT(OnTitleEdited(const QString&)));
    connect(m_instances_table, SIGNAL(cellActivated(int,int)), this,
        SLOT(OnInstActivated(int,int)));
    connect(m_instances_table, SIGNAL(currentCellChanged(int,int,int,int)), this,
        SLOT(OnInstSelected(int,int,int,int)));
}

void NodeEditPanel::OnNameEdited(const QString& text)
{
    /* first change the tree
     * WARNING this will invalidate any reference to this node, or the subnodes,
     * so we need to update the tree */
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

void NodeEditPanel::CreateNewRow(int row)
{
    QTableWidgetItem *item = new QTableWidgetItem(QIcon::fromTheme("list-add"), "",
        NodeInstNewType);
    item->setToolTip("New?");
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    m_instances_table->setItem(row, NodeInstIconColumn, item);
    item = new QTableWidgetItem("New instance...");
    QFont font = item->font();
    font.setItalic(true);
    item->setFont(font);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    m_instances_table->setItem(row, NodeInstNameColumn, item);
    item = new QTableWidgetItem("");
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    m_instances_table->setItem(row, NodeInstInfoColumn, item);
}

void NodeEditPanel::FillRow(int row, const soc_desc::instance_t& inst)
{
    QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(inst.name));
    item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    m_instances_table->setItem(row, NodeInstNameColumn, item);
    item = new QTableWidgetItem("info...");
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    m_instances_table->setItem(row, NodeInstInfoColumn, item);
    item = new QTableWidgetItem(QIcon::fromTheme("list-remove"), "", NodeInstDeleteType);
    item->setToolTip("Remove?");
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setData(NodeInstRefRole, inst.id);
    m_instances_table->setItem(row, NodeInstIconColumn, item);
}

soc_desc::instance_t *NodeEditPanel::GetInstanceById(soc_id_t id)
{
    std::vector< soc_desc::instance_t >& inst_list = m_ref.get()->instance;
    for(size_t i = 0; i < inst_list.size(); i++)
        if(inst_list[i].id == id)
            return &inst_list[i];
    return 0;
}

soc_desc::instance_t *NodeEditPanel::GetInstanceByRow(int row)
{
    if(row == -1)
        return 0;
    QTableWidgetItem *item = m_instances_table->item(row, NodeInstIconColumn);
    if(item->type() != NodeInstDeleteType)
        return 0;
    soc_id_t id = item->data(NodeInstRefRole).value< soc_id_t >();
    return GetInstanceById(id);
}

void NodeEditPanel::OnInstActivated(int row, int column)
{
    if(column != NodeInstIconColumn)
        return;
    int type = m_instances_table->item(row, column)->type();
    if(type == NodeInstDeleteType)
    {
        soc_id_t id = m_instances_table->item(row, column)->data(NodeInstRefRole)
            .value< soc_id_t >();
        std::vector< soc_desc::instance_t >& inst_list = m_ref.get()->instance;
        for(size_t i = 0; i < inst_list.size(); i++)
            if(inst_list[i].id == id)
            {
                inst_list.erase(inst_list.begin() + i);
                break;
            }
        m_instances_table->removeRow(row);
        OnModified();
    }
    else if(type == NodeInstNewType)
    {
        m_instances_table->insertRow(row);
        std::vector< soc_desc::instance_t >& inst_list = m_ref.get()->instance;
        soc_desc::instance_t inst;
        inst.id = GetFreshId(inst_list);
        inst.name = "UNNAMED";
        inst_list.push_back(inst);
        FillRow(row, inst);
    }
}

void NodeEditPanel::FillInstInfo(const soc_desc::instance_t& inst)
{
    m_inst_name_edit->setText(QString::fromStdString(inst.name));
    m_inst_title_edit->setText(QString::fromStdString(inst.title));
    m_instance_info_group->show();
}

void NodeEditPanel::OnInstSelected(int row, int col, int old_row, int old_col)
{
    Q_UNUSED(col);
    Q_UNUSED(old_row);
    Q_UNUSED(old_col);
    soc_desc::instance_t *inst = GetInstanceByRow(row);
    if(inst)
        FillInstInfo(*inst);
    else
        m_instance_info_group->hide();
}

#if 0
void NodeEditPanel::OnInstChanged(int row, int column)
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
#endif

#if 0
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

    m_value_table = new QTableView(this);
    m_value_model = new RegFieldTableModel(m_value_table); // view takes ownership
    m_value_model->SetRegister(m_ref.GetReg());
    m_value_model->SetReadOnly(true);
    m_value_table->setModel(m_value_model);
    m_value_table->verticalHeader()->setVisible(false);
    m_value_table->horizontalHeader()->setStretchLastSection(true);
    m_value_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // FIXME we cannot use setAlternatingRowColors() because we override the
    // background color, should it be part of the model ?
    m_table_delegate = new SocFieldCachedItemDelegate(this);
    m_value_table->setItemDelegate(m_table_delegate);
    m_value_table->resizeColumnsToContents();

    m_sexy_display2 = new Unscroll<RegSexyDisplay2>(this);
    m_sexy_display2->setFont(m_reg_font);
    m_sexy_display2->setModel(m_value_model);
    m_sexy_display2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QHBoxLayout *field_layout = new QHBoxLayout;
    field_layout->addWidget(m_value_table);
    m_field_group = new QGroupBox("Flags", this);
    m_field_group->setLayout(field_layout);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(top_layout, 0);
    layout->addWidget(m_sexy_display2, 0);
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
        if(!evaluate_formula(formula, map, res, err))
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
#endif

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
    m_soc_tree->setContextMenuPolicy(Qt::CustomContextMenu);

    m_delete_action = new QAction("&Delete", this);
    m_delete_action->setIcon(QIcon::fromTheme("list-remove"));
    m_new_action = new QAction("&New", this);
    m_new_action->setIcon(QIcon::fromTheme("list-add"));
    m_create_action = new QAction("&Create register", this);
    m_create_action->setIcon(QIcon::fromTheme("folder-new"));

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

    m_file_group->setLayout(m_file_group_layout);
    m_vert_layout->addWidget(m_file_group);
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
    soc_desc::error_context_t ctx;
    if(!soc_desc::produce_xml(filename.toStdString(), m_cur_socfile.GetSoc(), ctx))
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
        case SocTreeSocType: return style()->standardIcon(QStyle::SP_ComputerIcon);
        case SocTreeNodeType: return style()->standardIcon(QStyle::SP_DriveHDIcon);
        case SocTreeRegType: return style()->standardIcon(QStyle::SP_DirIcon);
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
        item->setIcon(0, QIcon::fromTheme("dialog-error"));
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

namespace
{

template< typename T >
void my_remove_at(std::vector< T >& v, size_t at)
{
    v.erase(v.begin() + at);
}

}

void RegEdit::OnSocItemNew()
{
}

void RegEdit::OnSocItemCreate()
{
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
#if 0
    if(current->type() == SocTreeSocType)
    {
        SocTreeItemVal< soc_desc::node_ref_t >(current).soc().get()->nodes.clear();
        item->GetRef().GetSoc().dev.clear();
        current->takeChildren();
        FillNodeTreeItem(current);
        m_soc_tree->expandItem(current);
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
#endif
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
    //SetPanel(new RegEditPanel(ref, this));
}

void RegEdit::DisplayField(const soc_desc::field_ref_t& ref)
{
    //SetPanel(new FieldEditPanel(ref, this));
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
            m_delete_action->setText("Delete all nodes...");
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
    if(current == 0)
        return;
    if(current->type() == SocTreeSocType)
        DisplaySoc(SocTreeItemVal< soc_desc::node_ref_t >(current).soc());
    else if(current->type() == SocTreeNodeType)
        DisplayNode(SocTreeItemVal< soc_desc::node_ref_t >(current));
    else if(current->type() == SocTreeRegType)
        DisplayReg(SocTreeItemVal< soc_desc::register_ref_t >(current));
}

void RegEdit::OnSocItemActivated(QTreeWidgetItem *current, int column)
{
    Q_UNUSED(column);
    if(current == 0)
        return;
#if 0
    if(current->type() == SocTreeNewNodeType)
        AddNode(current);
    else if(current->type() == SocTreeNewRegType)
        AddRegister(current);
#endif
}

#if 0
void RegEdit::AddDevice(QTreeWidgetItem *_item)
{
    NewDevTreeItem *item = dynamic_cast< NewDevTreeItem * >(_item);
    item->GetRef().GetSoc().dev.push_back(soc_dev_t());
    DevTreeItem *dev_item = new DevTreeItem("",
        SocDevRef(item->GetRef(), item->GetRef().GetSoc().dev.size() - 1, -1));
    FixupItem(dev_item);
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
    FixupItem(reg_item);
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
    FixupItem(field_item);
    item->parent()->insertChild(item->parent()->indexOfChild(item), field_item);
    m_soc_tree->setCurrentItem(field_item);
    OnModified(true);
}
#endif

bool RegEdit::Quit()
{
    return CloseSoc();
}