/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2013 Dominik Riebeling
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

#include <QtTest/QtTest>
#include <QtCore/QObject>
#include "httpget.h"

#define TEST_USER_AGENT "TestAgent/2.3"
#define TEST_HTTP_TIMEOUT 1000
#define TEST_BINARY_BLOB "\x01\x10\x20\x30\x40\x50\x60\x70" \
                         "\x80\x90\xff\xee\xdd\xcc\xbb\xaa"

 // HttpDaemon is the the class that implements the simple HTTP server.
 class HttpDaemon : public QTcpServer
{
    Q_OBJECT
    public:
        HttpDaemon(quint16 port = 0, QObject* parent = 0) : QTcpServer(parent)
        {
            listen(QHostAddress::Any, port);
        }

        quint16 port(void) { return this->serverPort(); }

#if QT_VERSION < 0x050000
        void incomingConnection(int socket)
#else
        // Qt 5 uses a different prototype for this function!
        void incomingConnection(qintptr socket)
#endif
        {
            // When a new client connects, the server constructs a QTcpSocket and all
            // communication with the client is done over this QTcpSocket. QTcpSocket
            // works asynchronously, this means that all the communication is done
            // in the two slots readClient() and discardClient().
            QTcpSocket* s = new QTcpSocket(this);
            connect(s, SIGNAL(readyRead()), this, SLOT(readClient()));
            connect(s, SIGNAL(disconnected()), this, SLOT(discardClient()));
            s->setSocketDescriptor(socket);
        }
        QList<QString> lastRequestData(void)
        {
            return m_lastRequestData;
        }
        void setResponsesToSend(QList<QByteArray> response)
        {
            m_requestNumber = 0;
            m_responsesToSend = response;
        }
        void reset(void)
        {
            m_requestNumber = 0;
            m_lastRequestData.clear();
            QString now =
                QDateTime::currentDateTime().toString("ddd, d MMM yyyy hh:mm:ss");
            m_defaultResponse = QByteArray(
                "HTTP/1.1 404 Not Found\r\n"
                "Date: " + now.toLatin1() + "\r\n"
                "Last-Modified: " + now.toLatin1() + "\r\n"
                "Connection: close\r\n"
                "\r\n");
        }

    private slots:
        void readClient()
        {
            // This slot is called when the client sent data to the server.
            QTcpSocket* socket = (QTcpSocket*)sender();
            // read whole request
            QString request;
            while(socket->canReadLine()) {
                QString line = socket->readLine();
                request.append(line);
                if(request.endsWith("\r\n\r\n")) {
                    m_lastRequestData.append(request);

                        if(m_requestNumber < m_responsesToSend.size())
                            socket->write(m_responsesToSend.at(m_requestNumber));
                        else
                            socket->write(m_defaultResponse);
                    socket->close();
                    m_requestNumber++;
                }
                if (socket->state() == QTcpSocket::UnconnectedState)
                    delete socket;
            }
        }
        void discardClient()
        {
            QTcpSocket* socket = (QTcpSocket*)sender();
            socket->deleteLater();
        }

    private:
        int m_requestNumber;
        QList<QByteArray> m_responsesToSend;
        QList<QString> m_lastRequestData;
        QByteArray m_defaultResponse;
};


class TestHttpGet : public QObject
{
    Q_OBJECT
    private slots:
        void testFileUrlRequest(void);
        void testCachedRequest(void);
        void testUncachedRepeatedRequest(void);
        void testUncachedMovedRequest(void);
        void testUserAgent(void);
        void testResponseCode(void);
        void testContentToBuffer(void);
        void testContentToFile(void);
        void testNoServer(void);
        void testServerTimestamp(void);
        void testMovedQuery(void);
        void init(void);
        void cleanup(void);

    public slots:
        void waitTimeout(void)
        {
            m_waitTimeoutOccured = true;
        }
        QDir temporaryFolder(void)
        {
            // Qt unfortunately doesn't support creating temporary folders so
            // we need to do that ourselves.
            QString tempdir;
            for(int i = 0; i < 100000; i++) {
                tempdir = QDir::tempPath() + QString("/qttest-temp-%1").arg(i);
                if(!QFileInfo(tempdir).exists()) break;
            }
            QDir().mkpath(tempdir);
            return QDir(tempdir);
        }
        void rmTree(QString folder)
        {
            // no function in Qt to recursively delete a folder :(
            QDir dir(folder);
            Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot
                        | QDir::System | QDir::Hidden | QDir::AllDirs
                        | QDir::Files, QDir::DirsFirst)) {
                if(info.isDir()) rmTree(info.absoluteFilePath());
                else QFile::remove(info.absoluteFilePath());
            }
            dir.rmdir(folder);
        }
    private:
        HttpDaemon *m_daemon;
        QByteArray m_port;
        bool m_waitTimeoutOccured;
        QString m_now;
        QDir m_cachedir;
        HttpGet *m_getter;
        QSignalSpy *m_doneSpy;
        QSignalSpy *m_progressSpy;
};


void TestHttpGet::init(void)
{
    m_now = QDateTime::currentDateTime().toString("ddd, d MMM yyyy hh:mm:ss");
    m_daemon = new HttpDaemon(0, this);  // use port 0 to auto-pick
    m_daemon->reset();
    m_port = QString("%1").arg(m_daemon->port()).toLatin1();
    m_cachedir = temporaryFolder();
    m_getter = new HttpGet(this);
    m_doneSpy = new QSignalSpy(m_getter, SIGNAL(done(bool)));
    m_progressSpy = new QSignalSpy(m_getter, SIGNAL(dataReadProgress(int, int)));
    m_waitTimeoutOccured = false;
}

void TestHttpGet::cleanup(void)
{
    rmTree(m_cachedir.absolutePath());
    if(m_getter) {
        m_getter->abort(); delete m_getter; m_getter = NULL;
    }
    if(m_daemon) { delete m_daemon; m_daemon = NULL; }
    if(m_doneSpy) { delete m_doneSpy; m_doneSpy = NULL; }
    if(m_progressSpy) { delete m_progressSpy; m_progressSpy = NULL; }
}

void TestHttpGet::testFileUrlRequest(void)
{
    QTimer::singleShot(TEST_HTTP_TIMEOUT, this, SLOT(waitTimeout(void)));

    QString teststring = "The quick brown fox jumps over the lazy dog.";
    QTemporaryFile datafile;
    datafile.open();
    datafile.write(teststring.toLatin1());
    m_getter->getFile(QUrl::fromLocalFile(datafile.fileName()));
    datafile.close();
    while(m_doneSpy->count() == 0 && m_waitTimeoutOccured == false)
        QCoreApplication::processEvents();

    QCOMPARE(m_doneSpy->count(), 1);
    QCOMPARE(m_waitTimeoutOccured, false);
    QCOMPARE(m_daemon->lastRequestData().size(), 0);
    QCOMPARE(m_getter->readAll(), teststring.toLatin1());
    QCOMPARE(m_getter->httpResponse(), 200);
    QCOMPARE(m_progressSpy->at(0).at(0).toInt(), 0);
}


/* On uncached requests, HttpGet is supposed to sent a GET request only.
 */
void TestHttpGet::testUncachedRepeatedRequest(void)
{
    QList<QByteArray> responses;
    responses << QByteArray(
        "HTTP/1.1 200 OK\r\n"
        "Date: " + m_now.toLatin1() + "\r\n"
        "Last-Modified: " + m_now.toLatin1() + "\r\n"
        "\r\n\r\n");
    responses << QByteArray(
        "HTTP/1.1 200 OK\r\n"
        "Last-Modified: " + m_now.toLatin1() + "\r\n"
        "Date: " + m_now.toLatin1() + "\r\n"
        "\r\n"
        "<html></html>\r\n\r\n");
    m_daemon->setResponsesToSend(responses);

    QTimer::singleShot(TEST_HTTP_TIMEOUT, this, SLOT(waitTimeout(void)));

    m_getter->getFile(QUrl("http://localhost:" + m_port + "/test1.txt"));
    while(m_doneSpy->count() == 0 && m_waitTimeoutOccured == false)
        QCoreApplication::processEvents();

    QCOMPARE(m_doneSpy->count(), 1);
    QCOMPARE(m_waitTimeoutOccured, false);
    QCOMPARE(m_daemon->lastRequestData().size(), 1);
    QCOMPARE(m_daemon->lastRequestData().at(0).startsWith("GET"), true);

    // request second time
    m_getter->getFile(QUrl("http://localhost:" + m_port + "/test1.txt"));
    while(m_doneSpy->count() < 2 && m_waitTimeoutOccured == false)
        QCoreApplication::processEvents();
    QCOMPARE(m_doneSpy->count(), 2);
    QCOMPARE(m_waitTimeoutOccured, false);
    QCOMPARE(m_daemon->lastRequestData().size(), 2);
    QCOMPARE(m_daemon->lastRequestData().at(1).startsWith("GET"), true);
    QCOMPARE(m_getter->httpResponse(), 200);
}

/* With enabled cache HttpGet is supposed to check the server file using a HEAD
 * request first, then request the file using GET if the server file is newer
 * than the cached one (or the file does not exist in the cache)
 */
void TestHttpGet::testCachedRequest(void)
{
    QList<QByteArray> responses;
    responses << QByteArray(
        "HTTP/1.1 302 Found\r\n"
        "Location: http://localhost:" + m_port + "/test2.txt\r\n"
        "Date: " + m_now.toLatin1() + "\r\n"
        "Last-Modified: " + m_now.toLatin1() + "\r\n"
        "\r\n");
    responses << QByteArray(
        "HTTP/1.1 200 OK\r\n"
        "Last-Modified: " + m_now.toLatin1() + "\r\n"
        "Date: " + m_now.toLatin1() + "\r\n"
        "\r\n"
        "<html></html>\r\n\r\n");
    responses << QByteArray(
        "HTTP/1.1 200 OK\r\n"
        "Last-Modified: 1 Jan 2000 00:00:00\r\n"
        "Date: " + m_now.toLatin1() + "\r\n"
        "\r\n");
    m_daemon->setResponsesToSend(responses);

    QTimer::singleShot(TEST_HTTP_TIMEOUT, this, SLOT(waitTimeout(void)));

    m_getter->setCache(m_cachedir);
    m_getter->getFile(QUrl("http://localhost:" + m_port + "/test1.txt"));
    while(m_doneSpy->count() == 0 && m_waitTimeoutOccured == false)
        QCoreApplication::processEvents();

    QList<QString> requests = m_daemon->lastRequestData();
    QCOMPARE(m_doneSpy->count(), 1);
    QCOMPARE(m_doneSpy->at(0).at(0).toBool(), false);
    QCOMPARE(m_waitTimeoutOccured, false);
    QCOMPARE(requests.size(), 2);
    QCOMPARE(requests.at(0).startsWith("GET"), true);
    QCOMPARE(requests.at(1).startsWith("GET"), true);
    QCOMPARE(m_getter->httpResponse(), 200);

    // request real file, this time the response should come from cache.
    m_getter->getFile(QUrl("http://localhost:" + m_port + "/test2.txt"));
    while(m_doneSpy->count() < 2 && m_waitTimeoutOccured == false)
        QCoreApplication::processEvents();
    QCOMPARE(m_doneSpy->count(), 2);  // 2 requests, 2 times done()
    QCOMPARE(m_doneSpy->at(1).at(0).toBool(), false);
    QCOMPARE(m_waitTimeoutOccured, false);
    QCOMPARE(m_daemon->lastRequestData().size(), 3);
    // redirect will not cache as the redirection target file.
    QCOMPARE(m_daemon->lastRequestData().at(2).startsWith("GET"), true);
    QCOMPARE(m_getter->httpResponse(), 200);
}

/* When a custom user agent is set all requests are supposed to contain it.
 * Enable cache to make HttpGet performs a HEAD request. Answer with 302, so
 * HttpGet follows and sends another HEAD request before finally doing a GET.
 */
void TestHttpGet::testUserAgent(void)
{
    QList<QByteArray> responses;
    responses << QByteArray(
        "HTTP/1.1 200 OK\r\n"
        "Date: " + m_now.toLatin1() + "\r\n"
        "Last-Modified: " + m_now.toLatin1() + "\r\n"
        "\r\n\r\n");
    responses << QByteArray(
        "HTTP/1.1 200 OK\r\n"
        "Last-Modified: " + m_now.toLatin1() + "\r\n"
        "Date: " + m_now.toLatin1() + "\r\n"
        "\r\n"
        "<html></html>\r\n\r\n");
    m_daemon->setResponsesToSend(responses);

    QTimer::singleShot(TEST_HTTP_TIMEOUT, this, SLOT(waitTimeout(void)));

    m_getter->setGlobalUserAgent(TEST_USER_AGENT);
    m_getter->setCache(m_cachedir);
    m_getter->getFile(QUrl("http://localhost:" + m_port + "/test1.txt"));
    while(m_doneSpy->count() == 0 && m_waitTimeoutOccured == false)
        QCoreApplication::processEvents();

    QList<QString> requests = m_daemon->lastRequestData();
    QCOMPARE(m_doneSpy->count(), 1);
    QCOMPARE(m_waitTimeoutOccured, false);
    QCOMPARE(requests.size(), 1);
    QCOMPARE(requests.at(0).startsWith("GET"), true);

    for(int i = 0; i < requests.size(); ++i) {
        QRegExp rx("User-Agent:[\t ]+([a-zA-Z0-9\\./]+)");
        bool userAgentFound = rx.indexIn(requests.at(i)) > 0 ? true : false;
        QCOMPARE(userAgentFound, true);
        QString userAgentString = rx.cap(1);
        QCOMPARE(userAgentString, QString(TEST_USER_AGENT));
    }
}

void TestHttpGet::testUncachedMovedRequest(void)
{
    QList<QByteArray> responses;
    responses << QByteArray(
        "HTTP/1.1 302 Found\r\n"
        "Location: http://localhost:" + m_port + "/test2.txt\r\n"
        "Date: " + m_now.toLatin1() + "\r\n"
        "Last-Modified: " + m_now.toLatin1() + "\r\n"
        "\r\n");
    responses << QByteArray(
        "HTTP/1.1 200 OK\r\n"
        "Last-Modified: " + m_now.toLatin1() + "\r\n"
        "Date: " + m_now.toLatin1() + "\r\n"
        "\r\n"
        "<html></html>\r\n\r\n");
    m_daemon->setResponsesToSend(responses);

    QTimer::singleShot(TEST_HTTP_TIMEOUT, this, SLOT(waitTimeout(void)));

    m_getter->getFile(QUrl("http://localhost:" + m_port + "/test1.php?var=1&b=foo"));
    while(m_doneSpy->count() == 0 && m_waitTimeoutOccured == false)
        QCoreApplication::processEvents();

    QCOMPARE(m_doneSpy->count(), 1);
    QCOMPARE(m_waitTimeoutOccured, false);
    QCOMPARE(m_daemon->lastRequestData().size(), 2);
    QCOMPARE(m_daemon->lastRequestData().at(0).startsWith("GET"), true);
    QCOMPARE(m_daemon->lastRequestData().at(1).startsWith("GET"), true);
}

void TestHttpGet::testResponseCode(void)
{
    QTimer::singleShot(TEST_HTTP_TIMEOUT, this, SLOT(waitTimeout(void)));

    m_getter->getFile(QUrl("http://localhost:" + m_port + "/test1.txt"));
    while(m_doneSpy->count() == 0 && m_waitTimeoutOccured == false)
        QCoreApplication::processEvents();

    QCOMPARE(m_doneSpy->count(), 1);
    QCOMPARE(m_doneSpy->at(0).at(0).toBool(), true);
    QCOMPARE(m_waitTimeoutOccured, false);
    QCOMPARE(m_daemon->lastRequestData().size(), 1);
    QCOMPARE(m_daemon->lastRequestData().at(0).startsWith("GET"), true);
    QCOMPARE(m_getter->httpResponse(), 404);
}

void TestHttpGet::testContentToBuffer(void)
{
    QList<QByteArray> responses;
    responses << QByteArray(
        "HTTP/1.1 200 OK\r\n"
        "Last-Modified: " + m_now.toLatin1() + "\r\n"
        "Date: " + m_now.toLatin1() + "\r\n"
        "\r\n"
        TEST_BINARY_BLOB);
    m_daemon->setResponsesToSend(responses);

    QTimer::singleShot(TEST_HTTP_TIMEOUT, this, SLOT(waitTimeout(void)));

    m_getter->getFile(QUrl("http://localhost:" + m_port + "/test1.txt"));
    while(m_doneSpy->count() == 0 && m_waitTimeoutOccured == false)
        QCoreApplication::processEvents();

    QCOMPARE(m_doneSpy->count(), 1);
    QCOMPARE(m_waitTimeoutOccured, false);
    QCOMPARE(m_getter->readAll(), QByteArray(TEST_BINARY_BLOB));
    // sizeof(TEST_BINARY_BLOB) will include an additional terminating NULL.
    QCOMPARE(m_getter->readAll().size(), (int)sizeof(TEST_BINARY_BLOB) - 1);
    QCOMPARE(m_progressSpy->at(m_progressSpy->count() - 1).at(0).toInt(), (int)sizeof(TEST_BINARY_BLOB) - 1);
    QCOMPARE(m_progressSpy->at(m_progressSpy->count() - 1).at(1).toInt(), (int)sizeof(TEST_BINARY_BLOB) - 1);
}

void TestHttpGet::testContentToFile(void)
{
    QTemporaryFile tf(this);
    QList<QByteArray> responses;
    responses << QByteArray(
        "HTTP/1.1 200 OK\r\n"
        "Last-Modified: " + m_now.toLatin1() + "\r\n"
        "Date: " + m_now.toLatin1() + "\r\n"
        "\r\n"
        TEST_BINARY_BLOB);
    m_daemon->setResponsesToSend(responses);

    QTimer::singleShot(TEST_HTTP_TIMEOUT, this, SLOT(waitTimeout(void)));

    m_getter->setFile(&tf);
    m_getter->getFile(QUrl("http://localhost:" + m_port + "/test1.txt"));
    while(m_doneSpy->count() == 0 && m_waitTimeoutOccured == false)
        QCoreApplication::processEvents();

    QCOMPARE(m_doneSpy->count(), 1);
    QCOMPARE(m_waitTimeoutOccured, false);

    tf.open();
    QByteArray data = tf.readAll();
    QCOMPARE(data, QByteArray(TEST_BINARY_BLOB));
    QCOMPARE((unsigned long)data.size(), sizeof(TEST_BINARY_BLOB) - 1);
    tf.close();
}

void TestHttpGet::testNoServer(void)
{
    QTimer::singleShot(TEST_HTTP_TIMEOUT, this, SLOT(waitTimeout(void)));
    m_getter->getFile(QUrl("http://localhost:53/test1.txt"));
    while(m_doneSpy->count() == 0 && m_waitTimeoutOccured == false)
        QCoreApplication::processEvents();

    QCOMPARE(m_doneSpy->count(), 1);
    QCOMPARE(m_doneSpy->at(0).at(0).toBool(), true);
    QCOMPARE(m_waitTimeoutOccured, false);
}

void TestHttpGet::testServerTimestamp(void)
{
    QList<QByteArray> responses;
    responses << QByteArray(
        "HTTP/1.1 200 OK\r\n"
        "Last-Modified: Wed, 20 Jan 2010 10:20:30\r\n" // RFC 822
        "Date: Wed, 20 Jan 2010 10:20:30\r\n"
        "\r\n"
        "\r\n");
    responses << QByteArray(
        "HTTP/1.1 200 OK\r\n"
        "Last-Modified: Sat Feb 19 09:08:07 2011\r\n" // asctime
        "Date: Sat Feb 19 09:08:07 2011\r\n"
        "\r\n"
        "\r\n");

    QList<QDateTime> times;
    times << QDateTime::fromString("2010-01-20T11:20:30", Qt::ISODate);
    times << QDateTime::fromString("2011-02-19T10:08:07", Qt::ISODate);

    m_daemon->setResponsesToSend(responses);

    QTimer::singleShot(TEST_HTTP_TIMEOUT, this, SLOT(waitTimeout(void)));

    int count = m_doneSpy->count();
    for(int i = 0; i < responses.size(); ++i) {
        m_getter->getFile(QUrl("http://localhost:" + m_port + "/test1.txt"));
        while(m_doneSpy->count() == count && m_waitTimeoutOccured == false)
            QCoreApplication::processEvents();
        count = m_doneSpy->count();
        QCOMPARE(m_getter->timestamp(), times.at(i));
    }
}

void TestHttpGet::testMovedQuery(void)
{
    QList<QByteArray> responses;
    responses << QByteArray(
        "HTTP/1.1 302 Found\r\n"
        "Location: http://localhost:" + m_port + "/test2.php\r\n"
        "Date: " + m_now.toLatin1() + "\r\n"
        "Last-Modified: " + m_now.toLatin1() + "\r\n"
        "\r\n");
    responses << QByteArray(
        "HTTP/1.1 200 OK\r\n"
        "Last-Modified: " + m_now.toLatin1() + "\r\n"
        "Date: " + m_now.toLatin1() + "\r\n"
        "\r\n"
        "<html></html>\r\n\r\n");
    m_daemon->setResponsesToSend(responses);

    QTimer::singleShot(TEST_HTTP_TIMEOUT, this, SLOT(waitTimeout(void)));

    m_getter->getFile(QUrl("http://localhost:" + m_port + "/test1.php?var=1&b=foo"));
    while(m_doneSpy->count() == 0 && m_waitTimeoutOccured == false)
        QCoreApplication::processEvents();

    QCOMPARE(m_doneSpy->count(), 1);
    QCOMPARE(m_waitTimeoutOccured, false);
    QCOMPARE(m_getter->httpResponse(), 200);
    QCOMPARE(m_daemon->lastRequestData().size(), 2);
    QCOMPARE(m_daemon->lastRequestData().at(0).startsWith("GET"), true);
    QCOMPARE(m_daemon->lastRequestData().at(1).startsWith("GET"), true);
    // current implementation keeps order of query items.
    QCOMPARE((bool)m_daemon->lastRequestData().at(1).contains("/test2.php?var=1&b=foo"), true);
}

QTEST_MAIN(TestHttpGet)

// this include is needed because we don't use a separate header file for the
// test class. It also needs to be at the end.
#include "test-httpget.moc"

