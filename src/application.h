#ifndef APPLICATION_H
#define APPLICATION_H

class DataSource;
class Server;
class UserService;

class TCommandMessage;
class TExecuteCommandReplyData;

class QStringList;
class QTimerEvent;

#include <TCoreApplication>

#include <QElapsedTimer>
#include <QMutex>
#include <QObject>
#include <QString>

#if defined(bApp)
#   undef bApp
#endif
#define bApp static_cast<Application *>(BApplicationBase::binstance())

#if defined(tApp)
#   undef tApp
#endif
#define tApp static_cast<Application *>(BApplicationBase::binstance())

/*============================================================================
================================ Application =================================
============================================================================*/

class Application : public TCoreApplication
{
    Q_OBJECT
private:
    static QMutex serverMutex;
    static QString texsampleSty;
    static QString texsampleTex;
private:
    DataSource * const Source;
    UserService * const UserServ;
private:
    QElapsedTimer melapsedTimer;
    Server *mserver;
    int timerId;
public:
    explicit Application(int &argc, char **argv, const QString &applicationName, const QString &organizationName);
    ~Application();
public:
    static TExecuteCommandReplyData executeSetAppVersion(const QStringList &args);
    static TExecuteCommandReplyData executeStartServer(const QStringList &args);
    static TExecuteCommandReplyData executeStopServer(const QStringList &args);
    static TExecuteCommandReplyData executeUptime(const QStringList &args);
    static TExecuteCommandReplyData executeUser(const QStringList &args);
    static bool initializeStorage();
    static Server *server();
protected:
    void timerEvent(QTimerEvent *e);
private:
    static bool handleSetAppVersionCommand(const QString &cmd, const QStringList &args);
    static bool handleStartCommand(const QString &cmd, const QStringList &args);
    static bool handleStopCommand(const QString &cmd, const QStringList &args);
    static bool handleUnknownCommand(const QString &command, const QStringList &args);
    static bool handleUptimeCommand(const QString &cmd, const QStringList &args);
    static bool handleUserCommand(const QString &cmd, const QStringList &args);
    static void initTerminal();
    static QString msecsToString(qint64 msecs);
    static void writeCommandMessage(const TCommandMessage &msg, const QStringList &args);
private:
    void compatibility();
private:
    Q_DISABLE_COPY(Application)
};

#endif // APPLICATION_H
