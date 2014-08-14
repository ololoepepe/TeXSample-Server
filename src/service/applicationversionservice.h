#ifndef APPLICATIONVERSIONSERVICE_H
#define APPLICATIONVERSIONSERVICE_H

class ApplicationVersionRepository;
class DataSource;

#include <TeXSample>

#include <BeQt>
#include <BVersion>

#include <QCoreApplication>
#include <QUrl>

/*============================================================================
================================ ApplicationVersionService ===================
============================================================================*/

class ApplicationVersionService
{
    Q_DECLARE_TR_FUNCTIONS(ApplicationVersionService)
private:
    ApplicationVersionRepository * const ApplicationVersionRepo;
    DataSource * const Source;
public:
    explicit ApplicationVersionService(DataSource *source);
    ~ApplicationVersionService();
public:
    DataSource *dataSource() const;
    bool isValid() const;
    bool setApplicationVersion(Texsample::ClientType clienType, BeQt::OSType os, BeQt::ProcessorArchitecture arch,
                               bool portable, const BVersion &version, const QUrl &downloadUrl = QUrl());
private:
    Q_DISABLE_COPY(ApplicationVersionService)
};

#endif // APPLICATIONVERSIONSERVICE_H
