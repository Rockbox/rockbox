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
#include <QFormLayout>
#include <QTime>

#include <cstdlib>

DeviceState::DeviceState(QWidget *parent) :
    QWidget(parent), tabs(this)
{
    /* UI stuff */
    resize(500,400);
    setWindowIcon(QIcon(":/resources/windowicon.png"));
    setWindowTitle(tr("Device Settings"));

    QFormLayout* layout = new QFormLayout(this);
    layout->addWidget(&tabs);
    this->setLayout(layout);

    /* Loading the tabs */
    QScrollArea* currentArea = 0;
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
            layout = new QFormLayout(panel);
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

        elements = type.split("(");
        if(elements[0].trimmed() == "text")
        {
            QLineEdit* temp = new QLineEdit(defVal, currentArea);
            layout->addRow(title, temp);
            inputs.insert(tag, QPair<InputType, QWidget*>(Text, temp));

            temp->setSizePolicy(QSizePolicy(QSizePolicy::Preferred,
                                            QSizePolicy::Fixed));

            QObject::connect(temp, SIGNAL(textChanged(QString)),
                             this, SLOT(input()));
        }
        else if(elements[0].trimmed() == "check")
        {
            QCheckBox* temp = new QCheckBox(title, currentArea);
            layout->addRow(temp);
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
            layout->addRow(title, temp);
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
            layout->addRow(title, temp);
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
            layout->addRow(title, temp);
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
            layout->addRow(title, temp);
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
    /* Handling special cases */
    if(tag.toLower() == "fm")
    {
        QString path = tag[0].isLower()
                       ? data("file").toString() : data("nextfile").toString();
        return fileName(path, true);
    }
    else if(tag.toLower() == "fn")
    {
        QString path = tag[0].isLower()
                       ? data("file").toString() : data("nextfile").toString();
        return fileName(path, false);
    }
    else if(tag.toLower() == "fp")
    {
        if(tag[0].isLower())
            return data("file").toString();
        else
            return data("nextfile").toString();
    }
    else if(tag.toLower() == "d")
    {
        QString path = tag[0].isLower()
                       ? data("file").toString() : data("nextfile").toString();
        if(paramCount > 0)
            return directory(path, params[0].data.number);
        else
            return QVariant();
    }
    else if(tag == "pc")
    {
        int secs = data("?pc").toInt();
        return secsToString(secs);
    }
    else if(tag == "pr")
    {
        int secs = data("?pt").toInt() - data("?pc").toInt();
        if(secs < 0)
            secs = 0;
        return secsToString(secs);
    }
    else if(tag == "pt")
    {
        int secs = data("?pt").toInt();
        return secsToString(secs);
    }
    else if(tag == "px")
    {
        int totalTime = data("?pt").toInt();
        int currentTime = data("?pc").toInt();
        return currentTime * 100 / totalTime;
    }
    else if(tag == "pS")
    {
        double threshhold = paramCount > 0
                            ? params[0].data.number / 10. : 10;
        if(data("?pc").toDouble() <= threshhold)
            return true;
        else
            return false;
    }
    else if(tag == "pE")
    {
        double threshhold = paramCount > 0
                            ? params[0].data.number / 10. : 10;
        if(data("?pt").toDouble() - data("?pc").toDouble() <= threshhold)
            return true;
        else
            return false;
    }
    else if(tag == "ce")
    {
        return data("month");
    }
    else if(tag == "cH")
    {
        int hour = data("hour").toInt();
        if(hour < 10)
            return "0" + QString::number(hour);
        else
            return hour;
    }
    else if(tag == "cK")
    {
        return data("hour");
    }
    else if(tag == "cI")
    {
        int hour = data("hour").toInt();
        if(hour > 12)
            hour -= 12;
        if(hour == 0)
            hour = 12;

        if(hour < 10)
            return "0" + QString::number(hour);
        else
            return hour;
    }
    else if(tag == "cl")
    {
        int hour = data("hour").toInt();
        if(hour > 12)
            hour -= 12;
        if(hour == 0)
            hour = 12;

        return hour;
    }
    else if(tag == "cm")
    {
        int month = data("?cm").toInt() + 1;
        if(month < 10)
            return "0" + QString::number(month);
        else
            return month;
    }
    else if(tag == "cd")
    {
        int day = data("day").toInt();
        if(day < 10)
            return "0" + QString::number(day);
        else
            return day;
    }
    else if(tag == "cM")
    {
        int minute = data("minute").toInt();
        if(minute < 10)
            return "0" + QString::number(minute);
        else
            return minute;
    }
    else if(tag == "cS")
    {
        int second = data("second").toInt();
        if(second < 10)
            return "0" + QString::number(second);
        else
            return second;
    }
    else if(tag == "cy")
    {
        QString year = data("cY").toString();
        return year.right(2);
    }
    else if(tag == "cP")
    {
        if(data("hour").toInt() >= 12)
            return "PM";
        else
            return "AM";
    }
    else if(tag == "cp")
    {
        if(data("hour").toInt() >= 12)
            return "pm";
        else
            return "am";
    }
    else if(tag == "ca")
    {
        switch(data("cw").toInt())
        {
        case 0: return "Sun";
        case 1: return "Mon";
        case 2: return "Tue";
        case 3: return "Wed";
        case 4: return "Thu";
        case 5: return "Fri";
        case 6: return "Sat";
        default: return tr("Error, invalid weekday number");
        }
    }
    else if(tag == "cb")
    {
        int month = data("cm").toInt();
        switch(month)
        {
        case 1: return "Jan";
        case 2: return "Feb";
        case 3: return "Mar";
        case 4: return "Apr";
        case 5: return "May";
        case 6: return "Jun";
        case 7: return "Jul";
        case 8: return "Aug";
        case 9: return "Sep";
        case 10: return "Oct";
        case 11: return "Nov";
        case 12: return "Dec";
        }
    }
    else if(tag == "cu")
    {
        int day = data("?cw").toInt();
        if(day == 0)
            day = 7;
        return day;
    }
    else if(tag == "?cu")
    {
        int day = data("?cw").toInt() - 1;
        if(day == -1)
            day = 6;
        return day;
    }
    else if(tag == "cw")
    {
        return data("?cw");
    }
    else if(tag == "cs")
    {
        int seconds = data("seconds").toInt();
        if(seconds < 10)
            return "0" + QString::number(seconds);
        else
            return seconds;
    }

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
        if(data.type() == QVariant::String)
            dynamic_cast<QComboBox*>
                    (found.second)->
                    setCurrentIndex(dynamic_cast<QComboBox*>
                                    (found.second)->findText(data.toString()));
        else
            dynamic_cast<QComboBox*>(found.second)->
                    setCurrentIndex(data.toInt());
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

QString DeviceState::fileName(QString path, bool extension)
{
    path = path.split("/").last();
    if(!extension)
    {
        QString sum;
        QStringList name = path.split(".");
        for(int i = 0; i < name.count() - 1; i++)
            sum.append(name[i]);
        return sum;
    }
    else
    {
        return path;
    }
}

QString DeviceState::directory(QString path, int level)
{
    QStringList dirs = path.split("/");
    int index = dirs.count() - 1 - level;
    if(index < 0)
        index = 0;
    return dirs[index];
}

QString DeviceState::secsToString(int secs)
{
    int hours = 0;
    int minutes = 0;
    while(secs >= 60)
    {
        minutes++;
        secs -= 60;
    }

    while(minutes >= 60)
    {
        hours++;
        minutes -= 60;
    }

    QString retval;

    if(hours > 0)
    {
        retval += QString::number(hours);
        if(minutes < 10)
            retval += ":0";
        else
            retval += ":";
    }

    retval += QString::number(minutes);
    if(secs < 10)
        retval += ":0";
    else
        retval += ":";

    retval += QString::number(secs);
    return retval;
}
