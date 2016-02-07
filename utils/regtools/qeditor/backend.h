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
#include <QAbstractItemModel>
#ifdef HAVE_HWSTUB
#include "hwstub.hpp"
#include "hwstub_usb.hpp"
#include "hwstub_uri.hpp"
#endif
#include "soc_desc.hpp"

/* we don't want to import the entire soc_desc except for a few selected
 * pieces */
using soc_desc::soc_word_t;
using soc_desc::soc_addr_t;
using soc_desc::soc_id_t;

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

    /* report whether backend is valid */
    virtual bool IsValid() = 0;
    /* get SoC name */
    virtual QString GetSocName() = 0;
    /* read a register */
    virtual bool ReadRegister(soc_addr_t addr, soc_word_t& value, unsigned width) = 0;
    /* reload content (if it makes sense) */
    virtual bool Reload() = 0;
    /* check whether backend supports writing */
    virtual bool IsReadOnly() = 0;
    /* write a register by name or address
     * NOTE: even on a read-only backend, a write is allowed to be successful as long
     * as commit fails */
    virtual bool WriteRegister(soc_addr_t addr, soc_word_t value, unsigned width,
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
    DummyIoBackend();

    virtual bool IsValid();
    virtual QString GetSocName();
    virtual bool ReadRegister(soc_addr_t addr, soc_word_t& value, unsigned width);
    virtual bool Reload();
    virtual bool IsReadOnly();
    virtual bool WriteRegister(soc_addr_t addr, soc_word_t value, unsigned width,
        WriteMode mode);
    virtual bool IsDirty();
    virtual bool Commit();
};

/** The RAM backend doesn't have any backend storage and stores all values in
 * an associative map */
class RamIoBackend : public IoBackend
{
    Q_OBJECT
public:
    RamIoBackend(const QString& soc_name = "");

    virtual bool IsValid();
    virtual QString GetSocName();
    virtual void SetSocName(const QString& soc_name);
    virtual bool ReadRegister(soc_addr_t addr, soc_word_t& value, unsigned width);
    virtual bool Reload();
    virtual bool IsReadOnly();
    virtual bool WriteRegister(soc_addr_t addr, soc_word_t value, unsigned width,
        WriteMode mode);
    virtual bool IsDirty();
    virtual bool Commit();
    /* clear all entries of the backend */
    virtual void DeleteAll();

protected:
    QString m_soc;
    QMap< soc_addr_t, soc_word_t > m_map;
};

/** NOTE the File backend makes a difference between writes and commits:
 * a write will *never* touch the underlying file unless it was committed. */
class FileIoBackend : public RamIoBackend
{
    Q_OBJECT
public:
    FileIoBackend(const QString& filename, const QString& soc_name = "");

    virtual bool IsValid();
    virtual bool Reload();
    virtual bool IsReadOnly();
    virtual bool WriteRegister(soc_addr_t addr, soc_word_t value, unsigned width,
        WriteMode mode);
    virtual bool IsDirty();
    virtual bool Commit();
    QString GetFileName();

protected:
    QString m_filename;
    bool m_readonly;
    bool m_dirty;
    bool m_valid;
};

#ifdef HAVE_HWSTUB
/* HWStub context manager: provides a centralized place to add/remove/get
 * contexts */
class HWStubManager : public QAbstractTableModel
{
    Q_OBJECT
protected:
    HWStubManager();
public:
    virtual ~HWStubManager();
    /* Get manager */
    static HWStubManager *Get();
    /* Clear the context list */
    void Clear();
    bool Add(const QString& name, const QString& uri);
    /* Model */
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex& index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role);
    /* return predefined columns */
    int GetNameColumn() const;
    int GetUriColumn() const;
    std::shared_ptr< hwstub::context > GetContext(int row);
    /* return a friendly name for a device */
    static QString GetFriendlyName(std::shared_ptr< hwstub::device > device);

protected:
    struct Context
    {
        std::shared_ptr< hwstub::context > context;
        QString name;
        QString uri;
    };

    std::vector< Context > m_list; /* list of context */
    static HWStubManager *g_inst; /* unique instance */
};

/* HWStub context model: provides access to the device list using Qt MVC model. */
class HWStubContextModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    HWStubContextModel(QObject *parent = 0);
    virtual ~HWStubContextModel();
    void SetContext(std::shared_ptr< hwstub::context > context);
    /* Model */
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex& index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const;
    /* return predefined columns */
    int GetNameColumn() const;
    std::shared_ptr< hwstub::device > GetDevice(int row);
    /* Add an entry in the model for "no selection", which will correspond to
     * a dummy device. This function also allows the text to be customised. */
    void EnableDummy(bool en, const QString& text = "");

private slots:
    void OnDevChangeUnsafe(void *data);
protected:
    QString GetFriendlyName(std::shared_ptr< hwstub::device > device);
    void OnDevChangeLow(std::shared_ptr< hwstub::context > ctx, bool arrived,
        std::shared_ptr< hwstub::device > device);
    void OnDevChange(std::shared_ptr< hwstub::context > ctx, bool arrived,
        std::shared_ptr< hwstub::device > device);

    struct Device
    {
        QString name;
        std::shared_ptr< hwstub::device > device;
    };

    std::vector< Device > m_list;
    std::weak_ptr< hwstub::context > m_context;
    hwstub::context::callback_ref_t m_callback_ref;
    bool m_has_dummy;
    QString m_dummy_text;
};

/* Abstract Virtual Class from where TCP and USB backend
 * child classes are derived
 */
class HWStubDevice
{
public:
    HWStubDevice(std::shared_ptr<hwstub::device> device);
    virtual ~HWStubDevice();

    QString GetFriendlyName();
    bool IsValid();
    /* Calls below are cached */
    inline struct hwstub_version_desc_t GetVersionInfo() { return m_hwdev_ver; }
    inline struct hwstub_target_desc_t GetTargetInfo() { return m_hwdev_target; }
    inline struct hwstub_stmp_desc_t GetSTMPInfo() { return m_hwdev_stmp; }
    inline struct hwstub_pp_desc_t GetPPInfo() { return m_hwdev_pp; }
    inline struct hwstub_jz_desc_t GetJZInfo() { return m_hwdev_jz; }
    bool ReadMem(soc_addr_t addr, size_t length, void *buffer);
    bool WriteMem(soc_addr_t addr, size_t length, void *buffer);

protected:
    bool Probe(std::shared_ptr<hwstub::device> device);

    std::shared_ptr<hwstub::handle> m_handle;
    bool m_valid;
    struct hwstub_device_t *m_hwdev;
    struct hwstub_version_desc_t m_hwdev_ver;
    struct hwstub_target_desc_t m_hwdev_target;
    struct hwstub_stmp_desc_t m_hwdev_stmp;
    struct hwstub_pp_desc_t m_hwdev_pp;
    struct hwstub_jz_desc_t m_hwdev_jz;
    QString m_name;
};

/** NOTE the HWStub backend is never dirty: all wrnew ites are immediately committed */
class HWStubIoBackend : public IoBackend
{
    Q_OBJECT
public:
    // NOTE: HWStubIoBackend takes ownership of the device and will delete it
    HWStubIoBackend(HWStubDevice *dev);
    virtual ~HWStubIoBackend();

    virtual bool IsValid();
    virtual QString GetSocName();
    virtual bool ReadRegister(soc_addr_t addr, soc_word_t& value, unsigned width);
    virtual bool Reload();
    virtual bool IsReadOnly();
    virtual bool WriteRegister(soc_addr_t addr, soc_word_t value, unsigned width,
        WriteMode mode);
    virtual bool IsDirty();
    virtual bool Commit();
    HWStubDevice *GetDevice();

protected:
    QString m_soc;
    HWStubDevice *m_dev;
};
#endif

class SocFile
{
public:
    SocFile();
    SocFile(const QString& filename);
    bool IsValid();

    soc_desc::soc_ref_t GetSocRef();
    QString GetFilename();
    soc_desc::soc_t& GetSoc() { return m_soc; }

protected:
    bool m_valid;
    QString m_filename;
    soc_desc::soc_t m_soc;
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

Q_DECLARE_METATYPE(soc_desc::instance_t::type_t)
Q_DECLARE_METATYPE(soc_desc::range_t::type_t)

Q_DECLARE_METATYPE(soc_desc::soc_ref_t)
Q_DECLARE_METATYPE(soc_desc::node_ref_t)
Q_DECLARE_METATYPE(soc_desc::register_ref_t)
Q_DECLARE_METATYPE(soc_desc::field_ref_t)
Q_DECLARE_METATYPE(soc_desc::node_inst_t)

/** NOTE the Backend stores soc descriptions in a way that pointers are never
 * invalidated */
class Backend : public QObject
{
    Q_OBJECT
public:
    Backend();

    QList< SocFileRef > GetSocFileList();
    QList< soc_desc::soc_ref_t > GetSocList();
    bool LoadSocDesc(const QString& filename);
    IoBackend *CreateDummyIoBackend();
    IoBackend *CreateFileIoBackend(const QString& filename);
#ifdef HAVE_HWSTUB
    IoBackend *CreateHWStubIoBackend(HWStubDevice *dev);
#endif

signals:
    void OnSocAdded(const SocFileRef& ref);

private:
    /* store them as a list so that pointers are never invalidated */
    std::list< SocFile > m_socs;
};

class BackendHelper
{
public:
    BackendHelper(IoBackend *io_backend, const soc_desc::soc_ref_t& soc);
    QString GetPath(const soc_desc::node_inst_t& inst);
    soc_desc::node_inst_t ParsePath(const QString& path);
    bool ReadRegister(const soc_desc::node_inst_t& inst, soc_word_t& v);
    bool ReadRegisterField(const soc_desc::node_inst_t& inst,
        const QString& field, soc_word_t& v);
    bool WriteRegister(const soc_desc::node_inst_t& inst, soc_word_t v,
        IoBackend::WriteMode mode = IoBackend::Write);
    bool GetRegisterAddress(const soc_desc::node_inst_t& inst, soc_addr_t& addr);
    /* NOTE: does not commit writes to the backend
     * if ignore_errors is true, the dump will continue even on errors, and the
     * function will return false if one or more errors occured */
    bool DumpAllRegisters(IoBackend *backend, bool ignore_errors = true);
    bool DumpAllRegisters(const QString& filename, bool ignore_errors = true);

protected:
    bool DumpAllRegisters(BackendHelper *bh, const soc_desc::node_inst_t& inst,
        bool ignore_errors);

private:
    IoBackend *m_io_backend;
    const soc_desc::soc_ref_t& m_soc;
};

#endif /* __BACKEND_H__ */
