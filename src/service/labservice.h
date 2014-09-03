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

#ifndef LABSERVICE_H
#define LABSERVICE_H

class DataSource;
class Group;
class GroupRepository;
class Lab;
class LabRepository;
class TransactionHolder;
class UserRepository;

#include "requestin.h"
#include "requestout.h"
#include "translator.h"

#include <TeXSample/TeXSampleCore>

#include <QCoreApplication>
#include <QString>

/*============================================================================
================================ LabService ==================================
============================================================================*/

class LabService
{
    Q_DECLARE_TR_FUNCTIONS(LabService)
private:
    GroupRepository * const GroupRepo;
    LabRepository * const LabRepo;
    DataSource * const Source;
    UserRepository * const UserRepo;
public:
    explicit LabService(DataSource *source);
    ~LabService();
public:
    RequestOut<TAddLabReplyData> addLab(const RequestIn<TAddLabRequestData> &in, quint64 userId);
    DataSource *dataSource() const;
    RequestOut<TDeleteLabReplyData> deleteLab(const RequestIn<TDeleteLabRequestData> &in, quint64 userId);
    RequestOut<TEditLabReplyData> editLab(const RequestIn<TEditLabRequestData> &in, quint64 userId);
    RequestOut<TGetLabDataReplyData> getLabData(const RequestIn<TGetLabDataRequestData> &in);
    RequestOut<TGetLabExtraFileReplyData> getLabExtraFile(const RequestIn<TGetLabExtraFileRequestData> &in);
    RequestOut<TGetLabInfoListReplyData> getLabInfoList(const RequestIn<TGetLabInfoListRequestData> &in,
                                                        quint64 userId);
    bool isValid() const;
private:
private:
    template <typename T> bool commonCheck(const Translator &t, const T &data, QString *error) const
    {
        if (!commonCheck(t, error))
            return false;
        if (!data.isValid())
            return bRet(error, t.translate("LabService", "Invalid data", "error"), false);
        return bRet(error, QString(), true);
    }
private:
    bool checkUserId(const Translator &t, quint64 userId, QString *error);
    bool commit(const Translator &translator, TransactionHolder &holder, QString *error);
    bool commonCheck(const Translator &translator, QString *error) const;
    TIdList getGroups(quint64 userId, bool *ok = 0);
    TLabInfo labToLabInfo(const Lab &entity, bool *ok = 0);
private:
    Q_DISABLE_COPY(LabService)
};

#endif // LABSERVICE_H
