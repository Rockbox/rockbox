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
    :m_valid(true)
{
}

SocFile::SocFile(const QString& filename)
    :m_filename(filename)
{
    soc_desc::error_context_t ctx;
    m_valid = soc_desc::parse_xml(filename.toStdString(), m_soc, ctx);
}

bool SocFile::IsValid()
{
    return m_valid;
}

soc_desc::soc_ref_t SocFile::GetSocRef()
{
    if(m_valid)
        return soc_desc::soc_ref_t(&m_soc);
    else
        return soc_desc::soc_ref_t();
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
    {
        if(it->IsValid())
            list.append(SocFileRef(&(*it)));
    }
    return list;
}

QList< soc_desc::soc_ref_t > Backend::GetSocList()
{
    QList< soc_desc::soc_ref_t > list;
    for(std::list< SocFile >::iterator it = m_socs.begin(); it != m_socs.end(); ++it)
    {
        soc_desc::soc_ref_t r = it->GetSocRef();
        if(r.valid())
            list.append(r);
    }
    return list;
}

bool Backend::LoadSocDesc(const QString& filename)
{
    SocFile f(filename);
    if(!f.IsValid())
        return false;
    m_socs.push_back(f);
    emit OnSocAdded(SocFileRef(&m_socs.back()));
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
 * DummyIoBackend
 */

DummyIoBackend::DummyIoBackend()
{
}

bool DummyIoBackend::IsValid()
{
    return false;
}

QString DummyIoBackend::GetSocName()
{
    return "";
}

bool DummyIoBackend::ReadRegister(soc_addr_t addr, soc_word_t& value,
    unsigned width)
{
    Q_UNUSED(addr);
    Q_UNUSED(value);
    Q_UNUSED(width);
    return false;
}

bool DummyIoBackend::Reload()
{
    return false;
}

bool DummyIoBackend::IsReadOnly()
{
    return true;
}

bool DummyIoBackend::WriteRegister(soc_addr_t addr, soc_word_t value,
    unsigned width, WriteMode mode)
{
    Q_UNUSED(addr);
    Q_UNUSED(value);
    Q_UNUSED(mode);
    Q_UNUSED(width);
    return false;
}

bool DummyIoBackend::IsDirty()
{
    return false;
}

bool DummyIoBackend::Commit()
{
    return false;
}

/**
 * RamIoBackend
 */
RamIoBackend::RamIoBackend(const QString& soc_name)
{
    m_soc = soc_name;
}

bool RamIoBackend::IsValid()
{
    return m_soc != "";
}

QString RamIoBackend::GetSocName()
{
    return m_soc;
}

void RamIoBackend::SetSocName(const QString& soc_name)
{
    m_soc = soc_name;
}

bool RamIoBackend::RamIoBackend::Reload()
{
    return false;
}

bool RamIoBackend::IsReadOnly()
{
    return false;
}

bool RamIoBackend::ReadRegister(soc_addr_t addr, soc_word_t& value,
    unsigned width)
{
    Q_UNUSED(width);
    QMap<soc_addr_t, soc_word_t>::const_iterator it = m_map.find(addr);
    if(it == m_map.end())
        return false;
    value = it.value();
    return true;
}

void RamIoBackend::DeleteAll()
{
    m_map.clear();
}

bool RamIoBackend::WriteRegister(soc_addr_t addr, soc_word_t value,
    unsigned width, WriteMode mode)
{
    Q_UNUSED(width);
    switch(mode)
    {
        case Write: m_map[addr] = value; return true;
        case Set: m_map[addr] |= value; return true;
        case Clear: m_map[addr] &= ~value; return true;
        case Toggle: m_map[addr] ^= value; return true;
        default: return false;
    }
}

bool RamIoBackend::IsDirty()
{
    return false;
}

bool RamIoBackend::Commit()
{
    return false;
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

bool FileIoBackend::IsValid()
{
    return m_valid;
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
        QString key_str = line.left(idx).trimmed();
        QString val_str = line.mid(idx + 1).trimmed();
        bool key_ok,val_ok;
        soc_word_t val = val_str.toULong(&val_ok, 0);
        soc_word_t key = key_str.toULong(&key_ok, 0);
        if(key_str == "soc")
            m_soc = val_str;
        else if(key_ok && val_ok)
            RamIoBackend::WriteRegister(key, val, 32, Write);
    }
    m_readonly = !QFileInfo(file).isWritable();
    m_dirty = false;
    m_valid = true;
    return true;
}

bool FileIoBackend::WriteRegister(soc_addr_t addr, soc_word_t value,
    unsigned width, WriteMode mode)
{
    m_dirty = true;
    return RamIoBackend::WriteRegister(addr, value, width, mode);
}

bool FileIoBackend::Commit()
{
    if(!m_dirty)
        return true;
    QFile file(m_filename);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;
    QTextStream out(&file);
    out << "soc = " << m_soc << "\n";
    QMapIterator< soc_addr_t, soc_word_t > it(m_map);
    while(it.hasNext())
    {
        it.next();
        out << hex << showbase << it.key() << " = " << hex << showbase << it.value() << "\n";
    }
    out.flush();
    return file.flush();
}

bool FileIoBackend::IsReadOnly()
{
    return m_readonly;
}

bool FileIoBackend::IsDirty()
{
    return m_dirty;
}

QString FileIoBackend::GetFileName()
{
    return m_filename;
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
    else if(target.dID == HWSTUB_TARGET_ATJ)
        m_soc = "atj213x";
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

bool HWStubIoBackend::IsValid()
{
    return m_dev->IsValid();
}

bool HWStubIoBackend::IsReadOnly()
{
    return false;
}

bool HWStubIoBackend::IsDirty()
{
    return false;
}

bool HWStubIoBackend::Commit()
{
    return true;
}

HWStubDevice *HWStubIoBackend::GetDevice()
{
    return m_dev;
}

bool HWStubIoBackend::ReadRegister(soc_addr_t addr, soc_word_t& value,
    unsigned width)
{
    if(width != 8 && width != 16 && width != 32)
        return false;
    return m_dev->ReadMem(addr, width / 8, &value);
}

bool HWStubIoBackend::WriteRegister(soc_addr_t addr, soc_word_t value,
    unsigned width, WriteMode mode)
{
    if(width != 8 && width != 16 && width != 32)
        return false;
    switch(mode)
    {
        case Set: addr += 4; break;
        case Clear: addr += 8; break;
        case Toggle: addr += 12; break;
        default: break;
    }
    return m_dev->WriteMem(addr, width / 8, &value);
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

BackendHelper::BackendHelper(IoBackend *io_backend, const soc_desc::soc_ref_t& soc)
    :m_io_backend(io_backend), m_soc(soc)
{
}

QString BackendHelper::GetPath(const soc_desc::node_inst_t& inst)
{
    if(!inst.valid() || inst.is_root())
        return QString();
    QString s = GetPath(inst.parent());
    if(!s.isEmpty())
        s += ".";
    s += inst.name().c_str();
    if(inst.is_indexed())
        s = QString("%1[%2]").arg(s).arg(inst.index());
    return s;
}

soc_desc::node_inst_t BackendHelper::ParsePath(const QString& path)
{
    soc_desc::node_inst_t inst = m_soc.root_inst();
    /* empty path is root */
    if(path.isEmpty())
        return inst;
    int pos = 0;
    while(pos < path.size())
    {
        /* try to find the next separator */
        int next = path.indexOf('.', pos);
        if(next == -1)
            next = path.size();
        /* try to find the index, if any */
        int lidx = path.indexOf('[', pos);
        if(lidx == -1 || lidx > next)
            lidx = next;
        /* extract name */
        std::string name = path.mid(pos, lidx - pos).toStdString();
        /* and index */
        if(lidx < next)
        {
            int ridx = path.indexOf(']', lidx + 1);
            /* syntax error ? */
            if(ridx == -1 || ridx > next)
                return soc_desc::node_inst_t();
            /* invalid number ? */
            bool ok = false;
            size_t idx = path.mid(lidx + 1, ridx - lidx - 1).toUInt(&ok);
            if(ok)
                inst = inst.child(name, idx);
            else
                inst = soc_desc::node_inst_t();
        }
        else
            inst = inst.child(name);
        /* advance right after the separator */
        pos = next + 1;
    }
    return inst;
}

bool BackendHelper::ReadRegister(const soc_desc::node_inst_t& inst,
    soc_word_t& v)
{
    soc_addr_t addr;
    if(!GetRegisterAddress(inst, addr))
        return false;
    return m_io_backend->ReadRegister(addr, v, inst.node().reg().get()->width);
}

bool BackendHelper::WriteRegister(const soc_desc::node_inst_t& inst,
    soc_word_t v, IoBackend::WriteMode mode)
{
    soc_addr_t addr;
    if(!GetRegisterAddress(inst, addr))
        return false;
    return m_io_backend->WriteRegister(addr, v, inst.node().reg().get()->width, mode);
}

bool BackendHelper::GetRegisterAddress(const soc_desc::node_inst_t& inst,
    soc_addr_t& addr)
{
    if(!inst.valid())
        return false;
    addr = inst.addr();
    return true;
}

bool BackendHelper::ReadRegisterField(const soc_desc::node_inst_t& inst,
    const QString& field, soc_word_t& v)
{
    soc_desc::field_ref_t ref = inst.node().reg().field(field.toStdString());
    if(!ref.valid())
        return false;
    if(!ReadRegister(inst, v))
        return false;
    v = (v & ref.get()->bitmask()) >> ref.get()->pos;
    return true;
}

bool BackendHelper::DumpAllRegisters(const QString& filename, bool ignore_errors)
{
    FileIoBackend b(filename, QString::fromStdString(m_soc.get()->name));
    bool ret = DumpAllRegisters(&b, ignore_errors);
    return ret && b.Commit();
}

bool BackendHelper::DumpAllRegisters(IoBackend *backend, bool ignore_errors)
{
    BackendHelper helper(backend, m_soc);
    return DumpAllRegisters(&helper, m_soc.root_inst(), ignore_errors);
}

bool BackendHelper::DumpAllRegisters(BackendHelper *bh,
    const soc_desc::node_inst_t& inst, bool ignore_errors)
{
    bool ret = true;
    if(inst.node().reg().valid())
    {
        soc_word_t val;
        if(!ReadRegister(inst, val))
        {
            ret = false;
            if(!ignore_errors)
                return false;
        }
        else if(!bh->WriteRegister(inst, val))
        {
            ret = false;
            if(!ignore_errors)
                return false;
        }
    }
    std::vector< soc_desc::node_inst_t > list = inst.children();
    for(size_t i = 0; i < list.size(); i++)
    {
        if(!DumpAllRegisters(bh, list[i], ignore_errors))
        {
            ret = false;
            if(!ignore_errors)
                return false;
        }
    }
    return ret;
}
