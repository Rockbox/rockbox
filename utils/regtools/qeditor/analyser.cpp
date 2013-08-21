#include "analyser.h"

Analyser::Analyser(const soc_t& soc, IoBackend *backend)
    :m_soc(soc), m_io_backend(backend)
{
}

Analyser::~Analyser()
{
}

AnalyserFactory::AnalyserFactory(bool _register)
{
    if(_register)
        RegisterAnalyser(this);
}

AnalyserFactory::~AnalyserFactory()
{
}

QVector< AnalyserFactory * > AnalyserFactory::m_factories;

QStringList AnalyserFactory::GetAnalysersForSoc(const QString& soc_name)
{
    QStringList list;
    for(int i = 0; i < m_factories.size(); i++)
        if(m_factories[i]->SupportSoc(soc_name))
            list.append(m_factories[i]->GetName());
    return list;
}

AnalyserFactory *AnalyserFactory::GetAnalyserByName(const QString& name)
{
    for(int i = 0; i < m_factories.size(); i++)
        if(m_factories[i]->GetName() == name)
            return m_factories[i];
    return 0;
}

void AnalyserFactory::RegisterAnalyser(AnalyserFactory *factory)
{
    m_factories.append(factory);
}
