#include "parsetreenode.h"

ParseTreeNode::ParseTreeNode(struct skin_element* data, ParseTreeNode* parent,
                             bool stop):
        parentLink(parent), element(data)
{

    if(stop)
        return;
    for(int i = 0; i < 5; i++)
        appendChild(new ParseTreeNode(data, this, true));
}

ParseTreeNode::~ParseTreeNode()
{
    qDeleteAll(children);
}

void ParseTreeNode::appendChild(ParseTreeNode* child)
{
    children.append(child);
}

ParseTreeNode* ParseTreeNode::child(int row)
{
    return children[row];
}

int ParseTreeNode::childCount() const
{
    return children.count();
}

int ParseTreeNode::columnCount() const
{
    return 2;
}

QVariant ParseTreeNode::data(int column) const
{
    if(column == 0)
        return element->type;
    else
        return element->line;
}
int ParseTreeNode::row() const
{
    if(parentLink)
        return parentLink->children.indexOf(const_cast<ParseTreeNode*>(this));
    return 0;
}

ParseTreeNode* ParseTreeNode::parent()
{
    return parentLink;
}
