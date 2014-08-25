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
public:
    enum UserMatchType
    {
        MatchLogin = 0x01,
        MatchUniqueId = 0x02,
        MatchLoginAndUniqueId = MatchLogin | MatchUniqueId
    };
private:
    const QString Location;
public:
    explicit Server(const QString &location, QObject *parent = 0);
public:
    TUserConnectionInfoList userConnections(const QString &matchPattern, UserMatchType type = MatchLoginAndUniqueId,
                                            int *total = 0) const;
protected:
    BNetworkConnection *createConnection(BGenericSocket *socket, const QString &serverAddress, quint16 serverPort);
    BGenericSocket *createSocket();
};

#endif // SERVER_H
