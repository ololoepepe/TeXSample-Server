/****************************************************************************
**
** Copyright (C) 2012-2014 Andrey Bogdanov
**
** This file is part of TeXSample Server.
**
** TeXSample Server is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** TeXSample Server is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with TeXSample Server.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

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
