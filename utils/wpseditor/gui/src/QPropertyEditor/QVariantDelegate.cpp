// *************************************************************************************************
//
// QPropertyEditor v 0.1
//
// --------------------------------------
// Copyright (C) 2007 Volker Wiendl
//
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// *************************************************************************************************

#include "QVariantDelegate.h"

#include "Property.h"

#include <Qt/qabstractitemview.h>


QVariantDelegate::QVariantDelegate(QObject* parent) : QItemDelegate(parent) {}


QVariantDelegate::~QVariantDelegate() {}

QWidget *QVariantDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem& option , const QModelIndex & index ) const {
    QWidget* editor = 0;
    Property* p = static_cast<Property*>(index.internalPointer());
    switch (p->value().type()) {
    case QVariant::Color:
    case QVariant::Int:
    case QMetaType::Float:
    case QVariant::Double:
    case QVariant::UserType:
        editor = p->createEditor(parent, option);
        if (editor) break; // if no editor could be created take default case
    default:
        editor = QItemDelegate::createEditor(parent, option, index);
    }
    parseEditorHints(editor, p->editorHints());
    return editor;
}

void QVariantDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
    QVariant data = index.model()->data(index, Qt::EditRole);
    switch (data.type()) {
    case QVariant::Color:
    case QMetaType::Double:
    case QMetaType::Float:
    case QVariant::UserType:
        if (static_cast<Property*>(index.internalPointer())->setEditorData(editor, data)) // if editor couldn't be recognized use default
            break;
    default:
        QItemDelegate::setEditorData(editor, index);
        break;
    }
}

void QVariantDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
    QVariant data = index.model()->data(index, Qt::EditRole);
    switch (data.type()) {
    case QVariant::Color:
    case QMetaType::Double:
    case QMetaType::Float:
    case QVariant::UserType: {
            QVariant data = static_cast<Property*>(index.internalPointer())->editorData(editor);
            if (data.isValid()) {
                model->setData(index, data , Qt::EditRole);
                break;
            }
        }
    default:
        QItemDelegate::setModelData(editor, model, index);
        break;
    }
}

void QVariantDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex& index ) const {
    return QItemDelegate::updateEditorGeometry(editor, option, index);
}

void QVariantDelegate::parseEditorHints(QWidget* editor, const QString& editorHints) const {
    if (editor && !editorHints.isEmpty()) {
        // Parse for property values
        QRegExp rx("(.*)(=\\s*)(.*)(;{1})");
        rx.setMinimal(true);
        int pos = 0;
        while ((pos = rx.indexIn(editorHints, pos)) != -1) {
            qDebug("Setting %s to %s", qPrintable(rx.cap(1)), qPrintable(rx.cap(3)));
            editor->setProperty(qPrintable(rx.cap(1).trimmed()), rx.cap(3).trimmed());
            pos += rx.matchedLength();
        }
    }
}
