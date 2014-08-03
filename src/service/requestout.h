#ifndef REQUESTOUT_H
#define REQUESTOUT_H

#include <TMessage>
#include <TReply>

#include <QDateTime>

/*============================================================================
================================ RequestOut ==================================
============================================================================*/

template <typename T> class RequestOut
{
private:
    bool mcacheUpToDate;
    T mdata;
    TMessage mmessage;
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
    explicit RequestOut(const TMessage &message)
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
    TMessage message() const
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
    void setMessage(const TMessage &message)
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
