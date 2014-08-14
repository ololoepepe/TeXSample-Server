#include "applicationversionrepository.h"

#include "datasource.h"
#include "entity/applicationversion.h"
#include "transactionholder.h"

#include <TeXSample>

#include <BeQt>
#include <BSqlResult>
#include <BSqlWhere>

#include <QDebug>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariant>
#include <QVariantMap>

/*============================================================================
================================ ApplicationVersionRepository ================
============================================================================*/

/*============================== Public constructors =======================*/

ApplicationVersionRepository::ApplicationVersionRepository(DataSource *source) :
    Source(source)
{
    //
}

ApplicationVersionRepository::~ApplicationVersionRepository()
{
    //
}

/*============================== Public methods ============================*/

bool ApplicationVersionRepository::add(const ApplicationVersion &entity)
{
    if (!isValid() || !entity.isValid() || entity.isCreatedByRepo())
        return false;
    QVariantMap values;
    values.insert("client_type", int(entity.clienType()));
    values.insert("os_type", int(entity.os()));
    values.insert("portable", int(entity.portable()));
    values.insert("processor_architecture_type", int(entity.processorArchitecture()));
    values.insert("download_url", entity.downloadUrl().toString());
    values.insert("version", entity.version().toString());
    TransactionHolder holder(Source);
    return Source->insert("application_versions", values).success() && holder.doCommit();
}

DataSource *ApplicationVersionRepository::dataSource() const
{
    return Source;
}

bool ApplicationVersionRepository::edit(const ApplicationVersion &entity)
{
    if (!isValid() || !entity.isValid() || entity.isCreatedByRepo())
        return false;
    QVariantMap values;
    values.insert("download_url", entity.downloadUrl().toString());
    values.insert("version", entity.version().toString());
    QString ws = "client_type = :client_type AND os_type = :os_type AND portable = :portable "
        "AND processor_architecture_type = :processor_architecture_type";
    QVariantMap wvalues;
    wvalues.insert(":client_type", int(entity.clienType()));
    wvalues.insert(":os_type", int(entity.os()));
    wvalues.insert(":portable", int(entity.portable()));
    wvalues.insert(":processor_architecture_type", int(entity.processorArchitecture()));
    TransactionHolder holder(Source);
    if (!Source->update("application_versions", values, BSqlWhere(ws, wvalues)).success())
        return false;
    return holder.doCommit();
}

ApplicationVersion ApplicationVersionRepository::findOneByFields(Texsample::ClientType clienType, BeQt::OSType os,
                                                                 BeQt::ProcessorArchitecture arch, bool portable)
{
    ApplicationVersion entity(this);
    if (!isValid() || Texsample::UnknownClient == clienType || BeQt::UnknownOS == os
            || BeQt::UnknownArchitecture == arch) {
        return entity;
    }
    QString ws = "client_type = :client_type AND os_type = :os_type AND portable = :portable "
        "AND processor_architecture_type = :processor_architecture_type";
    QVariantMap values;
    values.insert(":client_type", int(clienType));
    values.insert(":os_type", int(os));
    values.insert(":portable", int(portable));
    values.insert(":processor_architecture_type", int(arch));
    BSqlResult result = Source->select("application_versions", QStringList() << "download_url" << "version",
                                       BSqlWhere(ws, values));
    if (!result.success() || result.values().isEmpty())
        return entity;
    entity.mclienType = clienType;
    entity.mdownloadUrl = QUrl(result.value("download_url").toString());
    entity.mos = os;
    entity.mportable = portable;
    entity.mprocessorArchitecture = arch;
    entity.mversion = BVersion(result.value("version").toString());
    entity.valid = true;
    return entity;
}

bool ApplicationVersionRepository::isValid() const
{
    return Source && Source->isValid();
}
