#include "server.h"

#include "connection.h"

#include <TUserConnectionInfo>
#include <TUserConnectionInfoList>
#include <TUserInfo>

#include <BGenericServer>
#include <BGenericSocket>
#include <BNetworkServer>
#include <BUuid>

#include <QDateTime>
#include <QObject>
#include <QRegExp>
#include <QString>
#include <QTcpSocket>

/*============================================================================
================================ Server ======================================
============================================================================*/

/*============================== Public constructors =======================*/

Server::Server(const QString &location, QObject *parent) :
    BNetworkServer(BGenericServer::TcpServer, parent), Location(location)
{
    //
}

/*============================== Static public methods =====================*/

TUserConnectionInfoList Server::userConnections(const QString &matchPattern, UserMatchType type, int *total) const
{
    int t = 0;
    QRegExp rx = QRegExp(matchPattern, Qt::CaseSensitive, QRegExp::WildcardUnix);
    TUserConnectionInfoList list;
    if (!rx.isValid())
        return bRet(total, t, list);
    const_cast<Server *>(this)->lock();
    foreach (BNetworkConnection *c, connections()) {
        Connection *cc = static_cast<Connection *>(c);
        TUserInfo userInfo = cc->userInfo();
        BUuid uniqueId = cc->uniqueId();
        if ((!(MatchLogin | type) || !rx.exactMatch(userInfo.login()))
                && (!(MatchUniqueId | type) || !rx.exactMatch(cc->uniqueId().toString(true)))) {
            continue;
        }
        ++t;
        TUserConnectionInfo info;
        info.setClientInfo(cc->clientInfo());
        info.setConnectionDateTime(cc->connectionDateTime());
        info.setPeerAddress(cc->peerAddress());
        info.setUniqueId(uniqueId);
        info.setUptime(cc->uptime());
        info.setUserInfo(userInfo);
        list << info;
    }
    const_cast<Server *>(this)->unlock();
    return bRet(total, t, list);
}

/*============================== Protected methods =========================*/

BNetworkConnection *Server::createConnection(BGenericSocket *socket, const QString &, quint16)
{
    return new Connection(this, socket, Location);
}

BGenericSocket *Server::createSocket()
{
    BGenericSocket *s = new BGenericSocket(BGenericSocket::TcpSocket);
    s->tcpSocket()->setSocketOption(QTcpSocket::KeepAliveOption, 1);
    return s;
}
