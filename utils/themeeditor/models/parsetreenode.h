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

#ifndef PARSETREENODE_H
#define PARSETREENODE_H

#include "skin_parser.h"
#include "rbviewport.h"
#include "rbscreen.h"
#include "rbrenderinfo.h"

#include <QString>
#include <QVariant>
#include <QList>

class ParseTreeNode
{
public:
    ParseTreeNode(struct skin_element* data, ParseTreeModel* model);
    ParseTreeNode(struct skin_element* data, ParseTreeNode* parent,
                  ParseTreeModel* model);
    ParseTreeNode(struct skin_tag_parameter* data, ParseTreeNode* parent,
                  ParseTreeModel* model);
    virtual ~ParseTreeNode();

    QString genCode() const;
    int genHash() const;

    bool isParam() const{ if(param) return true; else return false; }
    struct skin_tag_parameter* getParam(){ return param;}
    struct skin_element* getElement(){return element;}

    ParseTreeNode* child(int row);
    int numChildren() const;
    QVariant data(int column) const;
    int getRow() const;
    ParseTreeNode* getParent() const;
    ParseTreeNode* getChild(int row) const
    {
        if(row < children.count())
            return children[row];
        else
            return 0;
    }

    void render(const RBRenderInfo& info);
    void render(const RBRenderInfo &info, RBViewport* viewport,
                bool noBreak = false);

    double findBranchTime(ParseTreeNode* branch, const RBRenderInfo& info);
    double findConditionalTime(ParseTreeNode* conditional,
                               const RBRenderInfo& info);

    void modParam(QVariant value, int index = -1);

private:

    bool execTag(const RBRenderInfo& info, RBViewport* viewport);
    QVariant evalTag(const RBRenderInfo& info, bool conditional = false,
                     int branches = 0);

    ParseTreeNode* parent;
    struct skin_element* element;
    struct skin_tag_parameter* param;
    QList<ParseTreeNode*> children;

    static int openConditionals;
    static bool breakFlag;
    QGraphicsItem* rendered;

    ParseTreeModel* model;

    QList<int> extraParams;

};

#endif // PARSETREENODE_H
