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
#include "utils.h"
#include <QFontMetrics>
#include <QPainter>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QHeaderView>
#include <QDebug>
#include <QElapsedTimer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QTextBlock>
#include <QApplication>
#include <QPushButton>
#include <QMessageBox>
#include <QStandardItemModel>

/**
 * SocBitRangeValidator
 */
SocBitRangeValidator::SocBitRangeValidator(QObject *parent)
    :QValidator(parent)
{
    m_width = 32;
}

void SocBitRangeValidator::fixup(QString& input) const
{
    input = input.trimmed();
}

QValidator::State SocBitRangeValidator::validate(QString& input, int& pos) const
{
    Q_UNUSED(pos);
    int first, last;
    State state = parse(input, last, first);
    return state;
}

QValidator::State SocBitRangeValidator::parse(const QString& input, int& last, int& first) const
{
    // the empty string is always intermediate
    if(input.size() == 0)
        return Intermediate;
    // check if there is ':'
    int pos = input.indexOf(':');
    if(pos == -1)
        pos = input.size();
    // if field start with ':', the last bit is implicit and is 31
    if(pos > 0)
    {
        // parse last bit and check it's between 0 and 31
        bool ok = false;
        last = input.left(pos).toInt(&ok);
        if(!ok || last < 0 || last >= m_width)
            return Invalid;
    }
    else
        last = m_width - 1;
    // parse first bit
    if(pos < input.size() - 1)
    {
        bool ok = false;
        first = input.mid(pos + 1).toInt(&ok);
        if(!ok || first < 0 || first > last)
            return Invalid;
    }
    // if input ends with ':', first bit is implicit and is 0
    else if(pos == input.size() - 1)
        first = 0;
    // if there no ':', first=last
    else
        first = last;
    return Acceptable;
}

void SocBitRangeValidator::setWidth(int nr_bits)
{
    m_width = nr_bits;
}

QString SocBitRangeValidator::generate(int last_bit, int first_bit) const
{
    if(last_bit == first_bit)
        return QString("%1").arg(first_bit);
    else
        return QString("%1:%2").arg(last_bit).arg(first_bit);
}

/**
 * SocFieldValidator
 */

SocFieldValidator::SocFieldValidator(QObject *parent)
    :QValidator(parent)
{
    m_field.pos = 0;
    m_field.width = 32;
}

SocFieldValidator::SocFieldValidator(const soc_desc::field_t& field, QObject *parent)
    :QValidator(parent), m_field(field)
{
}

void SocFieldValidator::fixup(QString& input) const
{
    input = input.trimmed();
}

QValidator::State SocFieldValidator::validate(QString& input, int& pos) const
{
    Q_UNUSED(pos);
    soc_word_t val;
    State state = parse(input, val);
    return state;
}

QValidator::State SocFieldValidator::parse(const QString& input, soc_word_t& val) const
{
    // the empty string is always intermediate
    if(input.size() == 0)
        return Intermediate;
    // first check named values
    State state = Invalid;
    foreach(const soc_desc::enum_t& value, m_field.enum_)
    {
        QString name = QString::fromLocal8Bit(value.name.c_str());
        // cannot be a substring if too long or empty
        if(input.size() > name.size())
            continue;
        // check equal string
        if(input == name)
        {
            state = Acceptable;
            val = value.value;
            break;
        }
        // check substring
        if(name.startsWith(input))
            state = Intermediate;
    }
    // early return for exact match
    if(state == Acceptable)
        return state;
    // do a few special cases for convenience
    if(input.compare("0x", Qt::CaseInsensitive) == 0 ||
            input.compare("0b", Qt::CaseInsensitive) == 0)
        return Intermediate;
    // try by parsing
    unsigned basis, pos;
    if(input.size() >= 2 && input.startsWith("0x", Qt::CaseInsensitive))
    {
        basis = 16;
        pos = 2;
    }
    else if(input.size() >= 2 && input.startsWith("0b", Qt::CaseInsensitive))
    {
        basis = 2;
        pos = 2;
    }
    else if(input.size() >= 2 && input.startsWith("0"))
    {
        basis = 8;
        pos = 1;
    }
    else
    {
        basis = 10;
        pos = 0;
    }
    bool ok = false;
    unsigned long v = input.mid(pos).toULong(&ok, basis);
    // if not ok, return result of name parsing
    if(!ok)
        return state;
    // if ok, check if it fits in the number of bits
    unsigned nr_bits = m_field.width;
    unsigned long max = nr_bits == 32 ? 0xffffffff : (1 << nr_bits) - 1;
    if(v <= max)
    {
        val = v;
        return Acceptable;
    }

    return state;
}

/**
 * RegLineEdit
 */
RegLineEdit::RegLineEdit(QWidget *parent)
    :QWidget(parent)
{
    m_layout = new QHBoxLayout(this);
    m_button = new QToolButton(this);
    m_button->setCursor(Qt::ArrowCursor);
    m_button->setStyleSheet("QToolButton { font-weight: bold; color: white; background: black; }");
    m_button->setPopupMode(QToolButton::InstantPopup);
    m_edit = new QLineEdit(this);
    m_layout->addWidget(m_button);
    m_layout->addWidget(m_edit);
    m_menu = new QMenu(this);
    connect(m_menu->addAction("Write"), SIGNAL(triggered()), this, SLOT(OnWriteAct()));
    connect(m_menu->addAction("Set"), SIGNAL(triggered()), this, SLOT(OnSetAct()));
    connect(m_menu->addAction("Clear"), SIGNAL(triggered()), this, SLOT(OnClearAct()));
    connect(m_menu->addAction("Toggle"), SIGNAL(triggered()), this, SLOT(OnToggleAct()));
    EnableSCT(false);
    SetReadOnly(false);
    ShowMode(true);
    SetMode(Write);
}

void RegLineEdit::SetReadOnly(bool ro)
{
    m_edit->setReadOnly(ro);
    m_readonly = ro;
    ShowMode(!ro);
}

void RegLineEdit::EnableSCT(bool en)
{
    m_has_sct = en;
    if(!m_has_sct)
    {
        m_button->setMenu(0);
        SetMode(Write);
    }
    else
        m_button->setMenu(m_menu);
}

RegLineEdit::~RegLineEdit()
{
}

QLineEdit *RegLineEdit::GetLineEdit()
{
    return m_edit;
}

void RegLineEdit::ShowMode(bool show)
{
    if(show)
        m_button->show();
    else
        m_button->hide();
}

void RegLineEdit::OnWriteAct()
{
    SetMode(Write);
}

void RegLineEdit::OnSetAct()
{
    SetMode(Set);
}

void RegLineEdit::OnClearAct()
{
    SetMode(Clear);
}

void RegLineEdit::OnToggleAct()
{
    SetMode(Toggle);
}

void RegLineEdit::SetMode(EditMode mode)
{
    m_mode = mode;
    switch(m_mode)
    {
        case Write: m_button->setText("WR"); break;
        case Set: m_button->setText("SET"); break;
        case Clear: m_button->setText("CLR"); break;
        case Toggle: m_button->setText("TOG"); break;
        default: break;
    }
}

RegLineEdit::EditMode RegLineEdit::GetMode()
{
    return m_mode;
}

void RegLineEdit::setText(const QString& text)
{
    m_edit->setText(text);
}

QString RegLineEdit::text() const
{
    return m_edit->text();
}

/**
 * SocFieldItemDelegate
 */

QString SocFieldItemDelegate::displayText(const QVariant& value, const QLocale& locale) const
{
    if(value.type() == QVariant::UInt)
        return QString("0x%1").arg(value.toUInt(), (m_bitcount + 3) / 4, 16, QChar('0'));
    else
        return QStyledItemDelegate::displayText(value, locale);
}

void SocFieldItemDelegate::setWidth(int bitcount)
{
    m_bitcount = bitcount;
}

/**
 * SocFieldEditor
 */
SocFieldEditor::SocFieldEditor(const soc_desc::field_t& field, QWidget *parent)
    :QLineEdit(parent), m_reg_field(field)
{
    m_validator = new SocFieldValidator(field);
    setValidator(m_validator);
    connect(this, SIGNAL(editingFinished()), this, SLOT(editDone()));
    setAlignment(Qt::AlignCenter);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
}

SocFieldEditor::~SocFieldEditor()
{
    delete m_validator;
}

void SocFieldEditor::editDone()
{
    emit editingFinished(field());
}

uint SocFieldEditor::field() const
{
    soc_word_t v;
    /* in case validator fails to parse, return old value */
    if(m_validator->parse(text(), v) == QValidator::Acceptable)
        return v;
    else
        return m_field;
}

void SocFieldEditor::setField(uint field)
{
    m_field = field;
    int digits = (m_reg_field.width + 3) / 4;
    setText(QString("0x%1").arg(field, digits, 16, QChar('0')));
}

void SocFieldEditor::SetRegField(const soc_desc::field_t& field)
{
    setValidator(0);
    delete m_validator;
    m_validator = new SocFieldValidator(field);
    setValidator(m_validator);
    m_reg_field = field;
}

/**
 * SocFieldCachedValue
 */
SocFieldCachedValue::SocFieldCachedValue(const soc_desc::field_t& field, uint value)
    :m_field(field), m_value(value)
{
    int idx = field.find_value(value);
    if(idx != -1)
        m_name = QString::fromStdString(field.enum_[idx].name);
}

bool SocFieldCachedValue::operator<(const SocFieldCachedValue& o) const
{
    return m_value < o.m_value;
}

/**
 * SocFieldBitRange
 */

bool SocFieldBitRange::operator<(const SocFieldBitRange& o) const
{
    if(m_first_bit < o.m_first_bit)
        return true;
    if(m_first_bit > o.m_first_bit)
        return false;
    return m_last_bit < o.m_last_bit;
}

bool SocFieldBitRange::operator!=(const SocFieldBitRange& o) const
{
    return m_first_bit != o.m_first_bit || m_last_bit != o.m_last_bit;
}

/**
 * SocFieldCachedItemDelegate
 */

SocFieldCachedItemDelegate::SocFieldCachedItemDelegate(QObject *parent)
    :QStyledItemDelegate(parent)
{
    m_mode = DisplayValueAndName;
}

QString SocFieldCachedItemDelegate::displayText(const QVariant& value, const QLocale& locale) const
{
    if(isUserType< SocFieldCachedValue >(value))
    {
        const SocFieldCachedValue& v = value.value< SocFieldCachedValue >();
        int bitcount = v.field().width;
        QString name = v.value_name();
        QString strval = QString("0x%1").arg(v.value(), (bitcount + 3) / 4, 16, QChar('0'));
        switch(m_mode)
        {
            case DisplayName:
                if(name.size() > 0)
                    return name;
                /* fallthrough */
            case DisplayValueAndName:
                if(name.size() > 0)
                    return QString("%1 (%2)").arg(strval).arg(name);
                /* fallthrough */
            case DisplayValue:
            default:
                return strval;
        }
    }
    else if(value.type() == QVariant::UserType && value.userType() == qMetaTypeId< SocFieldBitRange >())
    {
        const SocFieldBitRange& br = value.value< SocFieldBitRange >();
        if(br.GetFirstBit() == br.GetLastBit())
            return QString("%1").arg(br.GetFirstBit());
        else
            return QString("%1:%2").arg(br.GetLastBit()).arg(br.GetFirstBit());
    }
    else
        return QStyledItemDelegate::displayText(value, locale);
}

/**
 * SocFieldCachedEditor
 */
SocFieldCachedEditor::SocFieldCachedEditor(QWidget *parent)
    :SocFieldEditor(soc_desc::field_t(), parent)
{
}

SocFieldCachedEditor::~SocFieldCachedEditor()
{
}

SocFieldCachedValue SocFieldCachedEditor::value() const
{
    return SocFieldCachedValue(m_value.field(), field());
}

void SocFieldCachedEditor::setValue(SocFieldCachedValue val)
{
    m_value = val;
    SetRegField(m_value.field());
    setField(m_value.value());
}

/**
 * SocAccessItemDelegate
 */
QString SocAccessItemDelegate::displayText(const QVariant& value, const QLocale& locale) const
{
    if(isUserType< soc_desc::access_t >(value))
    {
        soc_desc::access_t acc = value.value< soc_desc::access_t >();
        switch(acc)
        {
            case soc_desc::UNSPECIFIED: return m_unspec_text;
            case soc_desc::READ_ONLY: return "Read-Only";
            case soc_desc::READ_WRITE: return "Read-Write";
            case soc_desc::WRITE_ONLY: return "Write-Only";
            default: return "<bug>";
        }
    }
    else
        return QStyledItemDelegate::displayText(value, locale);
}

/**
 * SocAccessEditor
 */
SocAccessEditor::SocAccessEditor(const QString& unspec_text, QWidget *parent)
    :QComboBox(parent)
{
    addItem(unspec_text, QVariant::fromValue(soc_desc::UNSPECIFIED));
    addItem("Read-Only", QVariant::fromValue(soc_desc::READ_ONLY));
    addItem("Read-Write", QVariant::fromValue(soc_desc::READ_WRITE));
    addItem("Write-Only", QVariant::fromValue(soc_desc::WRITE_ONLY));
}

SocAccessEditor::~SocAccessEditor()
{
}

soc_desc::access_t SocAccessEditor::access() const
{
    return itemData(currentIndex()).value< soc_desc::access_t >();
}

void SocAccessEditor::setAccess(soc_desc::access_t acc)
{
    setCurrentIndex(findData(QVariant::fromValue(acc)));
}

/**
 * SocFieldEditorCreator
 */
QWidget *SocFieldEditorCreator::createWidget(QWidget *parent) const
{
    return new SocFieldEditor(m_field, parent);
}

QByteArray SocFieldEditorCreator::valuePropertyName() const
{
    return QByteArray("field");
}

void SocFieldEditorCreator::setWidth(int bitcount)
{
    m_field.width = bitcount;
}

/**
 * SocAccessEditorCreator
 */
QWidget *SocAccessEditorCreator::createWidget(QWidget *parent) const
{
    return new SocAccessEditor(m_unspec_text, parent);
}

QByteArray SocAccessEditorCreator::valuePropertyName() const
{
    return QByteArray("access");
}

/**
 * SocFieldCachedEditorCreator
 */
QWidget *SocFieldCachedEditorCreator::createWidget(QWidget *parent) const
{
    return new SocFieldCachedEditor(parent);
}

QByteArray SocFieldCachedEditorCreator::valuePropertyName() const
{
    return QByteArray("value");
}

/**
 * RegFieldTableModel
 */

RegFieldTableModel::RegFieldTableModel(QObject *parent)
    :QAbstractTableModel(parent)
{
    m_read_only = true;
}

int RegFieldTableModel::rowCount(const QModelIndex& /* parent */) const
{
    return m_reg.field.size();
}

int RegFieldTableModel::columnCount(const QModelIndex& /* parent */) const
{
    return ColumnCountOffset + m_value.size();
}

QVariant RegFieldTableModel::data(const QModelIndex& index, int role) const
{
    if(index.row() < 0 || (size_t)index.row() >= m_reg.field.size())
        return QVariant();
    int section = index.column();
    const soc_desc::field_t& field = m_reg.field[index.row()];
    /* column independent code */
    const RegThemeGroup *theme = 0;
    switch(m_status[index.row()])
    {
        case Normal: theme = &m_theme.normal; break;
        case Diff: theme = &m_theme.diff; break;
        case Error: theme = &m_theme.error; break;
        case None: default: break;
    }
    if(role == Qt::FontRole)
        return theme ? QVariant(theme->font) : QVariant();
    if(role == Qt::BackgroundRole)
        return theme ? QVariant(theme->background) : QVariant();
    if(role == Qt::ForegroundRole)
        return theme ? QVariant(theme->foreground) : QVariant();
    /* column dependent code */
    if(section == BitRangeColumn)
    {
        if(role == Qt::DisplayRole)
            return QVariant::fromValue(SocFieldBitRange(field));
        else if(role == Qt::TextAlignmentRole)
            return QVariant(Qt::AlignVCenter | Qt::AlignHCenter);
        else
            return QVariant();
    }
    if(section == NameColumn)
    {
        if(role == Qt::DisplayRole)
            return QVariant(QString::fromStdString(field.name));
        else
            return QVariant();
    }
    if(section < FirstValueColumn + m_value.size())
    {
        int idx = section - FirstValueColumn;
        if(role == Qt::DisplayRole)
        {
            if(!m_value[idx].isValid())
                return QVariant("<error>");
            return QVariant::fromValue(SocFieldCachedValue(field,
                field.extract(m_value[idx].value< soc_word_t >())));
        }
        else if(role == Qt::EditRole)
        {
            if(!m_value[idx].isValid())
                return QVariant();
            return QVariant::fromValue(SocFieldCachedValue(field,
                field.extract(m_value[idx].value< soc_word_t >())));
        }
        else if(role == Qt::TextAlignmentRole)
            return QVariant(Qt::AlignVCenter | Qt::AlignHCenter);
        else
            return QVariant();
    }
    section -= m_value.size();
    if(section == DescColumnOffset)
    {
        if(role == Qt::DisplayRole)
            return QVariant(QString::fromStdString(field.desc));
        else
            return QVariant();
    }
    return QVariant();
}

bool RegFieldTableModel::setData(const QModelIndex& idx, const QVariant& value, int role)
{
    if(role != Qt::EditRole)
        return false;
    int section = idx.column();
    if(section == BitRangeColumn)
    {
        if(idx.row() < 0 || idx.row() >= rowCount())
            return false;
        if(value.type() != QVariant::UserType && value.userType() == qMetaTypeId< SocFieldBitRange >())
            return false;
        SocFieldBitRange bitrange = value.value< SocFieldBitRange >();
        m_reg.field[idx.row()].pos = bitrange.GetFirstBit();
        m_reg.field[idx.row()].width = bitrange.GetLastBit() - bitrange.GetFirstBit() + 1;
        emit OnBitrangeModified(idx.row());
    }
    if(section < FirstValueColumn || section >= FirstValueColumn + m_value.size())
    {
        qDebug() << "ignore setData to column " << section;
        return false;
    }
    section -= FirstValueColumn;
    const SocFieldCachedValue& v = value.value< SocFieldCachedValue >();
    if(!m_value[section].isValid())
        return false;
    soc_word_t old_val = m_value[section].value< soc_word_t >();
    m_value[section] = QVariant(v.field().replace(old_val, v.value()));
    // update column
    RecomputeTheme();
    emit dataChanged(index(0, section), index(rowCount() - 1, section));
    emit OnValueModified(section);
    return true;
}

Qt::ItemFlags RegFieldTableModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    int section = index.column();
    if(section < FirstValueColumn || section >= FirstValueColumn + m_value.size())
    {
        /* bitrange or name */
        if(!m_read_only)
            flags |= Qt::ItemIsEditable;
        return flags;
    }
    section -= FirstValueColumn;
    if(m_value[section].isValid() && !m_read_only)
        flags |= Qt::ItemIsEditable;
    return flags;
}

QVariant RegFieldTableModel::headerData(int section, Qt::Orientation orientation,
    int role) const
{
    if(orientation == Qt::Vertical)
        return QVariant();
    if(role != Qt::DisplayRole)
        return QVariant();
    if(section == BitRangeColumn)
        return QVariant("Bits");
    if(section == NameColumn)
        return QVariant("Name");
    if(section < FirstValueColumn + m_value.size())
    {
        int idx = section - FirstValueColumn;
        if(m_value.size() == 1)
            return QVariant("Value");
        else
            return QVariant(QString("Value %1").arg((QChar)('A' + idx)));
    }
    section -= m_value.size();
    if(section == DescColumnOffset)
        return QVariant("Description");
    return QVariant();
}

void RegFieldTableModel::SetReadOnly(bool en)
{
    if(en == m_read_only)
        return;
    m_read_only = en;
}

void RegFieldTableModel::SetRegister(const soc_desc::register_t& reg)
{
    /* remove all rows */
    beginResetModel();
    m_reg.field.clear();
    endResetModel();
    /* add them all */
    beginInsertRows(QModelIndex(), 0, reg.field.size() - 1);
    m_reg = reg;
    RecomputeTheme();
    endInsertRows();
}

void RegFieldTableModel::UpdateRegister(const soc_desc::register_t& reg)
{
    m_reg = reg;
    RecomputeTheme();
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
}

soc_desc::register_t RegFieldTableModel::GetRegister() const
{
    return m_reg;
}

void RegFieldTableModel::SetValues(const QVector< QVariant >& values)
{
    /* remove all value columns */
    beginRemoveColumns(QModelIndex(), FirstValueColumn,
        FirstValueColumn + m_value.size() - 1);
    m_value.clear();
    endRemoveColumns();
    /* add them back */
    beginInsertColumns(QModelIndex(), FirstValueColumn,
        FirstValueColumn + values.size() - 1);
    m_value = values;
    RecomputeTheme();
    endInsertColumns();
}

QVariant RegFieldTableModel::GetValue(int index)
{
    return m_value[index];
}

void RegFieldTableModel::SetTheme(const RegTheme& theme)
{
    m_theme = theme;
    RecomputeTheme();
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
}

void RegFieldTableModel::RecomputeTheme()
{
    m_status.resize(m_reg.field.size());
    for(size_t i = 0; i < m_reg.field.size(); i++)
    {
        m_status[i] = None;
        if(!m_theme.valid || m_value.size() == 0)
            continue;
        m_status[i] = Normal;
        const soc_desc::field_t& field = m_reg.field[i];
        QVariant val;
        for(int j = 0; j < m_value.size(); j++)
        {
            QVariant val2 = m_value[j];
            if(!val2.isValid())
                continue;
            val2 = QVariant(field.extract(val2.value< soc_word_t >()));
            if(!val.isValid())
                val = val2;
            else if(val != val2)
                m_status[i] = Diff;
        }
    }
}

/**
 * RegFieldProxyModel
 */

bool RegFieldProxyModel::lessThan(const QModelIndex& left,
    const QModelIndex& right) const
{
    QVariant ldata = sourceModel()->data(left);
    QVariant rdata = sourceModel()->data(right);
    if(isUserType< SocFieldBitRange >(ldata) &&
            isUserType< SocFieldBitRange >(rdata))
    {
        return ldata.value< SocFieldBitRange >() <
                rdata.value< SocFieldBitRange >();
    }
    else if(isUserType< SocFieldCachedValue >(ldata) &&
            isUserType< SocFieldCachedValue >(rdata))
    {
        return ldata.value< SocFieldCachedValue >() <
                rdata.value< SocFieldCachedValue >();
    }
    else
        return QSortFilterProxyModel::lessThan(left, right);
}

/**
 * YRegDisplayItemEditor
 */
YRegDisplayItemEditor::YRegDisplayItemEditor(QWidget *parent, YRegDisplay *display,
        YRegDisplayItemDelegate *delegate, QModelIndex bitrange_index,
        QModelIndex name_index)
    :QWidget(parent), m_display_delegate(delegate),
    m_display(display), m_state(Idle)
{
    m_col_width = m_display->BitrangeRect(SocFieldBitRange(0, 0)).width();
    m_resize_margin = m_col_width / 4;
    setEditorData(bitrange_index, name_index);
    setMouseTracking(true);
    setAutoFillBackground(true);
    setFocusPolicy(Qt::StrongFocus); // QItemDelegate says it's important
}

void YRegDisplayItemEditor::setEditorData(QModelIndex bitrange_index, QModelIndex name_index)
{
    if(m_state != Idle)
    {
        m_state = Idle;
        QApplication::restoreOverrideCursor();
    }
    m_bitrange_index = bitrange_index;
    m_name_index = name_index;
    m_bitrange = bitrange_index.data().value< SocFieldBitRange >();
}

void YRegDisplayItemEditor::getEditorData(QVariant& name, QVariant& bitrange)
{
    name = QVariant(); /* don't touch the name */
    bitrange = QVariant::fromValue(m_bitrange);
}

YRegDisplayItemEditor::~YRegDisplayItemEditor()
{
    /* make sure to restore cursor if modified */
    if(m_state != Idle)
    {
        m_state = Idle;
        QApplication::restoreOverrideCursor();
    }
}

YRegDisplayItemEditor::Zone YRegDisplayItemEditor::GetZone(const QPoint& pt)
{
    if(!rect().contains(pt))
        return NoZone;
    if(pt.x() >= 0 && pt.x() <= m_resize_margin)
        return ResizeLeftZone;
    if(pt.x() >= width() - m_resize_margin && pt.x() <= width())
        return ResizeRightZone;
    return MoveZone;
}

void YRegDisplayItemEditor::mouseMoveEvent(QMouseEvent *event)
{
    Zone zone = GetZone(event->pos());
    bool in_resize_zone = (zone == ResizeLeftZone || zone == ResizeRightZone);
    /* resizing/moving has priority */
    if(m_state == ResizingLeft || m_state == ResizingRight || m_state == Moving)
    {
        SocFieldBitRange new_bitrange = m_bitrange;
        if(m_state == Moving)
        {
            /* Compute new bitrange: we know the offset of the mouse relative to the
            * left of the register: use that offset to compute the new position of
            * the MSB bit. To make it more natural, add half of a column of margin
            * so that the register does not move until half of a bit column displacement
            * was made */
            int bit = m_display->bitColumnAt(mapTo(m_display,
                event->pos() - QPoint(m_move_offset - m_col_width / 2, 0)));
            new_bitrange.SetLastBit(bit);
            int w = m_bitrange.GetLastBit() - m_bitrange.GetFirstBit();
            /* make sure range is valid */
            if(bit - w < 0)
                return;
            new_bitrange.SetFirstBit(bit - w);
        }
        else
        {
            /* Compute new bitrange. To make it more natural, add quarter of a column of margin
            * so that the register does not resize until quarter of a bit column displacement
            * was made */
            int bit = m_display->bitColumnAt(mapTo(m_display, event->pos()
                + QPoint(m_col_width / 4, 0)));
            if(m_state == ResizingLeft)
                new_bitrange.SetLastBit(bit);
            else
                new_bitrange.SetFirstBit(bit);
            /* make sure range is valid */
            if(new_bitrange.GetLastBit() < new_bitrange.GetFirstBit())
                return;
        }
        /* make sure range does not overlap with other fields */
        /* TODO */
        /* update current bitrange (display only) and resize widget */
        if(m_bitrange != new_bitrange)
        {
            m_bitrange = new_bitrange;
            /* resize widget */
            QRect rect = m_display->BitrangeRect(m_bitrange);
            rect.moveTopLeft(parentWidget()->mapFromGlobal(m_display->mapToGlobal(rect.topLeft())));
            setGeometry(rect);
        }
    }
    /* any zone -> resize zone */
    else if(in_resize_zone)
    {
        /* don't do unnecessary changes */
        if(m_state != InResizeZone)
        {
            /* restore old cursor if needed */
            if(m_state != Idle)
                QApplication::restoreOverrideCursor();
            m_state = InResizeZone;
            QApplication::setOverrideCursor(QCursor(Qt::SizeHorCursor));
        }
    }
    /* any zone -> move zone */
    else if(zone == MoveZone)
    {
        /* don't do unnecessary changes */
        if(m_state != InMoveZone)
        {
            /* restore old cursor if needed */
            if(m_state != Idle)
                QApplication::restoreOverrideCursor();
            m_state = InMoveZone;
            QApplication::setOverrideCursor(QCursor(Qt::SizeAllCursor));
        }
    }
    /* any zone -> no zone */
    else if(zone == NoZone)
    {
        if(m_state != Idle)
        {
            m_state = Idle;
            QApplication::restoreOverrideCursor();
        }
    }
}

void YRegDisplayItemEditor::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    if(m_state == InResizeZone)
    {
        m_state = Idle;
        QApplication::restoreOverrideCursor();
    }
}

void YRegDisplayItemEditor::mousePressEvent(QMouseEvent *event)
{
    /* just in case the mouseMove event was not done */
    mouseMoveEvent(event);
    /* we need to track mouse outside of widget but Qt already grabs the mouse
     * for us on mouse press in widget */
    if(m_state == InResizeZone)
    {
        if(GetZone(event->pos()) == ResizeLeftZone)
            m_state = ResizingLeft;
        else
            m_state = ResizingRight;
    }
    else if(m_state == InMoveZone)
    {
        m_state = Moving;
        /* store offset from the left, to keep relative position of the register
         * with respect to the mouse */
        m_move_offset = event->pos().x();
    }
}

void YRegDisplayItemEditor::mouseReleaseEvent(QMouseEvent *event)
{
    if(m_state == ResizingLeft ||  m_state == ResizingRight || m_state == Moving)
    {
        QApplication::restoreOverrideCursor();
        m_state = Idle;
        /* update cursor */
        mouseMoveEvent(event);
    }
}

void YRegDisplayItemEditor::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    /* reuse delegate code to paint */
    QStyleOptionViewItem opt = m_display->viewOptions();
    opt.state |= QStyle::State_HasFocus | QStyle::State_Selected | QStyle::State_Active;
    opt.displayAlignment = Qt::AlignHCenter | Qt::AlignVCenter;
    opt.rect = rect();
    opt.showDecorationSelected = true;
    m_display_delegate->initStyleOption(&opt, m_name_index);
    m_display_delegate->MyPaint(&painter, opt);
}

/**
 * YRegDisplayItemDelegate
 */

YRegDisplayItemDelegate::YRegDisplayItemDelegate(QObject *parent)
    :QStyledItemDelegate(parent)
{
}

void YRegDisplayItemDelegate::MyPaint(QPainter *painter, const QStyleOptionViewItem& option) const
{
    QStyleOptionViewItem opt = option;
    painter->save();
    // draw everything rotated, requires careful manipulation of the
    // rects involved
    painter->translate(opt.rect.bottomLeft());
    painter->rotate(-90);
    opt.rect = QRect(0, 0, opt.rect.height(), opt.rect.width());
    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
    painter->restore();
}

void YRegDisplayItemDelegate::paint(QPainter *painter,
    const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    // default alignment is centered unless specified
    opt.displayAlignment = Qt::AlignHCenter | Qt::AlignVCenter;
    initStyleOption(&opt, index);

    MyPaint(painter, opt);

}

QSize YRegDisplayItemDelegate::sizeHint(const QStyleOptionViewItem& option,
    const QModelIndex& index) const
{
    /* useless in our case, the view ignores this */
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize();
}

QWidget *YRegDisplayItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem& option,
    const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    YRegDisplay *display = dynamic_cast< YRegDisplay* >(parent->parent());
    Q_ASSERT(display != nullptr);
    /* column 0 is name, column 1 is range */
    return new YRegDisplayItemEditor(parent, display, const_cast< YRegDisplayItemDelegate* >(this),
        index.sibling(index.row(), 0), index.sibling(index.row(), 1));
}

void YRegDisplayItemDelegate::setEditorData(QWidget *editor, const QModelIndex& index) const
{
    dynamic_cast< YRegDisplayItemEditor* >(editor)->setEditorData(
        index.sibling(index.row(), 0), index.sibling(index.row(), 1));
}

void YRegDisplayItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
    const QModelIndex& index) const
{
    QVariant name, bitrange;
    dynamic_cast< YRegDisplayItemEditor* >(editor)->getEditorData(name, bitrange);
    if(name.isValid())
        model->setData(index.sibling(index.row(), 1), name);
    if(bitrange.isValid())
        model->setData(index.sibling(index.row(), 0), bitrange);
}

/**
 * YRegDisplay
 */

YRegDisplay::YRegDisplay(QWidget *parent)
    :QAbstractItemView(parent)
{
    m_is_dirty = true;
    m_range_col = 0;
    m_data_col = 1;
    m_nr_bits = 32;
    // the frame around the register is ugly, disable it
    setFrameShape(QFrame::NoFrame);
    setSelectionMode(SingleSelection);
    setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
    setItemDelegate(new YRegDisplayItemDelegate(this));
}

void YRegDisplay::setWidth(int nr_bits)
{
    m_nr_bits = nr_bits;
    m_is_dirty = true;
    recomputeGeometry();
    updateGeometries();
}

int YRegDisplay::bitColumnAt(const QPoint& point, bool closest) const
{
    int wx = point.x() + horizontalScrollBar()->value();
    for(int bit = 0; bit < m_nr_bits; bit++)
    {
        int off = columnOffset(bitToColumn(bit));
        int w = columnWidth(bitToColumn(bit));
        if(wx >= off && wx < off + w)
            return bit;
        if(wx >= off + w && closest)
            return bit;
    }
    return closest ? m_nr_bits - 1 : -1;
}

QModelIndex YRegDisplay::indexAt(const QPoint& point) const
{
    if(!model())
        return QModelIndex();
    int wx = point.x() + horizontalScrollBar()->value();
    int wy = point.y() + verticalScrollBar()->value();

    for(int i = 0; i < model()->rowCount(); i++)
    {
        QModelIndex index =  model()->index(i, m_data_col, rootIndex());
        QRect r = itemRect(index);
        if(!r.isValid())
            continue;
        if(r.contains(wx, wy))
            return index;
    }
    return QModelIndex();
}

void YRegDisplay::scrollTo(const QModelIndex& index, ScrollHint hint)
{
    Q_UNUSED(index);
    Q_UNUSED(hint);
}

QRect YRegDisplay::visualRect(const QModelIndex &index) const
{
    QRect rect = itemRect(index);
    if(rect.isValid())
        return QRect(rect.left() - horizontalScrollBar()->value(),
                    rect.top() - verticalScrollBar()->value(),
                    rect.width(), rect.height());
    else
        return rect;
}

bool YRegDisplay::isIndexHidden(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return false;
}

QModelIndex YRegDisplay::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    Q_UNUSED(cursorAction);
    Q_UNUSED(modifiers);
    return QModelIndex();
}

void YRegDisplay::setSelection(const QRect& r, QItemSelectionModel::SelectionFlags flags)
{
    if(!model())
        return;
    QRect rect = r.translated(horizontalScrollBar()->value(),
        verticalScrollBar()->value()).normalized();

    QItemSelection sel;
    for(int i = 0; i < model()->rowCount(); i++)
    {
        QModelIndex index =  model()->index(i, m_data_col, rootIndex());
        QRect r = itemRect(index);
        if(!r.isValid())
            continue;
        if(r.intersects(rect))
            sel.select(index, index);
    }
    selectionModel()->select(sel, flags);
}

int YRegDisplay::verticalOffset() const
{
    return verticalScrollBar()->value();
}

int YRegDisplay::horizontalOffset() const
{
    return horizontalScrollBar()->value();
}

void YRegDisplay::scrollContentsBy(int dx, int dy)
{
    viewport()->scroll(dx, dy);
}

void YRegDisplay::setModel(QAbstractItemModel *model)
{
    QAbstractItemView::setModel(model);
    m_is_dirty = true;
}

void YRegDisplay::dataChanged(const QModelIndex &topLeft,
    const QModelIndex &bottomRight)
{
    m_is_dirty = true;
    QAbstractItemView::dataChanged(topLeft, bottomRight);
}

void YRegDisplay::rowsInserted(const QModelIndex &parent, int start, int end)
{
    m_is_dirty = true;
    QAbstractItemView::rowsInserted(parent, start, end);
}

void YRegDisplay::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    m_is_dirty = true;
    QAbstractItemView::rowsAboutToBeRemoved(parent, start, end);
}

int YRegDisplay::separatorSize() const
{
    return 1;
}

int YRegDisplay::marginSize() const
{
    return viewOptions().fontMetrics.height() / 3;
}

int YRegDisplay::headerTextSep() const
{
    return marginSize() / 2;
}

int YRegDisplay::headerHeight() const
{
    return 2 * marginSize() + headerTextSep() + 2 * viewOptions().fontMetrics.height();
}

int YRegDisplay::minColumnWidth() const
{
    return 2 * marginSize() + viewOptions().fontMetrics.height();
}

int YRegDisplay::maxColumnWidth() const
{
    return 2 * minColumnWidth();
}

int YRegDisplay::columnWidth(int col) const
{
    int avail = width() - (m_nr_bits + 1) * separatorSize();
    int small_w = qMin(avail / m_nr_bits, maxColumnWidth());
    int nr_big = avail - small_w * m_nr_bits;
    if(col < nr_big)
        return small_w + 1;
    else
        return small_w;
}

int YRegDisplay::columnOffset(int col) const
{
    int off = separatorSize();
    for(int i = 0; i < col; i++)
        off += columnWidth(i) + separatorSize();
    int all_w = off;
    for(int i = col; i < m_nr_bits; i++)
        all_w += columnWidth(i) + separatorSize();
    return off + (width() - all_w) / 2;
}

int YRegDisplay::maxContentHeight() const
{
    int max = 0;
    QFontMetrics metrics = viewOptions().fontMetrics;
    if(model())
    {
        for(int i = 0; i < model()->rowCount(); i++)
        {
            QModelIndex index = model()->index(i, m_data_col, rootIndex());
            QString s = model()->data(index).toString();
            max = qMax(max, metrics.boundingRect(s).width());
        }
    }
    return 2 * marginSize() + max;
}

int YRegDisplay::gapHeight() const
{
    return marginSize() / 2;
}

int YRegDisplay::bitToColumn(int bit) const
{
    return m_nr_bits - 1 - bit;
}

QRegion YRegDisplay::visualRegionForSelection(const QItemSelection& selection) const
{
    QRegion region;
    foreach(const QItemSelectionRange &range, selection)
    {
        for(int row = range.top(); row <= range.bottom(); ++row)
        {
            for(int column = range.left(); column < range.right(); ++column)
            {
                QModelIndex index = model()->index(row, column, rootIndex());
                region += visualRect(index);
            }
        }
    }
    return region;
}

QRect YRegDisplay::itemRect(const QModelIndex& index) const
{
    if(!index.isValid())
        return QRect();
    QVariant vrange = model()->data(model()->index(index.row(), m_range_col, rootIndex()));
    if(!vrange.canConvert< SocFieldBitRange >())
        return QRect();
    SocFieldBitRange range = vrange.value< SocFieldBitRange >();
    return itemRect(range, index.column());
}

QRect YRegDisplay::BitrangeRect(const SocFieldBitRange& range) const
{
    return itemRect(range, m_data_col);
}

QRect YRegDisplay::itemRect(const SocFieldBitRange& range, int col) const
{
    int top, bot;
    if(col == m_range_col)
    {
        top = separatorSize();
        bot = separatorSize() + headerHeight() - 1;
    }
    else if(col == m_data_col)
    {
        top = headerHeight() + 3 * separatorSize() + gapHeight();
        bot = height() - separatorSize() - 1;
    }
    else
        return QRect();
    int first_col = bitToColumn(range.GetFirstBit());
    return QRect(QPoint(columnOffset(bitToColumn(range.GetLastBit())), top),
        QPoint(columnOffset(first_col) + columnWidth(first_col) - 1, bot));
}

void YRegDisplay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(viewport());
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.translate(-horizontalScrollBar()->value(), -verticalScrollBar()->value());
    QStyleOptionViewItem option = viewOptions();
    int txt_h = option.fontMetrics.height();
    int grid_hint = style()->styleHint(QStyle::SH_Table_GridLineColor, &option, this);
    QBrush grid_brush(static_cast< QRgb >(grid_hint));
    QBrush back_brush = option.palette.base();
    int sep_sz = separatorSize();
    QItemSelectionModel *selections = selectionModel();

    // paint header
    for(int bit = 0; bit < m_nr_bits; bit++)
    {
        QRect r = itemRect(SocFieldBitRange(bit, bit), m_range_col);
        // paint background
        painter.fillRect(r, back_brush);
        // paint top digit
        r.setTop(r.top() + marginSize());
        r.setBottom(r.top() + txt_h);
        style()->drawItemText(&painter, r, Qt::AlignHCenter, option.palette,
            true, QString("%1").arg(bit / 10), foregroundRole());
        // paint bottom digit
        r.setTop(r.bottom() + headerTextSep());
        r.setBottom(r.top() + txt_h);
        style()->drawItemText(&painter, r, Qt::AlignHCenter, option.palette,
            true, QString("%1").arg(bit % 10), foregroundRole());
    }
    // paint header grid
    for(int bit = 1; bit < m_nr_bits; bit++)
    {
        QRect r = itemRect(SocFieldBitRange(bit, bit), m_range_col);
        r.setCoords(r.right() + 1, r.top(), r.right() + sep_sz, r.bottom());
        if((bit % 4) == 0)
            r.setCoords(r.left() - sep_sz, r.top(), r.right() + sep_sz, r.bottom());
        painter.fillRect(r, grid_brush);
    }
    QRect hdr_r = itemRect(SocFieldBitRange(0, m_nr_bits - 1), m_range_col);
    painter.fillRect(QRect(hdr_r.left(), hdr_r.top() - sep_sz, hdr_r.width(), sep_sz), grid_brush);
    painter.fillRect(QRect(hdr_r.left(), hdr_r.bottom() + 1, hdr_r.width(), sep_sz), grid_brush);
    // paint header gap
    QRect gap_r(hdr_r.left(), hdr_r.bottom() + sep_sz + 1, hdr_r.width(), gapHeight());
    painter.fillRect(gap_r, back_brush);
    // paint header bottom line
    painter.fillRect(QRect(gap_r.left(), gap_r.bottom() + 1, gap_r.width(), sep_sz), grid_brush);
    // paint background
    QRect data_r = itemRect(SocFieldBitRange(0, m_nr_bits - 1), m_data_col);
    //painter.fillRect(data_r, back_brush);
    // paint data bottom line
    painter.fillRect(QRect(data_r.left(), data_r.bottom() + 1, data_r.width(), sep_sz), grid_brush);
    // paint left/right lines
    painter.fillRect(QRect(hdr_r.left() - sep_sz, hdr_r.top() - sep_sz, sep_sz, height()), grid_brush);
    painter.fillRect(QRect(hdr_r.right() + 1, hdr_r.top() - sep_sz, sep_sz, height()), grid_brush);

    // paint model
    if(!model())
        return;

    for(int i = 0; i < model()->rowCount(); i++)
    {
        QModelIndex index = model()->index(i, m_data_col, rootIndex());
        QRect r = itemRect(index);
        if(!r.isValid())
            continue;
        QString name = index.data().toString();
        // paint background
        QStyleOptionViewItem opt = viewOptions();
        opt.rect = r;
        //opt.showDecorationSelected = true;
        style()->drawPrimitive(QStyle::PE_PanelItemViewRow, &opt, &painter, this);
        if(selections->isSelected(index))
            opt.state |= QStyle::State_Selected;
        if(currentIndex() == index)
            opt.state |= QStyle::State_HasFocus;
        if(m_hover == index)
            opt.state |= QStyle::State_MouseOver;
        itemDelegate(index)->paint(&painter, opt, index);
        // paint left/right lines
        painter.fillRect(QRect(r.left() - sep_sz, r.top(), sep_sz, r.height()), grid_brush);
        painter.fillRect(QRect(r.right() + 1, r.top(), sep_sz, r.height()), grid_brush);
    }
}

void YRegDisplay::recomputeGeometry()
{
    if(!m_is_dirty)
        return;
    /* height: header + gap + sep + content + sep */
    m_minimum_height = 0;
    m_minimum_height += headerHeight() + gapHeight();
    m_minimum_height += 2 * separatorSize() + maxContentHeight();
    /* width: sep + (col + sep) * n */
    m_minimum_width = separatorSize() * (m_nr_bits + 1) + minColumnWidth() * m_nr_bits;
    m_is_dirty = false;
    viewport()->update();
}

void YRegDisplay::resizeEvent(QResizeEvent*)
{
    m_is_dirty = true;
    recomputeGeometry();
    updateGeometries();
}

void YRegDisplay::updateGeometries()
{
    horizontalScrollBar()->setSingleStep(1);
    horizontalScrollBar()->setPageStep(viewport()->width());
    horizontalScrollBar()->setRange(0, qMax(0, m_minimum_width - viewport()->width()));
    verticalScrollBar()->setSingleStep(1);
    verticalScrollBar()->setPageStep(viewport()->height());
    verticalScrollBar()->setRange(0, qMax(0, m_minimum_height - viewport()->height()));
}

bool YRegDisplay::viewportEvent(QEvent *event)
{
    /* FIXME Apparently QAbstractItemView tracks the hovered index but keeps it
     * in its private part which is not accessible, which makes it useless...
     * This code reimplements it */
    switch (event->type())
    {
        case QEvent::HoverEnter:
            m_hover = indexAt(static_cast<QHoverEvent*>(event)->pos());
            update(m_hover);
            break;
        case QEvent::HoverLeave:
            update(m_hover); // update old
            m_hover = QModelIndex();
            break;
        case QEvent::HoverMove:
        {
            QModelIndex old = m_hover;
            m_hover = indexAt(static_cast<QHoverEvent*>(event)->pos());
            if(m_hover != old)
                viewport()->update(visualRect(old)|visualRect(m_hover));
            break;
        }
        default:
            break;
    }
    return QAbstractItemView::viewportEvent(event);
}

/**
 * GrowingTableView
 */
GrowingTableView::GrowingTableView(QWidget *parent)
    :QTableView(parent)
{
}

void GrowingTableView::setModel(QAbstractItemModel *m)
{
    if(model())
        disconnect(model(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
        this, SLOT(DataChanged(const QModelIndex&, const QModelIndex&)));
    QTableView::setModel(m);
    connect(model(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
        this, SLOT(DataChanged(const QModelIndex&, const QModelIndex&)));
    DataChanged(QModelIndex(), QModelIndex());
}

void GrowingTableView::DataChanged(const QModelIndex& tl, const QModelIndex& br)
{
    Q_UNUSED(tl);
    Q_UNUSED(br);
    resizeColumnsToContents();
    int h = contentsMargins().top() + contentsMargins().bottom();
    h += horizontalHeader()->height();
    for(int i = 0; i < model()->rowCount(); i++)
        h += rowHeight(i);
    setMinimumHeight(h);
}

/**
 * MyTextEditor
 */
MyTextEditor::MyTextEditor(QWidget *parent)
    :QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout;
    m_toolbar = new QToolBar(this);
    m_edit = new QTextEdit(this);
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    /* Qt 5.2 have a hardcoded sizeHint for QAbstractScrollArea which makes it
     * hard to have a good behaviour for the text editor. Fortunately 5.2 introduces
     * a new option to adjust this to the content. */
    m_edit->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
#endif
    layout->addWidget(m_toolbar, 0);
    layout->addWidget(m_edit, 1);
    setLayout(layout);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_edit->setAcceptRichText(false);
    m_edit->setAutoFormatting(QTextEdit::AutoAll);

    m_bold_button = new QToolButton(this);
    m_bold_button->setIcon(YIconManager::Get()->GetIcon(YIconManager::FormatTextBold));
    m_bold_button->setText("bold");
    m_bold_button->setCheckable(true);

    m_italic_button = new QToolButton(this);
    m_italic_button->setIcon(YIconManager::Get()->GetIcon(YIconManager::FormatTextItalic));
    m_italic_button->setText("italic");
    m_italic_button->setCheckable(true);

    m_underline_button = new QToolButton(this);
    m_underline_button->setIcon(YIconManager::Get()->GetIcon(YIconManager::FormatTextUnderline));
    m_underline_button->setText("underline");
    m_underline_button->setCheckable(true);

    m_toolbar->addWidget(m_bold_button);
    m_toolbar->addWidget(m_italic_button);
    m_toolbar->addWidget(m_underline_button);

    connect(m_bold_button, SIGNAL(toggled(bool)), this, SLOT(OnTextBold(bool)));
    connect(m_italic_button, SIGNAL(toggled(bool)), this, SLOT(OnTextItalic(bool)));
    connect(m_underline_button, SIGNAL(toggled(bool)), this, SLOT(OnTextUnderline(bool)));
    connect(m_edit, SIGNAL(textChanged()), this, SLOT(OnInternalTextChanged()));
    connect(m_edit, SIGNAL(currentCharFormatChanged(const QTextCharFormat&)),
        this, SLOT(OnCharFormatChanged(const QTextCharFormat&)));

    m_edit->installEventFilter(this);

    SetGrowingMode(false);
    SetReadOnly(false);
    m_toolbar->hide();
}

void MyTextEditor::SetReadOnly(bool en)
{
    m_read_only = en;
    if(en)
        m_toolbar->hide();
    else
        m_toolbar->show();
    m_edit->setReadOnly(en);
}

void MyTextEditor::SetGrowingMode(bool en)
{
    m_growing_mode = en;
    if(en)
    {
        m_edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        m_edit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        OnInternalTextChanged();
    }
    else
    {
        m_edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_edit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }
}

void MyTextEditor::OnInternalTextChanged()
{
    if(m_growing_mode)
    {
        int content_size = m_edit->document()->documentLayout()->documentSize().height();
        content_size = qMax(content_size, m_edit->fontMetrics().height());
        m_edit->setMinimumHeight(content_size + m_edit->contentsMargins().top() +
            m_edit->contentsMargins().bottom());
    }
    emit OnTextChanged();
    emit OnTextChanged(GetTextHtml());
}

void MyTextEditor::OnTextBold(bool checked)
{
    QTextCursor cursor = m_edit->textCursor();
    QTextCharFormat fmt = cursor.charFormat();
    fmt.setFontWeight(checked ? QFont::Bold : QFont::Normal);
    cursor.setCharFormat(fmt);
    m_edit->setTextCursor(cursor);
}

void MyTextEditor::OnTextItalic(bool checked)
{
    QTextCursor cursor = m_edit->textCursor();
    QTextCharFormat fmt = cursor.charFormat();
    fmt.setFontItalic(checked);
    cursor.setCharFormat(fmt);
    m_edit->setTextCursor(cursor);
}

void MyTextEditor::OnTextUnderline(bool checked)
{
    QTextCursor cursor = m_edit->textCursor();
    QTextCharFormat fmt = cursor.charFormat();
    fmt.setFontUnderline(checked);
    cursor.setCharFormat(fmt);
    m_edit->setTextCursor(cursor);
}

void MyTextEditor::OnCharFormatChanged(const QTextCharFormat& fmt)
{
    /* NOTE: changing the button states programmaticaly doesn't trigger
     * the toggled() signals, otherwise it would result in a loop
     * between this function and OnText{Bold,Italic,Underline,...} */
    m_bold_button->setChecked(fmt.fontWeight() > QFont::Normal);
    m_italic_button->setChecked(fmt.fontItalic());
    m_underline_button->setChecked(fmt.fontUnderline());
}

void MyTextEditor::SetTextHtml(const QString& text)
{
    m_edit->setHtml(text);
}

QString MyTextEditor::GetTextHtml()
{
    return m_edit->toPlainText();
}

bool MyTextEditor::IsModified()
{
    return m_edit->document()->isModified();
}

bool MyTextEditor::eventFilter(QObject *object, QEvent *event)
{
    if(object != m_edit)
        return false;
    if(m_read_only)
        return false;
    if(event->type() == QEvent::FocusIn)
        m_toolbar->show();
    else if(event->type() == QEvent::FocusOut)
        m_toolbar->hide();
    return false;
}

/**
 * BackendSelector
 */
BackendSelector::BackendSelector(Backend *backend, QWidget *parent)
    :QWidget(parent), m_backend(backend)
{
    m_data_selector = new QComboBox(this);
    m_data_selector->addItem(YIconManager::Get()->GetIcon(YIconManager::TextGeneric),
        "Nothing...", QVariant(DataSelNothing));
    m_data_selector->addItem(YIconManager::Get()->GetIcon(YIconManager::DocumentOpen),
        "File...", QVariant(DataSelFile));
#ifdef HAVE_HWSTUB
    m_data_selector->addItem(YIconManager::Get()->GetIcon(YIconManager::MultimediaPlayer),
        "USB Device...", QVariant(DataSelDevice));
#endif
    m_data_sel_edit = new QLineEdit(this);
    m_data_sel_edit->setReadOnly(true);
    m_nothing_text = new QLabel(this);
    m_nothing_text->setTextFormat(Qt::RichText);
    QHBoxLayout *data_sel_layout = new QHBoxLayout(this);
    data_sel_layout->addWidget(m_data_selector);
    data_sel_layout->addWidget(m_data_sel_edit, 1);
    data_sel_layout->addWidget(m_nothing_text, 1);
    data_sel_layout->addStretch(0);
#ifdef HAVE_HWSTUB
    m_dev_selector = new QComboBox;
    m_ctx_model = new HWStubContextModel;
    m_ctx_model->EnableDummy(true, "Please select a device...");
    m_dev_selector->setModel(m_ctx_model); /* m_dev_selector will delete m_ctx_model */
    m_ctx_selector = new QComboBox;
    m_ctx_manager = HWStubManager::Get();
    m_ctx_selector->setModel(m_ctx_manager);
    m_ctx_manage_button = new QPushButton();
    m_ctx_manage_button->setIcon(YIconManager::Get()->GetIcon(YIconManager::Preferences));
    m_ctx_manage_button->setToolTip("Manage contexts");
    data_sel_layout->addWidget(m_dev_selector, 1);
    data_sel_layout->addWidget(m_ctx_selector);
    data_sel_layout->addWidget(m_ctx_manage_button);
#endif

    m_io_backend = m_backend->CreateDummyIoBackend();

    connect(m_data_selector, SIGNAL(activated(int)),
        this, SLOT(OnDataSelChanged(int)));
#ifdef HAVE_HWSTUB
    connect(m_ctx_selector, SIGNAL(currentIndexChanged(int)), this,
        SLOT(OnContextSelChanged(int)));
    connect(m_dev_selector, SIGNAL(currentIndexChanged(int)), this,
        SLOT(OnDeviceSelChanged(int)));
    connect(m_dev_selector, SIGNAL(activated(int)), this,
        SLOT(OnDeviceSelActivated(int)));
#endif

#ifdef HAVE_HWSTUB
    OnContextSelChanged(0);
#endif
    OnDataSelChanged(0);
}

BackendSelector::~BackendSelector()
{
    /* avoid m_ctx_selector from deleting HWStubManager */
#ifdef HAVE_HWSTUB
    m_ctx_selector->setModel(new QStandardItemModel());
#endif
    delete m_io_backend;
}

void BackendSelector::SetNothingMessage(const QString& msg)
{
    m_nothing_text->setText(msg);
}

void BackendSelector::OnDataSelChanged(int index)
{
    if(index == -1)
        return;
    QVariant var = m_data_selector->itemData(index);
    if(var == DataSelFile)
    {
        m_nothing_text->hide();
        m_data_sel_edit->show();
#ifdef HAVE_HWSTUB
        m_dev_selector->hide();
        m_ctx_selector->hide();
        m_ctx_manage_button->hide();
#endif
        QFileDialog *fd = new QFileDialog(m_data_selector);
        QStringList filters;
        filters << "Textual files (*.txt)";
        filters << "All files (*)";
        fd->setNameFilters(filters);
        fd->setDirectory(Settings::Get()->value("regtab/loaddatadir", QDir::currentPath()).toString());
        if(fd->exec())
        {
            QStringList filenames = fd->selectedFiles();
            ChangeBackend(m_backend->CreateFileIoBackend(filenames[0]));
            m_data_sel_edit->setText(filenames[0]);
        }
        Settings::Get()->setValue("regtab/loaddatadir", fd->directory().absolutePath());
    }
#ifdef HAVE_HWSTUB
    else if(var == DataSelDevice)
    {
        m_nothing_text->hide();
        m_data_sel_edit->hide();
        m_dev_selector->show();
        m_ctx_selector->show();
        m_ctx_manage_button->show();
        /* explicitely change the backend now */
        OnDeviceSelActivated(m_dev_selector->currentIndex());
    }
#endif
    else
    {
        m_data_sel_edit->hide();
        m_nothing_text->show();
#ifdef HAVE_HWSTUB
        m_dev_selector->hide();
        m_ctx_selector->hide();
        m_ctx_manage_button->hide();
#endif

        ChangeBackend(m_backend->CreateDummyIoBackend());
    }
}

IoBackend *BackendSelector::GetBackend()
{
    return m_io_backend;
}

void BackendSelector::ChangeBackend(IoBackend *new_backend)
{
    /* WARNING: delete old backend *after* calling the signal, otherwise the old backend
     * might get used after delete */
    emit OnSelect(new_backend);
    delete m_io_backend;
    m_io_backend = new_backend;
}

#ifdef HAVE_HWSTUB
void BackendSelector::OnContextSelChanged(int index)
{
    m_ctx_model->SetContext(m_ctx_manager->GetContext(index));
    m_dev_selector->setCurrentIndex(0);
}

void BackendSelector::OnDeviceSelChanged(int index)
{
    /* if current selection is -1, because device was removed or a new context
     * was selected, select entry 0, which is dummy. Not that this will not
     * call activate(), we don't want to change the current backend if the user
     * is using another type of backend. */
    if(index == -1)
        m_dev_selector->setCurrentIndex(0);
}

void BackendSelector::OnDeviceSelActivated(int index)
{
    auto dev = new HWStubDevice(m_ctx_model->GetDevice(index));
    if(!dev->IsValid())
    {
        delete dev;
        ChangeBackend(m_backend->CreateDummyIoBackend()); 
    }
    else
        ChangeBackend(new HWStubIoBackend(dev));
}
#endif

/**
 * YTabWidget
 */
YTabWidget::YTabWidget(QTabBar *bar, QWidget *parent)
    :QTabWidget(parent), m_other_button(0)
{
    if(bar != 0)
        setTabBar(bar);
    m_tab_open_button = new QToolButton(this);
    m_tab_open_button->setIcon(YIconManager::Get()->GetIcon(YIconManager::ListAdd));
    m_tab_open_button->setAutoRaise(true);
    m_tab_open_button->setPopupMode(QToolButton::InstantPopup);
    /* the arrow with an icon only is pretty ugly and QToolButton has no way
     * to remove the arrow programmaticaly, so use the CSS to do that */
    m_tab_open_button->setStyleSheet("QToolButton::menu-indicator { image: none; }");
    setCornerWidget(m_tab_open_button, Qt::TopLeftCorner);
    setTabOpenable(false);
    connect(m_tab_open_button, SIGNAL(clicked(bool)), this, SLOT(OnOpenButton(bool)));
    /* there is a quirk in the default QStyle: if the tab bar is empty, it
     * returns the minimum size of the corner widget, which is 0 for tool buttons */
    m_tab_open_button->setMinimumSize(m_tab_open_button->sizeHint());
    setMinimumSize(m_tab_open_button->sizeHint());
}

void YTabWidget::setTabOpenable(bool openable)
{
    m_tab_openable = openable;
    m_tab_open_button->setVisible(openable);
}

void YTabWidget::OnOpenButton(bool checked)
{
    Q_UNUSED(checked);
    emit tabOpenRequested();
}

void YTabWidget::setTabOpenMenu(QMenu *menu)
{
    m_tab_open_button->setMenu(menu);
}

void YTabWidget::setOtherMenu(QMenu *menu)
{
    if(menu == nullptr)
    {
        if(m_other_button)
            delete m_other_button;
        m_other_button = nullptr;
    }
    else
    {
        if(m_other_button == nullptr)
        {
            m_other_button = new QToolButton(this);
            m_other_button->setText("Menu");
            m_other_button->setAutoRaise(true);
            m_other_button->setPopupMode(QToolButton::InstantPopup);
            setCornerWidget(m_other_button, Qt::TopRightCorner);
        }
        m_other_button->setMenu(menu);
    }
}

/**
 * MessageWidget
 */
MessageWidget::MessageWidget(QWidget *parent)
    :QFrame(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    m_icon = new QLabel(this);
    m_icon->hide();
    m_text = new QLabel(this);
    m_text->setTextFormat(Qt::RichText);
    m_text->setWordWrap(true);
    m_close = new QToolButton(this);
    m_close->setText("close");
    m_close->setIcon(style()->standardIcon(QStyle::QStyle::SP_DialogCloseButton));
    m_close->setAutoRaise(true);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(m_icon, 0);
    layout->addWidget(m_text, 1);
    layout->addWidget(m_close, 0);

    m_id = 0;

    connect(m_close, SIGNAL(clicked(bool)), this, SLOT(OnClose(bool)));

    hide();
}

MessageWidget::~MessageWidget()
{
}

void MessageWidget::UpdateType()
{
    /* style stolen from KMessageWidget */
    QColor bg, border;
    switch(m_type)
    {
        case Positive:
            bg.setRgb(140, 228, 124);
            border.setRgb(56, 175, 58);
            break;
        case Information:
            bg.setRgb(161, 178, 202);
            border.setRgb(59, 79, 175);
            break;
        case Warning:
            bg.setRgb(228, 227, 127);
            border.setRgb(175, 169, 61);
            break;
        case Error:
            bg.setRgb(233, 199, 196);
            border.setRgb(175, 74, 60);
            break;
        default:
            break;
    }
    setStyleSheet(QString(
        "QFrame { background-color: %1;"
            "border-radius: 5px;"
            "border: 1px solid %2;"
        "}"
        "QLabel { border: none; }")
        .arg(bg.name())
        .arg(border.name()));
}

int MessageWidget::SetMessage(MessageType type, const QString& msg)
{
    m_type = type;
    m_text->setText(msg);
    UpdateType();
    show();
    return ++m_id;
}

void MessageWidget::HideMessage(int id)
{
    if(m_id == id)
        OnClose(true);
}

void MessageWidget::OnClose(bool clicked)
{
    Q_UNUSED(clicked);
    hide();
}

/*
 * YIconManager
 */
YIconManager *YIconManager::m_singleton = nullptr;

YIconManager::YIconManager()
{
    m_icon_name[ListAdd] = "list-add";
    m_icon_fallback[ListAdd] = QStyle::SP_CustomBase; /* drawn by RenderListAdd */
    m_icon_name[ListRemove] = "list-remove";
    m_icon_fallback[ListRemove] = QStyle::SP_CustomBase; /* drawn by RenderListAdd */
    m_icon_name[DocumentNew] = "document-new";
    m_icon_fallback[DocumentNew] = QStyle::SP_FileDialogNewFolder;
    m_icon_name[DocumentEdit] = "document-edit";
    m_icon_fallback[DocumentEdit] = QStyle::SP_FileDialogContentsView;
    m_icon_name[DocumentOpen] = "document-open";
    m_icon_fallback[DocumentOpen] = QStyle::SP_DialogOpenButton;
    m_icon_name[DocumentSave] = "document-save";
    m_icon_fallback[DocumentSave] = QStyle::SP_DialogSaveButton;
    m_icon_name[DocumentSaveAs] = "document-save-as";
    m_icon_fallback[DocumentSaveAs] = QStyle::SP_DialogSaveButton;
    m_icon_name[Preferences] = "preferences-system";
    m_icon_fallback[Preferences] = QStyle::SP_FileDialogInfoView;
    m_icon_name[FolderNew] = "folder-new";
    m_icon_fallback[FolderNew] = QStyle::SP_FileDialogNewFolder;
    m_icon_name[Computer] = "computer";
    m_icon_fallback[Computer] = QStyle::SP_ComputerIcon;
    m_icon_name[Cpu] = "cpu";
    m_icon_fallback[Cpu] = QStyle::SP_DriveHDIcon;
    m_icon_name[DialogError] = "dialog-error";
    m_icon_fallback[DialogError] = QStyle::SP_MessageBoxCritical;
    m_icon_name[ViewRefresh] = "view-refresh";
    m_icon_fallback[ViewRefresh] = QStyle::SP_BrowserReload;
    m_icon_name[SytemRun] = "system-run";
    m_icon_fallback[SytemRun] = QStyle::SP_MediaPlay;
    m_icon_name[ApplicationExit] = "application-exit";
    m_icon_fallback[ApplicationExit] = QStyle::SP_TitleBarCloseButton;
    m_icon_name[HelpAbout] = "help-about";
    m_icon_fallback[HelpAbout] = QStyle::SP_MessageBoxInformation;
    m_icon_name[FormatTextBold] = "format-text-bold";
    m_icon_fallback[FormatTextBold] = QStyle::SP_CustomBase; /* drawn by RenderLetter */
    m_icon_name[FormatTextItalic] = "format-text-italic";
    m_icon_fallback[FormatTextItalic] = QStyle::SP_CustomBase; /* drawn by RenderLetter */
    m_icon_name[FormatTextUnderline] = "format-text-underline";
    m_icon_fallback[FormatTextUnderline] = QStyle::SP_CustomBase; /* drawn by RenderLetter */
    m_icon_name[TextGeneric] = "text-x-generic";
    m_icon_fallback[TextGeneric] = QStyle::SP_FileDialogDetailedView;
    m_icon_name[MultimediaPlayer] = "multimedia-player";
    m_icon_fallback[MultimediaPlayer] = QStyle::SP_ComputerIcon;
}

YIconManager::~YIconManager()
{
}

YIconManager *YIconManager::Get()
{
    if(m_singleton == nullptr)
        m_singleton = new YIconManager();
    return m_singleton;
}

QIcon YIconManager::GetIcon(IconType type)
{
    if(type < 0 || type >= MaxIcon)
        return QIcon();
    /* cache icons */
    if(m_icon[type].isNull())
        m_icon[type] = QIcon::fromTheme(m_icon_name[type], GetFallbackIcon(type));
    return m_icon[type];
}

namespace
{
    QIcon RenderListAdd()
    {
        QPixmap pix(64, 64);
        pix.fill(Qt::transparent);
        QPainter paint(&pix);
        paint.fillRect(30, 12, 4, 40, QColor(255, 0, 0));
        paint.fillRect(12, 30, 40, 4, QColor(255, 0, 0));
        return QIcon(pix);
    }

    QIcon RenderListRemove()
    {
        QPixmap pix(64, 64);
        pix.fill(Qt::transparent);
        QPainter paint(&pix);
        paint.setPen(QColor(255, 0, 0));
        paint.drawLine(12, 12, 52, 52);
        paint.drawLine(12, 52, 52, 16);
        return QIcon(pix);
    }

    QIcon RenderUnknown()
    {
        QPixmap pix(64, 64);
        pix.fill();
        QPainter paint(&pix);
        paint.fillRect(0, 0, 64, 64, QColor(255, 0, 0));
        return QIcon(pix);
    }

    QIcon RenderText(const QString& str, bool bold, bool italic, bool underline)
    {
        QPixmap pix(64, 64);
        pix.fill();
        QPainter paint(&pix);
        QFont font = QApplication::font("QButton");
        font.setBold(bold);
        font.setItalic(italic);
        font.setUnderline(underline);
        font.setPixelSize(64);
        paint.setFont(font);
        paint.drawText(0, 0, 64, 64, Qt::AlignCenter, str);
        return QIcon(pix);
    }
}

QIcon YIconManager::GetFallbackIcon(IconType type)
{
    if(m_icon_fallback[type] != QStyle::SP_CustomBase)
        return QApplication::style()->standardIcon(m_icon_fallback[type]);
    switch(type)
    {
        case ListAdd: return RenderListAdd(); break;
        case ListRemove: return RenderListRemove(); break;
        case FormatTextBold: return RenderText("B", true, false, false);
        case FormatTextItalic: return RenderText("I", false, true, false);
        case FormatTextUnderline: return RenderText("U", false, false, true);
        default: return RenderUnknown(); break;
    }
}

/**
 * Misc
 */

QGroupBox *Misc::EncloseInBox(const QString& name, QWidget *widget)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(widget);
    return Misc::EncloseInBox(name, layout);
}

QGroupBox *Misc::EncloseInBox(const QString& name, QLayout *layout)
{
    QGroupBox *group = new QGroupBox(name);
    group->setLayout(layout);
    return group;
}
