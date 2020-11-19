/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Riebeling
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/


#ifndef HTTPGET_H
#define HTTPGET_H

#include <QtCore>
#include <QtNetwork>
#include <QNetworkAccessManager>
#include "Logger.h"

class HttpGet : public QObject
{
    Q_OBJECT

    public:
        HttpGet(QObject *parent = nullptr);

        void getFile(const QUrl &url);
        void setProxy(const QUrl &url);
        void setProxy(bool);
        QString errorString(void);
        void setFile(QFile*);
        void setCache(const QDir&);
        void setCache(bool);
        int httpResponse(void);
        QByteArray readAll(void);
        bool isCached()
            { return m_lastRequestCached; }
        QDateTime timestamp(void)
            { return m_lastServerTimestamp; }
        //< set global cache path
        static void setGlobalCache(const QDir& d)
        {
            LOG_INFO() << "Global cache set to" << d.absolutePath();
            m_globalCache = d;
        }
        //< set global proxy value
        static void setGlobalProxy(const QUrl& p)
        {
            LOG_INFO() << "setting global proxy" << p;
            if(!p.isValid() || p.isEmpty()) {
                HttpGet::m_globalProxy.setType(QNetworkProxy::NoProxy);
            }
            else {
                HttpGet::m_globalProxy.setType(QNetworkProxy::HttpProxy);
                HttpGet::m_globalProxy.setHostName(p.host());
                HttpGet::m_globalProxy.setPort(p.port());
                HttpGet::m_globalProxy.setUser(p.userName());
                HttpGet::m_globalProxy.setPassword(p.password());
            }
            QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
            QNetworkProxy::setApplicationProxy(HttpGet::m_globalProxy);
        }
        //< set global user agent string
        static void setGlobalUserAgent(const QString& u)
            { m_globalUserAgent = u; }

    public slots:
        void abort(void);

    signals:
        void done(bool);
        void dataReadProgress(int, int);
        void requestFinished(int, bool);
        void headerFinished(void);

    private slots:
        void requestFinished(QNetworkReply* reply);
        void startRequest(QUrl url);
        void downloadProgress(qint64 received, qint64 total);
        void networkError(QNetworkReply::NetworkError error);

    private:
        static QString m_globalUserAgent;
        static QNetworkProxy m_globalProxy;
        QNetworkAccessManager m_mgr;
        QNetworkReply *m_reply;
        QNetworkDiskCache *m_cache;
        QDir m_cachedir;
        static QDir m_globalCache; //< global cache path value
        QByteArray m_data;
        QFile *m_outputFile;
        int m_lastStatusCode;
        QString m_lastErrorString;
        QDateTime m_lastServerTimestamp;
        bool m_lastRequestCached;
        QNetworkProxy m_proxy;
};


#endif

