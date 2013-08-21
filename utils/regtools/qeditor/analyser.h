#ifndef _ANALYSER_H_
#define _ANALYSER_H_

#include <QObject>
#include <QVector>
#include <QString>
#include "backend.h"

class Analyser : public QObject
{
    Q_OBJECT
public:
    Analyser(const soc_t& soc, IoBackend *backend);
    virtual ~Analyser();
    virtual QWidget *GetWidget() = 0;

protected:
    soc_t m_soc;
    IoBackend *m_io_backend;
};

class AnalyserFactory
{
public:
    AnalyserFactory(bool _register);
    virtual ~AnalyserFactory();

    virtual QString GetName() = 0;
    virtual bool SupportSoc(const QString& soc_name) = 0;
    // return NULL of soc is not handled by the analyser
    virtual Analyser *Create(const soc_t& soc, IoBackend *backend) = 0;
private:
    QString m_name;

public:
    static QStringList GetAnalysersForSoc(const QString& soc_name);
    static AnalyserFactory *GetAnalyserByName(const QString& name);
    static void RegisterAnalyser(AnalyserFactory *factory);

private:
    static QVector< AnalyserFactory * > m_factories;
};

template< typename T >
class TmplAnalyserFactory : public AnalyserFactory
{
public:
    TmplAnalyserFactory(bool _register, const QString& name) :AnalyserFactory(_register) { m_name = name; }
    virtual ~TmplAnalyserFactory() {}

    virtual QString GetName() { return m_name; }
    virtual bool SupportSoc(const QString& soc_name) { return T::SupportSoc(soc_name); }
    // return NULL of soc is not handled by the analyser
    virtual T *Create(const soc_t& soc, IoBackend *backend)
    {
        if(!T::SupportSoc(soc.name.c_str()))
            return 0;
        return new T(soc, backend);
    }
private:
    QString m_name;
};

#endif /* _ANALYSER_H_ */
