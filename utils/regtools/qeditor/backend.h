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
#ifndef __BACKEND_H__
#define __BACKEND_H__

#include <QObject>
#include <QStringList>
#include <QMap>
#include <QVector>
#include <QMetaType>
#ifdef HAVE_HWSTUB
#include "hwstub.h"
#endif
#include "soc_desc.hpp"

class IoBackend : public QObject
{
    Q_OBJECT
public:
    IoBackend() {}
    virtual ~IoBackend() {}

    enum WriteMode
    {
        Write, Set, Clear, Toggle
    };

    enum AccessType
    {
        ByName,
        ByAddress,
    };

    /** Register naming convention: name based access are of the form:
     * HW.dev.reg
     * where <dev> is the device name (including index like APPUART1)
     * and <reg> is the register name (including index like PRIORITY29) */
    /* report whether backend is valid */
    virtual bool IsValid() = 0;
    /* report whether backend supports register access type */
    virtual bool SupportAccess(AccessType type) = 0;
    /* get SoC name */
    virtual QString GetSocName() = 0;
    /* read a register by name or address */
    virtual bool ReadRegister(const QString& name, soc_word_t& value) = 0;
    virtual bool ReadRegister(soc_addr_t addr, soc_word_t& value) = 0;
    /* reload content (if it makes sense) */
    virtual bool Reload() = 0;
    /* check whether backend supports writing */
    virtual bool IsReadOnly() = 0;
    /* write a register by name or address
     * NOTE: even on a read-only backend, a write is allowed be successful as long
     * as commit fails */
    virtual bool WriteRegister(const QString& name, soc_word_t value,
        WriteMode mode = Write) = 0;
    virtual bool WriteRegister(soc_addr_t addr, soc_word_t value,
        WriteMode mode = Write) = 0;
    /* check whether backend contains uncommitted (ie cached) writes */
    virtual bool IsDirty() = 0;
    /* commit all writes */
    virtual bool Commit() = 0;
};

class DummyIoBackend : public IoBackend
{
    Q_OBJECT
public:
    DummyIoBackend() {}

    virtual bool IsValid() { return false; }
    virtual bool SupportAccess(AccessType type) { Q_UNUSED(type); return false; }
    virtual QString GetSocName() { return ""; }
    virtual bool ReadRegister(const QString& name, soc_word_t& value) 
        { Q_UNUSED(name); Q_UNUSED(value); return false; }
    virtual bool ReadRegister(soc_addr_t addr, soc_word_t& value) 
        { Q_UNUSED(addr); Q_UNUSED(value); return false; }
    virtual bool Reload() { return false; }
    virtual bool IsReadOnly() { return true; }
    virtual bool WriteRegister(const QString& name, soc_word_t value, WriteMode mode)
        { Q_UNUSED(name); Q_UNUSED(value); Q_UNUSED(mode); return false; }
    virtual bool WriteRegister(soc_addr_t addr, soc_word_t value, WriteMode mode)
        { Q_UNUSED(addr); Q_UNUSED(value); Q_UNUSED(mode); return false; }
    virtual bool IsDirty() { return false; }
    virtual bool Commit() { return false; }
};

/** The RAM backend doesn't have any backend storage and stores all values in
 * an associative map */
class RamIoBackend : public IoBackend
{
    Q_OBJECT
public:
    RamIoBackend(const QString& soc_name = "");

    virtual bool IsValid() { return m_soc != ""; }
    virtual bool SupportAccess(AccessType type) { return type == ByName; }
    virtual QString GetSocName() { return m_soc; }
    virtual void SetSocName(const QString& soc_name) { m_soc = soc_name; }
    virtual bool ReadRegister(const QString& name, soc_word_t& value);
    virtual bool ReadRegister(soc_addr_t addr, soc_word_t& value)
        { Q_UNUSED(addr); Q_UNUSED(value); return false; }
    virtual bool Reload() { return false; }
    virtual bool IsReadOnly() { return false; }
    virtual bool WriteRegister(const QString& name, soc_word_t value, WriteMode mode);
    virtual bool WriteRegister(soc_addr_t addr, soc_word_t value, WriteMode mode)
        { Q_UNUSED(addr); Q_UNUSED(value); Q_UNUSED(mode); return false; }
    virtual bool IsDirty() { return false; }
    virtual bool Commit() { return false; }
    /* clear all entries of the backend */
    virtual void DeleteAll();

protected:
    QString m_soc;
    QMap< QString, soc_word_t > m_map;
};

/** NOTE the File backend makes a difference between writes and commits:
 * a write will *never* touch the underlying file unless it was committed. */
class FileIoBackend : public RamIoBackend
{
    Q_OBJECT
public:
    FileIoBackend(const QString& filename, const QString& soc_name = "");

    virtual bool IsValid() { return m_valid; }
    virtual bool SupportAccess(AccessType type) { return type == ByName; }
    virtual bool Reload();
    virtual bool IsReadOnly() { return m_readonly; }
    virtual bool WriteRegister(const QString& name, soc_word_t value, WriteMode mode);
    virtual bool IsDirty() { return m_dirty; }
    virtual bool Commit();
    QString GetFileName() { return m_filename; }

protected:
    QString m_filename;
    bool m_readonly;
    bool m_dirty;
    bool m_valid;
};

#ifdef HAVE_HWSTUB
class HWStubDevice
{
public:
    HWStubDevice(struct libusb_device *dev);
    HWStubDevice(const HWStubDevice *dev);
    ~HWStubDevice();
    bool IsValid();
    bool Open();
    void Close();
    int GetBusNumber();
    int GetDevAddress();
    /* Calls below are cached and do not require the device to be opened */
    inline struct hwstub_version_desc_t GetVersionInfo() { return m_hwdev_ver; }
    inline struct hwstub_target_desc_t GetTargetInfo() { return m_hwdev_target; }
    inline struct hwstub_stmp_desc_t GetSTMPInfo() { return m_hwdev_stmp; }
    inline struct hwstub_pp_desc_t GetPPInfo() { return m_hwdev_pp; }
    /* Calls below require the device to be opened */
    bool ReadMem(soc_addr_t addr, size_t length, void *buffer);
    bool WriteMem(soc_addr_t addr, size_t length, void *buffer);

protected:
    bool Probe();
    void Init(struct libusb_device *dev);

    bool m_valid;
    struct libusb_device *m_dev;
    libusb_device_handle *m_handle;
    struct hwstub_device_t *m_hwdev;
    struct hwstub_version_desc_t m_hwdev_ver;
    struct hwstub_target_desc_t m_hwdev_target;
    struct hwstub_stmp_desc_t m_hwdev_stmp;
    struct hwstub_pp_desc_t m_hwdev_pp;
};

/** NOTE the HWStub backend is never dirty: all writes are immediately committed */
class HWStubIoBackend : public IoBackend
{
    Q_OBJECT
public:
    // NOTE: HWStubIoBackend takes ownership of the device and will delete it
    HWStubIoBackend(HWStubDevice *dev);
    virtual ~HWStubIoBackend();

    virtual bool IsValid() { return m_dev->IsValid(); }
    virtual bool SupportAccess(AccessType type) { return type == ByAddress; }
    virtual QString GetSocName();
    virtual bool ReadRegister(const QString& name, soc_word_t& value)
        { Q_UNUSED(name); Q_UNUSED(value); return false; }
    virtual bool ReadRegister(soc_addr_t addr, soc_word_t& value);
    virtual bool Reload();
    virtual bool IsReadOnly() { return false; }
    virtual bool WriteRegister(const QString& name, soc_word_t value, WriteMode mode)
        { Q_UNUSED(name); Q_UNUSED(value); Q_UNUSED(mode); return false; }
    virtual bool WriteRegister(soc_addr_t addr, soc_word_t value, WriteMode mode);
    virtual bool IsDirty() { return false; }
    virtual bool Commit() { return true; }
    HWStubDevice *GetDevice() { return m_dev; }

protected:
    QString m_soc;
    HWStubDevice *m_dev;
};

#if LIBUSB_API_VERSION < 0x01000102
#define LIBUSB_NO_HOTPLUG
#endif

class HWStubBackendHelper : public QObject
{
    Q_OBJECT
public:
    HWStubBackendHelper();
    ~HWStubBackendHelper();
    bool HasHotPlugSupport();
    QList< HWStubDevice* > GetDevList();

signals:
    void OnDevListChanged(bool arrived, struct libusb_device *dev);

protected:
#ifndef LIBUSB_NO_HOTPLUG
    void OnHotPlug(bool arrived, struct libusb_device *dev);
    static int HotPlugCallback(struct libusb_context *ctx, struct libusb_device *dev,
        libusb_hotplug_event event, void *user_data);
    libusb_hotplug_callback_handle m_hotplug_handle;
#endif
    bool m_hotplug;
};
#endif

class SocRef;

class SocFile
{
public:
    SocFile();
    SocFile(const QString& filename);
    bool IsValid();

    SocRef GetSocRef();
    QString GetFilename();
    soc_t& GetSoc() { return m_soc; }

protected:
    bool m_valid;
    QString m_filename;
    soc_t m_soc;
};

class SocFileRef
{
public:
    SocFileRef():m_socfile(0) {}
    SocFileRef(SocFile *file):m_socfile(file) {}
    SocFile *GetSocFile() const { return m_socfile; }

protected:
    SocFile *m_socfile;
};

Q_DECLARE_METATYPE(SocFileRef)

class SocRef : public SocFileRef
{
public:
    SocRef() {}
    SocRef(SocFile *file):SocFileRef(file) {}
    soc_t& GetSoc() const { return GetSocFile()->GetSoc(); }
};

Q_DECLARE_METATYPE(SocRef)

class SocDevRef : public SocRef
{
public:
    SocDevRef() {}
    SocDevRef(const SocRef& soc, int dev_idx, int dev_addr_idx)
        :SocRef(soc), m_dev_idx(dev_idx), m_dev_addr_idx(dev_addr_idx) {}
    int GetDevIndex() const { return m_dev_idx; }
    soc_dev_t& GetDev() const { return GetSoc().dev[GetDevIndex()]; }
    int GetDevAddrIndex() const { return m_dev_addr_idx; }
    soc_dev_addr_t& GetDevAddr() const { return GetDev().addr[GetDevAddrIndex()]; }
protected:
    int m_dev_idx, m_dev_addr_idx;
};

class SocRegRef : public SocDevRef
{
public:
    SocRegRef() {}
    SocRegRef(const SocDevRef& dev, int reg_idx, int reg_addr_idx)
        :SocDevRef(dev), m_reg_idx(reg_idx), m_reg_addr_idx(reg_addr_idx) {}
    int GetRegIndex() const { return m_reg_idx; }
    soc_reg_t& GetReg() const { return GetDev().reg[GetRegIndex()]; }
    int GetRegAddrIndex() const { return m_reg_addr_idx; }
    soc_reg_addr_t& GetRegAddr() const { return GetReg().addr[GetRegAddrIndex()]; }
protected:
    int m_reg_idx, m_reg_addr_idx;
};

class SocFieldRef : public SocRegRef
{
public:
    SocFieldRef(){}
    SocFieldRef(const SocRegRef& reg, int field_idx)
        :SocRegRef(reg), m_field_idx(field_idx) {}
    int GetFieldIndex() const { return m_field_idx; }
    soc_reg_field_t& GetField() const { return GetReg().field[GetFieldIndex()]; }
protected:
    int m_field_idx;
};

class Backend : public QObject
{
    Q_OBJECT
public:
    Backend();

    QList< SocFileRef > GetSocFileList();
    QList< SocRef > GetSocList();
    bool LoadSocDesc(const QString& filename);
    IoBackend *CreateDummyIoBackend();
    IoBackend *CreateFileIoBackend(const QString& filename);
#ifdef HAVE_HWSTUB
    IoBackend *CreateHWStubIoBackend(HWStubDevice *dev);
#endif

signals:
    void OnSocListChanged();
private:
    std::list< SocFile > m_socs;
};

class BackendHelper
{
public:
    BackendHelper(IoBackend *io_backend, const SocRef& soc);
    bool ReadRegister(const QString& dev, const QString& reg, soc_word_t& v);
    bool ReadRegisterField(const QString& dev, const QString& reg,
        const QString& field, soc_word_t& v);
    bool WriteRegister(const QString& dev, const QString& reg, soc_word_t v,
        IoBackend::WriteMode mode = IoBackend::Write);
    bool GetDevRef(const QString& dev, SocDevRef& ref);
    bool GetRegRef(const SocDevRef& dev, const QString& reg, SocRegRef& ref);
    bool GetFieldRef(const SocRegRef& reg, const QString& field, SocFieldRef& ref);
    bool GetRegisterAddress(const QString& dev, const QString& reg, soc_addr_t& addr);
    /* NOTE: does not commit writes to the backend
     * if ignore_errors is true, the dump will continue even on errors, and the
     * function will return false if one or more errors occured */
    bool DumpAllRegisters(IoBackend *backend, bool ignore_errors = true);
    bool DumpAllRegisters(const QString& filename, bool ignore_errors = true);

private:
    IoBackend *m_io_backend;
    const SocRef& m_soc;
};

#endif /* __BACKEND_H__ */
