#ifndef TERMINALIOHANDLER_H
#define TERMINALIOHANDLER_H

class Server;
class RegistrationServer;

class BNetworkConnection;
class BNetworkOperation;

class QStringList;

#include <BTerminalIOHandler>

#include <QObject>
#include <QElapsedTimer>
#include <QString>

/*============================================================================
================================ TerminalIOHandler ===========================
============================================================================*/

class TerminalIOHandler : public BTerminalIOHandler
{
    Q_OBJECT
public:
    static void write(const QString &text);
    static void writeLine(const QString &text);
public:
    explicit TerminalIOHandler(bool local, QObject *parent = 0);
    ~TerminalIOHandler();
public:
    void executeCommand(const QString &cmd, const QStringList &args);
private:
    void handleQuit(const QString &cmd, const QStringList &args);
    void handleUser(const QString &cmd, const QStringList &args);
    void handleUptime(const QString &cmd, const QStringList &args);
    void handleConnect(const QString &cmd, const QStringList &args);
    void handleTest(const QString &cmd, const QStringList &args);
private slots:
    void connectToHost(const QString &hostName);
    void sendCommand(const QString &cmd, const QStringList &args);
    void remoteRequest(BNetworkOperation *op);
private:
    Server *mserver;
    RegistrationServer *mrserver;
    BNetworkConnection *mremote;
    QElapsedTimer melapsedTimer;
};

#endif // TERMINALIOHANDLER_H
