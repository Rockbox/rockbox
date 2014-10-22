/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Amaury Pouly
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
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QFileInfo>
#include "backend.h"

/**
 * SocFile
 */
SocFile::SocFile()
    :m_valid(false)
{
}

SocFile::SocFile(const QString& filename)
    :m_filename(filename)
{
    m_valid = soc_desc_parse_xml(filename.toStdString(), m_soc);
    soc_desc_normalize(m_soc);
}

bool SocFile::IsValid()
{
    return m_valid;
}

SocRef SocFile::GetSocRef()
{
    return SocRef(this);
}

QString SocFile::GetFilename()
{
    return m_filename;
}

/**
 * Backend
 */

Backend::Backend()
{
}


QList< SocFileRef > Backend::GetSocFileList()
{
    QList< SocFileRef > list;
    for(std::list< SocFile >::iterator it = m_socs.begin(); it != m_socs.end(); ++it)
        list.append(SocFileRef(&(*it)));
    return list;
}

QList< SocRef > Backend::GetSocList()
{
    QList< SocRef > list;
    for(std::list< SocFile >::iterator it = m_socs.begin(); it != m_socs.end(); ++it)
        list.append(it->GetSocRef());
    return list;
}

bool Backend::LoadSocDesc(const QString& filename)
{
    SocFile f(filename);
    if(!f.IsValid())
        return false;
    m_socs.push_back(f);
    emit OnSocListChanged();
    return true;
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
 * RamIoBackend
 */
RamIoBackend::RamIoBackend(const QString& soc_name)
{
    m_soc = soc_name;
}

bool RamIoBackend::ReadRegister(const QString& name, soc_word_t& value)
{
    QMap<QString, soc_word_t>::const_iterator it = m_map.find(name);
    if(it == m_map.end())
        return false;
    value = it.value();
    return true;
}

void RamIoBackend::DeleteAll()
{
    m_map.clear();
}

bool RamIoBackend::WriteRegister(const QString& name, soc_word_t value, WriteMode mode)
{
    switch(mode)
    {
        case Write: m_map[name] = value; return true;
        case Set: m_map[name] |= value; return true;
        case Clear: m_map[name] &= ~value; return true;
        case Toggle: m_map[name] ^= value; return true;
        default: return false;
    }
}



/**
 * FileIoBackend
 */

FileIoBackend::FileIoBackend(const QString& filename, const QString& soc_name)
    :RamIoBackend(soc_name)
{
    m_filename = filename;
    m_valid = false;
    Reload();
}


bool FileIoBackend::Reload()
{
    m_valid = false;
    QFile file(m_filename);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    DeleteAll();

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
            RamIoBackend::WriteRegister(key, val, Write);
    }

    m_readonly = !QFileInfo(file).isWritable();
    m_dirty = false;
    m_valid = true;
    return true;
}

bool FileIoBackend::WriteRegister(const QString& name, soc_word_t value, WriteMode mode)
{
    m_dirty = true;
    return RamIoBackend::WriteRegister(name, value, mode);
}

bool FileIoBackend::Commit()
{
    if(!m_dirty)
        return true;
    QFile file(m_filename);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;
    QTextStream out(&file);
    out << "HW = " << m_soc << "\n";
    QMapIterator< QString, soc_word_t > it(m_map);
    while(it.hasNext())
    {
        it.next();
        out << it.key() << " = " << hex << showbase << it.value() << "\n";
    }
    out.flush();
    return file.flush();
}

#ifdef HAVE_HWSTUB
/**
 * HWStubDevice
 */
HWStubDevice::HWStubDevice(struct libusb_device *dev)
{
    Init(dev);
}

HWStubDevice::HWStubDevice(const HWStubDevice *dev)
{
    Init(dev->m_dev);
}

void HWStubDevice::Init(struct libusb_device *dev)
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
    if(!Open())
        return false;
    // get target
    int ret = hwstub_get_desc(m_hwdev, HWSTUB_DT_TARGET, &m_hwdev_target, sizeof(m_hwdev_target));
    if(ret != sizeof(m_hwdev_target))
        goto Lerr;
    // get STMP information
    if(m_hwdev_target.dID == HWSTUB_TARGET_STMP)
    {
        ret = hwstub_get_desc(m_hwdev, HWSTUB_DT_STMP, &m_hwdev_stmp, sizeof(m_hwdev_stmp));
        if(ret != sizeof(m_hwdev_stmp))
            goto Lerr;
    }
    else if(m_hwdev_target.dID == HWSTUB_TARGET_PP)
    {
        ret = hwstub_get_desc(m_hwdev, HWSTUB_DT_PP, &m_hwdev_pp, sizeof(m_hwdev_pp));
        if(ret != sizeof(m_hwdev_pp))
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
        m_handle = 0;
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
    int ret = hwstub_rw_mem_atomic(m_hwdev, 1, addr, buffer, length);
    return ret >= 0 && (size_t)ret == length;
}

bool HWStubDevice::WriteMem(soc_addr_t addr, size_t length, void *buffer)
{
    if(!m_hwdev)
        return false;
    int ret = hwstub_rw_mem_atomic(m_hwdev, 0, addr, buffer, length);
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
    else if(target.dID == HWSTUB_TARGET_PP)
    {
        struct hwstub_pp_desc_t pp = m_dev->GetPPInfo();
        if(pp.wChipID == 0x6110 )
            m_soc = "pp6110";
        else
            m_soc = QString("pp%1").arg(pp.wChipID, 4, 16, QChar('0'));
    }
    else
        m_soc = target.bName;
}

QString HWStubIoBackend::GetSocName()
{
    return m_soc;
}

HWStubIoBackend::~HWStubIoBackend()
{
    delete m_dev;
}

bool HWStubIoBackend::ReadRegister(soc_addr_t addr, soc_word_t& value)
{
    return m_dev->ReadMem(addr, sizeof(value), &value);
}

bool HWStubIoBackend::WriteRegister(soc_addr_t addr, soc_word_t value, WriteMode mode)
{
    switch(mode)
    {
        case Set: addr += 4; break;
        case Clear: addr += 8; break;
        case Toggle: addr += 12; break;
        default: break;
    }
    return m_dev->WriteMem(addr, sizeof(value), &value);
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
#ifdef LIBUSB_NO_HOTPLUG
    m_hotplug = false;
#else
    m_hotplug = libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG);
    if(m_hotplug)
    {
        int evt = LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
            LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT;
        m_hotplug = LIBUSB_SUCCESS == libusb_hotplug_register_callback(
            NULL, (libusb_hotplug_event)evt, LIBUSB_HOTPLUG_ENUMERATE,
            LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
            &HWStubBackendHelper::HotPlugCallback, reinterpret_cast< void* >(this),
            &m_hotplug_handle);
    }
#endif /* LIBUSB_NO_HOTPLUG */
}

HWStubBackendHelper::~HWStubBackendHelper()
{
#ifndef LIBUSB_NO_HOTPLUG
    if(m_hotplug)
        libusb_hotplug_deregister_callback(NULL, m_hotplug_handle);
#endif /* LIBUSB_NO_HOTPLUG */
}

QList< HWStubDevice* > HWStubBackendHelper::GetDevList()
{
    QList< HWStubDevice* > list;
    libusb_device **dev_list;
    ssize_t cnt = hwstub_get_device_list(NULL, &dev_list);
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
    Q_UNUSED(ctx);
    HWStubBackendHelper *helper = reinterpret_cast< HWStubBackendHelper* >(user_data);
    switch(event)
    {
        case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: helper->OnHotPlug(true, dev); break;
        case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: helper->OnHotPlug(false, dev); break;
        default: break;
    }
    return 0;
}
#endif /* LIBUSB_NO_HOTPLUG */

bool HWStubBackendHelper::HasHotPlugSupport()
{
    return m_hotplug;
}

namespace
{
class lib_usb_init
{
public:
    lib_usb_init()
    {
        libusb_init(NULL);
    }
};

lib_usb_init __lib_usb_init;
}

#endif /* HAVE_HWSTUB */

/**
 * BackendHelper
 */

BackendHelper::BackendHelper(IoBackend *io_backend, const SocRef& soc)
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

bool BackendHelper::WriteRegister(const QString& dev, const QString& reg,
    soc_word_t v, IoBackend::WriteMode mode)
{
    if(m_io_backend->SupportAccess(IoBackend::ByName))
        return m_io_backend->WriteRegister("HW." + dev + "." + reg, v, mode);
    if(m_io_backend->SupportAccess(IoBackend::ByAddress))
    {
        soc_addr_t addr;
        if(GetRegisterAddress(dev, reg, addr))
            return m_io_backend->WriteRegister(addr, v, mode);
    }
    return false;
}

bool BackendHelper::GetDevRef(const QString& sdev, SocDevRef& ref)
{
    for(size_t i = 0; i < m_soc.GetSoc().dev.size(); i++)
    {
        const soc_dev_t& dev = m_soc.GetSoc().dev[i];
        for(size_t j = 0; j < dev.addr.size(); j++)
            if(dev.addr[j].name.c_str() == sdev)
            {
                ref = SocDevRef(m_soc, i, j);
                return true;
            }
    }
    return false;
}

bool BackendHelper::GetRegRef(const SocDevRef& dev, const QString& sreg, SocRegRef& ref)
{
    const soc_dev_t& sdev = dev.GetDev();
    for(size_t i = 0; i < sdev.reg.size(); i++)
    {
        const soc_reg_t& reg = sdev.reg[i];
        for(size_t j = 0; j < reg.addr.size(); j++)
        {
            if(reg.addr[j].name.c_str() == sreg)
            {
                ref = SocRegRef(dev, i, j);
                return true;
            }
        }
    }
    return false;
}

bool BackendHelper::GetFieldRef(const SocRegRef& reg, const QString& sfield, SocFieldRef& ref)
{
    for(size_t i = 0; i < reg.GetReg().field.size(); i++)
        if(reg.GetReg().field[i].name.c_str() == sfield)
        {
            ref = SocFieldRef(reg, i);
            return true;
        }
    return false;
}

bool BackendHelper::GetRegisterAddress(const QString& dev, const QString& reg,
    soc_addr_t& addr)
{
    SocDevRef dev_ref;
    SocRegRef reg_ref;
    if(!GetDevRef(dev, dev_ref) || !GetRegRef(dev_ref, reg, reg_ref))
        return false;
    addr = dev_ref.GetDevAddr().addr + reg_ref.GetRegAddr().addr;
    return true;
}

bool BackendHelper::ReadRegisterField(const QString& dev, const QString& reg,
    const QString& field, soc_word_t& v)
{
    SocDevRef dev_ref;
    SocRegRef reg_ref;
    SocFieldRef field_ref;
    if(!GetDevRef(dev, dev_ref) || !GetRegRef(dev_ref, reg, reg_ref) || 
            !GetFieldRef(reg_ref, field, field_ref))
        return false;
    if(!ReadRegister(dev, reg, v))
        return false;
    v = (v & field_ref.GetField().bitmask()) >> field_ref.GetField().first_bit;
    return true;
}

bool BackendHelper::DumpAllRegisters(const QString& filename, bool ignore_errors)
{
    FileIoBackend b(filename, QString::fromStdString(m_soc.GetSoc().name));
    bool ret = DumpAllRegisters(&b, ignore_errors);
    return ret && b.Commit();
}

bool BackendHelper::DumpAllRegisters(IoBackend *backend, bool ignore_errors)
{
    BackendHelper bh(backend, m_soc);
    bool ret = true;
    for(size_t i = 0; i < m_soc.GetSoc().dev.size(); i++)
    {
        const soc_dev_t& dev = m_soc.GetSoc().dev[i];
        for(size_t j = 0; j < dev.addr.size(); j++)
        {
            QString devname = QString::fromStdString(dev.addr[j].name);
            for(size_t k = 0; k < dev.reg.size(); k++)
            {
                const soc_reg_t& reg = dev.reg[k];
                for(size_t l = 0; l < reg.addr.size(); l++)
                {
                    QString regname = QString::fromStdString(reg.addr[l].name);
                    soc_word_t val;
                    if(!ReadRegister(devname, regname, val))
                    {
                        ret = false;
                        if(!ignore_errors)
                            return false;
                    }
                    else if(!bh.WriteRegister(devname, regname, val))
                    {
                        ret = false;
                        if(!ignore_errors)
                            return false;
                    }
                }
            }
        }
    }
    return ret;
}
