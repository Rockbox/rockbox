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
#include "analyser.h"

Analyser::Analyser(const soc_desc::soc_ref_t& soc, IoBackend *backend)
    :m_soc(soc), m_io_backend(backend)
{
}

Analyser::~Analyser()
{
}

AnalyserFactory::AnalyserFactory(bool _register)
{
    if(_register)
        RegisterAnalyser(this);
}

AnalyserFactory::~AnalyserFactory()
{
}

QVector< AnalyserFactory * > AnalyserFactory::m_factories;

QStringList AnalyserFactory::GetAnalysersForSoc(const QString& soc_name)
{
    QStringList list;
    for(int i = 0; i < m_factories.size(); i++)
        if(m_factories[i]->SupportSoc(soc_name))
            list.append(m_factories[i]->GetName());
    return list;
}

AnalyserFactory *AnalyserFactory::GetAnalyserByName(const QString& name)
{
    for(int i = 0; i < m_factories.size(); i++)
        if(m_factories[i]->GetName() == name)
            return m_factories[i];
    return 0;
}

void AnalyserFactory::RegisterAnalyser(AnalyserFactory *factory)
{
    m_factories.append(factory);
}
