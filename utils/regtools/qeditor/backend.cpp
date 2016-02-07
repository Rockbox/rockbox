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
#include <QFont>
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
 * HWStubManager
 */
HWStubManager *HWStubManager::g_inst = nullptr;

HWStubManager::HWStubManager()
{
    Add("Default", QString::fromStdString(hwstub::uri::default_uri().full_uri()));
}

HWStubManager::~HWStubManager()
{
}

HWStubManager *HWStubManager::Get()
{
    if(g_inst == nullptr)
        g_inst = new HWStubManager();
    return g_inst;
}

bool HWStubManager::Add(const QString& name, const QString& uri)
{
    struct Context ctx;
    ctx.name = name;
    ctx.uri = uri;
    ctx.context = hwstub::uri::create_context(uri.toStdString());
    if(!ctx.context)
        return false;
    ctx.context->start_polling();
    beginInsertRows(QModelIndex(), m_list.size(), m_list.size());
    m_list.push_back(ctx);
    endInsertRows();
    return true;
}

void HWStubManager::Clear()
{
    m_list.clear();
}

int HWStubManager::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return m_list.size();
}

int HWStubManager::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 2;
}

std::shared_ptr< hwstub::context > HWStubManager::GetContext(int row)
{
    if(row < 0 || (size_t)row >= m_list.size())
        return std::shared_ptr< hwstub::context >();
    else
        return m_list[row].context;
}

QVariant HWStubManager::data(const QModelIndex& index, int role) const
{
    if(index.row() < 0 || (size_t)index.row() >= m_list.size())
        return QVariant();
    int section = index.column();
    const Context& ctx = m_list[index.row()];
    if(section == GetNameColumn())
    {
        if(role == Qt::DisplayRole || role == Qt::EditRole)
            return QVariant(ctx.name);
    }
    else if(section == GetUriColumn())
    {
        if(role == Qt::DisplayRole)
            return QVariant(ctx.uri);
    }
    return QVariant();
}

QVariant HWStubManager::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Vertical)
        return QVariant();
    if(role != Qt::DisplayRole)
        return QVariant();
    if(section == GetNameColumn())
        return QVariant("Name");
    else if(section == GetUriColumn())
        return QVariant("URI");
    return QVariant();
}

Qt::ItemFlags HWStubManager::flags(const QModelIndex& index) const
{
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    int section = index.column();
    if(section == GetNameColumn())
        flags |= Qt::ItemIsEditable;
    return flags;
}

bool HWStubManager::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if(role != Qt::EditRole)
        return false;
    if(index.row() < 0 || (size_t)index.row() >= m_list.size())
        return false;
    if(index.column() != GetNameColumn())
        return false;
    m_list[index.row()].name = value.toString();
    emit dataChanged(index, index);
    return true;
}

int HWStubManager::GetNameColumn() const
{
    return 0;
}

int HWStubManager::GetUriColumn() const
{
    return 1;
}

QString HWStubManager::GetFriendlyName(std::shared_ptr< hwstub::device > device)
{
    /* try to open the device */
    std::shared_ptr< hwstub::handle > handle;
    hwstub::error err = device->open(handle);
    if(err != hwstub::error::SUCCESS)
        goto Lfallback;
    /* get target descriptor */
    struct hwstub_target_desc_t target_desc;
    err = handle->get_target_desc(target_desc);
    if(err != hwstub::error::SUCCESS)
        goto Lfallback;
    return QString::fromStdString(target_desc.bName);

    /* fallback: don't open the device */
Lfallback:
    hwstub::usb::device *udev = dynamic_cast< hwstub::usb::device* >(device.get());
    if(udev)
    {
        return QString("USB Bus %1 Device %2: ID %3:%4")
            .arg(udev->get_bus_number()).arg(udev->get_address(), 3, 10, QChar('0'))
            .arg(udev->get_vid(), 4, 16, QChar('0')).arg(udev->get_pid(), 4, 16, QChar('0'));
    }
    else
        return QString("<Unknown device>");
}

/**
 * HWStubContextModel
 */
HWStubContextModel::HWStubContextModel(QObject *parent)
    :QAbstractTableModel(parent), m_has_dummy(false)
{
}

HWStubContextModel::~HWStubContextModel()
{
    SetContext(std::shared_ptr< hwstub::context >());
}

void HWStubContextModel::SetContext(std::shared_ptr< hwstub::context > context)
{
    int first_row = m_has_dummy ? 1: 0;
    /* clear previous model if any */
    if(m_list.size() > 0)
    {
        beginRemoveRows(QModelIndex(), first_row, first_row + m_list.size() - 1);
        m_list.clear();
        endRemoveRows();
    }
    /* don't forget to unregister callback if context still exists */
    std::shared_ptr< hwstub::context > ctx = m_context.lock();
    if(ctx)
        ctx->unregister_callback(m_callback_ref);
    /* get new context */
    m_context = context;
    if(context)
    {
        /* register new callback */
        m_callback_ref = context->register_callback(
            std::bind(&HWStubContextModel::OnDevChangeLow, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3));
        /* get dev list */
        std::vector< std::shared_ptr< hwstub::device > > list;
        hwstub::error err = context->get_device_list(list);
        if(err == hwstub::error::SUCCESS)
        {
            beginInsertRows(QModelIndex(), first_row, first_row + list.size() - 1);
            for(auto& d : list)
            {
                Device dev;
                dev.name = GetFriendlyName(d);
                dev.device = d;
                m_list.push_back(dev);
            }
            endInsertRows();
        }
    }
}

void HWStubContextModel::EnableDummy(bool en, const QString& text)
{
    /* if needed, create/remove raw */
    if(m_has_dummy && !en)
    {
        /* remove row */
        beginRemoveRows(QModelIndex(), 0, 0);
        m_has_dummy = false;
        endRemoveRows();
    }
    else if(!m_has_dummy && en)
    {
        /* add row */
        beginInsertRows(QModelIndex(), 0, 0);
        m_has_dummy = true;
        m_dummy_text = text;
        endInsertRows();
    }
    else if(en)
    {
        /* text change only */
        emit dataChanged(index(0, GetNameColumn()), index(0, GetNameColumn()));
    }
}

int HWStubContextModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return m_list.size() + (m_has_dummy ? 1 : 0);
}

int HWStubContextModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant HWStubContextModel::data(const QModelIndex& index, int role) const
{
    int first_row = m_has_dummy ? 1: 0;
    /* special case for dummy */
    if(m_has_dummy && index.row() == 0)
    {
        int section = index.column();
        if(section == GetNameColumn())
        {
            if(role == Qt::DisplayRole)
                return QVariant(m_dummy_text);
            else if(role == Qt::FontRole)
            {
                QFont font;
                font.setItalic(true);
                return QVariant(font);
            }
        }
        return QVariant();
    }

    if(index.row() < first_row || (size_t)index.row() >= first_row + m_list.size())
        return QVariant();
    int section = index.column();
    if(section == GetNameColumn())
    {
        if(role == Qt::DisplayRole)
            return QVariant(m_list[index.row() - first_row].name);
    }
    return QVariant();
}

QVariant HWStubContextModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Vertical)
        return QVariant();
    if(role != Qt::DisplayRole)
        return QVariant();
    if(section == GetNameColumn())
        return QVariant("Friendly name");
    return QVariant();
}

Qt::ItemFlags HWStubContextModel::flags(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

int HWStubContextModel::GetNameColumn() const
{
    return 0;
}

std::shared_ptr< hwstub::device > HWStubContextModel::GetDevice(int row)
{
    int first_row = m_has_dummy ? 1: 0;
    /* special case for dummy */
    if(row < first_row || (size_t)row >= first_row + m_list.size())
        return std::shared_ptr< hwstub::device >();
    else
        return m_list[row - first_row].device;
}

QString HWStubContextModel::GetFriendlyName(std::shared_ptr< hwstub::device > device)
{
    return HWStubManager::GetFriendlyName(device);
}

namespace
{
    struct dev_change_t
    {
        std::shared_ptr< hwstub::context > ctx;
        bool arrived;
        std::shared_ptr< hwstub::device > device;
    };
}

void HWStubContextModel::OnDevChangeLow(std::shared_ptr< hwstub::context > ctx,
    bool arrived, std::shared_ptr< hwstub::device > device)
{
    /* calling Qt function from non-Qt thread is unsafe. Since the polling thread
     * is a pthread, the safest way to use Qt invoke mecanism to make it run
     * on the event loop */
    dev_change_t *evt = new dev_change_t;
    evt->ctx = ctx;
    evt->arrived = arrived;
    evt->device = device;
    QMetaObject::invokeMethod(this, "OnDevChangeUnsafe", Q_ARG(void *, (void *)evt));
}

void HWStubContextModel::OnDevChangeUnsafe(void *data)
{
    dev_change_t *evt = (dev_change_t *)data;
    OnDevChange(evt->ctx, evt->arrived, evt->device);
    delete evt;
}

void HWStubContextModel::OnDevChange(std::shared_ptr< hwstub::context > ctx, bool arrived,
    std::shared_ptr< hwstub::device > device)
{
    int first_row = m_has_dummy ? 1: 0;
    Q_UNUSED(ctx);
    if(arrived)
    {
        Device dev;
        dev.name = GetFriendlyName(device);
        dev.device = device;
        beginInsertRows(QModelIndex(), first_row + m_list.size(),
            first_row + m_list.size());
        m_list.push_back(dev);
        endInsertRows();
    }
    else
    {
        /* find device in the list */
        auto it = m_list.begin();
        int idx = 0;
        for(; it != m_list.end(); ++it, ++idx)
            if(it->device == device)
                break;
        if(it == m_list.end())
            return;
        /* remove it */
        beginRemoveRows(QModelIndex(), first_row + idx, first_row + idx);
        m_list.erase(it);
        endRemoveRows();
    }
}

/**
 * HWStubDevice
 */
HWStubDevice::HWStubDevice(std::shared_ptr< hwstub::device > device)
{
    m_valid = Probe(device);
}

HWStubDevice::~HWStubDevice()
{
}

bool HWStubDevice::Probe(std::shared_ptr<hwstub::device> device)
{
    if(!device)
        return false;
    hwstub::error err = device->open(m_handle);
    if(err != hwstub::error::SUCCESS)
        return false;
    // get target information
    err = m_handle->get_target_desc(m_hwdev_target);
    if(err != hwstub::error::SUCCESS)
        return false;
    // get STMP/PP information
    if(m_hwdev_target.dID == HWSTUB_TARGET_STMP)
    {
        err = m_handle->get_stmp_desc(m_hwdev_stmp);
        if(err != hwstub::error::SUCCESS)
            return false;
    }
    else if(m_hwdev_target.dID == HWSTUB_TARGET_PP)
    {
        err = m_handle->get_pp_desc(m_hwdev_pp);
        if(err != hwstub::error::SUCCESS)
            return false;
    }
    else if(m_hwdev_target.dID == HWSTUB_TARGET_JZ)
    {
        err = m_handle->get_jz_desc(m_hwdev_jz);
        if(err != hwstub::error::SUCCESS)
            return false;
    }
    m_name = HWStubManager::GetFriendlyName(device);
    return true;
}

bool HWStubDevice::ReadMem(soc_addr_t addr, size_t length, void *buffer)
{
    size_t len = length;
    hwstub::error err = m_handle->read(addr, buffer, len, true);
    return err == hwstub::error::SUCCESS && len == length;
}

bool HWStubDevice::WriteMem(soc_addr_t addr, size_t length, void *buffer)
{
    size_t len = length;
    hwstub::error err = m_handle->write(addr, buffer, len, true);
    return err == hwstub::error::SUCCESS && len == length;
}

bool HWStubDevice::IsValid()
{
    return m_valid;
}

QString HWStubDevice::GetFriendlyName()
{
    return m_name;
}

/**
 * HWStubIoBackend
 */

HWStubIoBackend::HWStubIoBackend(HWStubDevice *dev)
{
    m_dev = dev;

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
    else if(target.dID == HWSTUB_TARGET_JZ)
    {
        struct hwstub_jz_desc_t jz = m_dev->GetJZInfo();
        m_soc = QString("jz%1").arg(jz.wChipID, 4, 16, QChar('0'));
        if(jz.bRevision != 0)
            m_soc.append(QChar(jz.bRevision).toLower());
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
