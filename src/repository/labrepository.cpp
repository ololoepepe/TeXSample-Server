#include "labrepository.h"

#include "datasource.h"
#include "entity/lab.h"
#include "transactionholder.h"

#include <TBinaryFile>
#include <TBinaryFileList>
#include <TIdList>
#include <TTexProject>

#include <BSqlResult>
#include <BSqlWhere>

#include <QList>
#include <QString>

/*============================================================================
================================ LabRepository ===============================
============================================================================*/

/*============================== Public constructors =======================*/

LabRepository::LabRepository(DataSource *source) :
    Source(source)
{
    //
}

LabRepository::~LabRepository()
{
    //
}

/*============================== Public methods ============================*/

long LabRepository::count()
{
    //
}

DataSource *LabRepository::dataSource() const
{
    return Source;
}

bool LabRepository::deleteAll()
{
    //
}

bool LabRepository::deleteOne(quint64 id)
{
    if (!isValid() || !id)
        return false;
    TransactionHolder holder(Source);
    holder.setSuccess(Source->deleteFrom("labs", BSqlWhere("id = :id", ":id", id)).success());
    return holder.success();
}

bool LabRepository::deleteSome(const TIdList &ids)
{
    //
}

bool LabRepository::exists(quint64 id)
{
    //
}

QList<Lab> LabRepository::findAll()
{
    //
}

QList<Lab> LabRepository::findAll(const TIdList &ids)
{
    //
}

Lab LabRepository::findOne(quint64 id)
{
    //
}

bool LabRepository::isValid() const
{
    return Source && Source->isValid();
}

quint64 LabRepository::save(const Lab &entity)
{
    //
}

TIdList LabRepository::save(const QList<Lab> &entities)
{
    //
}

/*============================== Private methods ===========================*/

//fetch
