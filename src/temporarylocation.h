#ifndef TEMPORARYLOCATION_H
#define TEMPORARYLOCATION_H

class DataSource;

#include <QDir>

/*============================================================================
================================ TemporaryLocation ===========================
============================================================================*/

class TemporaryLocation : public QDir
{
private:
    DataSource * const Source;
public:
    explicit TemporaryLocation(DataSource *source);
    ~TemporaryLocation();
public:
    bool isValid() const;
    DataSource *cource() const;
public:
    Q_DISABLE_COPY(TemporaryLocation)
};

#endif // TEMPORARYLOCATION_H
