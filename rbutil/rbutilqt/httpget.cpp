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


HttpGet::HttpGet(QObject *parent)
    : QObject(parent)
{
    qDebug() << "--> HttpGet::HttpGet()";
    outputToBuffer = true;
    getRequest = -1;
    connect(&http, SIGNAL(done(bool)), this, SLOT(httpDone(bool)));
    connect(&http, SIGNAL(dataReadProgress(int, int)), this, SLOT(httpProgress(int, int)));
    connect(&http, SIGNAL(requestFinished(int, bool)), this, SLOT(httpFinished(int, bool)));
    connect(&http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader&)), this, SLOT(httpResponseHeader(const QHttpResponseHeader&)));
    connect(&http, SIGNAL(stateChanged(int)), this, SLOT(httpState(int)));
    connect(&http, SIGNAL(requestStarted(int)), this, SLOT(httpStarted(int)));

    connect(&http, SIGNAL(readyRead(const QHttpResponseHeader&)), this, SLOT(httpResponseHeader(const QHttpResponseHeader&)));

}


QByteArray HttpGet::readAll()
{
    return dataBuffer;
}


QHttp::Error HttpGet::error()
{
    return http.error();
}


void HttpGet::httpProgress(int read, int total)
{
    emit dataReadProgress(read, total);
}


void HttpGet::setProxy(const QUrl &proxy)
{
    qDebug() << "HttpGet::setProxy" << proxy.toString();
    http.setProxy(proxy.host(), proxy.port(), proxy.userName(), proxy.password());
}


void HttpGet::setFile(QFile *file)
{
    outputFile = file;
    outputToBuffer = false;
    qDebug() << "HttpGet::setFile" << outputFile->fileName();
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
        qDebug() << "Error: Invalid URL" << endl;
        return false;
    }

    if (url.scheme() != "http") {
        qDebug() << "Error: URL must start with 'http:'" << endl;
        return false;
    }

    if (url.path().isEmpty()) {
        qDebug() << "Error: URL has no path" << endl;
        return false;
    }
    // if no output file was set write to buffer
    if(!outputToBuffer) {
        if (!outputFile->open(QIODevice::ReadWrite)) {
            qDebug() << "Error: Cannot open " << qPrintable(outputFile->fileName())
                << " for writing: " << qPrintable(outputFile->errorString())
                << endl;
            return false;
        }
    }
    http.setHost(url.host(), url.port(80));
    if(outputToBuffer) {
        qDebug() << "downloading to buffer:" << QString(url.toEncoded());
        getRequest = http.get(QString(url.toEncoded()));
    }
    else {
        qDebug() << "downloading to file:" << QString(url.toEncoded()) << qPrintable(outputFile->fileName());
        getRequest = http.get(QString(url.toEncoded()), outputFile);
    }
    qDebug() << "request scheduled: GET" << getRequest;
    
    http.close();
    return true;
}


void HttpGet::httpDone(bool error)
{
    if (error) {
        qDebug() << "Error: " << qPrintable(http.errorString()) << endl;
    }
    if(!outputToBuffer) {
        outputFile->close();
        qDebug() << "File downloaded as" << qPrintable(outputFile->fileName());
    }
    else qDebug() << "file downloaded to buffer";
    
    emit done(error);
}


void HttpGet::httpFinished(int id, bool error)
{
    qDebug() << "HttpGet::httpFinished(int, bool) =" << id << error;
    if(id == getRequest) dataBuffer = http.readAll();
    qDebug() << "pending:" << http.hasPendingRequests();
    //if(!http.hasPendingRequests()) httpDone(error);
    emit requestFinished(id, error);

}

void HttpGet::httpStarted(int id)
{
    qDebug() << "HttpGet::httpStarted(int) =" << id;
}


QString HttpGet::errorString()
{
    return http.errorString();
}


void HttpGet::httpResponseHeader(const QHttpResponseHeader &resp)
{
    // if there is a network error abort all scheduled requests for
    // this download
    response = resp.statusCode();
    if(response != 200) http.abort();
}


int HttpGet::httpResponse()
{
    return response;
}


void HttpGet::httpState(int state)
{
    QString s[] = {"Unconnected", "HostLookup", "Connecting", "Sending",
        "Reading", "Connected", "Closing"};
    if(state <= 6)
        qDebug() << "HttpGet::httpState() = " << s[state];
    else qDebug() << "HttpGet::httpState() = " << state;
}

