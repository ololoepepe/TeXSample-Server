#ifndef APPLICATIONVERSION_H
#define APPLICATIONVERSION_H

class ApplicationVersionRepository;

#include <TeXSample>

#include <BeQt>
#include <BVersion>

#include <QUrl>

/*============================================================================
================================ ApplicationVersion ==========================
============================================================================*/

class ApplicationVersion
{
private:
    Texsample::ClientType mclienType;
    BeQt::OSType mos;
    bool mportable;
    BeQt::ProcessorArchitecture mprocessorArchitecture;
    QUrl mdownloadUrl;
    BVersion mversion;
    bool createdByRepo;
    ApplicationVersionRepository *repo;
    bool valid;
public:
    explicit ApplicationVersion();
    ApplicationVersion(const ApplicationVersion &other);
    ~ApplicationVersion();
protected:
    explicit ApplicationVersion(ApplicationVersionRepository *repo);
public:
    Texsample::ClientType clienType() const;
    void convertToCreatedByUser();
    QUrl downloadUrl() const;
    bool isCreatedByRepo() const;
    bool isValid() const;
    BeQt::OSType os() const;
    bool portable() const;
    BeQt::ProcessorArchitecture processorArchitecture() const;
    ApplicationVersionRepository *repository() const;
    void setClientType(Texsample::ClientType clienType);
    void setDownloadUrl(const QUrl &url);
    void setOs(BeQt::OSType os);
    void setPortable(bool portable);
    void setProcessorArchitecture(BeQt::ProcessorArchitecture arch);
    void setVersion(const BVersion &version);
    BVersion version() const;
public:
    ApplicationVersion &operator =(const ApplicationVersion &other);
private:
    void init();
private:
    friend class ApplicationVersionRepository;
};

#endif // APPLICATIONVERSION_H
