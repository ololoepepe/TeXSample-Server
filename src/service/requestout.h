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

#ifndef REQUESTOUT_H
#define REQUESTOUT_H

#include <TReply>

#include <QDateTime>
#include <QString>

/*============================================================================
================================ RequestOut ==================================
============================================================================*/

template <typename T> class RequestOut
{
private:
    bool mcacheUpToDate;
    T mdata;
    QString mmessage;
    QDateTime mrequestDateTime;
    bool msuccess;
public:
    explicit RequestOut()
    {
        mcacheUpToDate = false;
        msuccess = false;
    }
    explicit RequestOut(const T &data, const QDateTime &requestDateTime = QDateTime())
    {
        mcacheUpToDate = false;
        mrequestDateTime = requestDateTime.toUTC();
        msuccess = true;
        mdata = data;
    }
    explicit RequestOut(const QDateTime &requestDateTime)
    {
        mcacheUpToDate = true;
        mrequestDateTime = requestDateTime.toUTC();
        msuccess = true;
    }
    explicit RequestOut(const QString &message)
    {
        mcacheUpToDate = false;
        msuccess = false;
        mmessage = message;
    }
    RequestOut(const RequestOut &other)
    {
        *this = other;
    }
    ~RequestOut()
    {
        //
    }
public:
    bool cacheUpToDate() const
    {
        return mcacheUpToDate;
    }
    TReply createReply() const
    {
        TReply reply;
        reply.setCacheUpToDate(mcacheUpToDate);
        reply.setData(QVariant::fromValue(mdata));
        reply.setMessage(mmessage);
        reply.setRequestDateTime(mrequestDateTime);
        reply.setSuccess(msuccess);
        return reply;
    }
    const T &data() const
    {
        return mdata;
    }
    QString message() const
    {
        return mmessage;
    }
    QDateTime requestDateTime() const
    {
        return mrequestDateTime;
    }
    void setCacheUpToDate(bool upToDate)
    {
        mcacheUpToDate = upToDate;
    }
    void setData(const T &data)
    {
        mdata = data;
    }
    void setMessage(const QString &message)
    {
        mmessage = message;
    }
    void setRequestDateTime(const QDateTime &dt)
    {
        mrequestDateTime = dt.toUTC();
    }
    void setSuccess(bool success)
    {
        msuccess = success;
    }
    bool success() const
    {
        return msuccess;
    }
public:
    RequestOut &operator =(const RequestOut &other)
    {
        mcacheUpToDate = other.mcacheUpToDate;
        mdata = other.mdata;
        mmessage = other.mmessage;
        mrequestDateTime = other.mrequestDateTime;
        msuccess = other.msuccess;
        return *this;
    }
};

#endif // REQUESTOUT_H
