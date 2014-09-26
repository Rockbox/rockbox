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
#include "regdiff.h"

/**
 * RegTab
 */

RegDiff::RegDiff(Backend *backend, QWidget *parent)
    :QSplitter(parent), m_backend(backend)
{
    m_reg_tree = new QTreeWidget();
    m_reg_tree->setColumnCount(1);
    m_reg_tree->setHeaderLabel(QString("Name"));
    this->addWidget(m_reg_tree);
    this->setStretchFactor(0, 1);

    m_right_panel = new QVBoxLayout;
    for(int i = 0; i < 2; i++)
    {
        QGroupBox *data_sel_group = new QGroupBox(QString("Data selection #%1").arg(i + 1));
        QHBoxLayout *data_sel_layout = new QHBoxLayout;
        m_backend_selector[i] = new BackendSelector(m_backend, this);
        m_data_soc_label[i] = new QLabel;
        m_data_sel_reload[i] = new QPushButton(this);
        m_data_sel_reload[i]->setIcon(QIcon::fromTheme("view-refresh"));
        m_data_sel_reload[i]->setToolTip("Reload data");
        data_sel_layout->addWidget(m_backend_selector[i]);
        data_sel_layout->addWidget(m_data_soc_label[i]);
        data_sel_layout->addWidget(m_data_sel_reload[i]);
        data_sel_group->setLayout(data_sel_layout);
        m_data_soc_label[i]->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

        m_right_panel->addWidget(data_sel_group, 0);
        m_io_backend[i] = m_backend_selector[i]->GetBackend();
    }
    m_right_content = 0;
    //SetPanel(new EmptyRegTabPanel);
    QWidget *w = new QWidget;
    w->setLayout(m_right_panel);
    this->addWidget(w);
    this->setStretchFactor(1, 2);

    SetTabName("Register Diff");
}

QWidget *RegDiff::GetWidget()
{
    return this;
}

RegDiff::~RegDiff()
{
    /* backend will be deleted by backend selector */
}

bool RegDiff::Quit()
{
    return true;
}
