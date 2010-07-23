/*
 * Copyright (C) 2009  Lorenzo Bettini <http://www.lorenzobettini.it>
 * See COPYING file that comes with this distribution
 */

#include <QtGui>
#include <QTextEdit>
#include <QRegExp>

#include "findform.h"
#include "ui_findreplaceform.h"

FindForm::FindForm(QWidget *parent) :
    FindReplaceForm(parent)
{
    hideReplaceWidgets();
}

FindForm::~FindForm()
{

}

void FindForm::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void FindForm::writeSettings(QSettings &settings, const QString &prefix) {
    FindReplaceForm::writeSettings(settings, prefix);
}

void FindForm::readSettings(QSettings &settings, const QString &prefix) {
    FindReplaceForm::readSettings(settings, prefix);
}

