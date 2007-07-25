/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Riebeling
 *   $Id:$
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

    outputFile = new QFile(this);
    connect(&http, SIGNAL(done(bool)), this, SLOT(httpDone(bool)));
    connect(&http, SIGNAL(dataReadProgress(int, int)), this, SLOT(httpProgress(int, int)));
    connect(&http, SIGNAL(requestFinished(int, bool)), this, SLOT(httpFinished(int, bool)));
    connect(&http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader&)), this, SLOT(httpResponseHeader(const QHttpResponseHeader&)));
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
    qDebug() << "HttpGet::setFile" << outputFile->fileName();
}


void HttpGet::abort()
{
    http.abort();
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

    QString localFileName = outputFile->fileName();
    if (localFileName.isEmpty())
        outputFile->setFileName(QFileInfo(url.path()).fileName());

    if (!outputFile->open(QIODevice::ReadWrite)) {
        qDebug() << "Error: Cannot open " << qPrintable(outputFile->fileName())
            << " for writing: " << qPrintable(outputFile->errorString())
            << endl;
        return false;
    }

    http.setHost(url.host(), url.port(80));
    http.get(url.path(), outputFile);
    http.close();
    return true;
}

void HttpGet::httpDone(bool error)
{
    if (error) {
        qDebug() << "Error: " << qPrintable(http.errorString()) << endl;
    } else {
        qDebug() << "File downloaded as " << qPrintable(outputFile->fileName())
             << endl;
    }
    outputFile->close();
    emit done(error);
}


void HttpGet::httpFinished(int id, bool error)
{
    qDebug() << "HttpGet::httpFinished";
    qDebug() << "id:" << id << "error:" << error;
    emit requestFinished(id, error);

}


QString HttpGet::errorString()
{
    return http.errorString();
}


void HttpGet::httpResponseHeader(const QHttpResponseHeader &resp)
{
    qDebug() << "HttpGet::httpResponseHeader()" << resp.statusCode();
    response = resp.statusCode();
    if(response != 200) http.abort();
}


int HttpGet::httpResponse()
{
    return response;
}
