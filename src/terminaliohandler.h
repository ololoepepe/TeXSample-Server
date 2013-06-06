#ifndef TERMINALIOHANDLER_H
#define TERMINALIOHANDLER_H

class Server;
class RegistrationServer;

class QStringList;

#include <BTerminalIOHandler>

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
    static void write(const QString &text);
    static void writeLine(const QString &text = QString());
    static QString mailPassword();
public:
    explicit TerminalIOHandler(QObject *parent = 0);
    ~TerminalIOHandler();
public:
    void executeCommand(const QString &cmd, const QStringList &args);
    Server *server() const;
private:
    void handleUser(const QString &cmd, const QStringList &args);
    void handleUptime(const QString &cmd, const QStringList &args);
    void handleSet(const QString &cmd, const QStringList &args);
    void handleStart(const QString &cmd, const QStringList &args);
    void handleStop(const QString &cmd, const QStringList &args);
private:
    static QString mmailPassword;
private:
    Server *mserver;
    RegistrationServer *mrserver;
    QElapsedTimer melapsedTimer;
};

#endif // TERMINALIOHANDLER_H
