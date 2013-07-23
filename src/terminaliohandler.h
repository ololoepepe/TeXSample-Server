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
    static TerminalIOHandler *instance();
public:
    explicit TerminalIOHandler(QObject *parent = 0);
    ~TerminalIOHandler();
protected:
    bool handleCommand(const QString &command, const QStringList &arguments);
private:
    static QString msecsToString(qint64 msecs);
    static QString userPrefix(Connection *user);
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
