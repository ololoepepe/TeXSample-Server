#ifndef TERMINALIOHANDLER_H
#define TERMINALIOHANDLER_H

class Server;
class RegistrationServer;
class RecoveryServer;
class Connection;

class QStringList;

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
    static void write(const QString &text, Connection *c = 0);
    static void writeLine(const QString &text, Connection *c = 0);
    static void writeLine(Connection *c = 0);
    static void sendLogRequest(const QString &text, BLogger::Level lvl = BLogger::NoLevel);
    static void executeCommand(const QString &cmd, const QStringList &args, Connection *c = 0);
    static TerminalIOHandler *instance();
    static Server *server();
    static QString mailPassword();
public:
    explicit TerminalIOHandler(QObject *parent = 0);
    ~TerminalIOHandler();
protected:
    bool handleCommand(const QString &command, const QStringList &arguments);
private:
    static QString msecsToString(qint64 msecs, Connection *c = 0);
    static QString userPrefix(Connection *user);
    static void writeHelpLine(const QString &command, const QString &description);
private:
    void handleUser(const QString &cmd, const QStringList &args);
    void handleUptime(const QString &cmd, const QStringList &args);
    void handleSet(const QString &cmd, const QStringList &args);
    void handleStart(const QString &cmd, const QStringList &args);
    void handleStop(const QString &cmd, const QStringList &args);
    void handleHelp(const QString &cmd, const QStringList &args);
    void handleUserImplementation(const QString &cmd, const QStringList &args, Connection *c = 0);
    void handleUptimeImplementation(const QString &cmd, const QStringList &args, Connection *c = 0);
    void handleSetImplementation(const QString &cmd, const QStringList &args, Connection *c = 0);
    void handleStartImplementation(const QString &cmd, const QStringList &args, Connection *c = 0);
    void handleStopImplementation(const QString &cmd, const QStringList &args, Connection *c = 0);
private:
    static QString mmailPassword;
private:
    Server *mserver;
    QElapsedTimer melapsedTimer;
};

#endif // TERMINALIOHANDLER_H
