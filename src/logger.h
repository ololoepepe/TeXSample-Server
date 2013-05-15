#ifndef LOGGER_H
#define LOGGER_H

class LoggerPrivate;

#include <BLogger>

#include <QObject>

/*============================================================================
================================ Logger ======================================
============================================================================*/

class Logger : public BLogger
{
    Q_OBJECT
    B_DECLARE_PRIVATE(Logger)
public:
    static void sendWriteRequest(const QString &text);
    static void sendWriteLineRequest(const QString &text);
public:
    explicit Logger(QObject *parent  = 0);
};

#endif // LOGGER_H
