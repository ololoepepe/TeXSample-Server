#ifndef REGISTRATIONCONFIRMATIONCODEREPOSITORY_H
#define REGISTRATIONCONFIRMATIONCODEREPOSITORY_H

class DataSource;

class BUuid;

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
    bool add(const RegistrationConfirmationCode &entity);
    bool deleteExpired();
    bool deleteOneByUserId(quint64 userId);
    QList<RegistrationConfirmationCode> findExpired();
    RegistrationConfirmationCode findOneByCode(const BUuid &code);
    DataSource *dataSource() const;
    bool isValid() const;
private:
    friend class RegistrationConfirmationCode;
    Q_DISABLE_COPY(RegistrationConfirmationCodeRepository)
};

#endif // REGISTRATIONCONFIRMATIONCODEREPOSITORY_H
