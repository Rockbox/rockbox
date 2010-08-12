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

#include "rbconsole.h"
#include "ui_rbconsole.h"

RBConsole::RBConsole(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RBConsole)
{
    ui->setupUi(this);
}

RBConsole::~RBConsole()
{
    delete ui;
}

void RBConsole::addWarning(QString warning)
{
    ui->output->appendHtml("<span style = \"color:orange\">" + warning
                           + "</span>");
}
