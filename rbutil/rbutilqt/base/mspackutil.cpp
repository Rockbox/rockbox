/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2013 Amaury Pouly
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QtCore>
#include "Logger.h"
#include "mspackutil.h"
#include "progressloggerinterface.h"

MsPackUtil::MsPackUtil(QObject* parent)
    :ArchiveUtil(parent)
{
    m_cabd = mspack_create_cab_decompressor(nullptr);
    m_cabinet = nullptr;
    if(!m_cabd)
        LOG_ERROR() << "CAB decompressor creation failed!";
}

MsPackUtil::~MsPackUtil()
{
    close();
    if(m_cabd)
        mspack_destroy_cab_decompressor(m_cabd);
}

bool MsPackUtil::open(QString& mspackfile)
{
    close();

    if(m_cabd == nullptr)
    {
        LOG_ERROR() << "No CAB decompressor available: cannot open file!";
        return false;
    }
    m_cabinet = m_cabd->search(m_cabd, QFile::encodeName(mspackfile).constData());
    return m_cabinet != nullptr;
}

bool MsPackUtil::close(void)
{
    if(m_cabd && m_cabinet)
        m_cabd->close(m_cabd, m_cabinet);
    m_cabinet = nullptr;
    return true;
}

bool MsPackUtil::extractArchive(const QString& dest, QString file)
{
    LOG_INFO() << "extractArchive" << dest << file;
    if(!m_cabinet)
    {
        LOG_ERROR() << "CAB file not open!";
        return false;
    }

    // construct the filename when extracting a single file from an archive.
    // if the given destination is a full path use it as output name,
    // otherwise use it as path to place the file as named in the archive.
    QString singleoutfile;
    if(!file.isEmpty() && QFileInfo(dest).isDir())
        singleoutfile = dest + "/" + file;
    else if(!file.isEmpty())
        singleoutfile = dest;
    struct mscabd_file *f = m_cabinet->files;
    if(f == nullptr)
    {
        LOG_WARNING() << "CAB doesn't contain file" << file;
        return true;
    }
    bool found = false;
    while(f)
    {
        QString name = QFile::decodeName(f->filename);
        name.replace("\\", "/");
        if(name.at(0) == '/')
            name.remove(0, 1);
        if(name == file || file.isEmpty())
        {
            QString path;
            if(!singleoutfile.isEmpty())
                path = singleoutfile;
            else
                path = dest + "/" + name;
            // make sure the output path exists
            if(!QDir().mkpath(QFileInfo(path).absolutePath()))
            {
                emit logItem(tr("Creating output path failed"), LOGERROR);
                LOG_ERROR() << "creating output path failed for:" << path;
                emit logProgress(1, 1);
                return false;
            }
            int ret = m_cabd->extract(m_cabd, f, QFile::encodeName(path).constData());
            if(ret != MSPACK_ERR_OK)
            {
                emit logItem(tr("Error during CAB operation"), LOGERROR);
                LOG_ERROR() << "mspack error: " << ret
                            << "(" << errorStringMsPack(ret) << ")";
                emit logProgress(1, 1);
                return false;
            }
            found = true;
        }
        f = f->next;
    }
    emit logProgress(1, 1);

    return found;
}

QStringList MsPackUtil::files(void)
{
    QStringList list;
    if(!m_cabinet)
    {
        LOG_WARNING() << "CAB file not open!";
        return list;
    }
    struct mscabd_file *file = m_cabinet->files;
    while(file)
    {
        QString name = QFile::decodeName(file->filename);
        name.replace("\\", "/");
        if(name.at(0) == '/')
            name.remove(0, 1);
        list.append(name);
        file = file->next;
    }

    return list;
}

QString MsPackUtil::errorStringMsPack(int error) const
{
    switch(error)
    {
        case MSPACK_ERR_OK: return "Ok";
        case MSPACK_ERR_ARGS: return "Bad arguments";
        case MSPACK_ERR_OPEN: return "Open error";
        case MSPACK_ERR_READ: return "Read error";
        case MSPACK_ERR_WRITE: return "Write error";
        case MSPACK_ERR_SEEK: return "Seek error";
        case MSPACK_ERR_NOMEMORY: return "Out of memory";
        case MSPACK_ERR_SIGNATURE: return "Bad signature";
        case MSPACK_ERR_DATAFORMAT: return "Bad data format";
        case MSPACK_ERR_CHECKSUM: return "Checksum error";
        case MSPACK_ERR_CRUNCH: return "Compression error";
        case MSPACK_ERR_DECRUNCH: return "Decompression error";
        default: return "Unknown";
    }
}
