#include "registrationconnection.h"
#include "global.h"
#include "storage.h"
#include "terminaliohandler.h"
#include "connection.h"

#include <TAccessLevel>
#include <TUserInfo>
#include <TeXSample>
#include <TOperationResult>

#include <BNetworkConnection>
#include <BNetworkServer>
#include <BGenericSocket>
#include <BNetworkOperation>
#include <BCoreApplication>

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <QVariantMap>
#include <QTcpSocket>

#include <QDebug>

/*============================================================================
================================ RegistrationConnection ======================
============================================================================*/

/*============================== Public constructors =======================*/

RegistrationConnection::RegistrationConnection(BNetworkServer *server, BGenericSocket *socket) :
    BNetworkConnection(server, socket)
{
    setCriticalBufferSize(BeQt::Megabyte + 500 * BeQt::Kilobyte);
    setCloseOnCriticalBufferSize(true);
    socket->tcpSocket()->setSocketOption(QTcpSocket::KeepAliveOption, 1);
    mstorage = new Storage;
    installRequestHandler(Texsample::RegisterRequest,
                          (InternalHandler) &RegistrationConnection::handleRegisterRequest);
    QTimer::singleShot(BeQt::Minute, this, SLOT(close()));
}

RegistrationConnection::~RegistrationConnection()
{
    delete mstorage;
}

/*============================== Protected methods =========================*/

void RegistrationConnection::log(const QString &text, BLogger::Level lvl)
{
    TerminalIOHandler::sendLogRequest("[" + peerAddress() + "] " + text, lvl);
    BNetworkConnection::log(text, lvl);
}

/*============================== Private methods ===========================*/

void RegistrationConnection::handleRegisterRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    QUuid invite = BeQt::uuidFromText(in.value("invite").toString());
    TUserInfo info = in.value("user_info").value<TUserInfo>();
    log(tr("Register request:", "log text") + " " + info.login());
    if (invite.isNull() || !info.isValid(TUserInfo::RegisterContext))
        return Global::sendReply(op, Storage::invalidParametersResult());
    info.setContext(TUserInfo::AddContext);
    info.setAccessLevel(TAccessLevel::UserLevel);
    quint64 id = mstorage->inviteId(invite);
    if (!id)
        return Global::sendReply(op, TOperationResult(tr("Invalid invite", "errorString")));
    Global::sendReply(op, mstorage->addUser(info, invite));
    op->waitForFinished();
    close();
}
