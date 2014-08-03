#include "registrationconfirmationcoderepository.h"

#include "datasource.h"
#include "entity/registrationconfirmationcode.h"
#include "transactionholder.h"

#include <TIdList>

#include <BSqlResult>
#include <BSqlWhere>

#include <QList>
#include <QString>

/*============================================================================
================================ RegistrationConfirmationCodeRepository ======
============================================================================*/

/*============================== Public constructors =======================*/

RegistrationConfirmationCodeRepository::RegistrationConfirmationCodeRepository(DataSource *source) :
    Source(source)
{
    //
}

RegistrationConfirmationCodeRepository::~RegistrationConfirmationCodeRepository()
{
    //
}

/*============================== Public methods ============================*/

long RegistrationConfirmationCodeRepository::count()
{
    //
}

DataSource *RegistrationConfirmationCodeRepository::dataSource() const
{
    return Source;
}

bool RegistrationConfirmationCodeRepository::deleteAll()
{
    //
}

QList<RegistrationConfirmationCode> RegistrationConfirmationCodeRepository::findAll()
{
    //
}

bool RegistrationConfirmationCodeRepository::isValid() const
{
    return Source && Source->isValid();
}

bool RegistrationConfirmationCodeRepository::save(const RegistrationConfirmationCode &entity)
{
    //
}

bool RegistrationConfirmationCodeRepository::save(const QList<RegistrationConfirmationCode> &entities)
{
    //
}
