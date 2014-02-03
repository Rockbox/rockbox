#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "backend.h"

/**
 * Backend
 */

Backend::Backend()
{
}

QStringList Backend::GetSocNameList()
{
    QStringList sl;
    for(size_t i = 0; i < m_socs.size(); i++)
        sl.append(QString(m_socs[i].name.c_str()));
    return sl;
}

bool Backend::GetSocByName(const QString& name, soc_t& s)
{
    for(size_t i = 0; i < m_socs.size(); i++)
        if(m_socs[i].name == name.toStdString())
        {
            s = m_socs[i];
            return true;
        }
    return false;
}

bool Backend::LoadSocDesc(const QString& filename)
{
    bool ret = soc_desc_parse_xml(filename.toStdString(), m_socs);
    emit OnSocListChanged();
    return ret;
}

IoBackend *Backend::CreateFileIoBackend(const QString& filename)
{
    return new FileIoBackend(filename);
}

IoBackend *Backend::CreateDummyIoBackend()
{
    return new DummyIoBackend();
}

#ifdef HAVE_HWSTUB
IoBackend *Backend::CreateHWStubIoBackend(HWStubDevice *dev)
{
    return new HWStubIoBackend(dev);
}
#endif

/**
 * FileIoBackend
 */

FileIoBackend::FileIoBackend(const QString& filename)
{
    m_filename = filename;
    Reload();
}

QString FileIoBackend::GetSocName()
{
    return m_soc;
}

bool FileIoBackend::ReadRegister(const QString& name, soc_word_t& value)
{
    if(m_map.find(name) == m_map.end())
        return false;
    value = m_map[name];
    return true;
}

bool FileIoBackend::Reload()
{
    QFile file(m_filename);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    m_map.clear();

    QTextStream in(&file);
    while(!in.atEnd())
    {
        QString line = in.readLine();
        int idx = line.indexOf('=');
        if(idx == -1)
            continue;
        QString key = line.left(idx).trimmed();
        bool ok;
        soc_word_t val = line.mid(idx + 1).trimmed().toULong(&ok, 0);
        if(key == "HW")
            m_soc = line.mid(idx + 1).trimmed();
        else if(ok)
            m_map[key] = val;
    }
    return true;
}

#ifdef HAVE_HWSTUB
/**
 * HWStubDevice
 */
HWStubDevice::HWStubDevice(struct libusb_device *dev)
{
    libusb_ref_device(dev);
    m_dev = dev;
    m_handle = 0;
    m_hwdev = 0;
    m_valid = Probe();
}

HWStubDevice::~HWStubDevice()
{
    Close();
    libusb_unref_device(m_dev);
}

int HWStubDevice::GetBusNumber()
{
    return libusb_get_bus_number(m_dev);
}

int HWStubDevice::GetDevAddress()
{
    return libusb_get_device_address(m_dev);
}

bool HWStubDevice::Probe()
{
    struct libusb_device_descriptor desc;
    if(libusb_get_device_descriptor(m_dev, &desc))
        return false;
    if(desc.idVendor != HWSTUB_USB_VID || desc.idProduct != HWSTUB_USB_PID)
        return false;
    if(!Open())
        return false;
    int ret = hwstub_get_desc(m_hwdev, HWSTUB_DT_VERSION, &m_hwdev_ver, sizeof(m_hwdev_ver));
    if(ret != sizeof(m_hwdev_ver))
        goto Lerr;
    if(m_hwdev_ver.bMajor != HWSTUB_VERSION_MAJOR || m_hwdev_ver.bMinor < HWSTUB_VERSION_MINOR)
        goto Lerr;
    // get target
    ret = hwstub_get_desc(m_hwdev, HWSTUB_DT_TARGET, &m_hwdev_target, sizeof(m_hwdev_target));
    if(ret != sizeof(m_hwdev_target))
        goto Lerr;
    // get STMP information
    if(m_hwdev_target.dID == HWSTUB_TARGET_STMP)
    {
        ret = hwstub_get_desc(m_hwdev, HWSTUB_DT_STMP, &m_hwdev_stmp, sizeof(m_hwdev_stmp));
        if(ret != sizeof(m_hwdev_stmp))
            goto Lerr;
    }
    Close();
    return true;

    Lerr:
    Close();
    return false;
}

bool HWStubDevice::Open()
{
    if(libusb_open(m_dev, &m_handle))
        return false;
    m_hwdev = hwstub_open(m_handle);
    if(m_hwdev == 0)
    {
        libusb_close(m_handle);
        return false;
    }
    return true;
}

void HWStubDevice::Close()
{
    if(m_hwdev)
        hwstub_release(m_hwdev);
    m_hwdev = 0;
    if(m_handle)
        libusb_close(m_handle);
    m_handle = 0;
}

bool HWStubDevice::ReadMem(soc_addr_t addr, size_t length, void *buffer)
{
    if(!m_hwdev)
        return false;
    int ret = hwstub_rw_mem(m_hwdev, 1, addr, buffer, length);
    return ret >= 0 && (size_t)ret == length;
}

bool HWStubDevice::IsValid()
{
    return m_valid;
}


/**
 * HWStubIoBackend
 */

HWStubIoBackend::HWStubIoBackend(HWStubDevice *dev)
{
    m_dev = dev;
    m_dev->Open();
    struct hwstub_target_desc_t target = m_dev->GetTargetInfo();
    if(target.dID == HWSTUB_TARGET_STMP)
    {
        struct hwstub_stmp_desc_t stmp = m_dev->GetSTMPInfo();
        if(stmp.wChipID == 0x3780)
            m_soc = "imx233";
        else if(stmp.wChipID >= 0x3700 && stmp.wChipID < 0x3780)
            m_soc = "stmp3700";
        else if(stmp.wChipID >= 0x3600 && stmp.wChipID < 0x3700)
            m_soc = "stmp3600";
        else
            m_soc = QString("stmp%1").arg(stmp.wChipID, 4, 16, QChar('0'));
    }
    else if(target.dID == HWSTUB_TARGET_RK27)
        m_soc = "rk27x";
    else
        m_soc = target.bName;
}

QString HWStubIoBackend::GetSocName()
{
    return m_soc;
}

HWStubIoBackend::~HWStubIoBackend()
{
    m_dev->Close();
}

bool HWStubIoBackend::ReadRegister(soc_addr_t addr, soc_word_t& value)
{
    return m_dev->ReadMem(addr, sizeof(value), &value);
}

bool HWStubIoBackend::Reload()
{
    return true;
}

/**
 * HWStubBackendHelper
 */
HWStubBackendHelper::HWStubBackendHelper()
{
    libusb_init(NULL);
#ifdef LIBUSB_NO_HOTPLUG
    m_hotplug = false;
#else
    m_hotplug = libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG);
    if(m_hotplug)
    {
        m_hotplug = LIBUSB_SUCCESS == libusb_hotplug_register_callback(
            NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
            LIBUSB_HOTPLUG_ENUMERATE, HWSTUB_USB_VID, HWSTUB_USB_PID, HWSTUB_CLASS,
            &HWStubBackendHelper::HotPlugCallback, reinterpret_cast< void* >(this), &m_hotplug_handle);
    }
#endif
}

HWStubBackendHelper::~HWStubBackendHelper()
{
#ifndef LIBUSB_NO_HOTPLUG
    if(m_hotplug)
        libusb_hotplug_deregister_callback(NULL, m_hotplug_handle);
#endif
}

QList< HWStubDevice* > HWStubBackendHelper::GetDevList()
{
    QList< HWStubDevice* > list;
    libusb_device **dev_list;
    ssize_t cnt = libusb_get_device_list(NULL, &dev_list);
    for(int i = 0; i < cnt; i++)
    {
        HWStubDevice *dev = new HWStubDevice(dev_list[i]);
        /* filter out non-hwstub devices */
        if(dev->IsValid())
            list.push_back(dev);
        else
            delete dev;
    }
    libusb_free_device_list(dev_list, 1);
    return list;
}

#ifndef LIBUSB_NO_HOTPLUG
void HWStubBackendHelper::OnHotPlug(bool arrived, struct libusb_device *dev)
{
    /* signal it */
    emit OnDevListChanged(arrived, dev);
}

int HWStubBackendHelper::HotPlugCallback(struct libusb_context *ctx, struct libusb_device *dev,
    libusb_hotplug_event event, void *user_data)
{
    HWStubBackendHelper *helper = reinterpret_cast< HWStubBackendHelper* >(user_data);
    switch(event)
    {
        case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: helper->OnHotPlug(true, dev); break;
        case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: helper->OnHotPlug(false, dev); break;
        default: break;
    }
    return 0;
}
#endif

bool HWStubBackendHelper::HasHotPlugSupport()
{
    return m_hotplug;
}

#endif

/**
 * BackendHelper
 */

BackendHelper::BackendHelper(IoBackend *io_backend, const soc_t& soc)
    :m_io_backend(io_backend), m_soc(soc)
{
}

bool BackendHelper::ReadRegister(const QString& dev, const QString& reg, soc_word_t& v)
{
    if(m_io_backend->SupportAccess(IoBackend::ByName))
        return m_io_backend->ReadRegister("HW." + dev + "." + reg, v);
    if(m_io_backend->SupportAccess(IoBackend::ByAddress))
    {
        soc_addr_t addr;
        if(GetRegisterAddress(dev, reg, addr))
            return m_io_backend->ReadRegister(addr, v);
    }
    return false;
}


bool BackendHelper::GetDeviceDesc(const QString& dev, soc_dev_t& dev_desc, size_t& index)
{
    for(size_t i = 0; i < m_soc.dev.size(); i++)
    {
        for(size_t j = 0; j < m_soc.dev[i].addr.size(); j++)
            if(m_soc.dev[i].addr[j].name.c_str() == dev)
            {
                dev_desc = m_soc.dev[i];
                index = j;
                return true;
            }
    }
    return false;
}

bool BackendHelper::GetRegisterDesc(const soc_dev_t& dev, const QString& reg,
    soc_reg_t& reg_desc, size_t& index)
{
    for(size_t i = 0; i < dev.reg.size(); i++)
    {
        for(size_t j = 0; j < dev.reg[i].addr.size(); j++)
            if(dev.reg[i].addr[j].name.c_str() == reg)
            {
                index = j;
                reg_desc = dev.reg[i];
                return true;
            }
    }
    return false;
}

bool BackendHelper::GetFieldDesc(const soc_reg_t& reg_desc, const QString& field,
    soc_reg_field_t& field_desc)
{
    for(size_t i = 0; i < reg_desc.field.size(); i++)
        if(reg_desc.field[i].name.c_str() == field)
            field_desc = reg_desc.field[i];
    return false;
}

bool BackendHelper::GetRegisterAddress(const QString& dev, const QString& reg,
    soc_addr_t& addr)
{
    size_t dev_index, reg_index;
    soc_dev_t dev_desc;
    soc_reg_t reg_desc;
    if(!GetDeviceDesc(dev, dev_desc, dev_index))
        return false;
    if(!GetRegisterDesc(dev_desc, reg, reg_desc, reg_index))
        return false;
    addr = dev_desc.addr[dev_index].addr + reg_desc.addr[reg_index].addr;
    return true;
}

bool BackendHelper::ReadRegisterField(const QString& dev, const QString& reg,
    const QString& field, soc_word_t& v)
{
    size_t dev_index, reg_index;
    soc_dev_t dev_desc;
    soc_reg_t reg_desc;
    soc_reg_field_t field_desc;
    if(!GetDeviceDesc(dev, dev_desc, dev_index))
        return false;
    if(!GetRegisterDesc(dev_desc, reg, reg_desc, reg_index))
        return false;
    if(!GetFieldDesc(reg_desc, field, field_desc))
        return false;
    if(!ReadRegister(dev, reg, v))
        return false;
    v = (v & field_desc.bitmask()) >> field_desc.first_bit;
    return true;
}