/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: encoders.h 17902 2008-06-30 22:09:45Z bluebrother $
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

#include <QSpacerItem>
#include <QPushButton>
#include <QHBoxLayout>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QFileDialog>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QProgressDialog>
#include "encttscfggui.h"
#include "Logger.h"

EncTtsCfgGui::EncTtsCfgGui(QDialog* parent, EncTtsSettingInterface* iface, QString name)
        : QDialog(parent)
{
    m_settingInterface = iface;

    m_busyCnt=0;
    // create a busy Dialog
    m_busyDlg= new QProgressDialog("", "", 0, 0,this);
    m_busyDlg->setWindowTitle(tr("Waiting for engine..."));
    m_busyDlg->setModal(true);
    m_busyDlg->setLabel(nullptr);
    m_busyDlg->setCancelButton(nullptr);
    m_busyDlg->hide();
    connect(iface,SIGNAL(busy()),this,SLOT(showBusy()));
    connect(iface,SIGNAL(busyEnd()),this,SLOT(hideBusy()));

    //setup the window
    setWindowTitle(name);
    setUpWindow();
}

void EncTtsCfgGui::setUpWindow()
{
    m_settingsList = m_settingInterface->getSettings();

    // layout
    QVBoxLayout *mainLayout = new QVBoxLayout;

    // groupbox
    QGroupBox *groupBox = new QGroupBox(this);
    QGridLayout *gridLayout = new QGridLayout(groupBox);
    // setting widgets
    for(int i = 0; i < m_settingsList.size(); i++)
    {
        QLabel *label = new QLabel(m_settingsList.at(i)->name());
        gridLayout->addWidget(label, i, 0);
        QWidget *widget = createWidgets(m_settingsList.at(i));
        gridLayout->addWidget(widget, i, 1);
        widget->setLayoutDirection(Qt::LeftToRight);
        QWidget *btn = createButton(m_settingsList.at(i));
        if(btn != nullptr)
        {
            gridLayout->addWidget(btn, i, 2);
        }
    }
    // add hidden spacers to make the dialog scale properly
    QSpacerItem* spacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    gridLayout->addItem(spacer, m_settingsList.size(), 0);
    spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    gridLayout->addItem(spacer, m_settingsList.size(), 1);

    groupBox->setLayout(gridLayout);
    mainLayout->addWidget(groupBox);

    // connect browse btn
    connect(&m_browseBtnMap,SIGNAL(mapped(QObject*)),this,SLOT(browse(QObject*)));

    // ok - cancel buttons
    QPushButton* okBtn = new QPushButton(tr("Ok"),this);
    okBtn->setDefault(true);
    okBtn->setIcon(QIcon(":icons/go-next.svg"));
    QPushButton* cancelBtn = new QPushButton(tr("Cancel"),this);
    cancelBtn->setIcon(QIcon(":icons/process-stop.svg"));
    connect(okBtn,SIGNAL(clicked()),this,SLOT(accept()));
    connect(cancelBtn,SIGNAL(clicked()),this,SLOT(reject()));

    QHBoxLayout *btnbox = new QHBoxLayout;
    btnbox->addWidget(okBtn);
    btnbox->addWidget(cancelBtn);
    btnbox->insertStretch(0,1);

    mainLayout->addLayout(btnbox);

    this->setLayout(mainLayout);
}

QWidget* EncTtsCfgGui::createWidgets(EncTtsSetting* setting)
{
    // value display
    QWidget* value = nullptr;
    switch(setting->type())
    {
        case EncTtsSetting::eDOUBLE:
        {
            QDoubleSpinBox *spinBox = new QDoubleSpinBox(this);
            spinBox->setAccessibleName(setting->name());
            spinBox->setMinimum(setting->min().toDouble());
            spinBox->setMaximum(setting->max().toDouble());
            spinBox->setSingleStep(0.01);
            spinBox->setValue(setting->current().toDouble());
            connect(spinBox,SIGNAL(valueChanged(double)),this,SLOT(updateSetting()));
            value = spinBox;
            break;
        }
        case EncTtsSetting::eINT:
        {
            QSpinBox *spinBox = new QSpinBox(this);
            spinBox->setAccessibleName(setting->name());
            spinBox->setMinimum(setting->min().toInt());
            spinBox->setMaximum(setting->max().toInt());
            spinBox->setValue(setting->current().toInt());
            connect(spinBox,SIGNAL(valueChanged(int)),this,SLOT(updateSetting()));
            value = spinBox;
            break;
        }
        case EncTtsSetting::eSTRING:
        {
            QLineEdit *lineEdit = new QLineEdit(this);
            lineEdit->setAccessibleName(setting->name());
            lineEdit->setText(setting->current().toString());
            connect(lineEdit,SIGNAL(textChanged(QString)),this,SLOT(updateSetting()));
            value = lineEdit;
            break;
        }
        case EncTtsSetting::eREADONLYSTRING:
        {
            value = new QLabel(setting->current().toString(),this);
            break;
        }
        case EncTtsSetting::eSTRINGLIST:
        {
            QComboBox *comboBox = new QComboBox(this);
            comboBox->setAccessibleName(setting->name());
            comboBox->addItems(setting->list());
            int index = comboBox->findText(setting->current().toString());
            comboBox->setCurrentIndex(index);
            connect(comboBox,SIGNAL(currentIndexChanged(QString)),this,SLOT(updateSetting()));
            value = comboBox;
            break;
        }
        case EncTtsSetting::eBOOL:
        {
            QCheckBox *checkbox = new QCheckBox(this);
            checkbox->setAccessibleName(setting->name());
            checkbox->setCheckState(setting->current().toBool() == true ? Qt::Checked : Qt::Unchecked);
            connect(checkbox,SIGNAL(stateChanged(int)),this,SLOT(updateSetting()));
            value = checkbox;
            break;
        }
        default:
        {
            LOG_WARNING() << "Warning: unknown EncTTsSetting type" << setting->type();
            break;
        }
    }

    // remember widget
    if(value != nullptr)
    {
        m_settingsWidgetsMap.insert(setting,value);
        connect(setting,SIGNAL(updateGui()),this,SLOT(updateWidget()));
    }

    return value;
}

QWidget* EncTtsCfgGui::createButton(EncTtsSetting* setting)
{
    if(setting->button() == EncTtsSetting::eBROWSEBTN)
    {
        QPushButton* browsebtn = new QPushButton(tr("Browse"),this);
        browsebtn->setIcon(QIcon(":/icons/system-search.svg"));
        m_browseBtnMap.setMapping(browsebtn,setting);
        connect(browsebtn,SIGNAL(clicked()),&m_browseBtnMap,SLOT(map()));
        return browsebtn;
    }
    else if(setting->button() == EncTtsSetting::eREFRESHBTN)
    {
        QPushButton* refreshbtn = new QPushButton(tr("Refresh"),this);
        refreshbtn->setIcon(QIcon(":/icons/view-refresh.svg"));
        connect(refreshbtn,SIGNAL(clicked()),setting,SIGNAL(refresh()));
        return refreshbtn;
    }
    else
        return nullptr;
}

void EncTtsCfgGui::updateSetting()
{
    //cast and get the sender widget
    QWidget* widget = qobject_cast<QWidget*>(QObject::sender());
    if(widget == nullptr) return;
    // get the corresponding setting
    EncTtsSetting* setting = m_settingsWidgetsMap.key(widget);

    // update widget based on setting type
    switch(setting->type())
    {
        case EncTtsSetting::eDOUBLE:
        {
            setting->setCurrent(((QDoubleSpinBox*)widget)->value(),false);
            break;
        }
        case EncTtsSetting::eINT:
        {
            setting->setCurrent(((QSpinBox*)widget)->value(),false);
            break;
        }
        case EncTtsSetting::eSTRING:
        {
            setting->setCurrent(((QLineEdit*)widget)->text(),false);
            break;
        }
        case EncTtsSetting::eREADONLYSTRING:
        {
             setting->setCurrent(((QLabel*)widget)->text(),false);
            break;
        }
        case EncTtsSetting::eSTRINGLIST:
        {
            setting->setCurrent(((QComboBox*)widget)->currentText(),false);
            break;
        }
        case EncTtsSetting::eBOOL:
        {
            setting->setCurrent(((QCheckBox*)widget)->isChecked(),false);
            break;
        }
        default:
        {
            LOG_WARNING() << "unknown setting type!";
            break;
        }
    }
}

void EncTtsCfgGui::updateWidget()
{
    // get sender setting
    EncTtsSetting* setting = qobject_cast<EncTtsSetting*>(QObject::sender());
    if(setting == nullptr) return;
    // get corresponding widget
    QWidget* widget = m_settingsWidgetsMap.value(setting);

    // update Widget based on setting type
    switch(setting->type())
    {
        case EncTtsSetting::eDOUBLE:
        {
            QDoubleSpinBox* spinbox = (QDoubleSpinBox*) widget;
            spinbox->setMinimum(setting->min().toDouble());
            spinbox->setMaximum(setting->max().toDouble());
            spinbox->blockSignals(true);
            spinbox->setValue(setting->current().toDouble());
            spinbox->blockSignals(false);
            break;
        }
        case EncTtsSetting::eINT:
        {
            QSpinBox* spinbox = (QSpinBox*) widget;
            spinbox->setMinimum(setting->min().toInt());
            spinbox->setMaximum(setting->max().toInt());
            spinbox->blockSignals(true);
            spinbox->setValue(setting->current().toInt());
            spinbox->blockSignals(false);
            break;
        }
        case EncTtsSetting::eSTRING:
        {
            QLineEdit* lineedit = (QLineEdit*) widget;

            lineedit->blockSignals(true);
            lineedit->setText(setting->current().toString());
            lineedit->blockSignals(false);
            break;
        }
        case EncTtsSetting::eREADONLYSTRING:
        {
            QLabel* label = (QLabel*) widget;

            label->blockSignals(true);
            label->setText(setting->current().toString());
            label->blockSignals(false);
            break;
        }
        case EncTtsSetting::eSTRINGLIST:
        {
            QComboBox* combobox = (QComboBox*) widget;

            combobox->blockSignals(true);
            combobox->clear();
            combobox->addItems(setting->list());
            int index = combobox->findText(setting->current().toString());
            combobox->setCurrentIndex(index);
            combobox->blockSignals(false);

            break;
        }
        case EncTtsSetting::eBOOL:
        {
            QCheckBox* checkbox = (QCheckBox*) widget;

            checkbox->blockSignals(true);
            checkbox->setCheckState(setting->current().toBool() == true ? Qt::Checked : Qt::Unchecked);
            checkbox->blockSignals(false);
            break;
        }
        default:
        {
            LOG_WARNING() << "unknown EncTTsSetting";
            break;
        }
    }
}

void EncTtsCfgGui::showBusy()
{
    if(m_busyCnt == 0) m_busyDlg->show();

    m_busyCnt++;
}

void EncTtsCfgGui::hideBusy()
{
    m_busyCnt--;

    if(m_busyCnt == 0) m_busyDlg->hide();
}


void EncTtsCfgGui::accept(void)
{
    m_settingInterface->saveSettings();
    this->done(0);
}

void EncTtsCfgGui::reject(void)
{
    this->done(0);
}

//! takes a QObject because of QsignalMapper
void EncTtsCfgGui::browse(QObject* settingObj)
{
    // cast top setting
    EncTtsSetting* setting= qobject_cast<EncTtsSetting*>(settingObj);
    if(setting == nullptr) return;

    //current path
    QString curPath = setting->current().toString();
    // show file dialog
    QString exe = QFileDialog::getOpenFileName(this, tr("Select executable"), curPath, "*");
    if(!QFileInfo(exe).isExecutable())
        return;
    // set new value, gui will update automatically
    setting->setCurrent(exe);
}

