#ifndef REGISTRATIONCONFIRMATIONCODEREPOSITORY_H
#define REGISTRATIONCONFIRMATIONCODEREPOSITORY_H

class DataSource;

#include "entity/registrationconfirmationcode.h"

#include <TIdList>

#include <QList>

/*============================================================================
================================ RegistrationConfirmationCodeRepository ======
============================================================================*/

class RegistrationConfirmationCodeRepository
{
private:
    DataSource * const Source;
public:
    explicit RegistrationConfirmationCodeRepository(DataSource *source);
    ~RegistrationConfirmationCodeRepository();
public:
    long count();
    DataSource *dataSource() const;
    bool deleteAll();
    QList<RegistrationConfirmationCode> findAll();
    bool isValid() const;
    bool save(const RegistrationConfirmationCode &entity);
    bool save(const QList<RegistrationConfirmationCode> &entities);
private:
    friend class RegistrationConfirmationCode;
    Q_DISABLE_COPY(RegistrationConfirmationCodeRepository)
};

#endif // REGISTRATIONCONFIRMATIONCODEREPOSITORY_H
