#ifndef SAMPLESERVICE_H
#define SAMPLESERVICE_H

class DataSource;
class SampleRepository;

#include <QCoreApplication>

/*============================================================================
================================ SampleService ===============================
============================================================================*/

class SampleService
{
    Q_DECLARE_TR_FUNCTIONS(SampleService)
private:
    SampleRepository * const SampleRepo;
    DataSource * const Source;
public:
    explicit SampleService(DataSource *source);
    ~SampleService();
public:
    DataSource *dataSource() const;
    bool isValid() const;
private:
    Q_DISABLE_COPY(SampleService)
};

#endif // SAMPLESERVICE_H
