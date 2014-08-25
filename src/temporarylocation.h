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

#ifndef TEMPORARYLOCATION_H
#define TEMPORARYLOCATION_H

class DataSource;

#include <QDir>

/*============================================================================
================================ TemporaryLocation ===========================
============================================================================*/

class TemporaryLocation : public QDir
{
private:
    DataSource * const Source;
public:
    explicit TemporaryLocation(DataSource *source);
    ~TemporaryLocation();
public:
    bool isValid() const;
    DataSource *cource() const;
public:
    Q_DISABLE_COPY(TemporaryLocation)
};

#endif // TEMPORARYLOCATION_H
