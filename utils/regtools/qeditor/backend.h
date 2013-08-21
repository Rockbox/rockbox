#ifndef __BACKEND_H__
#define __BACKEND_H__

#include <QObject>
#include <QStringList>
#include <QMap>
#include "soc_desc.hpp"

class IoBackend : public QObject
{
    Q_OBJECT
public:
    IoBackend();

    virtual QString GetSocName() = 0;
    virtual bool ReadRegister(const QString& name, soc_word_t& value) = 0;
    virtual bool Reload() = 0;
};

class DummyIoBackend : public IoBackend
{
    Q_OBJECT
public:
    DummyIoBackend();

    virtual QString GetSocName();
    virtual bool ReadRegister(const QString& name, soc_word_t& value);
    virtual bool Reload();
};

class FileIoBackend : public IoBackend
{
    Q_OBJECT
public:
    FileIoBackend(const QString& filename);

    virtual QString GetSocName();
    virtual bool ReadRegister(const QString& name, soc_word_t& value);
    virtual bool Reload();

protected:
    QString m_filename;
    QString m_soc;
    QMap< QString, soc_word_t > m_map;
};

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
private:
    IoBackend *m_io_backend;
    soc_t m_soc;
};

#endif /* __BACKEND_H__ */
