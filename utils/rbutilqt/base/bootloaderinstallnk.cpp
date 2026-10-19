/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2026 by Michael Giacomelli
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QtCore>
#include "bootloaderinstallbase.h"
#include "bootloaderinstallnk.h"
#include "mknkboot.h"
#include "Logger.h"

#if defined(HAVE_LIBMTP)
#include <libmtp.h>
#elif defined(Q_OS_WIN32)
/* MinGW's libportabledeviceguids only has the types' GUIDs: define the
 * rest here. */
#include <initguid.h>
#include <windows.h>
#include <portabledeviceapi.h>
#include <portabledevice.h>
#endif

/* USB IDs of the Gigabeat S. Rockbox uses the same ones, but only the
 * original firmware offers MTP. */
#define NK_USB_VID 0x0930
#define NK_USB_PID 0x0010


bool BootloaderThreadNk::supported(void)
{
#if defined(HAVE_LIBMTP) || defined(Q_OS_WIN32)
    return true;
#else
    return false;
#endif
}


#if !defined(HAVE_LIBMTP) && defined(Q_OS_WIN32)
/* WPD's object format GUIDs carry the MTP format code in the top half of
 * the first field. 0xB802 is MTP's "undefined firmware", which the
 * original firmware takes as an update. */
static const GUID nkFormatFirmware = { 0xB8020000, 0xAE6C, 0x4804,
    { 0x98, 0xBA, 0xC5, 0x7B, 0x46, 0x96, 0x5F, 0xE7 } };

template <class T> static void wpdRelease(T *&p)
{
    if(p) {
        p->Release();
        p = nullptr;
    }
}

static QString wpdError(const QString& what, HRESULT hr)
{
    return QString("%1 (0x%2)").arg(what)
        .arg((unsigned long)hr, 8, 16, QChar('0'));
}

/* The device ID of the first Gigabeat S, found by its USB IDs. */
static QString wpdFindDevice(IPortableDeviceManager *manager,
                             QString *name, QString *error)
{
    DWORD count = 0;
    QString found;
    QString want = QString("vid_%1&pid_%2")
        .arg(NK_USB_VID, 4, 16, QChar('0'))
        .arg(NK_USB_PID, 4, 16, QChar('0'));

    manager->RefreshDeviceList();
    HRESULT hr = manager->GetDevices(nullptr, &count);
    if(FAILED(hr) || count == 0) {
        *error = BootloaderThreadNk::tr("No MTP device found.");
        return QString();
    }
    PWSTR *ids = new PWSTR[count]();
    hr = manager->GetDevices(ids, &count);
    for(DWORD i = 0; SUCCEEDED(hr) && i < count; i++) {
        QString id = QString::fromWCharArray(ids[i]);
        LOG_INFO() << "WPD device" << i << id;
        if(found.isEmpty() && id.toLower().contains(want)) {
            found = id;
            DWORD len = 0;
            manager->GetDeviceFriendlyName(ids[i], nullptr, &len);
            if(len > 0) {
                WCHAR *buf = new WCHAR[len]();
                if(SUCCEEDED(manager->GetDeviceFriendlyName(ids[i], buf, &len)))
                    *name = QString::fromWCharArray(buf);
                delete[] buf;
            }
        }
    }
    for(DWORD i = 0; i < count; i++)
        CoTaskMemFree(ids[i]);
    delete[] ids;
    if(found.isEmpty())
        *error = BootloaderThreadNk::tr("No Gigabeat S found among the MTP "
                                        "devices.");
    return found;
}

/* The object ID of the device's first storage. */
static QString wpdFirstStorage(IPortableDeviceContent *content)
{
    IEnumPortableDeviceObjectIDs *objects = nullptr;
    IPortableDeviceProperties *props = nullptr;
    IPortableDeviceKeyCollection *keys = nullptr;
    QString storage;

    if(FAILED(content->EnumObjects(0, WPD_DEVICE_OBJECT_ID, nullptr,
                                   &objects))
       || FAILED(content->Properties(&props))
       || FAILED(CoCreateInstance(CLSID_PortableDeviceKeyCollection, nullptr,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IPortableDeviceKeyCollection,
                                  (void **)&keys))) {
        wpdRelease(objects);
        wpdRelease(props);
        return storage;
    }
    keys->Add(WPD_FUNCTIONAL_OBJECT_CATEGORY);

    PWSTR id = nullptr;
    ULONG fetched = 0;
    while(storage.isEmpty()
          && objects->Next(1, &id, &fetched) == S_OK && fetched == 1) {
        IPortableDeviceValues *values = nullptr;
        GUID category;
        if(SUCCEEDED(props->GetValues(id, keys, &values))
           && SUCCEEDED(values->GetGuidValue(WPD_FUNCTIONAL_OBJECT_CATEGORY,
                                             &category))
           && IsEqualGUID(category, WPD_FUNCTIONAL_CATEGORY_STORAGE)) {
            storage = QString::fromWCharArray(id);
        }
        wpdRelease(values);
        CoTaskMemFree(id);
    }
    wpdRelease(keys);
    wpdRelease(props);
    wpdRelease(objects);
    return storage;
}
#endif


#if defined(HAVE_LIBMTP)
struct nk_progress {
    BootloaderThreadNk *thread;
    uint64_t sent;
    uint64_t total;
};

static int nkProgress(uint64_t const sent, uint64_t const total,
                      void const *const data)
{
    struct nk_progress *p = (struct nk_progress *)data;
    p->sent = sent;
    p->total = total;
    emit p->thread->progress((int)sent, (int)total);
    return 0;
}
#endif


void BootloaderThreadNk::run(void)
{
    m_sent = false;
#if defined(HAVE_LIBMTP)
    LIBMTP_raw_device_t *raw = nullptr;
    int count = 0;
    int found = -1;

    LIBMTP_Init();
    LIBMTP_error_number_t err = LIBMTP_Detect_Raw_Devices(&raw, &count);
    if(err == LIBMTP_ERROR_NO_DEVICE_ATTACHED || count == 0) {
        m_error = tr("No MTP device found.");
        free(raw);
        return;
    }
    if(err != LIBMTP_ERROR_NONE) {
        m_error = tr("Looking for MTP devices failed (error %1).").arg(err);
        free(raw);
        return;
    }
    for(int i = 0; i < count; i++) {
        LOG_INFO() << "MTP device" << i
                   << QString("%1:%2")
                        .arg(raw[i].device_entry.vendor_id, 4, 16, QChar('0'))
                        .arg(raw[i].device_entry.product_id, 4, 16, QChar('0'))
                   << raw[i].device_entry.vendor << raw[i].device_entry.product;
        if(raw[i].device_entry.vendor_id == NK_USB_VID
           && raw[i].device_entry.product_id == NK_USB_PID)
            found = i;
    }
    if(found < 0) {
        m_error = tr("No Gigabeat S found among the MTP devices.");
        free(raw);
        return;
    }

    LIBMTP_mtpdevice_t *dev = LIBMTP_Open_Raw_Device_Uncached(&raw[found]);
    if(dev == nullptr) {
        m_error = tr("Could not open the player over MTP.");
        free(raw);
        return;
    }
    char *model = LIBMTP_Get_Modelname(dev);
    if(model) {
        m_device = QString::fromUtf8(model);
        free(model);
    }

    LIBMTP_file_t *file = LIBMTP_new_file_t();
    file->filetype = LIBMTP_FILETYPE_FIRMWARE;
    file->filename = strdup("nk.bin");
    file->filesize = QFileInfo(m_file).size();
    file->parent_id = 0;
    file->storage_id = 0;

    struct nk_progress p = { this, 0, 0 };
    int ret = LIBMTP_Send_File_From_File(dev,
            QFile::encodeName(m_file).constData(), file, nkProgress, &p);
    if(ret != 0) {
        LIBMTP_error_t *e = LIBMTP_Get_Errorstack(dev);
        for(; e != nullptr; e = e->next) {
            if(!m_error.isEmpty())
                m_error += " ";
            m_error += QString::fromUtf8(e->error_text);
        }
        LIBMTP_Clear_Errorstack(dev);
        LOG_ERROR() << "MTP send returned" << ret << m_error;
    }
    /* The player may report an error once it has the file, as it goes off
     * to install it: what counts is that all of it was sent. */
    m_sent = (ret == 0) || (p.total > 0 && p.sent >= p.total);

    LIBMTP_destroy_file_t(file);
    LIBMTP_Release_Device(dev);
    free(raw);
#elif defined(Q_OS_WIN32)
    IPortableDeviceManager *manager = nullptr;
    IPortableDevice *dev = nullptr;
    IPortableDeviceValues *client = nullptr;
    IPortableDeviceValues *values = nullptr;
    IPortableDeviceContent *content = nullptr;
    IStream *stream = nullptr;
    QString id, storage;
    QFile in(m_file);
    DWORD chunk = 0;
    qint64 total = in.size();
    qint64 done = 0;
    HRESULT hr;

    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if(FAILED(hr)) {
        m_error = wpdError(tr("Could not start COM"), hr);
        return;
    }

    hr = CoCreateInstance(CLSID_PortableDeviceManager, nullptr,
                          CLSCTX_INPROC_SERVER, IID_IPortableDeviceManager,
                          (void **)&manager);
    if(FAILED(hr)) {
        m_error = wpdError(tr("Could not start Windows Portable Devices"), hr);
        goto out;
    }
    id = wpdFindDevice(manager, &m_device, &m_error);
    if(id.isEmpty())
        goto out;

    hr = CoCreateInstance(CLSID_PortableDeviceValues, nullptr,
                          CLSCTX_INPROC_SERVER, IID_IPortableDeviceValues,
                          (void **)&client);
    if(SUCCEEDED(hr)) {
        client->SetStringValue(WPD_CLIENT_NAME, L"Rockbox Utility");
        client->SetUnsignedIntegerValue(WPD_CLIENT_MAJOR_VERSION, 1);
        client->SetUnsignedIntegerValue(WPD_CLIENT_MINOR_VERSION, 0);
        client->SetUnsignedIntegerValue(WPD_CLIENT_REVISION, 0);
        client->SetUnsignedIntegerValue(WPD_CLIENT_SECURITY_QUALITY_OF_SERVICE,
                                        SECURITY_IMPERSONATION);
        hr = CoCreateInstance(CLSID_PortableDeviceFTM, nullptr,
                              CLSCTX_INPROC_SERVER, IID_IPortableDevice,
                              (void **)&dev);
    }
    if(SUCCEEDED(hr))
        hr = dev->Open((LPCWSTR)id.utf16(), client);
    if(FAILED(hr)) {
        m_error = wpdError(tr("Could not open the player over MTP."), hr);
        goto out;
    }

    hr = dev->Content(&content);
    if(SUCCEEDED(hr))
        storage = wpdFirstStorage(content);
    if(storage.isEmpty()) {
        m_error = tr("The player shows no storage over MTP.");
        goto out;
    }

    hr = CoCreateInstance(CLSID_PortableDeviceValues, nullptr,
                          CLSCTX_INPROC_SERVER, IID_IPortableDeviceValues,
                          (void **)&values);
    if(FAILED(hr)) {
        m_error = wpdError(tr("Could not prepare the transfer"), hr);
        goto out;
    }
    values->SetStringValue(WPD_OBJECT_PARENT_ID, (LPCWSTR)storage.utf16());
    values->SetStringValue(WPD_OBJECT_NAME, L"nk.bin");
    values->SetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME, L"nk.bin");
    values->SetUnsignedLargeIntegerValue(WPD_OBJECT_SIZE, total);
    values->SetGuidValue(WPD_OBJECT_CONTENT_TYPE, WPD_CONTENT_TYPE_GENERIC_FILE);
    values->SetGuidValue(WPD_OBJECT_FORMAT, nkFormatFirmware);

    hr = content->CreateObjectWithPropertiesAndData(values, &stream, &chunk,
                                                    nullptr);
    if(FAILED(hr)) {
        m_error = wpdError(tr("The player refused the firmware file"), hr);
        goto out;
    }
    if(!in.open(QIODevice::ReadOnly)) {
        m_error = tr("Could not read the bootloader image");
        goto out;
    }
    if(chunk == 0)
        chunk = 0x10000;
    while(done < total) {
        QByteArray data = in.read(chunk);
        ULONG written = 0;
        if(data.isEmpty())
            break;
        hr = stream->Write(data.constData(), data.size(), &written);
        if(FAILED(hr) || written != (ULONG)data.size()) {
            m_error = wpdError(tr("Sending the file failed"), hr);
            goto out;
        }
        done += written;
        emit progress((int)done, (int)total);
    }
    in.close();
    hr = stream->Commit(STGC_DEFAULT);
    /* As with libmtp: the player may report an error once it has the
     * whole file. */
    if(FAILED(hr))
        m_error = wpdError(tr("Finishing the transfer reported an error"), hr);
    m_sent = (done == total);

out:
    wpdRelease(stream);
    wpdRelease(values);
    wpdRelease(content);
    if(dev)
        dev->Close();
    wpdRelease(dev);
    wpdRelease(client);
    wpdRelease(manager);
    CoUninitialize();
#else
    m_error = tr("Installing over MTP is not supported on this system yet.");
#endif
}


BootloaderInstallNk::BootloaderInstallNk(QObject *parent)
        : BootloaderInstallBase(parent)
{
    m_thread = nullptr;
}


/** Start bootloader installation.
 */
bool BootloaderInstallNk::install(void)
{
    if(!BootloaderThreadNk::supported()) {
        emit logItem(tr("Installing the bootloader on this player is not "
                        "supported on this system yet."), LOGERROR);
        emit done(true);
        return false;
    }

    LOG_INFO() << "downloading bootloader";
    emit logItem(tr("Downloading bootloader file"), LOGINFO);
    connect(this, &BootloaderInstallBase::downloadDone,
            this, &BootloaderInstallNk::installStage2);
    downloadBlStart(m_blurl);
    return true;
}


void BootloaderInstallNk::installStage2(void)
{
    struct filebuf boot, nk;

    emit logItem(tr("Creating the bootloader image"), LOGINFO);
    m_tempfile.open();
    QByteArray bootdata = m_tempfile.readAll();
    m_tempfile.close();
    if(bootdata.isEmpty()) {
        emit logItem(tr("Could not read the bootloader file"), LOGERROR);
        emit done(true);
        return;
    }

    boot.len = bootdata.size();
    boot.buf = (unsigned char *)bootdata.data();
    if(mknkboot_single(&boot, &nk) != 0) {
        emit logItem(tr("Could not create the bootloader image"), LOGERROR);
        emit done(true);
        return;
    }
    bool written = m_nkfile.open()
        && m_nkfile.write((const char *)nk.buf, nk.len) == nk.len;
    m_nkfile.close();
    free(nk.buf);
    if(!written) {
        emit logItem(tr("Could not write the bootloader image"), LOGERROR);
        emit done(true);
        return;
    }

    emit logItem(tr("Sending the bootloader to the player"), LOGINFO);
    m_thread = new BootloaderThreadNk();
    m_thread->setFile(m_nkfile.fileName());
    connect(m_thread, &BootloaderThreadNk::progress,
            this, &BootloaderInstallBase::logProgress);
    connect(m_thread, &QThread::finished,
            this, &BootloaderInstallNk::installStage3);
    m_thread->start();
}


void BootloaderInstallNk::installStage3(void)
{
    if(!m_thread->sent()) {
        if(!m_thread->error().isEmpty())
            emit logItem(m_thread->error(), LOGERROR);
        emit logItem(tr("Sending the bootloader failed"), LOGERROR);
        emit done(true);
    }
    else {
        if(!m_thread->device().isEmpty())
            emit logItem(tr("Bootloader sent to %1")
                    .arg(m_thread->device()), LOGOK);
        if(!m_thread->error().isEmpty())
            emit logItem(tr("The player reported after the transfer: %1")
                    .arg(m_thread->error()), LOGWARNING);
        emit logItem(tr("Bootloader successful installed"), LOGOK);
        /* There is no drive to keep the installation log on yet. */
        emit done(false);
    }
    delete m_thread;
    m_thread = nullptr;
}


bool BootloaderInstallNk::uninstall(void)
{
    emit logItem(tr("To uninstall, copy an unmodified original nk.bin to "
                    "the firmware partition."), LOGINFO);
    emit done(true);
    return false;
}


BootloaderInstallBase::BootloaderType BootloaderInstallNk::installed(void)
{
    return BootloaderUnknown;
}


BootloaderInstallBase::Capabilities BootloaderInstallNk::capabilities(void)
{
    return (Install | NoMountpoint);
}
