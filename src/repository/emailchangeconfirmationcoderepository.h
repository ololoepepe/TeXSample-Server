#ifndef EMAILCHANGECONFIRMATIONCODEREPOSITORY_H
#define EMAILCHANGECONFIRMATIONCODEREPOSITORY_H

class DataSource;

class BUuid;

#include "entity/emailchangeconfirmationcode.h"

#include <TIdList>

#include <QList>

/*============================================================================
================================ EmailChangeConfirmationCodeRepository =======
============================================================================*/

class EmailChangeConfirmationCodeRepository
{
private:
    DataSource * const Source;
public:
    explicit EmailChangeConfirmationCodeRepository(DataSource *source);
    ~EmailChangeConfirmationCodeRepository();
public:
    bool add(const EmailChangeConfirmationCode &entity);
    bool deleteExpired();
    bool deleteOneByUserId(quint64 userId);
    QList<EmailChangeConfirmationCode> findExpired();
    EmailChangeConfirmationCode findOneByCode(const BUuid &code);
    DataSource *dataSource() const;
    bool isValid() const;
private:
    friend class EmailChangeConfirmationCode;
    Q_DISABLE_COPY(EmailChangeConfirmationCodeRepository)
};

#endif // EMAILCHANGECONFIRMATIONCODEREPOSITORY_H
