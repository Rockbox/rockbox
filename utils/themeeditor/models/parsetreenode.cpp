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
#include "rbprogressbar.h"
#include "rbtoucharea.h"

#include <iostream>
#include <cmath>
#include <cassert>

#include <QDebug>

int ParseTreeNode::openConditionals = 0;
bool ParseTreeNode::breakFlag = false;

/* Root element constructor */
ParseTreeNode::ParseTreeNode(struct skin_element* data, ParseTreeModel* model)
    : parent(0), element(0), param(0), children(), model(model)
{
    while(data)
    {
        children.append(new ParseTreeNode(data, this, model));
        data = data->next;
    }
}

/* Normal element constructor */
ParseTreeNode::ParseTreeNode(struct skin_element* data, ParseTreeNode* parent,
                             ParseTreeModel* model)
                                 : parent(parent), element(data), param(0),
                                 children(), model(model)
{
    switch(element->type)
    {

    case TAG:
        for(int i = 0; i < element->params_count; i++)
        {
            if(element->params[i].type == skin_tag_parameter::CODE)
                children.append(new ParseTreeNode(element->params[i].data.code,
                                              this, model));
            else
                children.append(new ParseTreeNode(&element->params[i], this,
                                                  model));
        }
        break;

    case CONDITIONAL:
        for(int i = 0; i < element->params_count; i++)
            children.append(new ParseTreeNode(&data->params[i], this, model));
        for(int i = 0; i < element->children_count; i++)
            children.append(new ParseTreeNode(data->children[i], this, model));
        break;

    case LINE_ALTERNATOR:
        for(int i = 0; i < element->children_count; i++)
        {
            children.append(new ParseTreeNode(data->children[i], this, model));
        }
        break;

case VIEWPORT:
        for(int i = 0; i < element->params_count; i++)
            children.append(new ParseTreeNode(&data->params[i], this, model));
        /* Deliberate fall-through here */

    case LINE:
        for(int i = 0; i < data->children_count; i++)
        {
            for(struct skin_element* current = data->children[i]; current;
                current = current->next)
            {
                children.append(new ParseTreeNode(current, this, model));
            }
        }
        break;

    default:
        break;
    }
}

/* Parameter constructor */
ParseTreeNode::ParseTreeNode(skin_tag_parameter *data, ParseTreeNode *parent,
                             ParseTreeModel *model)
                                 : parent(parent), element(0), param(data),
                                 children(), model(model)
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
                        buffer.append(ARGLISTSEPARATESYM);
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
               && !(parent && parent->element->type == LINE_ALTERNATOR)
               && !(children.count() > 0 &&
                    children[children.count() - 1]->getElement()->type
                    == COMMENT))
            {
                buffer.append('\n');
            }
            break;

        case LINE_ALTERNATOR:
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
                        buffer.append(ARGLISTSEPARATESYM);
                    buffer.append(ARGLISTCLOSESYM);
                }
            }

            /* Inserting the sublines */
            buffer.append(ENUMLISTOPENSYM);
            for(int i = element->params_count; i < children.count(); i++)
            {
                buffer.append(children[i]->genCode());
                if(i != children.count() - 1)
                    buffer.append(ENUMLISTSEPARATESYM);
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
                        buffer.append(ARGLISTSEPARATESYM);
                }
                buffer.append(ARGLISTCLOSESYM);
            }
            if(element->tag->params[strlen(element->tag->params) - 1] == '\n')
                buffer.append('\n');
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

        case skin_tag_parameter::INTEGER:
            buffer.append(QString::number(param->data.number, 10));
            break;

        case skin_tag_parameter::DECIMAL:
            buffer.append(QString::number(param->data.number / 10., 'f', 1));
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
        case LINE_ALTERNATOR:
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

        case skin_tag_parameter::INTEGER:
            hash += param->data.number * (param->data.number / 4);
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

        case skin_tag_parameter::DECIMAL:
            hash += param->data.number;
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

            case LINE_ALTERNATOR:
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

            case skin_tag_parameter::INTEGER:
                return QObject::tr("Integer");

            case skin_tag_parameter::DECIMAL:
                return QObject::tr("Decimal");

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
            case LINE_ALTERNATOR:
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

            case skin_tag_parameter::INTEGER:
                return QString::number(param->data.number, 10);

            case skin_tag_parameter::DECIMAL:
                return QString::number(param->data.number / 10., 'f', 1);

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

    rendered = new RBViewport(element, info, this);

    for(int i = element->params_count; i < children.count(); i++)
        children[i]->render(info, dynamic_cast<RBViewport*>(rendered));

}

/* This version is called for logical lines, tags, conditionals and such */
void ParseTreeNode::render(const RBRenderInfo &info, RBViewport* viewport,
                           bool noBreak)
{
    if(!element)
        return;

    if(element->type == LINE)
    {
        for(int i = 0; i < children.count(); i++)
            children[i]->render(info, viewport);
        if(!noBreak && !breakFlag)
            viewport->newLine();
        else
            viewport->flushText();

        if(breakFlag)
            breakFlag = false;
    }
    else if(element->type == TEXT)
    {
        viewport->write(QString(static_cast<char*>(element->data)));
    }
    else if(element->type == TAG)
    {
        if(!execTag(info, viewport))
            viewport->write(evalTag(info).toString());
        if(element->tag->flags & NOBREAK)
            breakFlag = true;
    }
    else if(element->type == CONDITIONAL)
    {
        int child = evalTag(info, true, element->children_count).toInt();
        int max = children.count() - element->params_count;
        if(child < max)
        {
            children[element->params_count + child]
                    ->render(info, viewport, true);
        }
    }
    else if(element->type == LINE_ALTERNATOR)
    {
        /* First we build a list of the times for each branch */
        QList<double> times;
        for(int i = 0; i < children.count() ; i++)
            times.append(findBranchTime(children[i], info));

        double totalTime = 0;
        for(int i = 0; i < children.count(); i++)
            totalTime += times[i];

        /* Now we figure out which branch to select */
        double timeLeft = info.device()->data(QString("simtime")).toDouble();

        /* Skipping any full cycles */
        timeLeft -= totalTime * std::floor(timeLeft / totalTime);

        int branch = 0;
        while(timeLeft > 0)
        {
            timeLeft -= times[branch];
            if(timeLeft >= 0)
                branch++;
            else
                break;
            if(branch >= times.count())
                branch = 0;
        }

        /* In case we end up on a disabled branch, skip ahead.  If we find that
         * all the branches are disabled, don't render anything
         */
        int originalBranch = branch;
        while(times[branch] == 0)
        {
            branch++;
            if(branch == originalBranch)
            {
                branch = -1;
                break;
            }
            if(branch >= times.count())
                branch = 0;
        }

        /* ...and finally render the selected branch */
        if(branch >= 0)
            children[branch]->render(info, viewport, true);
    }
}

bool ParseTreeNode::execTag(const RBRenderInfo& info, RBViewport* viewport)
{

    QString filename;
    QString id;
    int x, y, tiles, tile, maxWidth, maxHeight, width, height;
    char c, hAlign, vAlign;
    RBImage* image;
    QPixmap temp;
    RBFont* fLoad;

    /* Two switch statements to narrow down the tag name */
    switch(element->tag->name[0])
    {

    case 'a':
        switch(element->tag->name[1])
        {
        case 'c':
            /* %ac */
            viewport->alignText(RBViewport::Center);
            return true;

        case 'l':
            /* %al */
            viewport->alignText(RBViewport::Left);
            return true;

        case 'r':
            /* %ar */
            viewport->alignText(RBViewport::Right);
            return true;

        case 'x':
            /* %ax */
            info.screen()->RtlMirror();
            return true;

        case 'L':
            /* %aL */
            if(info.device()->data("rtl").toBool())
                viewport->alignText(RBViewport::Right);
            else
                viewport->alignText(RBViewport::Left);
            return true;

        case 'R':
            /* %aR */
            if(info.device()->data("rtl").toBool())
                viewport->alignText(RBViewport::Left);
            else
                viewport->alignText(RBViewport::Right);
            return true;
        }

        return false;

    case 'p':
        switch(element->tag->name[1])
        {
        case 'b':
            /* %pb */
            new RBProgressBar(viewport, info, this);
            return true;

        case 'v':
            /* %pv */
            if(element->params_count > 0)
            {
                new RBProgressBar(viewport, info, this, true);
                return true;
            }
            else
                return false;
        }

        return false;

    case 's':
        switch(element->tag->name[1])
        {
        case '\0':
            /* %s */
            viewport->scrollText(info.device()->data("simtime").toDouble());
            return true;
        }

        return false;

    case 'w':
        switch(element->tag->name[1])
        {
        case 'd':
            /* %wd */
            info.screen()->breakSBS();
            return true;

        case 'e':
            /* %we */
            /* Totally extraneous */
            return true;

        case 'i':
            /* %wi */
            viewport->enableStatusBar();
            return true;
        }

        return false;

    case 'x':
        switch(element->tag->name[1])
        {
        case 'd':
            /* %xd */

            /* Breaking up into cases, getting the id first */
            if(element->params_count == 1)
            {
                /* The old fashioned style */
                id = "";
                id.append(element->params[0].data.text[0]);
            }
            else
            {
                id = QString(element->params[0].data.text);
            }


            if(info.screen()->getImage(id))
            {
                /* Fetching the image if available */
                image = new RBImage(*(info.screen()->getImage(id)), viewport);
            }
            else
            {
                image = 0;
            }

            /* Now determining the particular tile to load */
            if(element->params_count == 1)
            {
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

            }else{
                /* If the next param is just an int, use it as the tile */
                if(element->params[1].type == skin_tag_parameter::INTEGER)
                {
                    tile = element->params[1].data.number - 1;
                }
                else
                {
                    tile = children[1]->evalTag(info, true,
                                                image->numTiles()).toInt();

                    /* Adding the offset if there is one */
                    if(element->params_count == 3)
                        tile += element->params[2].data.number;
                    if(tile < 0)
                    {
                        /* If there is no image for the current status, then
                         * just refrain from showing anything
                         */
                        delete image;
                        return true;
                    }
                }
            }

            if(image)
            {
                image->setTile(tile);
                image->show();
                image->enableMovement();
            }

            return true;

        case 'l':
            /* %xl */
            id = element->params[0].data.text;
            if(element->params[1].data.text == QString("__list_icons__"))
            {
                filename = info.settings()->value("iconset", "");
                filename.replace(".rockbox",
                                 info.settings()->value("themebase"));
                temp.load(filename);
                if(!temp.isNull())
                {
                    tiles = temp.height() / temp.width();
                }
            }
            else
            {
                filename = info.settings()->value("imagepath", "") + "/" +
                           element->params[1].data.text;
                tiles = 1;
            }
            x = element->params[2].data.number;
            y = element->params[3].data.number;
            if(element->params_count > 4)
                tiles = element->params[4].data.number;

            info.screen()->loadImage(id, new RBImage(filename, tiles, x, y,
                                                     this, viewport));
            return true;

        case '\0':
            /* %x */
            id = element->params[0].data.text;
            filename = info.settings()->value("imagepath", "") + "/" +
                       element->params[1].data.text;
            x = element->params[2].data.number;
            y = element->params[3].data.number;
            image = new RBImage(filename, 1, x, y, this, viewport);
            info.screen()->loadImage(id, image);
            image->show();
            image->enableMovement();
            
            return true;

        }

        return false;

    case 'C':
        switch(element->tag->name[1])
        {
        case 'd':
            /* %Cd */
            info.screen()->showAlbumArt(viewport);
            return true;

        case 'l':
            /* %Cl */
            x = element->params[0].data.number;
            y = element->params[1].data.number;
            maxWidth = element->params[2].data.number;
            maxHeight = element->params[3].data.number;
            hAlign = element->params_count > 4
                     ? element->params[4].data.text[0] : 'c';
            vAlign = element->params_count > 5
                     ? element->params[5].data.text[0] : 'c';
            width = info.device()->data("artwidth").toInt();
            height = info.device()->data("artheight").toInt();
            info.screen()->setAlbumArt(new RBAlbumArt(viewport, x, y, maxWidth,
                                                      maxHeight, width, height,
                                                      this, hAlign, vAlign));
            return true;
        }

        return false;

    case 'F':

        switch(element->tag->name[1])
        {

        case 'l':
            /* %Fl */
            x = element->params[0].data.number;
            filename = info.settings()->value("themebase", "") + "/fonts/" +
                       element->params[1].data.text;
            fLoad = new RBFont(filename);
            if(!fLoad->isValid())
                dynamic_cast<RBScene*>(info.screen()->scene())
                ->addWarning(QObject::tr("Missing font file: ") + filename);
            info.screen()->loadFont(x, fLoad);
            return true;

        }

        return false;

    case 'T':
        switch(element->tag->name[1])
        {
        case '\0':
            /* %T */
            if(element->params_count < 5)
                return false;
            int x = element->params[0].data.number;
            int y = element->params[1].data.number;
            int width = element->params[2].data.number;
            int height = element->params[3].data.number;
            QString action(element->params[4].data.text);
            RBTouchArea* temp = new RBTouchArea(width, height, action, info);
            temp->setPos(x, y);
            return true;
        }

        return false;

    case 'V':

        switch(element->tag->name[1])
        {

        case 'b':
            /* %Vb */
            viewport->setBGColor(RBScreen::
                                 stringToColor(QString(element->params[0].
                                                       data.text),
                                               Qt::white));
            return true;

        case 'd':
            /* %Vd */
            id = element->params[0].data.text;
            info.screen()->showViewport(id);
            return true;

        case 'f':
            /* %Vf */
            viewport->setFGColor(RBScreen::
                                 stringToColor(QString(element->params[0].
                                                       data.text),
                                               Qt::black));
            return true;

        case 'p':
            /* %Vp */
            viewport->showPlaylist(info, element->params[0].data.number,
                                   children[1]);
            return true;

        case 'I':
            /* %VI */
            info.screen()->makeCustomUI(element->params[0].data.text);
            return true;

        }

        return false;

    case 'X':

        switch(element->tag->name[1])
        {
        case '\0':
            /* %X */
            filename = QString(element->params[0].data.text);
            info.screen()->setBackdrop(filename);
            return true;
        }

        return false;

    }

    return false;

}

QVariant ParseTreeNode::evalTag(const RBRenderInfo& info, bool conditional,
                                int branches)
{
    if(!conditional)
    {
        if(element->tag->name[0] == 'c' && !info.device()->data("cc").toBool())
            return QString();

        if(QString(element->tag->name) == "Sx")
            return element->params[0].data.text;
        return info.device()->data(QString(element->tag->name),
                                   element->params_count, element->params);
    }
    else
    {
        /* If we're evaluating for a conditional, we return the child branch
         * index that should be selected.  For true/false values, this is
         * 0 for true, 1 for false, and we also have to make sure not to
         * ever exceed the number of available children
         */

        int child;
        QVariant val = info.device()->data("?" + QString(element->tag->name));
        if(val.isNull())
            val = info.device()->data(QString(element->tag->name),
                                      element->params_count, element->params);

        if(val.isNull())
        {
            child = 1;
        }
        else if(QString(element->tag->name) == "bl")
        {
            /* bl has to be scaled to the number of available children, but it
             * also has an initial -1 value for an unknown state */
            child = val.toInt();
            if(child == -1)
            {
                child = 0;
            }
            else
            {
                child = ((branches - 1) * child / 100) + 1;
            }
        }
        else if(QString(element->tag->name) == "pv")
        {
            /* ?pv gets scaled to the number of available children, sandwiched
             * in between mute and 0/>0dB.  I assume a floor of -50dB for the
             * time being
             */
            int dB = val.toInt();

            if(dB < -50)
                child = 0;
            else if(dB == 0)
                child = branches - 2;
            else if(dB > 0)
                child = branches - 1;
            else
            {
                int options = branches - 3;
                child = (options * (dB + 50)) / 50;
            }
        }
        else if(QString(element->tag->name) == "px")
        {
            child = val.toInt();
            child = branches * child / 100;
        }
        else if(val.type() == QVariant::Bool)
        {
            /* Boolean values have to be reversed, because conditionals are
             * always of the form %?tag<true|false>
             */
            if(val.toBool())
                child = 0;
            else
                child = 1;
        }
        else if(element->tag->name[0] == 'i' || element->tag->name[0] == 'I'
                || element->tag->name[0] == 'f' || element->tag->name[0] == 'F')
        {
            if(info.device()->data("id3available").toBool())
                child = 0;
            else
                child = 1;
        }
        else if(val.type() == QVariant::String)
        {
            if(val.toString().length() > 0)
                child = 0;
            else
                child = 1;
        }
        else
        {
            child = val.toInt();
        }

        if(child < 0)
            child = 0;

        if(child < branches)
            return child;
        else if(branches == 1)
            return 2;
        else
            return branches - 1;
    }
}

double ParseTreeNode::findBranchTime(ParseTreeNode *branch,
                                     const RBRenderInfo& info)
{
    double retval = 2;
    for(int i = 0; i < branch->children.count(); i++)
    {
        ParseTreeNode* current = branch->children[i];
        if(current->element->type == TAG)
        {
            if(current->element->tag->name[0] == 't'
               && current->element->tag->name[1] == '\0')
            {
                retval = current->element->params[0].data.number / 10.;
            }
        }
        else if(current->element->type == CONDITIONAL)
        {
            retval = findConditionalTime(current, info);
        }
    }
    return retval;
}

double ParseTreeNode::findConditionalTime(ParseTreeNode *conditional,
                                          const RBRenderInfo& info)
{
    int child = conditional->evalTag(info, true,
                                     conditional->children.count()).toInt();
    if(child >= conditional->children.count())
        child = conditional->children.count() - 1;

    return findBranchTime(conditional->children[child], info);
}

void ParseTreeNode::modParam(QVariant value, int index)
{
    if(element)
    {
        if(index < 0)
            return;
        while(index >= children.count())
        {
            /* Padding children with defaults until we make the necessary
             * parameter available
             */
            skin_tag_parameter* newParam = new skin_tag_parameter;
            newParam->type = skin_tag_parameter::DEFAULT;
            /* We'll need to manually delete the extra parameters in the
             * destructor
             */
            extraParams.append(children.count());

            children.append(new ParseTreeNode(newParam, this, model));
            element->params_count++;
        }

        children[index]->modParam(value);
    }
    else if(param)
    {
        if(value.type() == QVariant::Double)
        {
            param->type = skin_tag_parameter::DECIMAL;
            param->data.number = static_cast<int>(value.toDouble() * 10);
        }
        else if(value.type() == QVariant::String)
        {
            param->type = skin_tag_parameter::STRING;
            free(param->data.text);
            param->data.text = strdup(value.toString().toStdString().c_str());
        }
        else if(value.type() == QVariant::Int)
        {
            param->type = skin_tag_parameter::INTEGER;
            param->data.number = value.toInt();
        }

        model->paramChanged(this);

    }
}
