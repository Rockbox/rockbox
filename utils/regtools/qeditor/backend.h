#ifndef __BACKEND_H__
#define __BACKEND_H__

#include <QObject>
#include <QStringList>
#include <QMap>
#include <QVector>
#include "soc_desc.hpp"
#ifdef HAVE_HWSTUB
#include "hwstub.h"
#endif

class IoBackend : public QObject
{
    Q_OBJECT
public:
    IoBackend() {}
    virtual ~IoBackend() {}

    enum AccessType
    {
        ByName,
        ByAddress,
    };

    virtual bool SupportAccess(AccessType type) = 0;
    virtual QString GetSocName() = 0;
    virtual bool ReadRegister(const QString& name, soc_word_t& value) = 0;
    virtual bool ReadRegister(soc_addr_t addr, soc_word_t& value) = 0;
    virtual bool Reload() = 0;
};

class DummyIoBackend : public IoBackend
{
    Q_OBJECT
public:
    DummyIoBackend() {}

    virtual bool SupportAccess(AccessType type) { (void) type; return false; }
    virtual QString GetSocName() { return ""; }
    virtual bool ReadRegister(const QString& name, soc_word_t& value) 
        { (void) name; (void) value; return false; }
    virtual bool ReadRegister(soc_addr_t addr, soc_word_t& value) 
        { (void) addr; (void) value; return false; }
    virtual bool Reload() { return false; }
};

class FileIoBackend : public IoBackend
{
    Q_OBJECT
public:
    FileIoBackend(const QString& filename);

    virtual bool SupportAccess(AccessType type) { return type == ByName; }
    virtual QString GetSocName();
    virtual bool ReadRegister(const QString& name, soc_word_t& value);
    virtual bool ReadRegister(soc_addr_t addr, soc_word_t& value)
        { (void) addr; (void) value; return false; }
    virtual bool Reload();

protected:
    QString m_filename;
    QString m_soc;
    QMap< QString, soc_word_t > m_map;
};

#ifdef HAVE_HWSTUB
class HWStubDevice
{
public:
    HWStubDevice(struct libusb_device *dev);
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
    /* Calls below require the device to be opened */
    bool ReadMem(soc_addr_t addr, size_t length, void *buffer);

protected:
    bool Probe();

    bool m_valid;
    struct libusb_device *m_dev;
    libusb_device_handle *m_handle;
    struct hwstub_device_t *m_hwdev;
    struct hwstub_version_desc_t m_hwdev_ver;
    struct hwstub_target_desc_t m_hwdev_target;
    struct hwstub_stmp_desc_t m_hwdev_stmp;
};

class HWStubIoBackend : public IoBackend
{
    Q_OBJECT
public:
    HWStubIoBackend(HWStubDevice *dev);
    virtual ~HWStubIoBackend();

    virtual bool SupportAccess(AccessType type) { return type == ByAddress; }
    virtual QString GetSocName();
    virtual bool ReadRegister(const QString& name, soc_word_t& value)
        { (void) name; (void) value; return false; }
    virtual bool ReadRegister(soc_addr_t addr, soc_word_t& value);
    virtual bool Reload();

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

class Backend : public QObject
{
    Q_OBJECT
public:
    Backend();

    QStringList GetSocNameList();
    bool LoadSocDesc(const QString& filename);
    bool GetSocByName(const QString& name, soc_t& s);
    IoBackend *CreateDummyIoBackend();
    IoBackend *CreateFileIoBackend(const QString& filename);
#ifdef HAVE_HWSTUB
    IoBackend *CreateHWStubIoBackend(HWStubDevice *dev);
#endif

signals:
    void OnSocListChanged();
private:
    std::vector< soc_t > m_socs;
};

class BackendHelper
{
public:
    BackendHelper(IoBackend *io_backend, const soc_t& soc);
    bool ReadRegister(const QString& dev, const QString& reg, soc_word_t& v);
    bool ReadRegisterField(const QString& dev, const QString& reg,
        const QString& field, soc_word_t& v);
    bool GetDeviceDesc(const QString& dev, soc_dev_t& dev_desc, size_t& index);
    bool GetRegisterDesc(const soc_dev_t& dev, const QString& reg, soc_reg_t& reg_desc, size_t& index);
    bool GetFieldDesc(const soc_reg_t& reg_desc, const QString& field, soc_reg_field_t& field_desc);
    bool GetRegisterAddress(const QString& dev, const QString& reg, soc_addr_t& addr);
private:
    IoBackend *m_io_backend;
    soc_t m_soc;
};

#endif /* __BACKEND_H__ */
