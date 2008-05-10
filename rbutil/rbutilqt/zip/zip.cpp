/****************************************************************************
** Filename: zip.cpp
** Last updated [dd/mm/yyyy]: 01/02/2007
**
** pkzip 2.0 file compression.
**
** Some of the code has been inspired by other open source projects,
** (mainly Info-Zip and Gilles Vollant's minizip).
** Compression and decompression actually uses the zlib library.
**
** Copyright (C) 2007 Angius Fabrizio. All rights reserved.
**
** This file is part of the OSDaB project (http://osdab.sourceforge.net/).
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See the file LICENSE.GPL that came with this software distribution or
** visit http://www.gnu.org/copyleft/gpl.html for GPL licensing information.
**
**********************************************************************/

#include "zip.h"
#include "zip_p.h"
#include "zipentry_p.h"

// we only use this to seed the random number generator
#include <time.h>

#include <QMap>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QCoreApplication>

// You can remove this #include if you replace the qDebug() statements.
#include <QtDebug>

//! Local header size (including signature, excluding variable length fields)
#define ZIP_LOCAL_HEADER_SIZE 30
//! Encryption header size
#define ZIP_LOCAL_ENC_HEADER_SIZE 12
//! Data descriptor size (signature included)
#define ZIP_DD_SIZE_WS 16
//! Central Directory record size (signature included)
#define ZIP_CD_SIZE 46
//! End of Central Directory record size (signature included)
#define ZIP_EOCD_SIZE 22

// Some offsets inside a local header record (signature included)
#define ZIP_LH_OFF_VERS 4
#define ZIP_LH_OFF_GPFLAG 6
#define ZIP_LH_OFF_CMET 8
#define ZIP_LH_OFF_MODT 10
#define ZIP_LH_OFF_MODD 12
#define ZIP_LH_OFF_CRC 14
#define ZIP_LH_OFF_CSIZE 18
#define ZIP_LH_OFF_USIZE 22
#define ZIP_LH_OFF_NAMELEN 26
#define ZIP_LH_OFF_XLEN 28

// Some offsets inside a data descriptor record (including signature)
#define ZIP_DD_OFF_CRC32 4
#define ZIP_DD_OFF_CSIZE 8
#define ZIP_DD_OFF_USIZE 12

// Some offsets inside a Central Directory record (including signature)
#define ZIP_CD_OFF_MADEBY 4
#define ZIP_CD_OFF_VERSION 6
#define ZIP_CD_OFF_GPFLAG 8
#define ZIP_CD_OFF_CMET 10
#define ZIP_CD_OFF_MODT 12
#define ZIP_CD_OFF_MODD 14
#define ZIP_CD_OFF_CRC 16
#define ZIP_CD_OFF_CSIZE 20
#define ZIP_CD_OFF_USIZE 24
#define ZIP_CD_OFF_NAMELEN 28
#define ZIP_CD_OFF_XLEN 30
#define ZIP_CD_OFF_COMMLEN 32
#define ZIP_CD_OFF_DISKSTART 34
#define ZIP_CD_OFF_IATTR 36
#define ZIP_CD_OFF_EATTR 38
#define ZIP_CD_OFF_LHOFF 42

// Some offsets inside a EOCD record (including signature)
#define ZIP_EOCD_OFF_DISKNUM 4
#define ZIP_EOCD_OFF_CDDISKNUM 6
#define ZIP_EOCD_OFF_ENTRIES 8
#define ZIP_EOCD_OFF_CDENTRIES 10
#define ZIP_EOCD_OFF_CDSIZE 12
#define ZIP_EOCD_OFF_CDOFF 16
#define ZIP_EOCD_OFF_COMMLEN 20

//! PKZip version for archives created by this API
#define ZIP_VERSION 0x14

//! Do not store very small files as the compression headers overhead would be to big
#define ZIP_COMPRESSION_THRESHOLD 60

//! This macro updates a one-char-only CRC; it's the Info-Zip macro re-adapted
#define CRC32(c, b) crcTable[((int)c^b) & 0xff] ^ (c >> 8)

/*!
	\class Zip zip.h

	\brief Zip file compression.

	Some quick usage examples.

	\verbatim
	Suppose you have this directory structure:

	/root/dir1/
	/root/dir1/file1.1
	/root/dir1/file1.2
	/root/dir1/dir1.1/
	/root/dir1/dir1.2/file1.2.1

	EXAMPLE 1:
	myZipInstance.addDirectory("/root/dir1");

	RESULT:
	Beheaves like any common zip software and creates a zip file with this structure:

	dir1/
	dir1/file1.1
	dir1/file1.2
	dir1/dir1.1/
	dir1/dir1.2/file1.2.1

	EXAMPLE 2:
	myZipInstance.addDirectory("/root/dir1", "myRoot/myFolder");

	RESULT:
	Adds a custom root to the paths and creates a zip file with this structure:

	myRoot/myFolder/dir1/
	myRoot/myFolder/dir1/file1.1
	myRoot/myFolder/dir1/file1.2
	myRoot/myFolder/dir1/dir1.1/
	myRoot/myFolder/dir1/dir1.2/file1.2.1

	EXAMPLE 3:
	myZipInstance.addDirectory("/root/dir1", Zip::AbsolutePaths);

	NOTE:
	Same as calling addDirectory(SOME_PATH, PARENT_PATH_of_SOME_PATH).

	RESULT:
	Preserves absolute paths and creates a zip file with this structure:

	/root/dir1/
	/root/dir1/file1.1
	/root/dir1/file1.2
	/root/dir1/dir1.1/
	/root/dir1/dir1.2/file1.2.1

	EXAMPLE 4:
	myZipInstance.setPassword("hellopass");
	myZipInstance.addDirectory("/root/dir1", "/");

	RESULT:
	Adds and encrypts the files in /root/dir1, creating the following zip structure:

	/dir1/
	/dir1/file1.1
	/dir1/file1.2
	/dir1/dir1.1/
	/dir1/dir1.2/file1.2.1

	\endverbatim
*/

/*! \enum Zip::ErrorCode The result of a compression operation.
	\value Zip::Ok No error occurred.
	\value Zip::ZlibInit Failed to init or load the zlib library.
	\value Zip::ZlibError The zlib library returned some error.
	\value Zip::FileExists The file already exists and will not be overwritten.
	\value Zip::OpenFailed Unable to create or open a device.
	\value Zip::NoOpenArchive CreateArchive() has not been called yet.
	\value Zip::FileNotFound File or directory does not exist.
	\value Zip::ReadFailed Reading of a file failed.
	\value Zip::WriteFailed Writing of a file failed.
	\value Zip::SeekFailed Seek failed.
*/

/*! \enum Zip::CompressionLevel Returns the result of a decompression operation.
	\value Zip::Store No compression.
	\value Zip::Deflate1 Deflate compression level 1(lowest compression).
	\value Zip::Deflate1 Deflate compression level 2.
	\value Zip::Deflate1 Deflate compression level 3.
	\value Zip::Deflate1 Deflate compression level 4.
	\value Zip::Deflate1 Deflate compression level 5.
	\value Zip::Deflate1 Deflate compression level 6.
	\value Zip::Deflate1 Deflate compression level 7.
	\value Zip::Deflate1 Deflate compression level 8.
	\value Zip::Deflate1 Deflate compression level 9 (maximum compression).
	\value Zip::AutoCPU Adapt compression level to CPU speed (faster CPU => better compression).
	\value Zip::AutoMIME Adapt compression level to MIME type of the file being compressed.
	\value Zip::AutoFull Use both CPU and MIME type detection.
*/


/************************************************************************
 Public interface
*************************************************************************/

/*!
	Creates a new Zip file compressor.
*/
Zip::Zip()
{
	d = new ZipPrivate;
}

/*!
	Closes any open archive and releases used resources.
*/
Zip::~Zip()
{
	closeArchive();
	delete d;
}

/*!
	Returns true if there is an open archive.
*/
bool Zip::isOpen() const
{
	return d->device != 0;
}

/*!
	Sets the password to be used for the next files being added!
	Files added before calling this method will use the previously
	set password (if any).
	Closing the archive won't clear the password!
*/
void Zip::setPassword(const QString& pwd)
{
	d->password = pwd;
}

//! Convenience method, clears the current password.
void Zip::clearPassword()
{
	d->password.clear();
}

//! Returns the currently used password.
QString Zip::password() const
{
	return d->password;
}

/*!
	Attempts to create a new Zip archive. If \p overwrite is true and the file
	already exist it will be overwritten.
	Any open archive will be closed.
 */
Zip::ErrorCode Zip::createArchive(const QString& filename, bool overwrite)
{
	QFile* file = new QFile(filename);

	if (file->exists() && !overwrite) {
		delete file;
		return Zip::FileExists;
	}

	if (!file->open(QIODevice::WriteOnly)) {
		delete file;
		return Zip::OpenFailed;
	}

	Zip::ErrorCode ec = createArchive(file);
	if (ec != Zip::Ok) {
		file->remove();
	}

	return ec;
}

/*!
	Attempts to create a new Zip archive. If there is another open archive this will be closed.
	\warning The class takes ownership of the device!
 */
Zip::ErrorCode Zip::createArchive(QIODevice* device)
{
	if (device == 0)
	{
		qDebug() << "Invalid device.";
		return Zip::OpenFailed;
	}

	return d->createArchive(device);
}

/*!
	Returns the current archive comment.
*/
QString Zip::archiveComment() const
{
	return d->comment;
}

/*!
	Sets the comment for this archive. Note: createArchive() should have been
	called before.
*/
void Zip::setArchiveComment(const QString& comment)
{
	if (d->device != 0)
		d->comment = comment;
}

/*!
	Convenience method, same as calling
	Zip::addDirectory(const QString&,const QString&,CompressionLevel)
	with an empty \p root parameter (or with the parent directory of \p path if the
	AbsolutePaths options is set).

	The ExtractionOptions are checked in the order they are defined in the zip.h heaser file.
	This means that the last one overwrites the previous one (if some conflict occurs), i.e.
	Zip::IgnorePaths | Zip::AbsolutePaths would be interpreted as Zip::IgnorePaths.
 */
Zip::ErrorCode Zip::addDirectory(const QString& path, CompressionOptions options, CompressionLevel level)
{
	return addDirectory(path, QString(), options, level);
}

/*!
	Convenience method, same as calling Zip::addDirectory(const QString&,const QString&,CompressionOptions,CompressionLevel)
	with the Zip::RelativePaths flag as compression option.
 */
Zip::ErrorCode Zip::addDirectory(const QString& path, const QString& root, CompressionLevel level)
{
	return addDirectory(path, root, Zip::RelativePaths, level);
}

/*!
	Convenience method, same as calling Zip::addDirectory(const QString&,const QString&,CompressionOptions,CompressionLevel)
	with the Zip::IgnorePaths flag as compression option and an empty \p root parameter.
*/
Zip::ErrorCode Zip::addDirectoryContents(const QString& path, CompressionLevel level)
{
	return addDirectory(path, QString(), IgnorePaths, level);
}

/*!
	Convenience method, same as calling Zip::addDirectory(const QString&,const QString&,CompressionOptions,CompressionLevel)
	with the Zip::IgnorePaths flag as compression option.
*/
Zip::ErrorCode Zip::addDirectoryContents(const QString& path, const QString& root, CompressionLevel level)
{
	return addDirectory(path, root, IgnorePaths, level);
}

/*!
	Recursively adds files contained in \p dir to the archive, using \p root as name for the root folder.
	Stops adding files if some error occurs.

	The ExtractionOptions are checked in the order they are defined in the zip.h heaser file.
	This means that the last one overwrites the previous one (if some conflict occurs), i.e.
	Zip::IgnorePaths | Zip::AbsolutePaths would be interpreted as Zip::IgnorePaths.

	The \p root parameter is ignored with the Zip::IgnorePaths parameter and used as path prefix (a trailing /
	is always added as directory separator!) otherwise (even with Zip::AbsolutePaths set!).
*/
Zip::ErrorCode Zip::addDirectory(const QString& path, const QString& root, CompressionOptions options, CompressionLevel level)
{
	// qDebug() << QString("addDir(path=%1, root=%2)").arg(path, root);

	// Bad boy didn't call createArchive() yet :)
	if (d->device == 0)
		return Zip::NoOpenArchive;

	QDir dir(path);
	if (!dir.exists())
		return Zip::FileNotFound;

	// Remove any trailing separator
	QString actualRoot = root.trimmed();

	// Preserve Unix root
	if (actualRoot != "/")
	{
		while (actualRoot.endsWith("/") || actualRoot.endsWith("\\"))
			actualRoot.truncate(actualRoot.length() - 1);
	}

	// QDir::cleanPath() fixes some issues with QDir::dirName()
	QFileInfo current(QDir::cleanPath(path));

	if (!actualRoot.isEmpty() && actualRoot != "/")
		actualRoot.append("/");

	/* This part is quite confusing and needs some test or check */
	/* An attempt to compress the / root directory evtl. using a root prefix should be a good test */
	if (options.testFlag(AbsolutePaths) && !options.testFlag(IgnorePaths))
	{
		QString absolutePath = d->extractRoot(path);
		if (!absolutePath.isEmpty() && absolutePath != "/")
			absolutePath.append("/");
		actualRoot.append(absolutePath);
	}

	if (!options.testFlag(IgnorePaths))
	{
		actualRoot = actualRoot.append(QDir(current.absoluteFilePath()).dirName());
		actualRoot.append("/");
	}

	// actualRoot now contains the path of the file relative to the zip archive
	// with a trailing /

	QFileInfoList list = dir.entryInfoList(
		QDir::Files |
		QDir::Dirs |
		QDir::NoDotAndDotDot |
		QDir::NoSymLinks);

	ErrorCode ec = Zip::Ok;
	bool filesAdded = false;

	CompressionOptions recursionOptions;
	if (options.testFlag(IgnorePaths))
		recursionOptions |= IgnorePaths;
	else recursionOptions |= RelativePaths;

	for (int i = 0; i < list.size() && ec == Zip::Ok; ++i)
	{
		QFileInfo info = list.at(i);

		if (info.isDir())
		{
			// Recursion :)
            progress();
			ec = addDirectory(info.absoluteFilePath(), actualRoot, recursionOptions, level); 
		}
		else
		{
            progress();
			ec = d->createEntry(info, actualRoot, level);
			filesAdded = true;
        }
	}


	// We need an explicit record for this dir
	// Non-empty directories don't need it because they have a path component in the filename
	if (!filesAdded && !options.testFlag(IgnorePaths))
		ec = d->createEntry(current, actualRoot, level);
   
	return ec;
}

/*!
	Closes the archive and writes any pending data.
*/
Zip::ErrorCode Zip::closeArchive()
{
	Zip::ErrorCode ec = d->closeArchive();
	d->reset();
	return ec;
}

/*!
	Returns a locale translated error string for a given error code.
*/
QString Zip::formatError(Zip::ErrorCode c) const
{
	switch (c)
	{
	case Ok: return QCoreApplication::translate("Zip", "ZIP operation completed successfully."); break;
	case ZlibInit: return QCoreApplication::translate("Zip", "Failed to initialize or load zlib library."); break;
	case ZlibError: return QCoreApplication::translate("Zip", "zlib library error."); break;
	case OpenFailed: return QCoreApplication::translate("Zip", "Unable to create or open file."); break;
	case NoOpenArchive: return QCoreApplication::translate("Zip", "No archive has been created yet."); break;
	case FileNotFound: return QCoreApplication::translate("Zip", "File or directory does not exist."); break;
	case ReadFailed: return QCoreApplication::translate("Zip", "File read error."); break;
	case WriteFailed: return QCoreApplication::translate("Zip", "File write error."); break;
	case SeekFailed: return QCoreApplication::translate("Zip", "File seek error."); break;
	default: ;
	}

	return QCoreApplication::translate("Zip", "Unknown error.");
}


/************************************************************************
 Private interface
*************************************************************************/

//! \internal
ZipPrivate::ZipPrivate()
{
	headers = 0;
	device = 0;

	// keep an unsigned pointer so we avoid to over bloat the code with casts
	uBuffer = (unsigned char*) buffer1;
	crcTable = (quint32*) get_crc_table();
}

//! \internal
ZipPrivate::~ZipPrivate()
{
	closeArchive();
}

//! \internal
Zip::ErrorCode ZipPrivate::createArchive(QIODevice* dev)
{
	Q_ASSERT(dev != 0);

	if (device != 0)
		closeArchive();

	device = dev;

	if (!device->isOpen())
	{
		if (!device->open(QIODevice::ReadOnly)) {
			delete device;
			device = 0;
			qDebug() << "Unable to open device for writing.";
			return Zip::OpenFailed;
		}
	}

	headers = new QMap<QString,ZipEntryP*>;
	return Zip::Ok;
}

//! \internal Writes a new entry in the zip file.
Zip::ErrorCode ZipPrivate::createEntry(const QFileInfo& file, const QString& root, Zip::CompressionLevel level)
{
	//! \todo Automatic level detection (cpu, extension & file size)

	// Directories and very small files are always stored
	// (small files would get bigger due to the compression headers overhead)

	// Need this for zlib
	bool isPNGFile = false;
	bool dirOnly = file.isDir();

	QString entryName = root;

	// Directory entry
	if (dirOnly)
		level = Zip::Store;
	else
	{
		entryName.append(file.fileName());

		QString ext = file.completeSuffix().toLower();
		isPNGFile = ext == "png";

		if (file.size() < ZIP_COMPRESSION_THRESHOLD)
			level = Zip::Store;
		else
			switch (level)
			{
			case Zip::AutoCPU:
				level = Zip::Deflate5;
				break;
			case Zip::AutoMIME:
				level = detectCompressionByMime(ext);
				break;
			case Zip::AutoFull:
				level = detectCompressionByMime(ext);
				break;
			default:
				;
			}
	}

	// entryName contains the path as it should be written
	// in the zip file records
	// qDebug() << QString("addDir(file=%1, root=%2, entry=%3)").arg(file.absoluteFilePath(), root, entryName);

	// create header and store it to write a central directory later
	ZipEntryP* h = new ZipEntryP;

	h->compMethod = (level == Zip::Store) ? 0 : 0x0008;

	// Set encryption bit and set the data descriptor bit
	// so we can use mod time instead of crc for password check
	bool encrypt = !dirOnly && !password.isEmpty();
	if (encrypt)
		h->gpFlag[0] |= 9;

	QDateTime dt = file.lastModified();
	QDate d = dt.date();
	h->modDate[1] = ((d.year() - 1980) << 1) & 254;
	h->modDate[1] |= ((d.month() >> 3) & 1);
	h->modDate[0] = ((d.month() & 7) << 5) & 224;
	h->modDate[0] |= d.day();

	QTime t = dt.time();
	h->modTime[1] = (t.hour() << 3) & 248;
	h->modTime[1] |= ((t.minute() >> 3) & 7);
	h->modTime[0] = ((t.minute() & 7) << 5) & 224;
	h->modTime[0] |= t.second() / 2;

	h->szUncomp = dirOnly ? 0 : file.size();

	// **** Write local file header ****

	// signature
	buffer1[0] = 'P'; buffer1[1] = 'K';
	buffer1[2] = 0x3; buffer1[3] = 0x4;

	// version needed to extract
	buffer1[ZIP_LH_OFF_VERS] = ZIP_VERSION;
	buffer1[ZIP_LH_OFF_VERS + 1] = 0;

	// general purpose flag
	buffer1[ZIP_LH_OFF_GPFLAG] = h->gpFlag[0];
	buffer1[ZIP_LH_OFF_GPFLAG + 1] = h->gpFlag[1];

	// compression method
	buffer1[ZIP_LH_OFF_CMET] = h->compMethod & 0xFF;
	buffer1[ZIP_LH_OFF_CMET + 1] = (h->compMethod>>8) & 0xFF;

	// last mod file time
	buffer1[ZIP_LH_OFF_MODT] = h->modTime[0];
	buffer1[ZIP_LH_OFF_MODT + 1] = h->modTime[1];

	// last mod file date
	buffer1[ZIP_LH_OFF_MODD] = h->modDate[0];
	buffer1[ZIP_LH_OFF_MODD + 1] = h->modDate[1];

	// skip crc (4bytes) [14,15,16,17]

	// skip compressed size but include evtl. encryption header (4bytes: [18,19,20,21])
	buffer1[ZIP_LH_OFF_CSIZE] =
	buffer1[ZIP_LH_OFF_CSIZE + 1] =
	buffer1[ZIP_LH_OFF_CSIZE + 2] =
	buffer1[ZIP_LH_OFF_CSIZE + 3] = 0;

	h->szComp = encrypt ? ZIP_LOCAL_ENC_HEADER_SIZE : 0;

	// uncompressed size [22,23,24,25]
	setULong(h->szUncomp, buffer1, ZIP_LH_OFF_USIZE);

	// filename length
	QByteArray entryNameBytes = entryName.toAscii();
	int sz = entryNameBytes.size();

	buffer1[ZIP_LH_OFF_NAMELEN] = sz & 0xFF;
	buffer1[ZIP_LH_OFF_NAMELEN + 1] = (sz >> 8) & 0xFF;

	// extra field length
	buffer1[ZIP_LH_OFF_XLEN] = buffer1[ZIP_LH_OFF_XLEN + 1] = 0;

	// Store offset to write crc and compressed size
	h->lhOffset = device->pos();
	quint32 crcOffset = h->lhOffset + ZIP_LH_OFF_CRC;

	if (device->write(buffer1, ZIP_LOCAL_HEADER_SIZE) != ZIP_LOCAL_HEADER_SIZE)
	{
		delete h;
		return Zip::WriteFailed;
	}

	// Write out filename
	if (device->write(entryNameBytes) != sz)
	{
		delete h;
		return Zip::WriteFailed;
	}

	// Encryption keys
	quint32 keys[3] = { 0, 0, 0 };

	if (encrypt)
	{
		// **** encryption header ****

		// XOR with PI to ensure better random numbers
		// with poorly implemented rand() as suggested by Info-Zip
		srand(time(NULL) ^ 3141592654UL);
		int randByte;

		initKeys(keys);
		for (int i=0; i<10; ++i)
		{
			randByte = (rand() >> 7) & 0xff;
			buffer1[i] = decryptByte(keys[2]) ^ randByte;
			updateKeys(keys, randByte);
		}

		// Encrypt encryption header
		initKeys(keys);
		for (int i=0; i<10; ++i)
		{
			randByte = decryptByte(keys[2]);
			updateKeys(keys, buffer1[i]);
			buffer1[i] ^= randByte;
		}

		// We don't know the CRC at this time, so we use the modification time
		// as the last two bytes
		randByte = decryptByte(keys[2]);
		updateKeys(keys, h->modTime[0]);
		buffer1[10] ^= randByte;

		randByte = decryptByte(keys[2]);
		updateKeys(keys, h->modTime[1]);
		buffer1[11] ^= randByte;

		// Write out encryption header
		if (device->write(buffer1, ZIP_LOCAL_ENC_HEADER_SIZE) != ZIP_LOCAL_ENC_HEADER_SIZE)
		{
			delete h;
			return Zip::WriteFailed;
		}
	}

	qint64 written = 0;
	quint32 crc = crc32(0L, Z_NULL, 0);

	if (!dirOnly)
	{
		QFile actualFile(file.absoluteFilePath());
		if (!actualFile.open(QIODevice::ReadOnly))
		{
			qDebug() << QString("An error occurred while opening %1").arg(file.absoluteFilePath());
			return Zip::OpenFailed;
		}

		// Write file data
		qint64 read = 0;
		qint64 totRead = 0;
		qint64 toRead = actualFile.size();

		if (level == Zip::Store)
		{
			while ( (read = actualFile.read(buffer1, ZIP_READ_BUFFER)) > 0 )
			{
				crc = crc32(crc, uBuffer, read);

				if (password != 0)
					encryptBytes(keys, buffer1, read);

				if ( (written = device->write(buffer1, read)) != read )
				{
					actualFile.close();
					delete h;
					return Zip::WriteFailed;
				}
			}
		}
		else
		{
			z_stream zstr;

			// Initialize zalloc, zfree and opaque before calling the init function
			zstr.zalloc = Z_NULL;
			zstr.zfree = Z_NULL;
			zstr.opaque = Z_NULL;

			int zret;

			// Use deflateInit2 with negative windowBits to get raw compression
			if ((zret = deflateInit2_(
					&zstr,
					(int)level,
					Z_DEFLATED,
					-MAX_WBITS,
					8,
					isPNGFile ? Z_RLE : Z_DEFAULT_STRATEGY,
					ZLIB_VERSION,
					sizeof(z_stream)
				)) != Z_OK )
			{
				actualFile.close();
				qDebug() << "Could not initialize zlib for compression";
				delete h;
				return Zip::ZlibError;
			}

			qint64 compressed;

			int flush = Z_NO_FLUSH;

			do
			{
				read = actualFile.read(buffer1, ZIP_READ_BUFFER);
				totRead += read;

				if (read == 0)
					break;
				if (read < 0)
				{
					actualFile.close();
					deflateEnd(&zstr);
					qDebug() << QString("Error while reading %1").arg(file.absoluteFilePath());
					delete h;
					return Zip::ReadFailed;
				}

				crc = crc32(crc, uBuffer, read);

				zstr.next_in = (Bytef*) buffer1;
				zstr.avail_in = (uInt)read;

				// Tell zlib if this is the last chunk we want to encode
				// by setting the flush parameter to Z_FINISH
				flush = (totRead == toRead) ? Z_FINISH : Z_NO_FLUSH;

				// Run deflate() on input until output buffer not full
				// finish compression if all of source has been read in
        		do
        		{
					zstr.next_out = (Bytef*) buffer2;
					zstr.avail_out = ZIP_READ_BUFFER;

					zret = deflate(&zstr, flush);
					// State not clobbered
					Q_ASSERT(zret != Z_STREAM_ERROR);

					// Write compressed data to file and empty buffer
					compressed = ZIP_READ_BUFFER - zstr.avail_out;

					if (password != 0)
						encryptBytes(keys, buffer2, compressed);

					if (device->write(buffer2, compressed) != compressed)
					{
						deflateEnd(&zstr);
						actualFile.close();
						qDebug() << QString("Error while writing %1").arg(file.absoluteFilePath());
						delete h;
						return Zip::WriteFailed;
					}

					written += compressed;

				} while (zstr.avail_out == 0);

				// All input will be used
				Q_ASSERT(zstr.avail_in == 0);

			} while (flush != Z_FINISH);

			// Stream will be complete
			Q_ASSERT(zret == Z_STREAM_END);

			deflateEnd(&zstr);

		} // if (level != STORE)

		actualFile.close();
	}

	// Store end of entry offset
	quint32 current = device->pos();

	// Update crc and compressed size in local header
	if (!device->seek(crcOffset))
	{
		delete h;
		return Zip::SeekFailed;
	}

	h->crc = dirOnly ? 0 : crc;
	h->szComp += written;

	setULong(h->crc, buffer1, 0);
	setULong(h->szComp, buffer1, 4);
	if ( device->write(buffer1, 8) != 8)
	{
		delete h;
		return Zip::WriteFailed;
	}

	// Seek to end of entry
	if (!device->seek(current))
	{
		delete h;
		return Zip::SeekFailed;
	}

	if ((h->gpFlag[0] & 8) == 8)
	{
		// Write data descriptor

		// Signature: PK\7\8
		buffer1[0] = 'P';
		buffer1[1] = 'K';
		buffer1[2] = 0x07;
		buffer1[3] = 0x08;

		// CRC
		setULong(h->crc, buffer1, ZIP_DD_OFF_CRC32);

		// Compressed size
		setULong(h->szComp, buffer1, ZIP_DD_OFF_CSIZE);

		// Uncompressed size
		setULong(h->szUncomp, buffer1, ZIP_DD_OFF_USIZE);

		if (device->write(buffer1, ZIP_DD_SIZE_WS) != ZIP_DD_SIZE_WS)
		{
			delete h;
			return Zip::WriteFailed;
		}
	}

	headers->insert(entryName, h);
	return Zip::Ok;
}

//! \internal
int ZipPrivate::decryptByte(quint32 key2) const
{
	quint16 temp = ((quint16)(key2) & 0xffff) | 2;
	return (int)(((temp * (temp ^ 1)) >> 8) & 0xff);
}

//! \internal Writes an quint32 (4 bytes) to a byte array at given offset.
void ZipPrivate::setULong(quint32 v, char* buffer, unsigned int offset)
{
	buffer[offset+3] = ((v >> 24) & 0xFF);
	buffer[offset+2] = ((v >> 16) & 0xFF);
	buffer[offset+1] = ((v >> 8) & 0xFF);
	buffer[offset] = (v & 0xFF);
}

//! \internal Initializes decryption keys using a password.
void ZipPrivate::initKeys(quint32* keys) const
{
	// Encryption keys initialization constants are taken from the
	// PKZip file format specification docs
	keys[0] = 305419896L;
	keys[1] = 591751049L;
	keys[2] = 878082192L;

	QByteArray pwdBytes = password.toAscii();
	int sz = pwdBytes.size();
	const char* ascii = pwdBytes.data();

	for (int i=0; i<sz; ++i)
		updateKeys(keys, (int)ascii[i]);
}

//! \internal Updates encryption keys.
void ZipPrivate::updateKeys(quint32* keys, int c) const
{
	keys[0] = CRC32(keys[0], c);
	keys[1] += keys[0] & 0xff;
	keys[1] = keys[1] * 134775813L + 1;
	keys[2] = CRC32(keys[2], ((int)keys[1]) >> 24);
}

//! \internal Encrypts a byte array.
void ZipPrivate::encryptBytes(quint32* keys, char* buffer, qint64 read)
{
	char t;

	for (int i=0; i<(int)read; ++i)
	{
		t = buffer[i];
		buffer[i] ^= decryptByte(keys[2]);
		updateKeys(keys, t);
	}
}

//! \internal Detects the best compression level for a given file extension.
Zip::CompressionLevel ZipPrivate::detectCompressionByMime(const QString& ext)
{
	// files really hard to compress
	if ((ext == "png") ||
		(ext == "jpg") ||
		(ext == "jpeg") ||
		(ext == "mp3") ||
		(ext == "ogg") ||
		(ext == "ogm") ||
		(ext == "avi") ||
		(ext == "mov") ||
		(ext == "rm") ||
		(ext == "ra") ||
		(ext == "zip") ||
		(ext == "rar") ||
		(ext == "bz2") ||
		(ext == "gz") ||
		(ext == "7z") ||
		(ext == "z") ||
		(ext == "jar")
	) return Zip::Store;

	// files slow and hard to compress
	if ((ext == "exe") ||
		(ext == "bin") ||
		(ext == "rpm") ||
		(ext == "deb")
	) return Zip::Deflate2;

	return Zip::Deflate9;
}

/*!
	Closes the current archive and writes out pending data.
*/
Zip::ErrorCode ZipPrivate::closeArchive()
{
	// Close current archive by writing out central directory
	// and free up resources

	if (device == 0)
		return Zip::Ok;

	if (headers == 0)
		return Zip::Ok;

	const ZipEntryP* h;

	unsigned int sz;
	quint32 szCentralDir = 0;
	quint32 offCentralDir = device->pos();

	for (QMap<QString,ZipEntryP*>::ConstIterator itr = headers->constBegin(); itr != headers->constEnd(); ++itr)
	{
		h = itr.value();

		// signature
		buffer1[0] = 'P';
		buffer1[1] = 'K';
		buffer1[2] = 0x01;
		buffer1[3] = 0x02;

		// version made by  (currently only MS-DOS/FAT - no symlinks or other stuff supported)
		buffer1[ZIP_CD_OFF_MADEBY] = buffer1[ZIP_CD_OFF_MADEBY + 1] = 0;

		// version needed to extract
		buffer1[ZIP_CD_OFF_VERSION] = ZIP_VERSION;
		buffer1[ZIP_CD_OFF_VERSION + 1] = 0;

		// general purpose flag
		buffer1[ZIP_CD_OFF_GPFLAG] = h->gpFlag[0];
		buffer1[ZIP_CD_OFF_GPFLAG + 1] = h->gpFlag[1];

		// compression method
		buffer1[ZIP_CD_OFF_CMET] = h->compMethod & 0xFF;
		buffer1[ZIP_CD_OFF_CMET + 1] = (h->compMethod >> 8) & 0xFF;

		// last mod file time
		buffer1[ZIP_CD_OFF_MODT] = h->modTime[0];
		buffer1[ZIP_CD_OFF_MODT + 1] = h->modTime[1];

		// last mod file date
		buffer1[ZIP_CD_OFF_MODD] = h->modDate[0];
		buffer1[ZIP_CD_OFF_MODD + 1] = h->modDate[1];

		// crc (4bytes) [16,17,18,19]
		setULong(h->crc, buffer1, ZIP_CD_OFF_CRC);

		// compressed size (4bytes: [20,21,22,23])
		setULong(h->szComp, buffer1, ZIP_CD_OFF_CSIZE);

		// uncompressed size [24,25,26,27]
		setULong(h->szUncomp, buffer1, ZIP_CD_OFF_USIZE);

		// filename
		QByteArray fileNameBytes = itr.key().toAscii();
		sz = fileNameBytes.size();
		buffer1[ZIP_CD_OFF_NAMELEN] = sz & 0xFF;
		buffer1[ZIP_CD_OFF_NAMELEN + 1] = (sz >> 8) & 0xFF;

		// extra field length
		buffer1[ZIP_CD_OFF_XLEN] = buffer1[ZIP_CD_OFF_XLEN + 1] = 0;

		// file comment length
		buffer1[ZIP_CD_OFF_COMMLEN] = buffer1[ZIP_CD_OFF_COMMLEN + 1] = 0;

		// disk number start
		buffer1[ZIP_CD_OFF_DISKSTART] = buffer1[ZIP_CD_OFF_DISKSTART + 1] = 0;

		// internal file attributes
		buffer1[ZIP_CD_OFF_IATTR] = buffer1[ZIP_CD_OFF_IATTR + 1] = 0;

		// external file attributes
		buffer1[ZIP_CD_OFF_EATTR] =
		buffer1[ZIP_CD_OFF_EATTR + 1] =
		buffer1[ZIP_CD_OFF_EATTR + 2] =
		buffer1[ZIP_CD_OFF_EATTR + 3] = 0;

		// relative offset of local header [42->45]
		setULong(h->lhOffset, buffer1, ZIP_CD_OFF_LHOFF);

		if (device->write(buffer1, ZIP_CD_SIZE) != ZIP_CD_SIZE)
		{
			//! \todo See if we can detect QFile objects using the Qt Meta Object System
			/*
			if (!device->remove())
				qDebug() << tr("Unable to delete corrupted archive: %1").arg(device->fileName());
			*/
			return Zip::WriteFailed;
		}

		// Write out filename
		if ((unsigned int)device->write(fileNameBytes) != sz)
		{
			//! \todo SAME AS ABOVE: See if we can detect QFile objects using the Qt Meta Object System
			/*
			if (!device->remove())
				qDebug() << tr("Unable to delete corrupted archive: %1").arg(device->fileName());
				*/
			return Zip::WriteFailed;
		}

		szCentralDir += (ZIP_CD_SIZE + sz);

	} // central dir headers loop


	// Write end of central directory

	// signature
	buffer1[0] = 'P';
	buffer1[1] = 'K';
	buffer1[2] = 0x05;
	buffer1[3] = 0x06;

	// number of this disk
	buffer1[ZIP_EOCD_OFF_DISKNUM] = buffer1[ZIP_EOCD_OFF_DISKNUM + 1] = 0;

	// number of disk with central directory
	buffer1[ZIP_EOCD_OFF_CDDISKNUM] = buffer1[ZIP_EOCD_OFF_CDDISKNUM + 1] = 0;

	// number of entries in this disk
	sz = headers->count();
    buffer1[ZIP_EOCD_OFF_ENTRIES] = sz & 0xFF;
	buffer1[ZIP_EOCD_OFF_ENTRIES + 1] = (sz >> 8) & 0xFF;

	// total number of entries
	buffer1[ZIP_EOCD_OFF_CDENTRIES] = buffer1[ZIP_EOCD_OFF_ENTRIES];
	buffer1[ZIP_EOCD_OFF_CDENTRIES + 1] = buffer1[ZIP_EOCD_OFF_ENTRIES + 1];

	// size of central directory [12->15]
	setULong(szCentralDir, buffer1, ZIP_EOCD_OFF_CDSIZE);

	// central dir offset [16->19]
	setULong(offCentralDir, buffer1, ZIP_EOCD_OFF_CDOFF);

	// ZIP file comment length
	QByteArray commentBytes = comment.toAscii();
	quint16 commentLength = commentBytes.size();

	if (commentLength == 0)
	{
		buffer1[ZIP_EOCD_OFF_COMMLEN] = buffer1[ZIP_EOCD_OFF_COMMLEN + 1] = 0;
	}
	else
	{
		buffer1[ZIP_EOCD_OFF_COMMLEN] = commentLength & 0xFF;
		buffer1[ZIP_EOCD_OFF_COMMLEN + 1] = (commentLength >> 8) & 0xFF;
	}

	if (device->write(buffer1, ZIP_EOCD_SIZE) != ZIP_EOCD_SIZE)
	{
		//! \todo SAME AS ABOVE: See if we can detect QFile objects using the Qt Meta Object System
		/*
		if (!device->remove())
			qDebug() << tr("Unable to delete corrupted archive: %1").arg(device->fileName());
			*/
		return Zip::WriteFailed;
	}

	if (commentLength != 0)
	{
		if ((unsigned int)device->write(commentBytes) != commentLength)
		{
			//! \todo SAME AS ABOVE: See if we can detect QFile objects using the Qt Meta Object System
			/*
			if (!device->remove())
				qDebug() << tr("Unable to delete corrupted archive: %1").arg(device->fileName());
				*/
			return Zip::WriteFailed;
		}
	}

	return Zip::Ok;
}

//! \internal
void ZipPrivate::reset()
{
	comment.clear();

	if (headers != 0)
	{
		qDeleteAll(*headers);
		delete headers;
		headers = 0;
	}

	delete device; device = 0;
}

//! \internal Returns the path of the parent directory
QString ZipPrivate::extractRoot(const QString& p)
{
	QDir d(QDir::cleanPath(p));
	if (!d.exists())
		return QString();

	if (!d.cdUp())
		return QString();

	return d.absolutePath();
}
