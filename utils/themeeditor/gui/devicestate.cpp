/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Robert Bieber
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

#include "devicestate.h"

#include <QScrollArea>
#include <QFile>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>


DeviceState::DeviceState(QWidget *parent) :
    QWidget(parent), tabs(this)
{
    /* UI stuff */
    resize(500,400);
    setWindowIcon(QIcon(":/resources/windowicon.png"));
    setWindowTitle(tr("Device Settings"));

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(&tabs);
    this->setLayout(layout);

    /* Loading the tabs */
    QScrollArea* currentArea = 0;
    QHBoxLayout* subLayout;
    QWidget* panel;

    QFile fin(":/resources/deviceoptions");
    fin.open(QFile::Text | QFile::ReadOnly);
    while(!fin.atEnd())
    {
        QString line = QString(fin.readLine());
        line = line.trimmed();

        /* Continue on a comment or an empty line */
        if(line[0] == '#' || line.length() == 0)
            continue;

        if(line[0] == '[')
        {
            QString buffer;
            for(int i = 1; line[i] != ']'; i++)
                buffer.append(line[i]);
            buffer = buffer.trimmed();

            panel = new QWidget();
            currentArea = new QScrollArea();
            layout = new QVBoxLayout(panel);
            currentArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            currentArea->setWidget(panel);
            currentArea->setWidgetResizable(true);

            tabs.addTab(currentArea, buffer);

            continue;
        }

        QStringList elements = line.split(";");
        QString tag = elements[0].trimmed();
        QString title = elements[1].trimmed();
        QString type = elements[2].trimmed();
        QString defVal = elements[3].trimmed();

        subLayout = new QHBoxLayout();
        if(type != "check")
            subLayout->addWidget(new QLabel(elements[1].trimmed(),
                                            currentArea));
        layout->addLayout(subLayout);


        elements = type.split("(");
        if(elements[0].trimmed() == "text")
        {
            QLineEdit* temp = new QLineEdit(defVal, currentArea);
            subLayout->addWidget(temp);
            inputs.insert(tag, QPair<InputType, QWidget*>(Text, temp));

            temp->setSizePolicy(QSizePolicy(QSizePolicy::Preferred,
                                            QSizePolicy::Fixed));

            QObject::connect(temp, SIGNAL(textChanged(QString)),
                             this, SLOT(input()));
        }
        else if(elements[0].trimmed() == "check")
        {
            QCheckBox* temp = new QCheckBox(title, currentArea);
            subLayout->addWidget(temp);
            if(defVal.toLower() == "true")
                temp->setChecked(true);
            else
                temp->setChecked(false);
            inputs.insert(tag, QPair<InputType, QWidget*>(Check, temp));

            QObject::connect(temp, SIGNAL(toggled(bool)),
                             this, SLOT(input()));
        }
        else if(elements[0].trimmed() == "slider")
        {
            elements = elements[1].trimmed().split(",");
            int min = elements[0].trimmed().toInt();
            QString maxS = elements[1].trimmed();
            maxS.chop(1);
            int max = maxS.toInt();

            QSlider* temp = new QSlider(Qt::Horizontal, currentArea);
            temp->setMinimum(min);
            temp->setMaximum(max);
            temp->setValue(defVal.toInt());
            subLayout->addWidget(temp);
            inputs.insert(tag, QPair<InputType, QWidget*>(Slide, temp));

            QObject::connect(temp, SIGNAL(valueChanged(int)),
                             this, SLOT(input()));
        }
        else if(elements[0].trimmed() == "spin")
        {
            elements = elements[1].trimmed().split(",");
            int min = elements[0].trimmed().toInt();
            QString maxS = elements[1].trimmed();
            maxS.chop(1);
            int max = maxS.toInt();

            QSpinBox* temp = new QSpinBox(currentArea);
            temp->setMinimum(min);
            temp->setMaximum(max);
            temp->setValue(defVal.toInt());
            subLayout->addWidget(temp);
            inputs.insert(tag, QPair<InputType, QWidget*>(Spin, temp));

            QObject::connect(temp, SIGNAL(valueChanged(int)),
                             this, SLOT(input()));
        }
        else if(elements[0].trimmed() == "fspin")
        {
            elements = elements[1].trimmed().split(",");
            int min = elements[0].trimmed().toDouble();
            QString maxS = elements[1].trimmed();
            maxS.chop(1);
            int max = maxS.toDouble();

            QDoubleSpinBox* temp = new QDoubleSpinBox(currentArea);
            temp->setMinimum(min);
            temp->setMaximum(max);
            temp->setValue(defVal.toDouble());
            temp->setSingleStep(0.1);
            subLayout->addWidget(temp);
            inputs.insert(tag, QPair<InputType, QWidget*>(DSpin, temp));

            QObject::connect(temp, SIGNAL(valueChanged(double)),
                             this, SLOT(input()));
        }
        else if(elements[0].trimmed() == "combo")
        {
            elements = elements[1].trimmed().split(",");

            int defIndex = 0;
            QComboBox* temp = new QComboBox(currentArea);
            for(int i = 0; i < elements.count(); i++)
            {
                QString current = elements[i].trimmed();
                if(i == elements.count() - 1)
                    current.chop(1);
                temp->addItem(current, i);
                if(current == defVal)
                    defIndex = i;
            }
            temp->setCurrentIndex(defIndex);
            subLayout->addWidget(temp);
            inputs.insert(tag, QPair<InputType, QWidget*>(Combo, temp));

            QObject::connect(temp, SIGNAL(currentIndexChanged(int)),
                             this, SLOT(input()));
        }

    }
}

DeviceState::~DeviceState()
{
}

QVariant DeviceState::data(QString tag, int paramCount,
                           skin_tag_parameter *params)
{
    QPair<InputType, QWidget*> found =
            inputs.value(tag, QPair<InputType, QWidget*>(Slide, 0));

    if(found.second == 0 && tag[0] == '?')
        found = inputs.value(tag.right(2), QPair<InputType, QWidget*>(Slide,0));

    if(found.second == 0)
        return QVariant();

    switch(found.first)
    {
    case Text:
        return dynamic_cast<QLineEdit*>(found.second)->text();

    case Slide:
        return dynamic_cast<QSlider*>(found.second)->value();

    case Spin:
        return dynamic_cast<QSpinBox*>(found.second)->value();

    case DSpin:
        return dynamic_cast<QDoubleSpinBox*>(found.second)->value();

    case Combo:
        if(tag[0] == '?')
            return dynamic_cast<QComboBox*>(found.second)->currentIndex();
        else
            return dynamic_cast<QComboBox*>(found.second)->currentText();

    case Check:
        return dynamic_cast<QCheckBox*>(found.second)->isChecked();
    }

    return QVariant();
}

void DeviceState::setData(QString tag, QVariant data)
{
    QPair<InputType, QWidget*> found =
            inputs.value(tag, QPair<InputType, QWidget*>(Slide, 0));

    if(found.second == 0)
        return;

    switch(found.first)
    {
    case Text:
        dynamic_cast<QLineEdit*>(found.second)->setText(data.toString());
        break;

    case Slide:
        dynamic_cast<QSlider*>(found.second)->setValue(data.toInt());
        break;

    case Spin:
        dynamic_cast<QSpinBox*>(found.second)->setValue(data.toInt());
        break;

    case DSpin:
        dynamic_cast<QDoubleSpinBox*>(found.second)->setValue(data.toDouble());
        break;

    case Combo:
        dynamic_cast<QComboBox*>
                (found.second)->
                setCurrentIndex(dynamic_cast<QComboBox*>
                                (found.second)->findText(data.toString()));
        break;

    case Check:
        dynamic_cast<QCheckBox*>(found.second)->setChecked(data.toBool());
        break;
    }

    emit settingsChanged();
}

void DeviceState::input()
{
    emit settingsChanged();
}
