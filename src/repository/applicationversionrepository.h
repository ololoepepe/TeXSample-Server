#ifndef APPLICATIONVERSIONREPOSITORY_H
#define APPLICATIONVERSIONREPOSITORY_H

class DataSource;

#include "entity/applicationversion.h"

#include <TeXSample>

#include <BeQt>

/*============================================================================
================================ ApplicationVersionRepository ================
============================================================================*/

class ApplicationVersionRepository
{
private:
    DataSource * const Source;
public:
    explicit ApplicationVersionRepository(DataSource *source);
    ~ApplicationVersionRepository();
public:
    bool add(const ApplicationVersion &entity);
    DataSource *dataSource() const;
    bool edit(const ApplicationVersion &entity);
    ApplicationVersion findOneByFields(Texsample::ClientType clienType, BeQt::OSType os,
                                       BeQt::ProcessorArchitecture arch, bool portable);
    bool isValid() const;
private:
    friend class ApplicationVersion;
    Q_DISABLE_COPY(ApplicationVersionRepository)
};

#endif // APPLICATIONVERSIONREPOSITORY_H
