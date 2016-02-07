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
#include "regdisplaypanel.h"
#include <QHeaderView>
#include <QDebug>
#include <QStandardItemModel>

/**
 * SocDisplayPanel
 */
SocDisplayPanel::SocDisplayPanel(QWidget *parent, IoBackend *io_backend,
        const soc_desc::soc_ref_t& ref)
    :QGroupBox(parent), m_soc(ref)
{
    Q_UNUSED(io_backend)
    QVBoxLayout *right_layout = new QVBoxLayout;

    m_name = new QLabel(this);
    m_name->setTextFormat(Qt::RichText);
    m_name->setText("<h1>" + QString::fromStdString(m_soc.get()->name) + "</h1>");

    m_desc = new QLabel(this);
    m_name->setTextFormat(Qt::RichText);
    m_desc->setText(QString::fromStdString(m_soc.get()->desc));
    m_desc->setVisible(m_desc->text().size() != 0);

    right_layout->addWidget(m_name, 0);
    right_layout->addWidget(m_desc, 0);
    right_layout->addStretch(1);

    setTitle("");
    setLayout(right_layout);
}

void SocDisplayPanel::AllowWrite(bool en)
{
    Q_UNUSED(en);
}

QWidget *SocDisplayPanel::GetWidget()
{
    return this;
}

/**
 * NodeDisplayPanel
 */
NodeDisplayPanel::NodeDisplayPanel(QWidget *parent, IoBackend *io_backend,
        const soc_desc::node_inst_t& ref)
    :QGroupBox(parent), m_node(ref)
{
    BackendHelper helper(io_backend, ref.soc());
    QVBoxLayout *right_layout = new QVBoxLayout;

    QString dev_name = helper.GetPath(ref);

    QLabel *label_names = new QLabel("<b>" + dev_name + "</b>");
    label_names->setTextFormat(Qt::RichText);

    QLabel *label_addr = new QLabel("<b>" + QString().sprintf("0x%03x", ref.addr()) + "</b>");
    label_addr->setTextFormat(Qt::RichText);

    QHBoxLayout *top_layout = new QHBoxLayout;
    top_layout->addStretch();
    top_layout->addWidget(label_names);
    top_layout->addWidget(label_addr);
    top_layout->addStretch();

    m_name = new QLabel(this);
    m_name->setTextFormat(Qt::RichText);
    /* if instance has a title, it overrides node title.*/
    std::string title = ref.get()->title;
    if(title.empty())
        title = ref.node().get()->title;
    m_name->setText("<h1>" + QString::fromStdString(title) + "</h1>");

    /* put description from the node and from the instance */
    m_node_desc = new QLabel(this);
    m_node_desc->setTextFormat(Qt::RichText);
    m_node_desc->setText(QString::fromStdString(ref.node().get()->desc));
    m_node_desc->setVisible(m_node_desc->text().size() != 0);
    m_inst_desc = new QLabel(this);
    m_inst_desc->setTextFormat(Qt::RichText);
    m_inst_desc->setText(QString::fromStdString(ref.get()->desc));
    m_inst_desc->setVisible(m_inst_desc->text().size() != 0);

    right_layout->addWidget(m_name, 0);
    right_layout->addLayout(top_layout, 0);
    right_layout->addWidget(m_node_desc, 0);
    right_layout->addWidget(m_inst_desc, 0);
    right_layout->addStretch(1);

    setTitle("");
    setLayout(right_layout);
}

void NodeDisplayPanel::AllowWrite(bool en)
{
    Q_UNUSED(en);
}

QWidget *NodeDisplayPanel::GetWidget()
{
    return this;
}

/**
 * RegDisplayPanel
 */

namespace
{
    QString access_string(soc_desc::access_t acc, QString dflt = "")
    {
        switch(acc)
        {
            case soc_desc::UNSPECIFIED: return dflt;
            case soc_desc::READ_ONLY: return "Read only";
            case soc_desc::READ_WRITE: return "Read-write";
            case soc_desc::WRITE_ONLY: return "Write-only";
            default: return "";
        }
    }
}

RegDisplayPanel::RegDisplayPanel(QWidget *parent, IoBackend *io_backend,
        const soc_desc::node_inst_t& ref)
    :QGroupBox(parent), m_io_backend(io_backend), m_node(ref), m_reg_font(font())
{
    bool read_only = m_io_backend->IsReadOnly();
    BackendHelper helper(m_io_backend, ref.soc());

    QVBoxLayout *right_layout = new QVBoxLayout;

    m_reg_font.setWeight(100);
    m_reg_font.setKerning(false);

    QString reg_name = helper.GetPath(ref);
    QStringList names, access;
    QVector< soc_addr_t > addresses;
    names.append(reg_name);
    addresses.append(ref.addr());
    access.append(access_string(ref.node().reg().get()->access));

    std::vector< soc_desc::variant_ref_t > variants = ref.node().reg().variants();
    for(size_t i = 0; i < variants.size(); i++)
    {
        names.append(reg_name + "/" + QString::fromStdString(variants[i].get()->type));
        addresses.append(ref.addr() + variants[i].get()->offset);
        access.append(access_string(ref.node().reg().get()->access, access[0]));
    }

    QString str;
    str += "<table align=left>";
    for(int i = 0; i < names.size(); i++)
        str += "<tr><td><b>" + names[i] + "</b></td></tr>";
    str += "</table>";
    QLabel *label_names = new QLabel;
    label_names->setTextFormat(Qt::RichText);
    label_names->setText(str);

    QString str_addr;
    str_addr += "<table align=left>";
    for(int i = 0; i < names.size(); i++)
        str_addr += "<tr><td><b>" + QString().sprintf("0x%03x", addresses[i]) + "</b></td></tr>";
    str_addr += "</table>";
    QLabel *label_addr = new QLabel;
    label_addr->setTextFormat(Qt::RichText);
    label_addr->setText(str_addr);

    QString str_acc;
    str_acc += "<table align=left>";
    for(int i = 0; i < names.size(); i++)
        str_acc += "<tr><td><b>" + access[i] + "</b></td></tr>";
    str_acc += "</table>";
    QLabel *label_access = new QLabel;
    label_access->setTextFormat(Qt::RichText);
    label_access->setText(str_acc);

    QHBoxLayout *top_layout = new QHBoxLayout;
    top_layout->addStretch();
    top_layout->addWidget(label_names);
    top_layout->addWidget(label_addr);
    top_layout->addWidget(label_access);
    top_layout->addStretch();

    m_raw_val_name = new QLabel;
    m_raw_val_name->setText("Raw value:");
    m_raw_val_edit = new RegLineEdit;
    m_raw_val_edit->SetReadOnly(read_only);
    m_raw_val_edit->GetLineEdit()->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_raw_val_edit->GetLineEdit()->setValidator(new SocFieldValidator(m_raw_val_edit));
    //m_raw_val_edit->EnableSCT(!!(reg.flags & REG_HAS_SCT));
    m_raw_val_edit->GetLineEdit()->setFont(m_reg_font);
    QHBoxLayout *raw_val_layout = new QHBoxLayout;
    raw_val_layout->addStretch();
    raw_val_layout->addWidget(m_raw_val_name);
    raw_val_layout->addWidget(m_raw_val_edit);
    raw_val_layout->addStretch();

    m_value_table = new GrowingTableView();
    m_value_model = new RegFieldTableModel(m_value_table); // view takes ownership
    m_value_model->SetRegister(*m_node.node().reg().get());
    m_value_model->SetReadOnly(read_only);
    RegFieldProxyModel *proxy_model = new RegFieldProxyModel(this);
    proxy_model->setSourceModel(m_value_model);
    m_value_table->setModel(proxy_model);
    m_value_table->setSortingEnabled(true);
    m_value_table->sortByColumn(0, Qt::DescendingOrder);
    m_value_table->verticalHeader()->setVisible(false);
    m_value_table->resizeColumnsToContents();
    m_value_table->horizontalHeader()->setStretchLastSection(true);
    m_value_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // FIXME we cannot use setAlternatingRowColors() because we override the
    // background color, should it be part of the model ?

    m_table_delegate = new SocFieldCachedItemDelegate(this);
    m_table_edit_factory = new QItemEditorFactory();
    SocFieldCachedEditorCreator *m_table_edit_creator = new SocFieldCachedEditorCreator();
    // FIXME see QTBUG-30392
    m_table_edit_factory->registerEditor((QVariant::Type)qMetaTypeId< SocFieldCachedValue >(),
        m_table_edit_creator);
    m_table_delegate->setItemEditorFactory(m_table_edit_factory);
    m_value_table->setItemDelegate(m_table_delegate);

    m_sexy_display2 = new Unscroll<YRegDisplay>(this);
    m_sexy_display2->setFont(m_reg_font);
    m_sexy_display2->setModel(m_value_model);
    m_sexy_display2->setWidth(m_node.node().reg().get()->width);
    m_sexy_display2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_name = new QLabel(this);
    m_name->setTextFormat(Qt::RichText);
    m_name->setText("<h1>" + QString::fromStdString(ref.node().get()->title) + "</h1>");

    /* put description from the node, from the instance and register */
    m_node_desc = new QLabel(this);
    m_node_desc->setTextFormat(Qt::RichText);
    m_node_desc->setText(QString::fromStdString(ref.node().get()->desc));
    m_node_desc->setVisible(m_node_desc->text().size() != 0);
    m_inst_desc = new QLabel(this);
    m_inst_desc->setTextFormat(Qt::RichText);
    m_inst_desc->setText(QString::fromStdString(ref.get()->desc));
    m_inst_desc->setVisible(m_inst_desc->text().size() != 0);
    m_desc = new QLabel(this);
    m_desc->setTextFormat(Qt::RichText);
    m_desc->setText(QString::fromStdString(m_node.node().reg().get()->desc));
    m_desc->setVisible(m_desc->text().size() != 0);

    right_layout->addWidget(m_name);
    right_layout->addWidget(m_desc);
    right_layout->addLayout(top_layout);
    right_layout->addWidget(m_node_desc);
    right_layout->addWidget(m_inst_desc);
    right_layout->addWidget(m_desc);
    if(raw_val_layout)
        right_layout->addLayout(raw_val_layout);
    right_layout->addWidget(m_sexy_display2);
    right_layout->addWidget(m_value_table);
    right_layout->addStretch();

    setTitle("");
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
    AllowWrite(false);

    // load data
    Reload();

    connect(m_raw_val_edit->GetLineEdit(), SIGNAL(returnPressed()), this,
        SLOT(OnRawRegValueReturnPressed()));
    connect(m_value_model, SIGNAL(OnValueModified(int)), this,
        SLOT(OnRegValueChanged(int)));
    connect(m_sexy_display2, SIGNAL(clicked(const QModelIndex&)), this,
        SLOT(OnRegFieldActivated(const QModelIndex&)));
}

RegDisplayPanel::~RegDisplayPanel()
{
    delete m_table_edit_factory;
}

void RegDisplayPanel::Reload()
{
    soc_word_t value;
    BackendHelper helper(m_io_backend, m_node.soc());
    bool has_value = helper.ReadRegister(m_node,  value);

    if(has_value)
    {
        m_raw_val_name->show();
        m_raw_val_edit->show();
        m_raw_val_edit->GetLineEdit()->setText(QString().sprintf("0x%08x", value));
        m_value_model->SetValues(QVector< QVariant >(1, QVariant(value)));
    }
    else
    {
        m_raw_val_name->hide();
        m_raw_val_edit->hide();
        m_value_model->SetValues(QVector< QVariant >());
    }

    m_value_table->resizeColumnsToContents();
    m_value_table->horizontalHeader()->setStretchLastSection(true);
}

void RegDisplayPanel::AllowWrite(bool en)
{
    m_allow_write = en;
    if(m_raw_val_edit)
    {
        m_raw_val_edit->SetReadOnly(m_io_backend->IsReadOnly() || !m_allow_write);
        m_value_model->SetReadOnly(m_io_backend->IsReadOnly() || !m_allow_write);
    }
}

IoBackend::WriteMode RegDisplayPanel::EditModeToWriteMode(RegLineEdit::EditMode mode)
{
    switch(mode)
    {
        case RegLineEdit::Write: return IoBackend::Write;
        case RegLineEdit::Set: return IoBackend::Set;
        case RegLineEdit::Clear: return IoBackend::Clear;
        case RegLineEdit::Toggle: return IoBackend::Toggle;
        default: return IoBackend::Write;
    }
}

void RegDisplayPanel::OnRawRegValueReturnPressed()
{
    soc_word_t val;
    QLineEdit *edit = m_raw_val_edit->GetLineEdit();
    const SocFieldValidator *validator = dynamic_cast< const SocFieldValidator *>(edit->validator());
    QValidator::State state = validator->parse(edit->text(), val);
    if(state != QValidator::Acceptable)
        return;
    IoBackend::WriteMode mode = EditModeToWriteMode(m_raw_val_edit->GetMode());
    BackendHelper helper(m_io_backend, m_node.soc());
    helper.WriteRegister(m_node, val, mode);
    // register write can change all fields
    Reload();
}

void RegDisplayPanel::OnRegValueChanged(int index)
{
    QVariant var = m_value_model->GetValue(index);
    if(!var.isValid())
        return;
    BackendHelper helper(m_io_backend, m_node.soc());
    helper.WriteRegister(m_node, var.value< soc_word_t >(), IoBackend::Write);
    // register write can change all fields
    Reload();
}

void RegDisplayPanel::OnRegFieldActivated(const QModelIndex& index)
{
    Q_UNUSED(index);
}

QWidget *RegDisplayPanel::GetWidget()
{
    return this;
}

