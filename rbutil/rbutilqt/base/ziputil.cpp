/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2011 Dominik Riebeling
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QtCore>
#include <QDebug>
#include "ziputil.h"
#include "progressloggerinterface.h"
#include "Logger.h"

#include "quazip/quazip.h"
#include "quazip/quazipfile.h"
#include "quazip/quazipfileinfo.h"


ZipUtil::ZipUtil(QObject* parent) : ArchiveUtil(parent)
{
    m_zip = nullptr;
}


ZipUtil::~ZipUtil()
{
    if(m_zip) {
        delete m_zip;
    }
}

//! @brief open zip file.
//! @param zipfile path to zip file
//! @param mode open mode (see QuaZip::Mode)
//! @return true on success, false otherwise
bool ZipUtil::open(QString& zipfile, QuaZip::Mode mode)
{
    m_zip = new QuaZip(zipfile);
    return m_zip->open(mode);
}


//! @brief close zip file.
//! @return true on success, false otherwise
bool ZipUtil::close(void)
{
    if(!m_zip) {
        return false;
    }

    int error = UNZ_OK;
    if(m_zip->isOpen()) {
        m_zip->close();
        error = m_zip->getZipError();
    }
    delete m_zip;
    m_zip = nullptr;
    return (error == UNZ_OK) ? true : false;
}


//! @brief extract currently opened archive
//! @brief dest path to extract archive to, can be filename when extracting a
//!             single file.
//! @brief file file to extract from archive, full archive if empty.
//! @return true on success, false otherwise
bool ZipUtil::extractArchive(const QString& dest, QString file)
{
    LOG_INFO() << "extractArchive" << dest << file;
    bool result = true;
    if(!m_zip) {
        return false;
    }
    QuaZipFile *currentFile = new QuaZipFile(m_zip);
    int entries = m_zip->getEntriesCount();
    int current = 0;
    // construct the filename when extracting a single file from an archive.
    // if the given destination is a full path use it as output name,
    // otherwise use it as path to place the file as named in the archive.
    QString singleoutfile;
    if(!file.isEmpty() && QFileInfo(dest).isDir()) {
        singleoutfile = dest + "/" + file;
    }
    else if(!file.isEmpty()){
        singleoutfile = dest;
    }
    for(bool more = m_zip->goToFirstFile(); more; more = m_zip->goToNextFile())
    {
        ++current;
        // if the entry is a path ignore it. Path existence is ensured separately.
        if(m_zip->getCurrentFileName().split("/").last() == "")
            continue;
        // some tools set the MS-DOS file attributes. Check those for D flag,
        // since in some cases a folder entry does not end with a /
        QuaZipFileInfo fi;
        currentFile->getFileInfo(&fi);
        if(fi.externalAttr & 0x10) // FAT entry bit 4 indicating directory
            continue;

        QString outfilename;
        if(!singleoutfile.isEmpty()
                && QFileInfo(m_zip->getCurrentFileName()).fileName() == file) {
            outfilename = singleoutfile;
        }
        else if(singleoutfile.isEmpty()) {
            outfilename = dest + "/" + m_zip->getCurrentFileName();
        }
        if(outfilename.isEmpty())
            continue;
        QFile outputFile(outfilename);
        // make sure the output path exists
        if(!QDir().mkpath(QFileInfo(outfilename).absolutePath())) {
            result = false;
            emit logItem(tr("Creating output path failed"), LOGERROR);
            LOG_INFO() << "creating output path failed for:"
                       << outfilename;
            break;
        }
        if(!outputFile.open(QFile::WriteOnly)) {
            result = false;
            emit logItem(tr("Creating output file failed"), LOGERROR);
            LOG_INFO() << "creating output file failed:"
                       << outfilename;
            break;
        }
        currentFile->open(QIODevice::ReadOnly);
        outputFile.write(currentFile->readAll());
        if(currentFile->getZipError() != UNZ_OK) {
            result = false;
            emit logItem(tr("Error during Zip operation"), LOGERROR);
            LOG_INFO() << "QuaZip error:" << currentFile->getZipError()
                       << "on file" << currentFile->getFileName();
            break;
        }
        currentFile->close();
        outputFile.close();

        emit logProgress(current, entries);
    }
    delete currentFile;
    emit logProgress(1, 1);

    return result;
}


//! @brief append a folder to current archive
//! @param source source folder
//! @param basedir base folder for archive. Will get stripped from zip paths.
//! @return true on success, false otherwise
bool ZipUtil::appendDirToArchive(QString& source, QString& basedir)
{
    bool result = true;
    if(!m_zip || !m_zip->isOpen()) {
        LOG_INFO() << "Zip file not open!";
        return false;
    }
    // get a list of all files and folders. Needed for progress info and avoids
    // recursive calls.
    QDirIterator iterator(source, QDirIterator::Subdirectories);
    QStringList fileList;
    while(iterator.hasNext()) {
        iterator.next();
        // skip folders, we can't add them.
        if(!QFileInfo(iterator.filePath()).isDir()) {
            fileList.append(iterator.filePath());
        }
    }
    LOG_INFO() << "Adding" << fileList.size() << "files to archive";

    int max = fileList.size();
    for(int i = 0; i < max; i++) {
        QString current = fileList.at(i);
        if(!appendFileToArchive(current, basedir)) {
            LOG_ERROR() << "Error appending file" << current
                        << "to archive" << m_zip->getZipName();
            result = false;
            break;
        }
        emit logProgress(i, max);
    }
    return result;
}


//! @brief append a single file to current archive
//!
bool ZipUtil::appendFileToArchive(QString& file, QString& basedir)
{
    bool result = true;
    if(!m_zip || !m_zip->isOpen()) {
        LOG_ERROR() << "Zip file not open!";
        return false;
    }
    // skip folders, we can't add them.
    QFileInfo fileinfo(file);
    if(fileinfo.isDir()) {
        return false;
    }
    QString infile = fileinfo.canonicalFilePath();
    QString newfile = infile;
    newfile.remove(QDir(basedir).canonicalPath() + "/");

    QuaZipFile fout(m_zip);
    QFile fin(file);

    if(!fin.open(QFile::ReadOnly)) {
        LOG_ERROR() << "Could not open file for reading:" << file;
        return false;
    }
    if(!fout.open(QIODevice::WriteOnly, QuaZipNewInfo(newfile, infile))) {
        fin.close();
        LOG_ERROR() << "Could not open file for writing:" << newfile;
        return false;
    }

    result = (fout.write(fin.readAll()) < 0) ? false : true;
    fin.close();
    fout.close();
    return result;
}


//! @brief calculate total size of extracted files
qint64 ZipUtil::totalUncompressedSize(unsigned int clustersize)
{
    qint64 uncompressed = 0;

    QList<QuaZipFileInfo> items = contentProperties();
    if(items.size() == 0) {
        return -1;
    }
    int max = items.size();
    if(clustersize > 0) {
        for(int i = 0; i < max; ++i) {
            qint64 item = items.at(i).uncompressedSize;
            uncompressed += (item + clustersize - (item % clustersize));
        }
    }
    else {
        for(int i = 0; i < max; ++i) {
            uncompressed += items.at(i).uncompressedSize;
        }
    }
    if(clustersize > 0) {
        LOG_INFO() << "calculation rounded to cluster size for each file:"
                   << clustersize;
    }
    LOG_INFO() << "size of archive files uncompressed:"
               << uncompressed;
    return uncompressed;
}


QStringList ZipUtil::files(void)
{
    QList<QuaZipFileInfo> items = contentProperties();
    QStringList fileList;
    if(items.size() == 0) {
        return fileList;
    }
    int max = items.size();
    for(int i = 0; i < max; ++i) {
        fileList.append(items.at(i).name);
    }
    return fileList;
}


QList<QuaZipFileInfo> ZipUtil::contentProperties()
{
    QList<QuaZipFileInfo> items;
    if(!m_zip || !m_zip->isOpen()) {
        LOG_ERROR() << "Zip file not open!";
        return items;
    }
    QuaZipFileInfo info;
    QuaZipFile currentFile(m_zip);
    for(bool more = m_zip->goToFirstFile(); more; more = m_zip->goToNextFile())
    {
        currentFile.getFileInfo(&info);
        if(currentFile.getZipError() != UNZ_OK) {
            LOG_ERROR() << "QuaZip error:" << currentFile.getZipError()
                        << "on file" << currentFile.getFileName();
            return QList<QuaZipFileInfo>();
        }
        items.append(info);
    }
    return items;
}

