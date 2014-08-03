#include "invitecoderepository.h"

#include "datasource.h"
#include "entity/invitecode.h"
#include "transactionholder.h"

#include <TIdList>

#include <BSqlResult>
#include <BSqlWhere>

#include <QList>
#include <QString>

/*============================================================================
================================ InviteCodeRepository ========================
============================================================================*/

/*============================== Public constructors =======================*/

InviteCodeRepository::InviteCodeRepository(DataSource *source) :
    Source(source)
{
    //
}

InviteCodeRepository::~InviteCodeRepository()
{
    //
}

/*============================== Public methods ============================*/

long InviteCodeRepository::count()
{
    //
}

DataSource *InviteCodeRepository::dataSource() const
{
    return Source;
}

bool InviteCodeRepository::deleteAll()
{
    //
}

bool InviteCodeRepository::deleteOne(quint64 id)
{
    if (!isValid() || !id)
        return false;
    TransactionHolder holder(Source);
    holder.setSuccess(Source->deleteFrom("invite_codes", BSqlWhere("id = :id", ":id", id)).success());
    return holder.success();
}

bool InviteCodeRepository::deleteSome(const TIdList &ids)
{
    //
}

bool InviteCodeRepository::exists(quint64 id)
{
    //
}

QList<InviteCode> InviteCodeRepository::findAll()
{
    //
}

QList<InviteCode> InviteCodeRepository::findAll(const TIdList &ids)
{
    //
}

InviteCode InviteCodeRepository::findOne(quint64 id)
{
    //
}

bool InviteCodeRepository::isValid() const
{
    return Source && Source->isValid();
}

quint64 InviteCodeRepository::save(const InviteCode &entity)
{
    //
}

TIdList InviteCodeRepository::save(const QList<InviteCode> &entities)
{
    //
}
