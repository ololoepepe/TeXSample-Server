#ifndef TERMINALIOHANDLER_H
#define TERMINALIOHANDLER_H

class Server;
class RemoteControlServer;

class QString;
class QStringList;

#include <BTerminalIOHandler>

#include <QObject>
#include <QElapsedTimer>

/*============================================================================
================================ TerminalIOHandler ===========================
============================================================================*/

class TerminalIOHandler : public BTerminalIOHandler
{
    Q_OBJECT
public:
    explicit TerminalIOHandler(Server *server = 0, RemoteControlServer *rcserver = 0, QObject *parent = 0);
public:
    void executeCommand(const QString &command, const QStringList &args);
private:
    void handleQuit(const QString &cmd, const QStringList &args);
    void handleUser(const QString &cmd, const QStringList &args);
    void handleUptime(const QString &cmd, const QStringList &args);
private:
    Server *mserver;
    RemoteControlServer *mrcserver;
    QElapsedTimer melapsedTimer;
};

#endif // TERMINALIOHANDLER_H
