#include <QTextCharFormat>

#include "qsyntaxer.h"

QSyntaxer::QSyntaxer(QTextDocument *parent)
        : QSyntaxHighlighter(parent) {
    HighlightingRule rule;

    hrules[0].pattern = QRegExp("%[^\\| \n<\\?%]{1,2}");
    hrules[0].format.setFontWeight(QFont::Bold);
    hrules[0].format.setForeground(Qt::darkBlue);

    
    hrules[1].pattern = QRegExp("%[\\?]{1}[^<]{1,2}");
    hrules[1].format.setForeground(Qt::darkMagenta);
    
    hrules[2].pattern = QRegExp("(<|>)");
    hrules[2].format.setForeground(Qt::red);

    hrules[3].pattern = QRegExp("\\|");
    hrules[3].format.setForeground(Qt::darkRed);
    
    hrules[4].pattern = QRegExp("#[^\n]*");
    hrules[4].format.setForeground(Qt::darkGreen);
    hrules[4].format.setFontItalic(true);
}
//
void QSyntaxer::highlightBlock(const QString &text) {
    QTextCharFormat wholeText;
    wholeText.setFont(QFont("arial",11,QFont::Normal));
    setFormat(0,text.length(),wholeText);

    foreach (HighlightingRule rule, hrules) {
        QRegExp expression(rule.pattern);
        int index = text.indexOf(expression);
        while (index >= 0) {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = text.indexOf(expression, index + length);
        }
    }
    
}
