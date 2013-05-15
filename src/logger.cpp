#include "logger.h"
#include "remotecontrolserver.h"
#include "remotecontrolconnection.h"

#include <BLogger>
#include <BCoreApplication>
#include <BeQtCore/private/blogger_p.h>

#include <QObject>
#include <QMutex>
#include <QString>
#include <QMutexLocker>
#include <QMetaObject>

/*============================================================================
================================ LoggerPrivate ===============================
============================================================================*/

class LoggerPrivate : public BLoggerPrivate
{
    B_DECLARE_PUBLIC(Logger)
public:
    explicit LoggerPrivate(Logger *q);
    ~LoggerPrivate();
public:
    void init();
    void tryLog(const QString &msg, bool stderrLevel);
    void tryLogToRemote(const QString &text, bool stderrLevel);
public:
    static QMutex remoteMutex;
    RemoteControlServer *server;
};

/*============================================================================
================================ LoggerPrivate ===============================
============================================================================*/

/*============================== Public constructors =======================*/

LoggerPrivate::LoggerPrivate(Logger *q) :
    BLoggerPrivate(q)
{
    //
}

LoggerPrivate::~LoggerPrivate()
{
    delete server;
}

/*============================== Public methods ============================*/

void LoggerPrivate::init()
{
    server = new RemoteControlServer;
    server->listen("0.0.0.0", 9043);
}

void LoggerPrivate::tryLog(const QString &msg, bool stderrLevel)
{
    BLoggerPrivate::tryLog(msg, stderrLevel);
    tryLogToRemote(msg, stderrLevel);
}

void LoggerPrivate::tryLogToRemote(const QString &text, bool stderrLevel)
{
    QMutexLocker locker(&remoteMutex);
    RemoteControlConnection *c = server->connection();
    if (!c)
        return;
    QMetaObject::invokeMethod(c, "sendLogRequest", Qt::QueuedConnection, Q_ARG(QString, text),
                              Q_ARG(bool, stderrLevel));
}

/*============================== Static public members =====================*/

QMutex LoggerPrivate::remoteMutex;

/*============================================================================
================================ Logger ======================================
============================================================================*/

/*============================== Static public methods =====================*/

void Logger::sendWriteRequest(const QString &text)
{
    Logger *l = dynamic_cast<Logger *>(BCoreApplication::logger());
    if (!l)
        return;
    RemoteControlServer *s = l->d_func()->server;
    QMutexLocker locker(&LoggerPrivate::remoteMutex);
    RemoteControlConnection *c = s->connection();
    if (!c)
        return;
    QMetaObject::invokeMethod(c, "sendWriteRequest", Qt::QueuedConnection, Q_ARG(QString, text));
}

void Logger::sendWriteLineRequest(const QString &text)
{
    sendWriteRequest(text + "\n");
}

/*============================== Public constructors =======================*/

Logger::Logger(QObject *parent) :
    BLogger(*new LoggerPrivate(this), parent)
{
    d_func()->init();
}
