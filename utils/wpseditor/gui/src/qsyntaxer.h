#ifndef QSYNTAXER_H
#define QSYNTAXER_H
//
#include <QSyntaxHighlighter>

class QTextCharFormat;

class QSyntaxer : public QSyntaxHighlighter {
    Q_OBJECT
    struct HighlightingRule {
        QRegExp pattern;
        QTextCharFormat format;
    };
    QMap<int,HighlightingRule> hrules;
public:
    QSyntaxer(QTextDocument *parent = 0);

protected:
    void highlightBlock(const QString &text);
};
#endif
