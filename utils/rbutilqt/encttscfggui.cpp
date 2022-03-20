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
        : QDialog(parent),
          m_settingInterface(iface),
          m_busyCnt(0)
{
    // create a busy Dialog
    m_busyDlg= new QProgressDialog("", "", 0, 1, this);
    m_busyDlg->setWindowTitle(tr("Waiting for engine..."));
    m_busyDlg->setModal(true);
    m_busyDlg->setLabel(nullptr);
    m_busyDlg->setCancelButton(nullptr);
    m_busyDlg->setMinimumDuration(100);
    m_busyDlg->setValue(1);
    connect(iface, &EncTtsSettingInterface::busy, this, &EncTtsCfgGui::busyDialog);

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

    // ok - cancel buttons
    QPushButton* okBtn = new QPushButton(tr("Ok"),this);
    okBtn->setDefault(true);
    okBtn->setIcon(QIcon(":icons/go-next.svg"));
    QPushButton* cancelBtn = new QPushButton(tr("Cancel"),this);
    cancelBtn->setIcon(QIcon(":icons/process-stop.svg"));
    connect(okBtn, &QPushButton::clicked, this, &EncTtsCfgGui::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &EncTtsCfgGui::reject);

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

            value = spinBox;

            connect(spinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                    this, [=](double value) {
                        this->m_settingsWidgetsMap.key(spinBox)->setCurrent(value, false);
                    });
            connect(setting, &EncTtsSetting::updateGui, this,
                    [spinBox, setting]() {
                        spinBox->setMinimum(setting->min().toDouble());
                        spinBox->setMaximum(setting->max().toDouble());
                        spinBox->blockSignals(true);
                        spinBox->setValue(setting->current().toDouble());
                        spinBox->blockSignals(false);
                    });
            break;
        }
        case EncTtsSetting::eINT:
        {
            QSpinBox *spinBox = new QSpinBox(this);
            spinBox->setAccessibleName(setting->name());
            spinBox->setMinimum(setting->min().toInt());
            spinBox->setMaximum(setting->max().toInt());
            spinBox->setValue(setting->current().toInt());
            value = spinBox;
            connect(spinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                    this, [=](int value) {
                        this->m_settingsWidgetsMap.key(spinBox)->setCurrent(value, false);
                    });
            connect(setting, &EncTtsSetting::updateGui, this,
                    [spinBox, setting]() {
                        spinBox->setMinimum(setting->min().toInt());
                        spinBox->setMaximum(setting->max().toInt());
                        spinBox->blockSignals(true);
                        spinBox->setValue(setting->current().toInt());
                        spinBox->blockSignals(false);
                    });
            break;
        }
        case EncTtsSetting::eSTRING:
        {
            QLineEdit *lineEdit = new QLineEdit(this);
            lineEdit->setAccessibleName(setting->name());
            lineEdit->setText(setting->current().toString());
            value = lineEdit;

            connect(lineEdit, &QLineEdit::textChanged,
                    this, [=](QString value) {
                        this->m_settingsWidgetsMap.key(lineEdit)->setCurrent(value, false);
                    });
            connect(setting, &EncTtsSetting::updateGui, this,
                    [lineEdit, setting]() {
                        lineEdit->blockSignals(true);
                        lineEdit->setText(setting->current().toString());
                        lineEdit->blockSignals(false);
                    });
            break;
        }
        case EncTtsSetting::eREADONLYSTRING:
        {
            QLabel *label = new QLabel(setting->current().toString(), this);
            label->setWordWrap(true);
            value = label;
            connect(setting, &EncTtsSetting::updateGui, this,
                    [label, setting]() {
                        label->blockSignals(true);
                        label->setText(setting->current().toString());
                        label->blockSignals(false);
                    });
            break;
        }
        case EncTtsSetting::eSTRINGLIST:
        {
            QComboBox *comboBox = new QComboBox(this);
            comboBox->setAccessibleName(setting->name());
            comboBox->addItems(setting->list());
            int index = comboBox->findText(setting->current().toString());
            comboBox->setCurrentIndex(index);
            connect(comboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                    this, [=](int) {
                        this->m_settingsWidgetsMap.key(comboBox)->setCurrent(comboBox->currentText(), false);
                    });
            value = comboBox;
            connect(setting, &EncTtsSetting::updateGui, this,
                    [comboBox, setting]() {
                        comboBox->blockSignals(true);
                        comboBox->clear();
                        comboBox->addItems(setting->list());
                        comboBox->setCurrentIndex(comboBox->findText(setting->current().toString()));
                        comboBox->blockSignals(false);
                    });

            break;
        }
        case EncTtsSetting::eBOOL:
        {
            QCheckBox *checkbox = new QCheckBox(this);
            checkbox->setAccessibleName(setting->name());
            checkbox->setCheckState(setting->current().toBool() == true
                                    ? Qt::Checked : Qt::Unchecked);
            connect(checkbox, &QCheckBox::stateChanged,
                    this, [=](int value) {
                        this->m_settingsWidgetsMap.key(checkbox)->setCurrent(value, false);
                    });
            value = checkbox;
            connect(setting, &EncTtsSetting::updateGui, this,
                    [checkbox, setting]() {
                        checkbox->blockSignals(true);
                        checkbox->setCheckState(setting->current().toBool() == true
                                                ? Qt::Checked : Qt::Unchecked);
                        checkbox->blockSignals(false);
                    });
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
        m_settingsWidgetsMap.insert(setting, value);
    }

    return value;
}

QWidget* EncTtsCfgGui::createButton(EncTtsSetting* setting)
{
    if(setting->button() == EncTtsSetting::eBROWSEBTN)
    {
        QPushButton* browsebtn = new QPushButton(tr("Browse"),this);
        browsebtn->setIcon(QIcon(":/icons/system-search.svg"));

        connect(browsebtn, &QPushButton::clicked, this,
                [this, setting]()
            {
                QString exe = QFileDialog::getOpenFileName(this, tr("Select executable"),
                                                           setting->current().toString(), "*");
                if(QFileInfo(exe).isExecutable())
                    setting->setCurrent(exe);
            });
        return browsebtn;
    }
    else if(setting->button() == EncTtsSetting::eREFRESHBTN)
    {
        QPushButton* refreshbtn = new QPushButton(tr("Refresh"),this);
        refreshbtn->setIcon(QIcon(":/icons/view-refresh.svg"));
        connect(refreshbtn, &QAbstractButton::clicked, setting, &EncTtsSetting::refresh);
        return refreshbtn;
    }
    else
        return nullptr;
}


void EncTtsCfgGui::busyDialog(bool show)
{
    if(show)
    {
        m_busyDlg->setValue(0);
        m_busyCnt++;
    }
    else
    {
        m_busyDlg->setValue(1);
        if(m_busyCnt > 0)
        {
            m_busyCnt--;
        }
    }
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


