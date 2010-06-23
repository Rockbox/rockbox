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

#include "rbimage.h"

#include <iostream>

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

    case CONDITIONAL:
        for(int i = 0; i < element->params_count; i++)
            children.append(new ParseTreeNode(&data->params[i], this));
        for(int i = 0; i < element->children_count; i++)
            children.append(new ParseTreeNode(data->children[i], this));
        break;

    case SUBLINES:
        for(int i = 0; i < element->children_count; i++)
        {
            children.append(new ParseTreeNode(data->children[i], this));
        }
        break;

case VIEWPORT:
        for(int i = 0; i < element->params_count; i++)
            children.append(new ParseTreeNode(&data->params[i], this));
        /* Deliberate fall-through here */

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

ParseTreeNode::~ParseTreeNode()
{
    for(int i = 0; i < children.count(); i++)
        delete children[i];
}

QString ParseTreeNode::genCode() const
{
    QString buffer = "";

    if(element)
    {
        switch(element->type)
        {
        case UNKNOWN:
            break;
        case VIEWPORT:
            /* Generating the Viewport tag, if necessary */
            if(element->tag)
            {
                buffer.append(TAGSYM);
                buffer.append(element->tag->name);
                buffer.append(ARGLISTOPENSYM);
                for(int i = 0; i < element->params_count; i++)
                {
                    buffer.append(children[i]->genCode());
                    if(i != element->params_count - 1)
                        buffer.append(ARGLISTSEPERATESYM);
                }
                buffer.append(ARGLISTCLOSESYM);
                buffer.append('\n');
            }

            for(int i = element->params_count; i < children.count(); i++)
                buffer.append(children[i]->genCode());
            break;

        case LINE:
            for(int i = 0; i < children.count(); i++)
            {
                buffer.append(children[i]->genCode());
            }
            if(openConditionals == 0
               && !(parent && parent->element->type == SUBLINES))
            {
                buffer.append('\n');
            }
            break;

        case SUBLINES:
            for(int i = 0; i < children.count(); i++)
            {
                buffer.append(children[i]->genCode());
                if(i != children.count() - 1)
                    buffer.append(MULTILINESYM);
            }
            if(openConditionals == 0)
                buffer.append('\n');
            break;

        case CONDITIONAL:
            openConditionals++;

            /* Inserting the tag part */
            buffer.append(TAGSYM);
            buffer.append(CONDITIONSYM);
            buffer.append(element->tag->name);
            if(element->params_count > 0)
            {
                buffer.append(ARGLISTOPENSYM);
                for(int i = 0; i < element->params_count; i++)
                {
                    buffer.append(children[i]->genCode());
                    if( i != element->params_count - 1)
                        buffer.append(ARGLISTSEPERATESYM);
                    buffer.append(ARGLISTCLOSESYM);
                }
            }

            /* Inserting the sublines */
            buffer.append(ENUMLISTOPENSYM);
            for(int i = element->params_count; i < children.count(); i++)
            {
                buffer.append(children[i]->genCode());
                if(i != children.count() - 1)
                    buffer.append(ENUMLISTSEPERATESYM);
            }
            buffer.append(ENUMLISTCLOSESYM);
            openConditionals--;
            break;

        case TAG:
            buffer.append(TAGSYM);
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
            for(char* cursor = (char*)element->data; *cursor; cursor++)
            {
                if(find_escape_character(*cursor))
                    buffer.append(TAGSYM);
                buffer.append(*cursor);
            }
            break;

        case COMMENT:
            buffer.append(COMMENTSYM);
            buffer.append((char*)element->data);
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
    char *text;

    if(element)
    {
        hash += element->type;
        switch(element->type)
        {
        case UNKNOWN:
            break;
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
            text = (char*)element->data;
            for(unsigned int i = 0; i < strlen(text); i++)
            {
                if(i % 2)
                    hash += text[i] % element->type;
                else
                    hash += text[i] % element->type * 2;
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
            case UNKNOWN:
                return QObject::tr("Unknown");
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
            case UNKNOWN:
            case VIEWPORT:
            case LINE:
            case SUBLINES:
                return QString();

            case CONDITIONAL:
                return QString(element->tag->name);

            case TEXT:
            case COMMENT:
                return QString((char*)element->data);

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

/* This version is called for the root node and for viewports */
void ParseTreeNode::render(const RBRenderInfo& info)
{
    /* Parameters don't get rendered */
    if(!element && param)
        return;

    /* If we're at the root, we need to render each viewport */
    if(!element && !param)
    {
        for(int i = 0; i < children.count(); i++)
        {
            children[i]->render(info);
        }

        return;
    }

    if(element->type != VIEWPORT)
    {
        std::cerr << QObject::tr("Error in parse tree").toStdString()
                << std::endl;
        return;
    }

    rendered = new RBViewport(element, info);

    for(int i = element->params_count; i < children.count(); i++)
        children[i]->render(info, dynamic_cast<RBViewport*>(rendered));

}

/* This version is called for logical lines and such */
void ParseTreeNode::render(const RBRenderInfo &info, RBViewport* viewport)
{
    if(element->type == LINE)
    {
        for(int i = 0; i < children.count(); i++)
            children[i]->render(info, viewport);
        viewport->newline();
    }
    else if(element->type == TAG)
    {
        QString filename;
        QString id;
        int x, y, tiles, tile;
        char c;
        RBImage* image;

        /* Two switch statements to narrow down the tag name */
        switch(element->tag->name[0])
        {

        case 'x':
            switch(element->tag->name[1])
            {
            case 'd':
                /* %xd */
                id = "";
                id.append(element->params[0].data.text[0]);
                c = element->params[0].data.text[1];

                if(c == '\0')
                {
                    tile = 1;
                }
                else
                {
                    if(isupper(c))
                        tile = c - 'A' + 25;
                    else
                        tile = c - 'a';
                }

                image = info.screen()->getImage(id);
                if(image)
                {
                    image->setTile(tile);
                    image->show();
                }
                break;

            case 'l':
                /* %xl */
                id = element->params[0].data.text;
                filename = info.settings()->value("imagepath", "") + "/" +
                           element->params[1].data.text;
                x = element->params[2].data.numeric;
                y = element->params[3].data.numeric;
                if(element->params_count > 4)
                    tiles = element->params[4].data.numeric;
                else
                    tiles = 1;

                info.screen()->loadImage(id, new RBImage(filename, tiles, x, y,
                                                         viewport));
                break;

            case '\0':
                /* %x */
                id = element->params[0].data.text;
                filename = info.settings()->value("imagepath", "") + "/" +
                           element->params[1].data.text;
                x = element->params[2].data.numeric;
                y = element->params[3].data.numeric;
                image = new RBImage(filename, 1, x, y, viewport);
                info.screen()->loadImage(id, new RBImage(filename, 1, x, y,
                                                         viewport));
                info.screen()->getImage(id)->show();
                break;

            }

            break;

        case 'X':

            switch(element->tag->name[1])
            {
            case '\0':
                /* %X */
                filename = QString(element->params[0].data.text);
                info.screen()->setBackdrop(filename);
                break;
            }

            break;

        }
    }
}
