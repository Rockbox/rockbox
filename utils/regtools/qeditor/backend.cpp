#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "backend.h"

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

IoBackend::IoBackend()
{
}

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

DummyIoBackend::DummyIoBackend()
{
}

QString DummyIoBackend::GetSocName()
{
    return "";
}

bool DummyIoBackend::ReadRegister(const QString& name, soc_word_t& value)
{
    (void) name;
    (void) value;
    return false;
}

bool DummyIoBackend::Reload()
{
    return true;
}

BackendHelper::BackendHelper(IoBackend *io_backend, const soc_t& soc)
    :m_io_backend(io_backend), m_soc(soc)
{
}

bool BackendHelper::ReadRegister(const QString& dev, const QString& reg, soc_word_t& v)
{
    return m_io_backend->ReadRegister("HW." + dev + "." + reg, v);
}

bool BackendHelper::ReadRegisterField(const QString& dev, const QString& reg,
    const QString& field, soc_word_t& v)
{
    soc_dev_t *sdev = 0;
    for(size_t i = 0; i < m_soc.dev.size(); i++)
    {
        for(size_t j = 0; j < m_soc.dev[i].addr.size(); j++)
            if(m_soc.dev[i].addr[j].name.c_str() == dev)
                sdev = &m_soc.dev[i];
    }
    if(sdev == 0)
        return false;
    soc_reg_t *sreg = 0;
    for(size_t i = 0; i < sdev->reg.size(); i++)
    {
        for(size_t j = 0; j < sdev->reg[i].addr.size(); j++)
            if(sdev->reg[i].addr[j].name.c_str() == reg)
                sreg = &sdev->reg[i];
    }
    if(sreg == 0)
        return false;
    soc_reg_field_t *sfield = 0;
    for(size_t i = 0; i < sreg->field.size(); i++)
        if(sreg->field[i].name.c_str() == field)
            sfield = &sreg->field[i];
    if(sfield == 0)
        return false;
    if(!ReadRegister(dev, reg, v))
        return false;
    v = (v & sfield->bitmask()) >> sfield->first_bit;
    return true;
}