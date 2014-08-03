#include "temporarylocation.h"

#include "datasource.h"

#include <BDirTools>
#include <BUuid>

#include <QString>

/*============================================================================
================================ TemporaryLocation ===========================
============================================================================*/

/*============================== Public constructors =======================*/

TemporaryLocation::TemporaryLocation(DataSource *source) :
    QDir((source && source->isValid()) ? (source->location() + "/" + BUuid::createUuid().toString(true)) : QString()),
    Source(source)
{
    if (source && source->isValid())
        BDirTools::mkpath(absolutePath());
}

TemporaryLocation::~TemporaryLocation()
{
    if (isValid())
        BDirTools::rmdir(absolutePath());
}

/*============================== Public methods ============================*/

bool TemporaryLocation::isValid() const
{
    return Source && Source->isValid() && exists();
}
