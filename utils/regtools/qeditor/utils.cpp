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

/**
 * SocBitRangeValidator
 */
SocBitRangeValidator::SocBitRangeValidator(QObject *parent)
    :QValidator(parent)
{
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
        if(!ok || last < 0 || last >= 32)
            return Invalid;
    }
    else
        last = 31;
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

/**
 * SocFieldValidator
 */

SocFieldValidator::SocFieldValidator(QObject *parent)
    :QValidator(parent)
{
    m_field.first_bit = 0;
    m_field.last_bit = 31;
}

SocFieldValidator::SocFieldValidator(const soc_reg_field_t& field, QObject *parent)
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
    foreach(const soc_reg_field_value_t& value, m_field.value)
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
    unsigned nr_bits = m_field.last_bit - m_field.first_bit + 1;
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

/**
 * SocFieldEditor
 */
SocFieldEditor::SocFieldEditor(const soc_reg_field_t& field, QWidget *parent)
    :QLineEdit(parent), m_reg_field(field)
{
    m_validator = new SocFieldValidator(field);
    setValidator(m_validator);
}

SocFieldEditor::~SocFieldEditor()
{
    delete m_validator;
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
    int digits = (m_reg_field.last_bit - m_reg_field.first_bit + 4) / 4;
    setText(QString("0x%1").arg(field, digits, 16, QChar('0')));
}

void SocFieldEditor::SetRegField(const soc_reg_field_t& field)
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
SocFieldCachedValue::SocFieldCachedValue(const soc_reg_field_t& field, uint value)
    :m_field(field), m_value(value)
{
    int idx = field.find_value(value);
    if(idx != -1)
        m_name = QString::fromStdString(field.value[idx].name);
}

/**
 * SocFieldCachedItemDelegate
 */

QString SocFieldCachedItemDelegate::displayText(const QVariant& value, const QLocale& locale) const
{
    // FIXME see QTBUG-30392
    if(value.type() == QVariant::UserType && value.userType() == qMetaTypeId< SocFieldCachedValue >())
    {
        const SocFieldCachedValue& v = value.value< SocFieldCachedValue >();
        int bitcount = v.field().last_bit - v.field().first_bit;
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
    :SocFieldEditor(soc_reg_field_t(), parent)
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
    int section = index.column();
    const soc_reg_field_t& field = m_reg.field[index.row()];
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
    if(section < FirstValueColumn || section >= FirstValueColumn + m_value.size())
        return false;
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
        return flags;
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

void RegFieldTableModel::SetRegister(const soc_reg_t& reg)
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
        const soc_reg_field_t& field = m_reg.field[i];
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
 * RegSexyDisplay2
 */

RegSexyDisplay2::RegSexyDisplay2(QWidget *parent)
    :QAbstractItemView(parent)
{
    m_is_dirty = true;
    // the frame around the register is ugly, disable it
    setFrameShape(QFrame::NoFrame);
}

QModelIndex RegSexyDisplay2::indexAt(const QPoint& point) const
{
    Q_UNUSED(point);
    return QModelIndex();
}

void RegSexyDisplay2::scrollTo(const QModelIndex& index, ScrollHint hint)
{
    Q_UNUSED(index);
    Q_UNUSED(hint);
}

QRect RegSexyDisplay2::visualRect(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QRect();
}

bool RegSexyDisplay2::isIndexHidden(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return false;
}

QModelIndex RegSexyDisplay2::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    Q_UNUSED(cursorAction);
    Q_UNUSED(modifiers);
    return QModelIndex();
}

void RegSexyDisplay2::setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags flags)
{
    Q_UNUSED(rect);
    Q_UNUSED(flags);
}

int RegSexyDisplay2::verticalOffset() const
{
    return verticalScrollBar()->value();
}

int RegSexyDisplay2::horizontalOffset() const
{
    return horizontalScrollBar()->value();
}

void RegSexyDisplay2::scrollContentsBy(int dx, int dy)
{
    viewport()->scroll(dx, dy);
}

void RegSexyDisplay2::setModel(QAbstractItemModel *model)
{
    QAbstractItemView::setModel(model);
    m_is_dirty = true;
}

void RegSexyDisplay2::dataChanged(const QModelIndex &topLeft,
    const QModelIndex &bottomRight)
{
    m_is_dirty = true;
    QAbstractItemView::dataChanged(topLeft, bottomRight);
}

void RegSexyDisplay2::rowsInserted(const QModelIndex &parent, int start, int end)
{
    m_is_dirty = true;
    QAbstractItemView::rowsInserted(parent, start, end);
}

void RegSexyDisplay2::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    m_is_dirty = true;
    QAbstractItemView::rowsAboutToBeRemoved(parent, start, end);
}

int RegSexyDisplay2::GetSeparatorSize() const
{
    return 1;
}

int RegSexyDisplay2::GetMarginSize() const
{
    return viewOptions().fontMetrics.height() / 3;
}

int RegSexyDisplay2::GetHeaderTextSep() const
{
    return GetMarginSize() / 2;
}

int RegSexyDisplay2::GetHeaderHeight() const
{
    return 2 * GetMarginSize() + GetHeaderTextSep() + 2 * viewOptions().fontMetrics.height();
}

int RegSexyDisplay2::GetColumnWidth() const
{
    return 2 * GetMarginSize() + viewOptions().fontMetrics.height();
}

int RegSexyDisplay2::GetMaxContentHeight() const
{
    int max = 0;
    QFontMetrics metrics = viewOptions().fontMetrics;
    if(model())
    {
        for(int i = 0; i < model()->rowCount(); i++)
        {
            QModelIndex index = model()->index(i, 1, rootIndex());
            QString s = model()->data(index).toString();
            max = qMax(max, metrics.boundingRect(s).width());
        }
    }
    return 2 * GetMarginSize() + max;
}

int RegSexyDisplay2::GetGapHeight() const
{
    return GetMarginSize() / 2;
}

QRegion RegSexyDisplay2::visualRegionForSelection(const QItemSelection& selection) const
{
    Q_UNUSED(selection);
    return QRegion();
}

void RegSexyDisplay2::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    int txt_h = viewOptions().fontMetrics.height();
    int sep_sz = GetSeparatorSize();
    int w = qMax(m_minimum_width, viewport()->width());
    int h = qMax(m_minimum_height, viewport()->height());
    int nr_bits = 32;
    int col_w = (w - (nr_bits + 1) * sep_sz) / nr_bits;
    int hdr_h = GetHeaderHeight();
    int gap_h = GetGapHeight();
    int tot_w = (nr_bits + 1) * sep_sz + nr_bits * col_w;
    int margin = GetMarginSize();
    int txt_sep = GetHeaderTextSep();
    int tot_hdr_sz = 2 * sep_sz + hdr_h;

    int x_shift = (w - tot_w) / 2;
#define ith_col_x(i) (x_shift + (i) * (sep_sz + col_w))

    QPainter painter(viewport());
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.translate(-horizontalScrollBar()->value(), -verticalScrollBar()->value());
    QStyleOptionViewItem option = viewOptions();
    QBrush back_brush = option.palette.base();
    QBrush line_brush = option.palette.dark();

    // fill interesting zone with base
    painter.fillRect(event->rect(), option.palette.window());
    painter.fillRect(x_shift, 0, tot_w, h, back_brush);

    // draw top and bottom lines
    painter.setPen(QPen(line_brush, sep_sz));
    painter.fillRect(x_shift, 0, tot_w, sep_sz, line_brush);
    painter.fillRect(x_shift, h - sep_sz, tot_w, sep_sz, line_brush);
    // draw intemediate lines
    for(int i = 0; i <= 32; i++)
        painter.fillRect(ith_col_x(i), 0, sep_sz, 2 * sep_sz + hdr_h, line_brush);
    // draw bottom header lines
    painter.fillRect(ith_col_x(0), sep_sz + hdr_h, tot_w, sep_sz, line_brush);
    painter.fillRect(ith_col_x(0), tot_hdr_sz + gap_h, tot_w, sep_sz, line_brush);
    // redraw some lines but wider
    for(int i = 4; i < nr_bits; i += 4)
        painter.fillRect(ith_col_x(i) - sep_sz, 0, 3 * sep_sz, tot_hdr_sz, line_brush);
    // draw numbers in the header
    painter.setPen(palette().brush(QPalette::ButtonText).color());
    for(int i = 0; i < nr_bits; i++)
    {
        QRect r(ith_col_x(i), sep_sz + margin, col_w, txt_h);
        painter.drawText(r, Qt::AlignCenter, QString("%1").arg((nr_bits - 1 - i) / 10));
        r.translate(0, txt_h + txt_sep);
        painter.drawText(r, Qt::AlignCenter, QString("%1").arg((nr_bits - 1 - i) % 10));
    }
    // display content
    if(model())
    {
        for(int i = 0; i < model()->rowCount(); i++)
        {
            QVariant vrange = model()->data(model()->index(i, 0, rootIndex()));
            if(!vrange.canConvert< SocFieldBitRange >())
                continue;
            SocFieldBitRange range = vrange.value< SocFieldBitRange >();
            QString name = model()->data(model()->index(i, 1, rootIndex())).toString();
            QRect r(QPoint(ith_col_x(nr_bits - 1 - range.GetLastBit()) + sep_sz, tot_hdr_sz),
                QPoint(ith_col_x(nr_bits - range.GetFirstBit()), h - sep_sz));
            painter.fillRect(r.x() - sep_sz, r.y(), sep_sz, r.height(), line_brush);
            painter.fillRect(r.right(), r.y(), sep_sz, r.height(), line_brush);
            r.setY(r.y() + gap_h + sep_sz);
            // draw rotated text
            painter.save();
            painter.translate(r.bottomLeft());
            painter.rotate(-90);
            //painter.fillRect(QRect(0, 0, r.height(), r.width()), QBrush(Qt::red));
            QRect r2(0, 0, r.height(), r.width());
            painter.drawText(r2, Qt::AlignCenter, name);
            painter.restore();
        }
    }
#undef ith_col_x
}

void RegSexyDisplay2::RecomputeGeometry()
{
    if(!m_is_dirty)
        return;
    /* height: header + gap + sep + content + sep */
    m_minimum_height = 0;
    m_minimum_height += GetHeaderHeight() + GetGapHeight();
    m_minimum_height += 2 * GetSeparatorSize() + GetMaxContentHeight();
    /* width: sep + (col + sep) * n */
    m_minimum_width = GetSeparatorSize() * 33 + GetColumnWidth() * 32;
    m_is_dirty = false;
    viewport()->update();
}

void RegSexyDisplay2::resizeEvent(QResizeEvent*)
{
    m_is_dirty = true;
    RecomputeGeometry();
    updateGeometries();
}

void RegSexyDisplay2::updateGeometries()
{
    horizontalScrollBar()->setSingleStep(1);
    horizontalScrollBar()->setPageStep(viewport()->width());
    horizontalScrollBar()->setRange(0, qMax(0, m_minimum_width - viewport()->width()));
    verticalScrollBar()->setSingleStep(1);
    verticalScrollBar()->setPageStep(viewport()->height());
    verticalScrollBar()->setRange(0, qMax(0, m_minimum_height - viewport()->height()));
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
    layout->addWidget(m_toolbar, 0);
    layout->addWidget(m_edit, 1);
    setLayout(layout);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_edit->setAcceptRichText(false);
    m_edit->setAutoFormatting(QTextEdit::AutoAll);

    m_bold_button = new QToolButton(this);
    m_bold_button->setIcon(QIcon::fromTheme("format-text-bold"));
    m_bold_button->setText("bold");
    m_bold_button->setCheckable(true);

    m_italic_button = new QToolButton(this);
    m_italic_button->setIcon(QIcon::fromTheme("format-text-italic"));
    m_italic_button->setText("italic");
    m_italic_button->setCheckable(true);

    m_underline_button = new QToolButton(this);
    m_underline_button->setIcon(QIcon::fromTheme("format-text-underline"));
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

    SetGrowingMode(false);
    SetReadOnly(false);
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
        OnTextChanged();
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

/**
 * BackendSelector
 */
BackendSelector::BackendSelector(Backend *backend, QWidget *parent)
    :QWidget(parent), m_backend(backend)
{
    m_data_selector = new QComboBox(this);
    m_data_selector->addItem(QIcon::fromTheme("text-x-generic"), "Nothing...", QVariant(DataSelNothing));
    m_data_selector->addItem(QIcon::fromTheme("document-open"), "File...", QVariant(DataSelFile));
#ifdef HAVE_HWSTUB
    m_data_selector->addItem(QIcon::fromTheme("multimedia-player"), "Device...", QVariant(DataSelDevice));
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
    data_sel_layout->addWidget(m_dev_selector, 1);
#endif

    m_io_backend = m_backend->CreateDummyIoBackend();

    connect(m_data_selector, SIGNAL(activated(int)),
        this, SLOT(OnDataSelChanged(int)));
#ifdef HAVE_HWSTUB
    connect(m_dev_selector, SIGNAL(currentIndexChanged(int)),
        this, SLOT(OnDevChanged(int)));
    connect(&m_hwstub_helper, SIGNAL(OnDevListChanged(bool, struct libusb_device *)),
        this, SLOT(OnDevListChanged2(bool, struct libusb_device *)));
#endif
    OnDataSelChanged(0);
}

BackendSelector::~BackendSelector()
{
#ifdef HAVE_HWSTUB
    ClearDevList();
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
#endif
        QFileDialog *fd = new QFileDialog(m_data_selector);
        fd->setFilter("Textual files (*.txt);;All files (*)");
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
        OnDevListChanged();
    }
#endif
    else
    {
        m_data_sel_edit->hide();
        m_nothing_text->show();
#ifdef HAVE_HWSTUB
        m_dev_selector->hide();
#endif

        ChangeBackend(m_backend->CreateDummyIoBackend());
    }
}

#ifdef HAVE_HWSTUB
void BackendSelector::OnDevListChanged2(bool arrived, struct libusb_device *dev)
{
    Q_UNUSED(arrived);
    Q_UNUSED(dev);
    OnDevListChanged();
}

void BackendSelector::OnDevListChanged()
{
    ClearDevList();
    QList< HWStubDevice* > list = m_hwstub_helper.GetDevList();
    foreach(HWStubDevice *dev, list)
    {
        QString name = QString("Bus %1 Device %2: %3").arg(dev->GetBusNumber())
            .arg(dev->GetDevAddress()).arg(dev->GetTargetInfo().bName);
        m_dev_selector->addItem(QIcon::fromTheme("multimedia-player"), name,
            QVariant::fromValue((void *)dev));
    }
    if(list.size() > 0)
        m_dev_selector->setCurrentIndex(0);
}

void BackendSelector::OnDevChanged(int index)
{
    if(index == -1)
        return;
    HWStubDevice *dev = reinterpret_cast< HWStubDevice* >(m_dev_selector->itemData(index).value< void* >());
    delete m_io_backend;
    /* NOTE: make a copy of the HWStubDevice device because the one in the list
     * might get destroyed when clearing the list while the backend is still
     * active: this would result in a double free when the backend is also destroyed */
    m_io_backend = m_backend->CreateHWStubIoBackend(new HWStubDevice(dev));
    emit OnSelect(m_io_backend);
}

void BackendSelector::ClearDevList()
{
    while(m_dev_selector->count() > 0)
    {
        HWStubDevice *dev = reinterpret_cast< HWStubDevice* >(m_dev_selector->itemData(0).value< void* >());
        delete dev;
        m_dev_selector->removeItem(0);
    }
}
#endif

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
    m_close = new QToolButton(this);
    m_close->setText("close");
    m_close->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
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
