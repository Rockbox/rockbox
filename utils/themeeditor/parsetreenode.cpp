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

extern "C"
{
#include "symbols.h"
}

#include "parsetreenode.h"

/* Root element constructor */
ParseTreeNode::ParseTreeNode(struct skin_element* data)
    : parent(0), element(0), param(0), children()
{
    while(data)
    {
        children.append(new ParseTreeNode(data, this));
        data = data->next;
    }
}

/* Normal element constructor */
ParseTreeNode::ParseTreeNode(struct skin_element* data, ParseTreeNode* parent)
    : parent(parent), element(data), param(0), children()
{
    switch(element->type)
    {

    case TAG:
        for(int i = 0; i < element->params_count; i++)
        {
            if(element->params[i].type == skin_tag_parameter::CODE)
                children.append(new ParseTreeNode(element->params[i].data.code,
                                              this));
            else
                children.append(new ParseTreeNode(&element->params[i], this));
        }
        break;

        /* CONDITIONAL and SUBLINES fall through to the same code */
    case CONDITIONAL:
    case SUBLINES:
        for(int i = 0; i < element->children_count; i++)
        {
            children.append(new ParseTreeNode(data->children[i], this));
        }
        break;

    case LINE:
        for(struct skin_element* current = data->children[0]; current;
            current = current->next)
        {
            children.append(new ParseTreeNode(current, this));
        }
        break;

    default:
        break;
    }
}

/* Parameter constructor */
ParseTreeNode::ParseTreeNode(skin_tag_parameter *data, ParseTreeNode *parent)
    : parent(parent), element(0), param(data), children()
{

}

QString ParseTreeNode::genCode() const
{
    QString buffer = "";

    if(element)
    {
        switch(element->type)
        {
        case LINE:
            for(int i = 0; i < children.count(); i++)
            {
                /*
               Adding a % in case of tag, because the tag rendering code
               doesn't insert its own
            */
                if(children[i]->element->type == TAG)
                    buffer.append(TAGSYM);
                buffer.append(children[i]->genCode());
            }
            break;

        case SUBLINES:
            for(int i = 0; i < children.count(); i++)
            {
                buffer.append(children[i]->genCode());
                if(i != children.count() - 1)
                    buffer.append(MULTILINESYM);
            }
            break;

        case CONDITIONAL:
            /* Inserts a %?, the tag renderer doesn't deal with the TAGSYM */
            buffer.append(TAGSYM);
            buffer.append(CONDITIONSYM);
            buffer.append(children[0]->genCode());

            /* Inserting the sublines */
            buffer.append(ENUMLISTOPENSYM);
            for(int i = 1; i < children.count(); i++)
            {
                buffer.append(children[i]->genCode());
                if(i != children.count() - 1)
                    buffer.append(ENUMLISTSEPERATESYM);
            }
            buffer.append(ENUMLISTCLOSESYM);
            break;

        case TAG:
            /* When generating code, we DO NOT insert the leading TAGSYM, leave
             * the calling functions to handle that
             */
            buffer.append(element->name);

            if(element->params_count > 0)
            {
                /* Rendering parameters if there are any */
                buffer.append(ARGLISTOPENSYM);
                for(int i = 0; i < children.count(); i++)
                {
                    buffer.append(children[i]->genCode());
                    if(i != children.count() - 1)
                        buffer.append(ARGLISTSEPERATESYM);
                }
                buffer.append(ARGLISTCLOSESYM);
            }
            break;

        case NEWLINE:
            buffer.append('\n');
            break;

        case TEXT:
            buffer.append(element->text);
            break;

        case COMMENT:
            buffer.append(COMMENTSYM);
            buffer.append(element->text);
            break;
        }
    }
    else if(param)
    {
        switch(param->type)
        {
        case skin_tag_parameter::STRING:
            buffer.append(param->data.text);
            break;

        case skin_tag_parameter::NUMERIC:
            buffer.append(QString::number(param->data.numeric, 10));
            break;

        case skin_tag_parameter::DEFAULT:
            buffer.append(DEFAULTSYM);
            break;

        }
    }
    else
    {
        for(int i = 0; i < children.count(); i++)
            buffer.append(children[i]->genCode());
    }

    return buffer;
}
/*
ParseTreeNode* child(int row);
int numChildren() const;
QVariant data(int column) const;
int getRow() const;
ParseTreeNode* getParent();
*/
