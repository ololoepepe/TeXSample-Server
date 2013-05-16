#include "remotecontrolconnection.h"
#include "global.h"
#include "storage.h"
#include "terminaliohandler.h"

#include <TAccessLevel>
#include <TeXSample>
#include <TOperationResult>

#include <BNetworkConnection>
#include <BNetworkServer>
#include <BGenericSocket>
#include <BNetworkOperation>
#include <BCoreApplication>
#include <BLogger>

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <QVariantMap>
#include <QStringList>
#include <QTcpSocket>

#include <QDebug>

/*============================================================================
================================ RemoteControlConnection =====================
============================================================================*/

/*============================== Public constructors =======================*/

RemoteControlConnection::RemoteControlConnection(BNetworkServer *server, BGenericSocket *socket) :
    BNetworkConnection(server, socket)
{
    setLogger(new BLogger);
    setCriticalBufferSize(BeQt::Kilobyte);
    setCloseOnCriticalBufferSize(true);
    socket->tcpSocket()->setSocketOption(QTcpSocket::KeepAliveOption, 1);
    mstorage = new Storage(BCoreApplication::location(BCoreApplication::DataPath, BCoreApplication::UserResources));
    muserId = 0;
    installRequestHandler(Texsample::AuthorizeRequest,
                          (InternalHandler) &RemoteControlConnection::handleAuthorizeRequest);
    installRequestHandler(Texsample::ExecuteCommandRequest,
                          (InternalHandler) &RemoteControlConnection::handleExecuteCommandRequest);
    QTimer::singleShot(15 * BeQt::Second, this, SLOT(testAuthorization()));
}

RemoteControlConnection::~RemoteControlConnection()
{
    delete mstorage;
}

/*============================== Public methods ============================*/

bool RemoteControlConnection::isAuthorized() const
{
    return muserId;
}

/*============================== Public slots ==============================*/

void RemoteControlConnection::sendLogRequest(const QString &text, bool stderrLevel)
{
    if (!muserId || text.isEmpty())
        return;
    QVariantMap out;
    out.insert("log_text", text);
    out.insert("stderr_level", stderrLevel);
    BNetworkOperation *op = sendRequest(Texsample::LogRequest, out);
    op->waitForFinished();
    op->deleteLater();
}

void RemoteControlConnection::sendWriteRequest(const QString &text)
{
    if (!muserId || text.isEmpty())
        return;
    QVariantMap out;
    out.insert("text", text);
    BNetworkOperation *op = sendRequest(Texsample::WriteRequest, out);
    op->waitForFinished();
    op->deleteLater();
}

/*============================== Private methods ===========================*/

void RemoteControlConnection::handleAuthorizeRequest(BNetworkOperation *op)
{
    if (muserId)
        return Global::sendReply(op, TOperationResult(true));
    QList<BNetworkConnection *> list = server()->connections();
    if (!list.isEmpty())
    {
        foreach (BNetworkConnection *c, list)
        {
            if (static_cast<RemoteControlConnection *>(c)->isAuthorized())
                return Global::sendReply(op, TOperationResult(tr("Only one connection allowed", "errorString")));
        }
    }
    QVariantMap in = op->variantData().toMap();
    QString login = in.value("login").toString();
    QByteArray password = in.value("password").toByteArray();
    log(tr("Remote control authorize request:", "log text") + " " + login);
    if (login.isEmpty() || password.isEmpty())
        return Global::sendReply(op, Storage::invalidParametersResult());
    quint64 id = mstorage->userId(login, password);
    if (!id)
        return Global::sendReply(op, TOperationResult(tr("No such user", "errorString")));
    if (mstorage->userAccessLevel(id) < TAccessLevel::AdminLevel)
        return Global::sendReply(op, TOperationResult(tr("Only admin can control remote server", "errorString")));
    muserId = id;
    setCriticalBufferSize(200 * BeQt::Megabyte);
    log(tr("Remote control authorized", "log text"));
    QVariantMap out;
    out.insert("user_id", id);
    Global::sendReply(op, out, TOperationResult(true));
}

void RemoteControlConnection::handleExecuteCommandRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, Global::notAuthorizedResult());
    QVariantMap in = op->variantData().toMap();
    QString command = in.value("command").toString();
    QStringList args = in.value("arguments").toStringList();
    log(tr("Execute command request:", "log text") + " " + command);
    if (command.isEmpty())
        return Global::sendReply(op, Storage::invalidParametersResult());
    static_cast<TerminalIOHandler *>(TerminalIOHandler::instance())->executeCommand(command, args);
    Global::sendReply(op, TOperationResult(true));
}

/*============================== Private slots =============================*/

void RemoteControlConnection::testAuthorization()
{
    if (muserId)
        return;
    log("Remote control authorization failed, closing connection");
    close();
}
