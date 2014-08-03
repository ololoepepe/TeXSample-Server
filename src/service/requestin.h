#ifndef REQUESTIN_H
#define REQUESTIN_H

#include <TRequest>

#include <QDateTime>
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
public:
    explicit RequestIn(const TRequest &request)
    {
        mdata = request.data().value<T>();
        mcachingEnabled = request.cachingEnabled();
        mlastRequestDateTime = request.lastRequestDateTime().toUTC();
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
public:
    RequestIn &operator =(const RequestIn &other)
    {
        mdata = other.mdata;
        mcachingEnabled = other.mcachingEnabled;
        mlastRequestDateTime = other.mlastRequestDateTime;
        return *this;
    }
};

#endif // REQUESTIN_H
