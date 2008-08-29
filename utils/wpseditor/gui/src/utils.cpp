#include "utils.h"
#include <QPointer>
#include <QtGlobal>
#include "qwpseditorwindow.h"

extern QPointer<QWpsEditorWindow> win;

int qlogger(const char* fmt,...) {
    va_list ap;
    va_start(ap, fmt);
    QString s;
    s.vsprintf(fmt,ap);
    va_end(ap);
    s.replace("\n","");
    //qDebug()<<s;
    if (win==0)
        qDebug()<<s;
    if (s.indexOf("ERR")>=0)
        s = "<font color=red>"+s+"</font>";
    if (win!=0)
        win->logMsg(s);
    va_end(ap);
    return s.length();
}

int qlogger(const QString& s) {
    return qlogger(s.toAscii().data());
}
