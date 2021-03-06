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

#ifndef SERVER_H
#define SERVER_H

class TUserConnectionInfoList;

class BGenericSocket;
class BNetworkConnection;

#include <TGetUserConnectionInfoListRequestData>

#include <BLogger>
#include <BNetworkServer>

#include <QObject>
#include <QString>

/*============================================================================
================================ Server ======================================
============================================================================*/

class Server : public BNetworkServer
{
    Q_OBJECT
private:
    const QString Location;
public:
    explicit Server(const QString &location, QObject *parent = 0);
public:
    void setAuthorityResolverEnabled(bool enabled);
    TUserConnectionInfoList userConnections(
            const QString &matchPattern, TGetUserConnectionInfoListRequestData::MatchType type =
            TGetUserConnectionInfoListRequestData::MatchLoginAndUniqueId, int *total = 0) const;
public slots:
    bool listenSlot(const QString &address, quint16 port);
protected:
    BNetworkConnection *createConnection(BGenericSocket *socket, const QString &serverAddress, quint16 serverPort);
    BGenericSocket *createSocket();
};

#endif // SERVER_H
