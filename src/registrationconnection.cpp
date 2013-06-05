#include "registrationconnection.h"
#include "global.h"
#include "storage.h"
#include "terminaliohandler.h"

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
#include <QSettings>

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
    BNetworkConnection::log(text, lvl);
}

/*============================== Private methods ===========================*/

void RegistrationConnection::handleRegisterRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    QString invite = in.value("invite").toString();
    TUserInfo info = in.value("user_info").value<TUserInfo>();
    log(tr("Register request:", "log text") + " " + info.login());
    if (QUuid(invite).isNull() || !info.isValid(TUserInfo::RegisterContext))
        return Global::sendReply(op, Storage::invalidParametersResult());
    info.setContext(TUserInfo::AddContext);
    info.setAccessLevel(TAccessLevel::UserLevel);
    quint64 id = mstorage->inviteId(invite);
    if (!id)
        return Global::sendReply(op, TOperationResult(tr("Invalid invite", "errorString")));
    if (!mstorage->deleteInvite(id))
        return Global::sendReply(op, Storage::databaseErrorResult());
    TOperationResult r = mstorage->addUser(info);
    Global::sendReply(op, r);
    op->waitForFinished();
    close();
    if (!r)
        return;
    Global::Host h;
    h.address = bSettings->value("Mail/server_address").toString();
    h.port = bSettings->value("Mail/server_port", h.port).toUInt();
    Global::User u;
    u.login = bSettings->value("Mail/login").toString();
    u.password = TerminalIOHandler::mailPassword();
    Global::Mail m;
    m.from = "TeXSample Team";
    m.to << info.email();
    m.subject = "TeXSample registration";
    m.body = "Hello, " + info.login() + "! You are now registered in TeXSample. Congratulations!";
    Global::sendMail(h, u, m, bSettings->value("Mail/ssl_required").toBool());
}
