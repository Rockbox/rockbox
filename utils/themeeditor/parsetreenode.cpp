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

#include "symbols.h"
#include "tag_table.h"

#include "parsetreenode.h"
#include "parsetreemodel.h"

int ParseTreeNode::openConditionals = 0;

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

    case VIEWPORT:
    case LINE:
        for(int i = 0; i < data->children_count; i++)
        {
            for(struct skin_element* current = data->children[i]; current;
                current = current->next)
            {
                children.append(new ParseTreeNode(current, this));
            }
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

        case VIEWPORT:
            if(children[0]->element->type == TAG)
                buffer.append(TAGSYM);
            buffer.append(children[0]->genCode());
            if(children[0]->element->type == TAG)
                buffer.append('\n');
            for(int i = 1; i < children.count(); i++)
                buffer.append(children[i]->genCode());
            break;

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
            if(openConditionals == 0)
                buffer.append('\n');
            break;

        case SUBLINES:
            for(int i = 0; i < children.count(); i++)
            {
                buffer.append(children[i]->genCode());
                if(i != children.count() - 1)
                    buffer.append(MULTILINESYM);
            }
            buffer.append('\n');
            break;

        case CONDITIONAL:
            openConditionals++;
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
            openConditionals--;
            break;

        case TAG:
            /* When generating code, we DO NOT insert the leading TAGSYM, leave
             * the calling functions to handle that
             */
            buffer.append(element->tag->name);

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

        case TEXT:
            for(char* cursor = element->text; *cursor; cursor++)
            {
                if(find_escape_character(*cursor))
                    buffer.append(TAGSYM);
                buffer.append(*cursor);
            }
            break;

        case COMMENT:
            buffer.append(COMMENTSYM);
            buffer.append(element->text);
            buffer.append('\n');
            break;
        }
    }
    else if(param)
    {
        switch(param->type)
        {
        case skin_tag_parameter::STRING:
            for(char* cursor = param->data.text; *cursor; cursor++)
            {
                if(find_escape_character(*cursor))
                    buffer.append(TAGSYM);
                buffer.append(*cursor);
            }
            break;

        case skin_tag_parameter::NUMERIC:
            buffer.append(QString::number(param->data.numeric, 10));
            break;

        case skin_tag_parameter::DEFAULT:
            buffer.append(DEFAULTSYM);
            break;

        case skin_tag_parameter::CODE:
            buffer.append(QObject::tr("This doesn't belong here"));
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

/* A more or less random hashing algorithm */
int ParseTreeNode::genHash() const
{
    int hash = 0;

    if(element)
    {
        hash += element->type;
        switch(element->type)
        {
        case VIEWPORT:
        case LINE:
        case SUBLINES:
        case CONDITIONAL:
            hash += element->children_count;
            break;

        case TAG:
            for(unsigned int i = 0; i < strlen(element->tag->name); i++)
                hash += element->tag->name[i];
            break;

        case COMMENT:
        case TEXT:
            for(unsigned int i = 0; i < strlen(element->text); i++)
            {
                if(i % 2)
                    hash += element->text[i] % element->type;
                else
                    hash += element->text[i] % element->type * 2;
            }
            break;
        }

    }

    if(param)
    {
        hash += param->type;
        switch(param->type)
        {
        case skin_tag_parameter::DEFAULT:
        case skin_tag_parameter::CODE:
            break;

        case skin_tag_parameter::NUMERIC:
            hash += param->data.numeric * (param->data.numeric / 4);
            break;

        case skin_tag_parameter::STRING:
            for(unsigned int i = 0; i < strlen(param->data.text); i++)
            {
                if(i % 2)
                    hash += param->data.text[i] * 2;
                else
                    hash += param->data.text[i];
            }
            break;
        }
    }

    for(int i = 0; i < children.count(); i++)
    {
        hash += children[i]->genHash();
    }

    return hash;
}

ParseTreeNode* ParseTreeNode::child(int row)
{
    if(row < 0 || row >= children.count())
        return 0;

    return children[row];
}

int ParseTreeNode::numChildren() const
{
        return children.count();
}


QVariant ParseTreeNode::data(int column) const
{
    switch(column)
    {
    case ParseTreeModel::typeColumn:
        if(element)
        {
            switch(element->type)
            {
            case VIEWPORT:
                return QObject::tr("Viewport");

            case LINE:
                return QObject::tr("Logical Line");

            case SUBLINES:
                return QObject::tr("Alternator");

            case COMMENT:
                return QObject::tr("Comment");

            case CONDITIONAL:
                return QObject::tr("Conditional Tag");

            case TAG:
                return QObject::tr("Tag");

            case TEXT:
                return QObject::tr("Plaintext");
            }
        }
        else if(param)
        {
            switch(param->type)
            {
            case skin_tag_parameter::STRING:
                return QObject::tr("String");

            case skin_tag_parameter::NUMERIC:
                return QObject::tr("Number");

            case skin_tag_parameter::DEFAULT:
                return QObject::tr("Default Argument");

            case skin_tag_parameter::CODE:
                return QObject::tr("This doesn't belong here");
            }
        }
        else
        {
            return QObject::tr("Root");
        }

        break;

    case ParseTreeModel::valueColumn:
        if(element)
        {
            switch(element->type)
            {
            case VIEWPORT:
            case LINE:
            case SUBLINES:
            case CONDITIONAL:
                return QString();

            case TEXT:
            case COMMENT:
                return QString(element->text);

            case TAG:
                return QString(element->tag->name);
            }
        }
        else if(param)
        {
            switch(param->type)
            {
            case skin_tag_parameter::DEFAULT:
                return QObject::tr("-");

            case skin_tag_parameter::STRING:
                return QString(param->data.text);

            case skin_tag_parameter::NUMERIC:
                return QString::number(param->data.numeric, 10);

            case skin_tag_parameter::CODE:
                return QObject::tr("Seriously, something's wrong here");
            }
        }
        else
        {
            return QString();
        }
        break;

    case ParseTreeModel::lineColumn:
        if(element)
            return QString::number(element->line, 10);
        else
            return QString();
        break;
    }

    return QVariant();
}


int ParseTreeNode::getRow() const
{
    if(!parent)
        return -1;

    return parent->children.indexOf(const_cast<ParseTreeNode*>(this));
}

ParseTreeNode* ParseTreeNode::getParent() const
{
    return parent;
}

ParseTreeNode::~ParseTreeNode()
{
    for(int i = 0; i < children.count(); i++)
        delete children[i];
}
