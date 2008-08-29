#ifndef __UTILS_H__
#define __UTILS_H__

#include <QDebug>

#define DEBUGF1 qlogger
#define DEBUGF2(...)

extern int qlogger(const char* fmt,...);
extern int qlogger(const QString& s);

#endif // __UTILS_H__
