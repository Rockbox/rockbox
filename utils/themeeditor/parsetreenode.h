#ifndef PARSETREENODE_H
#define PARSETREENODE_H

extern "C"
{
#include "skin_parser.h"
}

#include <QString>
#include <QVariant>
#include <QList>

class ParseTreeNode
{
public:
    ParseTreeNode(struct skin_element* data, ParseTreeNode* parent, bool stop = false);
    virtual ~ParseTreeNode();

    void appendChild(ParseTreeNode* child);

    ParseTreeNode* child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    ParseTreeNode* parent();

private:
    ParseTreeNode* parentLink;
    QList<ParseTreeNode*> children;
    struct skin_element* element;

};

#endif // PARSETREENODE_H
