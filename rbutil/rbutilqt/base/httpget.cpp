/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2013 by Dominik Riebeling
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QtNetwork>

#include <QNetworkAccessManager>
#include <QNetworkRequest>

#include "httpget.h"
#include "Logger.h"

QString HttpGet::m_globalUserAgent; //< globally set user agent for requests
QDir HttpGet::m_globalCache; //< global cach path value for new objects
QNetworkProxy HttpGet::m_globalProxy;

HttpGet::HttpGet(QObject *parent)
    : QObject(parent),
      m_mgr(this),
      m_reply(nullptr),
      m_cache(nullptr),
      m_cachedir(m_globalCache),
      m_outputFile(nullptr),
      m_proxy(QNetworkProxy::NoProxy)
{
    setCache(true);
    connect(&m_mgr, &QNetworkAccessManager::finished, this,
            static_cast<void (HttpGet::*)(QNetworkReply*)>(&HttpGet::requestFinished));
    m_lastServerTimestamp = QDateTime();
}


/** @brief set cache path
 *  @param d new directory to use as cache path
 */
void HttpGet::setCache(const QDir& d)
{
    if(m_cache && m_cachedir == d.absolutePath())
        return;
    m_cachedir.setPath(d.absolutePath());
    setCache(true);
}


/** @brief enable / disable cache useage
 *  @param c set cache usage
 */
void HttpGet::setCache(bool c)
{
    // don't change cache if it's already (un)set.
    if(c && m_cache) return;
    if(!c && !m_cache) return;
    // don't delete the old cache directly, it might still be in use. Just
    // instruct it to delete itself later.
    if(m_cache) m_cache->deleteLater();
    m_cache = nullptr;

    QString path = m_cachedir.absolutePath();

    if(!c || m_cachedir.absolutePath().isEmpty()) {
        LOG_INFO() << "disabling download cache";
    }
    else {
        // append the cache path to make it unique in case the path points to
        // the system temporary path. In that case using it directly might
        // cause problems. Extra path also used in configure dialog.
        path += "/rbutil-cache";
        LOG_INFO() << "setting cache folder to" << path;
        m_cache = new QNetworkDiskCache(this);
        m_cache->setCacheDirectory(path);
    }
    m_mgr.setCache(m_cache);
}


/** @brief read all downloaded data into a buffer
 *  @return data
 */
QByteArray HttpGet::readAll()
{
    return m_data;
}


/** @brief Set and enable Proxy to use.
 *  @param proxy Proxy URL.
 */
void HttpGet::setProxy(const QUrl &proxy)
{
    LOG_INFO() << "Proxy set to" << proxy;
    m_proxy.setType(QNetworkProxy::HttpProxy);
    m_proxy.setHostName(proxy.host());
    m_proxy.setPort(proxy.port());
    m_proxy.setUser(proxy.userName());
    m_proxy.setPassword(proxy.password());
    m_mgr.setProxy(m_proxy);
}


/** @brief Enable or disable use of previously set proxy.
 *  @param enable Enable proxy.
 */
void HttpGet::setProxy(bool enable)
{
    if(enable) m_mgr.setProxy(m_proxy);
    else m_mgr.setProxy(QNetworkProxy::NoProxy);
}


/** @brief Set output file.
 *
 *  Set filename for storing the downloaded file to. If no file is set the
 *  downloaded file will not be stored to disk but kept in memory. The result
 *  can then be retrieved using readAll().
 *
 *  @param file Output file.
 */
void HttpGet::setFile(QFile *file)
{
    m_outputFile = file;
}


void HttpGet::abort()
{
    if(m_reply) m_reply->abort();
}


void HttpGet::requestFinished(QNetworkReply* reply)
{
    m_lastStatusCode
        = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    LOG_INFO() << "Request finished, status code:" << m_lastStatusCode;
    m_lastServerTimestamp
        = reply->header(QNetworkRequest::LastModifiedHeader).toDateTime().toLocalTime();
    LOG_INFO() << "Data from cache:"
             << reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool();
    m_lastRequestCached =
        reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool();
    if(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).isValid()) {
        // handle relative URLs using QUrl::resolved()
        QUrl org = reply->request().url();
        QUrl url = QUrl(org).resolved(
                reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl());
        // reconstruct query
#if QT_VERSION < 0x050000
        QList<QPair<QByteArray, QByteArray> > qitms = org.encodedQueryItems();
        for(int i = 0; i < qitms.size(); ++i)
            url.addEncodedQueryItem(qitms.at(i).first, qitms.at(i).second);
#else
        url.setQuery(org.query());
#endif
        LOG_INFO() << "Redirected to" << url;
        startRequest(url);
        return;
    }
    else if(m_lastStatusCode == 200 ||
            (reply->url().scheme() == "file" && reply->error() == 0)) {
        // callers might not be aware if the request is file:// so fake 200.
        m_lastStatusCode = 200;
        m_data = reply->readAll();
        if(m_outputFile && m_outputFile->open(QIODevice::WriteOnly)) {
            m_outputFile->write(m_data);
            m_outputFile->close();
        }
        emit done(false);
    }
    else {
        m_data.clear();
        emit done(true);
    }
    reply->deleteLater();
    m_reply = nullptr;
}


void HttpGet::downloadProgress(qint64 received, qint64 total)
{
    emit dataReadProgress((int)received, (int)total);
}


void HttpGet::startRequest(QUrl url)
{
    LOG_INFO() << "Request started";
    QNetworkRequest req(url);
    if(!m_globalUserAgent.isEmpty())
        req.setRawHeader("User-Agent", m_globalUserAgent.toLatin1());

    m_reply = m_mgr.get(req);
#if QT_VERSION < 0x050f00
    connect(m_reply,
            static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            this, &HttpGet::networkError);
#else
    connect(m_reply, &QNetworkReply::errorOccurred, this, &HttpGet::networkError);
#endif
    connect(m_reply, &QNetworkReply::downloadProgress, this, &HttpGet::downloadProgress);
}


void HttpGet::networkError(QNetworkReply::NetworkError error)
{
    LOG_ERROR() << "NetworkError occured:" << error << m_reply->errorString();
    m_lastErrorString = m_reply->errorString();
}


/** @brief Retrieve the file pointed to by url.
 *
 *  Note: This also handles file:// URLs. Be aware that QUrl requires file://
 *  URLs to be absolute, i.e. file://filename.txt doesn't work. Use
 *  QDir::absoluteFilePath() to convert to an absolute path first.
 *
 *  @param url URL to download.
 */
void HttpGet::getFile(const QUrl &url)
{
    LOG_INFO() << "Get URI" << url.toString();
    m_data.clear();
    startRequest(url);
}


/** @brief Retrieve string representation for most recent error.
 *  @return Error string.
 */
QString HttpGet::errorString(void)
{
    return m_lastErrorString;
}


/** @brief Return last HTTP response code.
 *  @return Response code.
 */
int HttpGet::httpResponse(void)
{
    return m_lastStatusCode;
}

