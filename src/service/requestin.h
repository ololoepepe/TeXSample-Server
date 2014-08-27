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

#ifndef REQUESTIN_H
#define REQUESTIN_H

#include <TRequest>

#include <QDateTime>
#include <QLocale>
#include <QVariant>

/*============================================================================
================================ RequestIn ===================================
============================================================================*/

template <typename T> class RequestIn
{
private:
    bool mcachingEnabled;
    T mdata;
    QDateTime mlastRequestDateTime;
    QLocale mlocale;
public:
    explicit RequestIn(const TRequest &request)
    {
        mdata = request.data().value<T>();
        mcachingEnabled = request.cachingEnabled();
        mlastRequestDateTime = request.lastRequestDateTime().toUTC();
        mlocale = request.locale();
    }
    RequestIn(const RequestIn &other)
    {
        *this = other;
    }
    ~RequestIn()
    {
        //
    }
public:
    bool cachingEnabled() const
    {
        return mcachingEnabled;
    }
    const T &data() const
    {
        return mdata;
    }
    QDateTime lastRequestDateTime() const
    {
        return mlastRequestDateTime;
    }
    QLocale locale() const
    {
        return mlocale;
    }
public:
    RequestIn &operator =(const RequestIn &other)
    {
        mdata = other.mdata;
        mcachingEnabled = other.mcachingEnabled;
        mlastRequestDateTime = other.mlastRequestDateTime;
        mlocale = other.mlocale;
        return *this;
    }
};

#endif // REQUESTIN_H
