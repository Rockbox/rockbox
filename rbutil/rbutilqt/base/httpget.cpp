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
    m_useproxy = false;

    // default to global proxy / cache if not empty.
    // proxy is automatically enabled, disable it by setting an empty proxy
    // cache is enabled to be in line, can get disabled with setCache(bool)
    if(!m_globalProxy.isEmpty())
        setProxy(m_globalProxy);
    m_usecache = false;
    m_cachedir = m_globalCache;

    m_serverTimestamp = QDateTime();

    connect(&http, SIGNAL(done(bool)), this, SLOT(httpDone(bool)));
    connect(&http, SIGNAL(dataReadProgress(int, int)),
            this, SIGNAL(dataReadProgress(int, int)));
    connect(&http, SIGNAL(requestFinished(int, bool)),
            this, SLOT(httpFinished(int, bool)));
    connect(&http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader&)),
            this, SLOT(httpResponseHeader(const QHttpResponseHeader&)));
//    connect(&http, SIGNAL(stateChanged(int)), this, SLOT(httpState(int)));
    connect(&http, SIGNAL(requestStarted(int)), this, SLOT(httpStarted(int)));

    connect(&http, SIGNAL(readyRead(const QHttpResponseHeader&)),
            this, SLOT(httpResponseHeader(const QHttpResponseHeader&)));
    connect(this, SIGNAL(headerFinished()), this, SLOT(getFileFinish()));

}


//! @brief set cache path
//  @param d new directory to use as cache path
void HttpGet::setCache(const QDir& d)
{
    m_cachedir = d;
    bool result;
    result = initializeCache(d);
    m_usecache = result;
}


/** @brief enable / disable cache useage
 *  @param c set cache usage
 */
void HttpGet::setCache(bool c)
{
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
    m_proxy = proxy;
    m_useproxy = true;
    http.setProxy(m_proxy.host(), m_proxy.port(),
                  m_proxy.userName(), m_proxy.password());
}


void HttpGet::setProxy(bool enable)
{
    if(enable) {
        m_useproxy = true;
        http.setProxy(m_proxy.host(), m_proxy.port(),
                      m_proxy.userName(), m_proxy.password());
    }
    else {
        m_useproxy = false;
        http.setProxy("", 0);
    }
}


void HttpGet::setFile(QFile *file)
{
    outputFile = file;
    outputToBuffer = false;
}


void HttpGet::abort()
{
    qDebug() << "[HTTP] Aborting requests, pending:" << http.hasPendingRequests();
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
                     << " for writing: " << qPrintable(outputFile->errorString());
            return false;
        }
    }
    else {
        // output to buffer. Make sure buffer is empty so no old data gets
        // returned in case the object is reused.
        dataBuffer.clear();
    }
    qDebug() << "[HTTP] GET URI" << url.toEncoded();
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
    // RFC2616: the absoluteURI form must get used when the request is being
    // sent to a proxy.
    m_path.clear();
    if(m_useproxy)
        m_path = url.scheme() + "://" + url.host();
    m_path += QString(QUrl::toPercentEncoding(url.path(), "/"));

    // construct request header
    m_header.setValue("Host", url.host());
    m_header.setValue("User-Agent", m_globalUserAgent);
    m_header.setValue("Connection", "Keep-Alive");

    if(m_dumbCache || !m_usecache) {
        getFileFinish();
    }
    else {
        // schedule HTTP header request
        m_header.setRequest("HEAD", m_path + m_query);
        headRequest = http.request(m_header);
        qDebug() << "[HTTP] HEAD scheduled:  " << headRequest;
    }

    return true;
}


void HttpGet::getFileFinish()
{
    m_cachefile = m_cachedir.absolutePath() + "/rbutil-cache/" + m_hash;
    QString indexFile = m_cachedir.absolutePath() + "/rbutil-cache/cache.txt";
    if(m_usecache) {
        // check if the file is present in cache
        QFileInfo cachefile = QFileInfo(m_cachefile);
        if(cachefile.isReadable()
            && cachefile.size() > 0
            && cachefile.lastModified() > m_serverTimestamp) {

            qDebug() << "[HTTP] Cache: up-to-date file found:" << m_cachefile;

            getRequest = -1;
            QFile c(m_cachefile);
            if(!outputToBuffer) {
                qDebug() << "[HTTP] Cache: copying file to output"
                         << outputFile->fileName();
                c.open(QIODevice::ReadOnly);
                outputFile->open(QIODevice::ReadWrite);
                outputFile->write(c.readAll());
                outputFile->close();
                c.close();
            }
            else {
                qDebug() << "[HTTP] Cache: reading file into buffer";
                c.open(QIODevice::ReadOnly);
                dataBuffer = c.readAll();
                c.close();
            }
            m_response = 200; // fake "200 OK" HTTP response
            m_cached = true;
            httpDone(false); // we're done now. Handle http "done" signal.
            return;
        }
        else {
            // unlink old cache file
            if(cachefile.isReadable()) {
                QFile(m_cachefile).remove();
                qDebug() << "[HTTP] Cache: outdated, timestamp:"
                         << cachefile.lastModified();
            }
            qDebug() << "[HTTP] Cache: caching as" << m_cachefile;
            // update cache index file
            QFile idxFile(indexFile);
            idxFile.open(QIODevice::ReadOnly);
            QByteArray currLine;
            do {
                QByteArray currLine = idxFile.readLine(1000);
                if(currLine.startsWith(m_hash.toUtf8()))
                    break;
            } while(!currLine.isEmpty());
            idxFile.close();
            if(currLine.isEmpty()) {
                idxFile.open(QIODevice::Append);
                QString outline = m_hash + "\t" + m_header.value("Host") + "\t"
                    + m_path + "\t" + m_query + "\n";
                idxFile.write(outline.toUtf8());
                idxFile.close();
            }
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
    qDebug() << "[HTTP] GET scheduled:   " << getRequest;

    return;
}


void HttpGet::httpDone(bool error)
{
    if (error) {
        qDebug() << "[HTTP] Error:" << qPrintable(http.errorString()) << httpResponse();
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
    // take care of concurring requests. If there is still one running,
    // don't emit done(). That request will call this slot again.
    if(http.currentId() == 0 && !http.hasPendingRequests())
        emit done(error);
}


void HttpGet::httpFinished(int id, bool error)
{
    qDebug() << "[HTTP] Request finished:" << id << "Error:" << error
             << "pending requests:" << http.hasPendingRequests();
    if(id == getRequest) {
        dataBuffer = http.readAll();
        emit requestFinished(id, error);
    }

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
            m_serverTimestamp = QLocale::c().toDateTime(date, "dd MMM yyyy hh:mm:ss");
        qDebug() << "[HTTP] HEAD finished, server date:" << date << ", parsed:" << m_serverTimestamp;
        emit headerFinished();
        return;
    }
    if(id == getRequest)
        emit requestFinished(id, error);
}

void HttpGet::httpStarted(int id)
{
    qDebug() << "[HTTP] Request started: " << id << "Header req:"
             << headRequest << "Get req:" << getRequest;
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
   
    // 301 -- moved permanently
    // 302 -- found
    // 303 -- see other
    // 307 -- moved temporarily
    // in all cases, header: location has the correct address so we can follow.
    if(m_response == 301 || m_response == 302 || m_response == 303 || m_response == 307) {
        //abort without sending any signals
        http.blockSignals(true);
        http.abort();
        http.blockSignals(false);
        // start new request with new url
        qDebug() << "[HTTP] response =" << m_response << "- following";
        getFile(resp.value("location") + m_query);
    }
    else if(m_response != 200) {
        // all other errors are fatal.
        http.abort();
        qDebug() << "[HTTP] Response error:" << m_response << resp.reasonPhrase();
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
        qDebug() << "[HTTP] State:" << s[state];
    else qDebug() << "[HTTP] State:" << state;
}

