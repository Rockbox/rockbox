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
        bool isCached() { return m_cached; }
        void setDumbCache(bool b) //< disable checking of http header timestamp for caching
            { m_dumbCache = b; }
        static void setGlobalCache(const QDir d) //< set global cache path
            { m_globalCache = d; }
        static void setGlobalProxy(const QUrl p) //< set global proxy value
            { m_globalProxy = p; }
        static void setGlobalDumbCache(bool b) //< set "dumb" (ignore server status) caching mode
            { m_globalDumbCache = b; }

    public slots:
        void abort(void);

    signals:
        void done(bool);
        void dataReadProgress(int, int);
        void requestFinished(int, bool);
        void headerFinished(void);

    private slots:
        void httpDone(bool error);
        void httpProgress(int, int);
        void httpFinished(int, bool);
        void httpResponseHeader(const QHttpResponseHeader&);
        void httpState(int);
        void httpStarted(int);
        void getFileFinish(void);

    private:
        bool initializeCache(const QDir&);
        QHttp http; //< download object
        QFile *outputFile;
        int m_response; //< http response
        int getRequest;  //! get file http request id
        int headRequest; //! get http header request id
        QByteArray dataBuffer;
        bool outputToBuffer;
        bool m_usecache;
        QDir m_cachedir;
        QString m_cachefile; // cached filename
        bool m_cached;
        QUrl m_proxy;
        static QDir m_globalCache; //< global cache path value
        static QUrl m_globalProxy; //< global proxy value
        static bool m_globalDumbCache; //< cache "dumb" mode global setting
        QDateTime m_serverTimestamp; //< timestamp of file on server
        QString m_query; //< constructed query to pass http getter
        QString m_path; //< constructed path to pass http getter
        QString m_hash; //< caching hash
        bool m_dumbCache; //< true if caching should ignore the server header
};

#endif
