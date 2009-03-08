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

#include <QtCore>
#include <QtNetwork>
#include <QtDebug>

#include "httpget.h"

QDir HttpGet::m_globalCache; //< global cach path value for new objects
QUrl HttpGet::m_globalProxy; //< global proxy value for new objects
bool HttpGet::m_globalDumbCache = false; //< globally set cache "dumb" mode
QString HttpGet::m_globalUserAgent; //< globally set user agent for requests

HttpGet::HttpGet(QObject *parent)
    : QObject(parent)
{
    outputToBuffer = true;
    m_cached = false;
    m_dumbCache = m_globalDumbCache;
    getRequest = -1;
    headRequest = -1;
    // if a request is cancelled before a reponse is available return some
    // hint about this in the http response instead of nonsense.
    m_response = -1;

    // default to global proxy / cache if not empty.
    // proxy is automatically enabled, disable it by setting an empty proxy
    // cache is enabled to be in line, can get disabled with setCache(bool)
    if(!m_globalProxy.isEmpty())
        setProxy(m_globalProxy);
    m_usecache = false;
    m_cachedir = m_globalCache;

    m_serverTimestamp = QDateTime();

    connect(&http, SIGNAL(done(bool)), this, SLOT(httpDone(bool)));
    connect(&http, SIGNAL(dataReadProgress(int, int)), this, SIGNAL(dataReadProgress(int, int)));
    connect(&http, SIGNAL(requestFinished(int, bool)), this, SLOT(httpFinished(int, bool)));
    connect(&http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader&)), this, SLOT(httpResponseHeader(const QHttpResponseHeader&)));
    connect(&http, SIGNAL(stateChanged(int)), this, SLOT(httpState(int)));
    connect(&http, SIGNAL(requestStarted(int)), this, SLOT(httpStarted(int)));

    connect(&http, SIGNAL(readyRead(const QHttpResponseHeader&)), this, SLOT(httpResponseHeader(const QHttpResponseHeader&)));

}


//! @brief set cache path
//  @param d new directory to use as cache path
void HttpGet::setCache(const QDir& d)
{
    m_cachedir = d;
    bool result;
    result = initializeCache(d);
    qDebug() << "[HTTP]"<< __func__ << "(QDir)" << d.absolutePath() << result;
    m_usecache = result;
}


/** @brief enable / disable cache useage
 *  @param c set cache usage
 */
void HttpGet::setCache(bool c)
{
    qDebug() << "[HTTP]" << __func__ << "(bool) =" << c;
    m_usecache = c;
    // make sure cache is initialized
    if(c)
        m_usecache = initializeCache(m_cachedir);
}


bool HttpGet::initializeCache(const QDir& d)
{
    bool result;
    QString p = d.absolutePath() + "/rbutil-cache";
    if(QFileInfo(d.absolutePath()).isDir())
    {
        if(!QFileInfo(p).isDir())
            result = d.mkdir("rbutil-cache");
        else
            result = true;
    }
    else
        result = false;

    return result;

}


/** @brief read all downloaded data into a buffer
 *  @return data
 */
QByteArray HttpGet::readAll()
{
    return dataBuffer;
}


/** @brief get http error
 *  @return http error
 */
QHttp::Error HttpGet::error()
{
    return http.error();
}


void HttpGet::setProxy(const QUrl &proxy)
{
    qDebug() << "[HTTP]" << __func__ << "(QUrl)" << proxy.toString();
    m_proxy = proxy;
    http.setProxy(m_proxy.host(), m_proxy.port(), m_proxy.userName(), m_proxy.password());
}


void HttpGet::setProxy(bool enable)
{
    qDebug() << "[HTTP]" << __func__ << "(bool)" << enable;
    if(enable)
        http.setProxy(m_proxy.host(), m_proxy.port(), m_proxy.userName(), m_proxy.password());
    else
        http.setProxy("", 0);
}


void HttpGet::setFile(QFile *file)
{
    outputFile = file;
    outputToBuffer = false;
    qDebug() << "[HTTP]" << __func__ << "(QFile*)" << outputFile->fileName();
}


void HttpGet::abort()
{
    http.abort();
    if(!outputToBuffer)
        outputFile->close();
}


bool HttpGet::getFile(const QUrl &url)
{
    if (!url.isValid()) {
        qDebug() << "[HTTP] Error: Invalid URL" << endl;
        return false;
    }

    if (url.scheme() != "http") {
        qDebug() << "[HTTP] Error: URL must start with 'http:'" << endl;
        return false;
    }

    if (url.path().isEmpty()) {
        qDebug() << "[HTTP] Error: URL has no path" << endl;
        return false;
    }
    m_serverTimestamp = QDateTime();
    // if no output file was set write to buffer
    if(!outputToBuffer) {
        if (!outputFile->open(QIODevice::ReadWrite)) {
            qDebug() << "[HTTP] Error: Cannot open " << qPrintable(outputFile->fileName())
                << " for writing: " << qPrintable(outputFile->errorString())
                << endl;
            return false;
        }
    }
    qDebug() << "[HTTP] downloading" << url.toEncoded();
    // create request
    http.setHost(url.host(), url.port(80));
    // construct query (if any)
    QList<QPair<QString, QString> > qitems = url.queryItems();
    if(url.hasQuery()) {
        m_query = "?";
        for(int i = 0; i < qitems.size(); i++)
            m_query += QUrl::toPercentEncoding(qitems.at(i).first, "/") + "="
                  + QUrl::toPercentEncoding(qitems.at(i).second, "/") + "&";
    }

    // create hash used for caching
    m_hash = QCryptographicHash::hash(url.toEncoded(), QCryptographicHash::Md5).toHex();
    m_path = QString(QUrl::toPercentEncoding(url.path(), "/"));

    // construct request header
    m_header.setValue("Host", url.host());
    m_header.setValue("User-Agent", m_globalUserAgent);
    m_header.setValue("Connection", "Keep-Alive");

    if(m_dumbCache || !m_usecache) {
        getFileFinish();
    }
    else {
        // schedule HTTP header request
        connect(this, SIGNAL(headerFinished()), this, SLOT(getFileFinish()));
        m_header.setRequest("HEAD", m_path + m_query);
        headRequest = http.request(m_header);
    }

    return true;
}


void HttpGet::getFileFinish()
{
    m_cachefile = m_cachedir.absolutePath() + "/rbutil-cache/" + m_hash;
    if(m_usecache) {
        // check if the file is present in cache
        qDebug() << "[HTTP] cache ENABLED";
        QFileInfo cachefile = QFileInfo(m_cachefile);
        if(cachefile.isReadable()
            && cachefile.size() > 0
            && cachefile.lastModified() > m_serverTimestamp) {

            qDebug() << "[HTTP] cached file found:" << m_cachefile;

            getRequest = -1;
            QFile c(m_cachefile);
            if(!outputToBuffer) {
                qDebug() << "[HTTP] copying cache file to output" << outputFile->fileName();
                c.open(QIODevice::ReadOnly);
                outputFile->open(QIODevice::ReadWrite);
                outputFile->write(c.readAll());
                outputFile->close();
                c.close();
            }
            else {
                qDebug() << "[HTTP] reading cache file into buffer";
                c.open(QIODevice::ReadOnly);
                dataBuffer = c.readAll();
                c.close();
            }
            m_response = 200; // fake "200 OK" HTTP response
            m_cached = true;
            httpDone(false); // we're done now. Fake http "done" signal.
            return;
        }
        else {
            if(cachefile.isReadable())
                qDebug() << "[HTTP] file in cache timestamp:" << cachefile.lastModified();
            else
                qDebug() << "[HTTP] file not in cache.";
            qDebug() << "[HTTP] server file timestamp:" << m_serverTimestamp;
            qDebug() << "[HTTP] downloading file to" << m_cachefile;
            // unlink old cache file
            if(cachefile.isReadable())
                QFile(m_cachefile).remove();
        }

    }
    else {
        qDebug() << "[HTTP] cache DISABLED";
    }
    // schedule GET request
    m_header.setRequest("GET", m_path + m_query);
    if(outputToBuffer) {
        qDebug() << "[HTTP] downloading to buffer.";
        getRequest = http.request(m_header);
    }
    else {
        qDebug() << "[HTTP] downloading to file:"
            << qPrintable(outputFile->fileName());
       getRequest = http.request(m_header, 0, outputFile);
    }
    qDebug() << "[HTTP] GET request scheduled, id:" << getRequest;

    return;
}


void HttpGet::httpDone(bool error)
{
    if (error) {
        qDebug() << "[HTTP] Error: " << qPrintable(http.errorString()) << httpResponse();
    }
    if(!outputToBuffer)
        outputFile->close();

    if(m_usecache && !m_cached && !error) {
        qDebug() << "[HTTP] creating cache file" << m_cachefile;
        QFile c(m_cachefile);
        c.open(QIODevice::ReadWrite);
        if(!outputToBuffer) {
            outputFile->open(QIODevice::ReadOnly | QIODevice::Truncate);
            c.write(outputFile->readAll());
            outputFile->close();
        }
        else
            c.write(dataBuffer);

        c.close();
    }
    // if cached file found and cache enabled ignore http errors
    if(m_usecache && m_cached && !http.hasPendingRequests()) {
        error = false;
    }
    // take care of concurring requests. If there is still one running,
    // don't emit done(). That request will call this slot again.
    if(http.currentId() == 0 && !http.hasPendingRequests())
        emit done(error);
}


void HttpGet::httpFinished(int id, bool error)
{
    qDebug() << "[HTTP]" << __func__ << "(int, bool) =" << id << error;
    if(id == getRequest) {
        dataBuffer = http.readAll();

        emit requestFinished(id, error);
    }
    qDebug() << "[HTTP] hasPendingRequests =" << http.hasPendingRequests();


    if(id == headRequest) {
        QHttpResponseHeader h = http.lastResponse();

        QString date = h.value("Last-Modified").simplified();
        if(date.isEmpty()) {
            m_serverTimestamp = QDateTime(); // no value = invalid
            emit headerFinished();
            return;
        }
        // to successfully parse the date strip weekday and timezone
        date.remove(0, date.indexOf(" ") + 1);
        if(date.endsWith("GMT"))
            date.truncate(date.indexOf(" GMT"));
        // distinguish input formats (see RFC1945)
        // RFC 850
        if(date.contains("-"))
            m_serverTimestamp = QDateTime::fromString(date, "dd-MMM-yy hh:mm:ss");
        // asctime format
        else if(date.at(0).isLetter())
            m_serverTimestamp = QDateTime::fromString(date, "MMM d hh:mm:ss yyyy");
        // RFC 822
        else
            m_serverTimestamp = QDateTime::fromString(date, "dd MMM yyyy hh:mm:ss");
        qDebug() << "[HTTP] Header Request Date:" << date << ", parsed:" << m_serverTimestamp;
        emit headerFinished();
        return;
    }
    if(id == getRequest)
        emit requestFinished(id, error);
}

void HttpGet::httpStarted(int id)
{
    qDebug() << "[HTTP]" << __func__ << "(int) =" << id;
    qDebug() << "headRequest" << headRequest << "getRequest" << getRequest;
}


QString HttpGet::errorString()
{
    return http.errorString();
}


void HttpGet::httpResponseHeader(const QHttpResponseHeader &resp)
{
    // if there is a network error abort all scheduled requests for
    // this download
    m_response = resp.statusCode();
    if(m_response != 200) {
        qDebug() << "[HTTP] response error =" << m_response << resp.reasonPhrase();
        http.abort();
    }
    // 301 -- moved permanently
    // 302 -- found
    // 303 -- see other
    // 307 -- moved temporarily
    // in all cases, header: location has the correct address so we can follow.
    if(m_response == 301 || m_response == 302 || m_response == 303 || m_response == 307) {
        // start new request with new url
        qDebug() << "[HTTP] response =" << m_response << "- following";
        getFile(resp.value("location") + m_query);
    }
}


int HttpGet::httpResponse()
{
    return m_response;
}


void HttpGet::httpState(int state)
{
    QString s[] = {"Unconnected", "HostLookup", "Connecting", "Sending",
        "Reading", "Connected", "Closing"};
    if(state <= 6)
        qDebug() << "[HTTP]" << __func__ << "() = " << s[state];
    else qDebug() << "[HTTP]" << __func__ << "() = " << state;
}

