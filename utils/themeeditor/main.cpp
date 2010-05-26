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

namespace wps
{
    extern "C"
    {
#include "skin_parser.h"
#include "skin_debug.h"
    }
}

#include <cstdlib>
#include <cstdio>

#include <QtGui/QApplication>
#include <QFileSystemModel>
#include <QTreeView>

int main(int argc, char* argv[])
{

    char* doc = "%Vd(U)\n\n%?bl(test,3,5,2,1)<param2|param3>";

    struct wps::skin_element* test = wps::skin_parse(doc);

    wps::skin_debug_tree(test);

    wps::skin_free_tree(test);


    return 0;
}

