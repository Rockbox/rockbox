#ifndef SKINHIGHLIGHTER_H
#define SKINHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QPlainTextEdit>

#include "tag_table.h"
#include "symbols.h"

class SkinHighlighter : public QSyntaxHighlighter
{
Q_OBJECT
public:
    /*
     * font - The font used for all text
     * normal - The normal text color
     * escaped - The color for escaped characters
     * tag - The color for tags and their delimiters
     * conditional - The color for conditionals and their delimiters
     *
     */
    SkinHighlighter(QColor comment, QColor tag, QColor conditional,
                    QColor escaped, QTextDocument* doc);
    virtual ~SkinHighlighter();

    void highlightBlock(const QString& text);

private:
    QColor escaped;
    QColor tag;
    QColor conditional;
    QColor comment;

};

#endif // SKINHIGHLIGHTER_H
