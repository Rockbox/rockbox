#include "skinhighlighter.h"

SkinHighlighter::SkinHighlighter(QColor comment, QColor tag, QColor conditional,
                                 QColor escaped, QTextDocument* doc)
                                     :QSyntaxHighlighter(doc),
                                     escaped(escaped), tag(tag),
                                     conditional(conditional), comment(comment)
{

}

SkinHighlighter::~SkinHighlighter()
{

}

void SkinHighlighter::highlightBlock(const QString& text)
{
    for(int i = 0; i < text.length(); i++)
    {
        QChar c = text[i];

        /* Checking for delimiters */
        if(c == ARGLISTOPENSYM
           || c == ARGLISTCLOSESYM
           || c == ARGLISTSEPERATESYM)
            setFormat(i, 1, tag);

        if(c == ENUMLISTOPENSYM
           || c == ENUMLISTCLOSESYM
           || c == ENUMLISTSEPERATESYM)
            setFormat(i, 1, conditional);

        /* Checking for comments */
        if(c == COMMENTSYM)
        {
            setFormat(i, text.length() - i, comment);
            return;
        }

        if(c == TAGSYM)
        {
            if(text.length() - i < 2)
                return;

            if(find_escape_character(text[i + 1].toAscii()))
            {
                /* Checking for escaped characters */

                setFormat(i, 2, escaped);
                i++;
            }
            else if(text[i + 1] != CONDITIONSYM)
            {
                /* Checking for normal tags */

                char lookup[3];
                struct tag_info* found = 0;

                /* First checking for a two-character tag name */
                lookup[2] = '\0';

                if(text.length() - i >= 3)
                {
                    lookup[0] = text[i + 1].toAscii();
                    lookup[1] = text[i + 2].toAscii();

                    found = find_tag(lookup);
                }

                if(found)
                {
                    setFormat(i, 3, tag);
                    i += 2;
                }
                else
                {
                    lookup[1] = '\0';
                    lookup[0] = text[i + 1].toAscii();
                    found = find_tag(lookup);

                    if(found)
                    {
                        setFormat(i, 2, tag);
                        i++;
                    }
                }

            }
            else if(text[i + 1] == CONDITIONSYM)
            {
                /* Checking for conditional tags */

                if(text.length() - i < 3)
                    return;

                char lookup[3];
                struct tag_info* found = 0;

                lookup[2] = '\0';

                if(text.length() - i >= 4)
                {
                    lookup[0] = text[i + 2].toAscii();
                    lookup[1] = text[i + 3].toAscii();

                    found = find_tag(lookup);
                }

                if(found)
                {
                    setFormat(i, 4, conditional);
                    i += 3;
                }
                else
                {
                    lookup[1] = '\0';
                    lookup[0] = text[i + 2].toAscii();

                    found = find_tag(lookup);

                    if(found)
                    {
                        setFormat(i, 3, conditional);
                        i += 2;
                    }
                }

            }
        }
    }
}
