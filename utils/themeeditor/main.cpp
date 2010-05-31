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

#include "skin_parser.h"
#include "skin_debug.h"

#include <cstdlib>
#include <cstdio>
#include <iostream>

#include <QtGui/QApplication>
#include <QTreeView>

#include "parsetreemodel.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    char doc[] = "#Comment\n%Vd(U);Hey\n%?bl(test,3,5,2,1)<param2|param3>";

    ParseTreeModel tree(doc);

    QTreeView view;
    view.setModel(&tree);
    view.show();

    return app.exec();

    /*
    struct skin_element* test = skin_parse(doc);

    ParseTreeModel tree(doc);
    std::cout << "----" << std::endl;
    if(std::string(doc) == tree.genCode().toStdString())
        std::cout << "Code in/out matches" << std::endl;
    else
        std::cout << "Match error" << std::endl;


    skin_free_tree(test);
    */

}

