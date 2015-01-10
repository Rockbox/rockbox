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
            inst_list.erase(inst_list.begin() + i);
            return true;
        }
    return false;
}

enum
{
    InstTypeRole = Qt::UserRole,
};

}

NodeInstanceEditPanel::NodeInstanceEditPanel(const soc_desc::node_ref_t& ref,
    soc_id_t inst_id, QWidget *parent)
    :QWidget(parent), m_ref(ref), m_id(inst_id)
{
    QLineEdit *name_edit = new QLineEdit(this);
    QLineEdit *title_edit = new QLineEdit(this);
    YPathBar *type_bar = new YPathBar(this);
    m_type_choose_hint = new QLabel("Choose type...", this);
    QFont f = m_type_choose_hint->font();
    f.setItalic(true);
    m_type_choose_hint->setFont(f);
    QLabel *type_label = new QLabel("Type", this);
    f = type_label->font();
    f.setBold(true);
    type_label->setFont(f);

    QHBoxLayout *type_layout = new QHBoxLayout;
    type_layout->addWidget(type_label);
    type_layout->addWidget(type_bar);
    type_layout->addWidget(m_type_choose_hint);
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

    QHBoxLayout *inst_layout = new QHBoxLayout;
    inst_layout->addWidget(m_single_group);
    inst_layout->addWidget(m_range_group);
    inst_layout->addWidget(m_stride_group);
    inst_layout->addWidget(m_formula_group);
    inst_layout->addStretch(0);

    QGroupBox *inst_groupbox = new QGroupBox(this);
    inst_groupbox->setLayout(inst_layout);

    QHBoxLayout *iint_layout = new QHBoxLayout;
    iint_layout->addWidget(Misc::EncloseInBox("Name", name_edit), 0);
    iint_layout->addWidget(Misc::EncloseInBox("Title", title_edit), 1);
    QLineEdit *desc_edit = new QLineEdit(this);
    QVBoxLayout *ii_layout = new QVBoxLayout;
    ii_layout->addLayout(iint_layout);
    ii_layout->addWidget(Misc::EncloseInBox("Description", desc_edit));
    ii_layout->addLayout(type_layout);
    ii_layout->addWidget(inst_groupbox);
    ii_layout->addStretch(1);

    QStandardItemModel *type_model = new QStandardItemModel(this);
    QStandardItem *single_item = new QStandardItem("Single");
    single_item->setData(QVariant::fromValue(soc_desc::instance_t::SINGLE), InstTypeRole);
    type_model->setItem(0, 0, single_item);
    QStandardItem *range_item = new QStandardItem("Range");
    range_item->setData(QVariant::fromValue(soc_desc::instance_t::RANGE), InstTypeRole);
    type_model->setItem(1, 0, range_item);
    QStandardItem *stride_item = new QStandardItem("Stride");
    stride_item->setData(QVariant::fromValue(soc_desc::range_t::STRIDE), InstTypeRole);
    range_item->setChild(0, 0, stride_item);
    QStandardItem *formula_item = new QStandardItem("Formula");
    formula_item->setData(QVariant::fromValue(soc_desc::range_t::FORMULA), InstTypeRole);
    range_item->setChild(1, 0, formula_item);
    type_bar->setModel(type_model);

    /* fill info */
    soc_desc::instance_t& inst = GetInstance();
    name_edit->setText(QString::fromStdString(inst.name));
    title_edit->setText(QString::fromStdString(inst.title));
    desc_edit->setText(QString::fromStdString(inst.desc));
    addr_edit->setField(inst.addr);
    base_edit->setField(inst.range.base);
    stride_edit->setField(inst.range.stride);
    first_spin->setValue(inst.range.first);
    count_spin->setValue(inst.range.count);
    formula_edit->setText(QString::fromStdString(inst.range.formula));
    variable_edit->setText(QString::fromStdString(inst.range.variable));

    setLayout(ii_layout);

    connect(name_edit, SIGNAL(textChanged(const QString&)), this,
        SLOT(OnNameEdited(const QString&)));
    connect(desc_edit, SIGNAL(textChanged(const QString&)), this,
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
    connect(type_bar, SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
        this, SLOT(OnTypeChanged(const QModelIndex&, const QModelIndex&)));

    /* fill info */
    if(inst.type == soc_desc::instance_t::RANGE)
    {
        if(inst.range.type == soc_desc::range_t::STRIDE)
            type_bar->setCurrentIndex(stride_item->index());
        else
            type_bar->setCurrentIndex(formula_item->index());
    }
    else
        type_bar->setCurrentIndex(single_item->index());
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

bool NodeInstanceEditPanel::IsComplete(const QModelIndex& index)
{
    QVariant var = index.data(InstTypeRole);
    return isUserType< soc_desc::range_t::type_t >(var) ||
        (isUserType< soc_desc::instance_t::type_t >(var) &&
            var.value< soc_desc::instance_t::type_t>() == soc_desc::instance_t::SINGLE);
}

void NodeInstanceEditPanel::OnTypeChanged(const QModelIndex& current,
    const QModelIndex& previous)
{
    Q_UNUSED(previous);

    m_single_group->hide();
    m_range_group->hide();
    m_stride_group->hide();
    m_formula_group->hide();

    QModelIndex idx = current;
    while(idx.isValid())
    {
        QVariant var = idx.data(InstTypeRole);
        if(isUserType< soc_desc::instance_t::type_t >(var))
        {
            soc_desc::instance_t::type_t type = var.value< soc_desc::instance_t::type_t >();
            if(type == soc_desc::instance_t::SINGLE)
            {
                m_single_group->show();
            }
            else
                m_range_group->show();
            GetInstance().type = type;
        }
        else if(isUserType< soc_desc::range_t::type_t >(var))
        {
            soc_desc::range_t::type_t type = var.value< soc_desc::range_t::type_t >();
            if(type == soc_desc::range_t::STRIDE)
                m_stride_group->show();
            else
                m_formula_group->show();
            GetInstance().range.type = type;
            m_type_choose_hint->hide();
        }
        idx = idx.parent();
    }
    m_type_choose_hint->setVisible(!IsComplete(current));
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
    layout->addLayout(name_title_layout);
    layout->addWidget(desc_group);
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
}

void NodeEditPanel::OnInstModified()
{
    int index = m_instances_tab->currentIndex();
    NodeInstanceEditPanel *panel =
        dynamic_cast< NodeInstanceEditPanel * >(m_instances_tab->widget(index));
    m_instances_tab->setTabText(index, QString::fromStdString(panel->GetInstance().name));
    OnModified();
}

void NodeEditPanel::OnInstCreate()
{
    std::vector< soc_desc::instance_t >& inst_list = m_ref.get()->instance;
    soc_desc::instance_t inst;
    inst.id = GetFreshId(inst_list);
    inst.name = "UNNAMED";
    inst.type = soc_desc::instance_t::SINGLE;
    inst.addr = 0;
    inst.range.type = soc_desc::range_t::STRIDE;
    inst.range.first = 0;
    inst.range.count = 0;
    inst.range.stride = 0;
    inst_list.push_back(inst);
    m_instances_tab->addTab(new NodeInstanceEditPanel(m_ref, inst.id, this),
        QString::fromStdString(inst.name));
}


/**
 * RegEditPanel
 */

RegEditPanel::RegEditPanel(const soc_desc::register_ref_t& ref, QWidget *parent)
    :QWidget(parent), m_ref(ref), m_reg_font(font())
{
    m_reg_font.setWeight(100);
    m_reg_font.setKerning(false);

    m_value_model = new RegFieldTableModel(this); // view takes ownership
    m_value_model->SetRegister(*ref.get());
    m_value_model->SetReadOnly(true);

    m_sexy_display2 = new Unscroll<YRegDisplay>(this);
    m_sexy_display2->setFont(m_reg_font);
    m_sexy_display2->setModel(m_value_model);
    m_sexy_display2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QHBoxLayout *namepos_layout = new QHBoxLayout;
    m_name_edit = new QLineEdit(this);
    m_pos_spin = new QSpinBox(this);
    m_width_spin = new QSpinBox(this);
    namepos_layout->addWidget(Misc::EncloseInBox("Name", m_name_edit));
    namepos_layout->addWidget(Misc::EncloseInBox("Position", m_pos_spin));
    namepos_layout->addWidget(Misc::EncloseInBox("Width", m_width_spin));
    namepos_layout->addStretch(0);

    m_desc_edit = new QLineEdit(this);

    QVBoxLayout *field_layout = new QVBoxLayout;
    field_layout->addLayout(namepos_layout);
    field_layout->addWidget(Misc::EncloseInBox("Description", m_desc_edit));

    m_field_box = new QGroupBox("Field Description");
    m_field_box->setLayout(field_layout);

    QVBoxLayout *main_layout = new QVBoxLayout;
    main_layout->addWidget(Misc::EncloseInBox("Register", m_sexy_display2));
    main_layout->addWidget(m_field_box);
    main_layout->addStretch(0);

    setLayout(main_layout);

    OnRegFieldActivated(QModelIndex());
    connect(m_sexy_display2, SIGNAL(clicked(const QModelIndex&)), this,
        SLOT(OnRegFieldActivated(const QModelIndex&)));
    connect(m_name_edit, SIGNAL(textChanged(const QString&)), this,
        SLOT(OnFieldNameChanged(const QString&)));
    connect(m_pos_spin, SIGNAL(valueChanged(int)), this,
        SLOT(OnFieldPosChanged(int)));
    connect(m_width_spin, SIGNAL(valueChanged(int)), this,
        SLOT(OnFieldWidthChanged(int)));
    connect(m_desc_edit, SIGNAL(textChanged(const QString&)), this,
        SLOT(OnFieldDescChanged(const QString&)));
}

soc_desc::field_t *RegEditPanel::GetCurField()
{
    QModelIndex index = m_sexy_display2->currentIndex();
    if(!index.isValid())
        return 0;
    return &m_ref.get()->field[index.row()];
}

void RegEditPanel::OnRegFieldActivated(const QModelIndex& index)
{
    m_field_box->setVisible(index.isValid());
    if(!index.isValid())
        return;
    soc_desc::field_t field = *GetCurField();
    m_name_edit->setText(QString::fromStdString(field.name));
    m_pos_spin->setValue(field.pos);
    m_width_spin->setValue(field.width);
    m_desc_edit->setText(QString::fromStdString(field.desc));
}

void RegEditPanel::OnFieldNameChanged(const QString& name)
{
    if(!GetCurField())
        return;
    GetCurField()->name = name.toStdString();
    DoModify();
}

void RegEditPanel::OnFieldPosChanged(int value)
{
    if(!GetCurField())
        return;
    GetCurField()->pos = value;
    DoModify();
}

void RegEditPanel::OnFieldWidthChanged(int value)
{
    if(!GetCurField())
        return;
    GetCurField()->width = value;
    DoModify();
}

void RegEditPanel::OnFieldDescChanged(const QString& desc)
{
    if(!GetCurField())
        return;
    GetCurField()->desc = desc.toStdString();
    DoModify();
}

void RegEditPanel::DoModify()
{
    m_value_model->UpdateRegister(*m_ref.get());
    OnModified();
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
    m_file_open->setPopupMode(QToolButton::MenuButtonPopup);
    m_file_open->setMenu(file_open_menu);

    m_file_save = new QToolButton(this);
    m_file_save->setText("Save");
    m_file_save->setIcon(QIcon::fromTheme("document-save"));
    m_file_save->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_file_save->setPopupMode(QToolButton::MenuButtonPopup);
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