/****************************************************************************
**
** Copyright (C) 2012-2014 Andrey Bogdanov
**
** This file is part of TeXSample Server.
**
** TeXSample Server is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** TeXSample Server is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with TeXSample Server.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

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
