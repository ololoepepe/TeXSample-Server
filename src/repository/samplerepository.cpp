#include "samplerepository.h"

#include "datasource.h"
#include "entity/sample.h"
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
================================ SampleRepository ============================
============================================================================*/

/*============================== Public constructors =======================*/

SampleRepository::SampleRepository(DataSource *source) :
    Source(source)
{
    //
}

SampleRepository::~SampleRepository()
{
    //
}

/*============================== Public methods ============================*/

long SampleRepository::count()
{
    //
}

DataSource *SampleRepository::dataSource() const
{
    return Source;
}

bool SampleRepository::deleteAll()
{
    //
}

bool SampleRepository::deleteOne(quint64 id)
{
    if (!isValid() || !id)
        return false;
    TransactionHolder holder(Source);
    holder.setSuccess(Source->deleteFrom("samples", BSqlWhere("id = :id", ":id", id)).success());
    return holder.success();
}

bool SampleRepository::deleteSome(const TIdList &ids)
{
    //
}

bool SampleRepository::exists(quint64 id)
{
    //
}

QList<Sample> SampleRepository::findAll()
{
    //
}

QList<Sample> SampleRepository::findAll(const TIdList &ids)
{
    //
}

Sample SampleRepository::findOne(quint64 id)
{
    //
}

bool SampleRepository::isValid() const
{
    return Source && Source->isValid();
}

quint64 SampleRepository::save(const Sample &entity)
{
    //
}

TIdList SampleRepository::save(const QList<Sample> &entities)
{
    //
}

/*============================== Private methods ===========================*/

bool SampleRepository::fetchPreview(quint64 sampleId, TBinaryFile &mainFile, TBinaryFileList &extraFiles)
{
    //
}

TTexProject SampleRepository::fetchSource(quint64 sampleId, bool *ok)
{
    //
}
