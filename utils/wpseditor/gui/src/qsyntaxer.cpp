#include <QTextCharFormat>

#include "qsyntaxer.h"

QSyntaxer::QSyntaxer(QTextDocument *parent)
        : QSyntaxHighlighter(parent) {
    HighlightingRule rule;

    hrules["operator"].pattern = QRegExp("%[^\\| \n<\\?%]{1,2}");
    hrules["operator"].format.setFontWeight(QFont::Bold);
    hrules["operator"].format.setForeground(Qt::darkBlue);

    
    hrules["question"].pattern = QRegExp("%[\\?]{1}[^<]{1,2}");
    hrules["question"].format.setForeground(Qt::darkMagenta);
    
    hrules["question2"].pattern = QRegExp("(<|>)");
    hrules["question2"].format.setForeground(Qt::red);
    

    hrules["limiter"].pattern = QRegExp("\\|");
    hrules["limiter"].format.setForeground(Qt::darkRed);
    
    hrules["comment"].pattern = QRegExp("#[^\n]*");
    hrules["comment"].format.setForeground(Qt::darkGreen);
    hrules["comment"].format.setFontItalic(true);
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
