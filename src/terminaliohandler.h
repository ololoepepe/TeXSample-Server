#ifndef TERMINALIOHANDLER_H
#define TERMINALIOHANDLER_H

class Server;
class RegistrationServer;
class RecoveryServer;
class Connection;

class TOperationResult;

class QStringList;
class QVariant;

#include <BTerminalIOHandler>
#include <BLogger>

#include <QObject>
#include <QElapsedTimer>
#include <QString>
#include <QAbstractSocket>

/*============================================================================
================================ TerminalIOHandler ===========================
============================================================================*/

class TerminalIOHandler : public BTerminalIOHandler
{
    Q_OBJECT
public:
    static TerminalIOHandler *instance();
public:
    explicit TerminalIOHandler(QObject *parent = 0);
    ~TerminalIOHandler();
public:
    TOperationResult startServer(const QString &address = QString());
    TOperationResult stopServer();
    qint64 uptime() const;
    TOperationResult user(const QStringList &args, QVariant &result);
protected:
    bool handleCommand(const QString &command, const QStringList &arguments);
private:
    static QString msecsToString(qint64 msecs);
    static QVariantMap packUserData(Connection *user);
    static QString userPrefix(const QVariantMap &userData);
    static void writeHelpLine(const QString &command, const QString &description);
private:
    bool handleUser(const QString &cmd, const QStringList &args);
    bool handleUptime(const QString &cmd, const QStringList &args);
    bool handleStart(const QString &cmd, const QStringList &args);
    bool handleStop(const QString &cmd, const QStringList &args);
private:
    QElapsedTimer melapsedTimer;
};

#endif // TERMINALIOHANDLER_H
