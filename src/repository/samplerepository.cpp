#include "samplerepository.h"

#include "datasource.h"
#include "entity/sample.h"
#include "repositorytools.h"
#include "transactionholder.h"

#include <TBinaryFile>
#include <TBinaryFileList>
#include <TIdList>
#include <TSampleType>
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

quint64 SampleRepository::add(const Sample &entity)
{
    if (!isValid() || !entity.isValid() || entity.isCreatedByRepo() || entity.id())
        return 0;
    QDateTime dt = QDateTime::currentDateTimeUtc();
    QVariantMap values;
    values.insert("sender_id", entity.senderId());
    values.insert("deleted", false);
    values.insert("admin_remark", QString());
    values.insert("creation_date_time", dt.toMSecsSinceEpoch());
    values.insert("description", entity.description());
    values.insert("last_modification_date_time", dt.toMSecsSinceEpoch());
    values.insert("main_file_name", entity.source().rootFile().fileName());
    values.insert("rating", quint8(0));
    values.insert("title", entity.title());
    values.insert("type", int(entity.type()));
    TransactionHolder holder(Source);
    BSqlResult result = Source->insert("samples", values);
    if (!result.success())
        return 0;
    quint64 id = result.lastInsertId().toULongLong();
    if (!RepositoryTools::setAuthorInfoList(Source, "sample_authors", "sample_id", id, entity.authors()))
        return 0;
    if (!RepositoryTools::setTags(Source, "sample_tags", "sample_id", id, entity.tags()))
        return 0;
    if (!createSource(id, entity.source()) || !holder.doCommit())
        return 0;
    return id;
}

DataSource *SampleRepository::dataSource() const
{
    return Source;
}

bool SampleRepository::edit(const Sample &entity)
{
    if (!isValid() || !entity.isValid() || !entity.id() || entity.isCreatedByRepo())
        return false;
    QDateTime dt = QDateTime::currentDateTimeUtc();
    QVariantMap values;
    values.insert("sender_id", entity.senderId());
    values.insert("deleted", false);
    if (entity.saveAdminRemark())
        values.insert("admin_remark", entity.adminRemark());
    values.insert("description", entity.description());
    values.insert("last_modification_date_time", dt.toMSecsSinceEpoch());
    values.insert("main_file_name", entity.source().rootFile().fileName());
    values.insert("rating", quint8(0));
    values.insert("title", entity.title());
    values.insert("type", int(entity.type()));
    TransactionHolder holder(Source);
    BSqlResult result = Source->update("samples", values, BSqlWhere("id = :id", ":id", entity.id()));
    if (!result.success())
        return false;
    if (!RepositoryTools::deleteHelper(Source, QStringList() << "sample_authors" << "sample_tags", "sample_id",
                                       entity.id())) {
        return false;
    }
    if (!RepositoryTools::setAuthorInfoList(Source, "sample_authors", "sample_id", entity.id(), entity.authors()))
        return false;
    if (!RepositoryTools::setTags(Source, "sample_tags", "sample_id", entity.id(), entity.tags()))
        return false;
    if (entity.saveData() && !updateSource(entity.id(), entity.source()))
        return false;
    return holder.doCommit();
}

QList<Sample> SampleRepository::findAllNewerThan(const QDateTime &newerThan)
{
    QList<Sample> list;
    if (!isValid())
        return list;
    static const QStringList Fields = QStringList() << "id" << "sender_id" << "deleted" << "admin_remark"
        << "creation_date_time" << "description" << "last_modification_date_time" << "rating" << "title" << "type";
    BSqlWhere where;
    if (newerThan.isValid())
        where = BSqlWhere("last_modification_date_time > :last_modification_date_time", ":last_modification_date_time",
                          newerThan.toUTC().toMSecsSinceEpoch());
    BSqlResult result = Source->select("samples", Fields, where);
    if (!result.success() || result.values().isEmpty())
        return list;
    foreach (const QVariantMap &m, result.values()) {
        Sample entity(this);
        entity.mid = m.value("id").toULongLong();
        entity.msenderId = m.value("sender_id").toULongLong();
        entity.mdeleted = m.value("deleted").toBool();
        entity.madminRemark = m.value("admin_remark").toString();
        entity.mcreationDateTime.setMSecsSinceEpoch(m.value("creation_date_time").toLongLong());
        entity.mdescription = m.value("description").toString();
        entity.mlastModificationDateTime.setMSecsSinceEpoch(m.value("last_modification_date_time").toLongLong());
        entity.mrating = m.value("rating").toUInt();
        entity.mtitle = m.value("title").toString();
        entity.mtype = m.value("type").toInt();
        bool ok = false;
        entity.mauthors = RepositoryTools::getAuthorInfoList(Source, "sample_authors", "sample_id", entity.id(), &ok);
        if (!ok)
            return QList<Sample>();
        entity.mtags = RepositoryTools::getTags(Source, "sample_tags", "sample_id", entity.id(), &ok);
        if (!ok)
            return QList<Sample>();
        entity.valid = true;
        list << entity;
    }
    return list;
}

Sample SampleRepository::findOne(quint64 id)
{
    Sample entity(this);
    if (!isValid() || !id)
        return entity;
    static const QStringList Fields = QStringList() << "sender_id" << "deleted" << "admin_remark"
        << "creation_date_time" << "description" << "last_modification_date_time" << "rating" << "title" << "type";
    BSqlResult result = Source->select("samples", Fields, BSqlWhere("id = :id", ":id", id));
    if (!result.success() || result.value().isEmpty())
        return entity;
    entity.mid = result.value("id").toULongLong();
    entity.msenderId = result.value("sender_id").toULongLong();
    entity.mdeleted = result.value("deleted").toBool();
    entity.madminRemark = result.value("admin_remark").toString();
    entity.mcreationDateTime.setMSecsSinceEpoch(result.value("creation_date_time").toLongLong());
    entity.mdescription = result.value("description").toString();
    entity.mlastModificationDateTime.setMSecsSinceEpoch(result.value("last_modification_date_time").toLongLong());
    entity.mrating = result.value("rating").toUInt();
    entity.mtitle = result.value("title").toString();
    entity.mtype = result.value("type").toInt();
    bool ok = false;
    entity.mauthors = RepositoryTools::getAuthorInfoList(Source, "sample_authors", "sample_id", id, &ok);
    if (!ok)
        return entity;
    entity.mtags = RepositoryTools::getTags(Source, "sample_tags", "sample_id", id, &ok);
    if (!ok)
        return entity;
    entity.valid = true;
    return entity;
}

bool SampleRepository::isValid() const
{
    return Source && Source->isValid();
}

/*============================== Private methods ===========================*/

bool SampleRepository::createSource(quint64 sampleId, const TTexProject &data)
{
    //
}

bool SampleRepository::fetchPreview(quint64 sampleId, TBinaryFile &mainFile, TBinaryFileList &extraFiles)
{
    //
}

TTexProject SampleRepository::fetchSource(quint64 sampleId, bool *ok)
{
    //
}


bool SampleRepository::updateSource(quint64 sampleId, const TTexProject &data)
{
    //
}
