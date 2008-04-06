/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Riebeling
 *   $Id$
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/


#ifndef HTTPGET_H
#define HTTPGET_H

#include <QtCore>
#include <QtNetwork>

class QUrl;

class HttpGet : public QObject
{
    Q_OBJECT

    public:
        HttpGet(QObject *parent = 0);

        bool getFile(const QUrl &url);
        void setProxy(const QUrl &url);
        void setProxy(bool);
        QHttp::Error error(void);
        QString errorString(void);
        void setFile(QFile*);
        void setCache(QDir);
        void setCache(bool);
        int httpResponse(void);
        QByteArray readAll(void);
        bool isCached() { return cached; }
        static void setGlobalCache(const QDir d) //< set global cache path
            { m_globalCache = d; }
        static void setGlobalProxy(const QUrl p) //< set global proxy value
            { m_globalProxy = p; }

    public slots:
        void abort(void);

    signals:
        void done(bool);
        void dataReadProgress(int, int);
        void requestFinished(int, bool);

    private slots:
        void httpDone(bool error);
        void httpProgress(int, int);
        void httpFinished(int, bool);
        void httpResponseHeader(const QHttpResponseHeader&);
        void httpState(int);
        void httpStarted(int);

    private:
        bool initializeCache(const QDir&);
        QHttp http; //< download object
        QFile *outputFile;
        int response; //< http response
        int getRequest;
        QByteArray dataBuffer;
        bool outputToBuffer;
        QString query;
        bool m_usecache;
        QDir m_cachedir;
        QString cachefile;
        bool cached;
        QUrl m_proxy;
        static QDir m_globalCache; //< global cache path value
        static QUrl m_globalProxy; //< global proxy value
};

#endif
