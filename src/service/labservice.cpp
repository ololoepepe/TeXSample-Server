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

#include "labservice.h"

#include "datasource.h"
#include "entity/group.h"
#include "entity/lab.h"
#include "repository/grouprepository.h"
#include "repository/labrepository.h"
#include "repository/userrepository.h"
#include "requestin.h"
#include "requestout.h"
#include "transactionholder.h"
#include "translator.h"

#include <TeXSample/TeXSampleCore>

#include <QDateTime>
#include <QDebug>
#include <QList>
#include <QString>
#include <QStringList>

/*============================================================================
================================ LabService ==================================
============================================================================*/

/*============================== Public constructors =======================*/

LabService::LabService(DataSource *source) :
    GroupRepo(new GroupRepository(source)), LabRepo(new LabRepository(source)), Source(source),
    UserRepo(new UserRepository(source))
{
    //
}

LabService::~LabService()
{
    delete GroupRepo;
    delete LabRepo;
    delete UserRepo;
}

/*============================== Public methods ============================*/

RequestOut<TAddLabReplyData> LabService::addLab(const RequestIn<TAddLabRequestData> &in, quint64 userId)
{
    typedef RequestOut<TAddLabReplyData> Out;
    Translator t(in.locale());
    QString error;
    const TAddLabRequestData &requestData = in.data();
    if (!commonCheck(t, requestData, &error))
        return Out(error);
    if (!userId)
        return Out(t.translate("LabService", "Invalid user ID (internal)", "error"));
    bool ok = false;
    Lab entity;
    entity.setAuthors(requestData.authors());
    entity.setDataList(requestData.dataList());
    entity.setDescription(requestData.description());
    entity.setExtraFiles(requestData.extraFiles());
    entity.setGroups(requestData.groups());
    entity.setSenderId(userId);
    entity.setTags(requestData.tags());
    entity.setTitle(requestData.title());
    entity.setType(requestData.dataList().first().type());
    TransactionHolder holder(Source);
    quint64 id = LabRepo->add(entity, &ok);
    if (!ok || !id)
        return Out(t.translate("LabService", "Failed to add lab (internal)", "error"));
    entity = LabRepo->findOne(id, &ok);
    if (!ok || !entity.isValid())
        return Out(t.translate("LabService", "Failed to get lab (internal)", "error"));
    TLabInfo info = labToLabInfo(entity, &ok);
    if (!ok)
        return Out(t.translate("LabService", "Failed to create lab info (internal)", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
    TAddLabReplyData replyData;
    replyData.setLabInfo(info);
    return Out(replyData, info.creationDateTime());
}

DataSource *LabService::dataSource() const
{
    return Source;
}

RequestOut<TDeleteLabReplyData> LabService::deleteLab(const RequestIn<TDeleteLabRequestData> &in, quint64 userId)
{
    typedef RequestOut<TDeleteLabReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    if (!checkUserId(t, userId, &error))
        return Out(error);
    bool ok = false;
    const TDeleteLabRequestData &requestData = in.data();
    Lab entity = LabRepo->findOne(requestData.id(), &ok);
    if (!ok)
        return Out(t.translate("LabService", "Failed to get lab (internal)", "error"));
    if (!entity.isValid())
        return Out(t.translate("LabService", "No such lab", "error"));
    if (entity.senderId() != userId) {
        int lvlSelf = UserRepo->findAccessLevel(userId, &ok).level();
        if (!ok)
            return Out(t.translate("LabService", "Failed to get user access level (internal)", "error"));
        int lvlSender = UserRepo->findAccessLevel(entity.senderId(), &ok).level();
        if (!ok)
            return Out(t.translate("LabService", "Failed to get user access level (internal)", "error"));
        if (lvlSelf < TAccessLevel::SuperuserLevel && lvlSelf <= lvlSender)
            return Out(t.translate("LabService", "Not enough rights to delete lab", "error"));
    }
    TransactionHolder holder(Source);
    QDateTime dt = LabRepo->deleteOne(requestData.id(), &ok);
    if (!ok)
        return Out(t.translate("LabService", "Failed to delete lab (internal)", "error"));
    TDeleteLabReplyData replyData;
    if (!commit(t, holder, &error))
        return Out(error);
    return Out(replyData, dt);
}

RequestOut<TEditLabReplyData> LabService::editLab(const RequestIn<TEditLabRequestData> &in, quint64 userId)
{
    typedef RequestOut<TEditLabReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    if (!checkUserId(t, userId, &error))
        return Out(error);
    bool ok = false;
    const TEditLabRequestData &requestData = in.data();
    Lab entity = LabRepo->findOne(requestData.id(), &ok);
    if (!ok)
        return Out(t.translate("LabService", "Failed to get lab (internal)", "error"));
    if (!entity.isValid())
        return Out(t.translate("LabService", "No such lab", "error"));
    if (entity.senderId() != userId) {
        int lvlSelf = UserRepo->findAccessLevel(userId, &ok).level();
        if (!ok)
            return Out(t.translate("LabService", "Failed to get user access level (internal)", "error"));
        int lvlSender = UserRepo->findAccessLevel(entity.senderId(), &ok).level();
        if (!ok)
            return Out(t.translate("LabService", "Failed to get user access level (internal)", "error"));
        if (lvlSelf < TAccessLevel::SuperuserLevel && lvlSelf <= lvlSender)
            return Out(t.translate("LabService", "Not enough rights to edit lab", "error"));
    }
    entity.convertToCreatedByUser();
    entity.setAuthors(requestData.authors());
    entity.setDeletedExtraFiles(requestData.deletedExtraFiles());
    entity.setDescription(requestData.description());
    entity.setExtraFiles(requestData.newExtraFiles());
    entity.setGroups(requestData.groups());
    entity.setSaveData(requestData.editData());
    if (requestData.editData()) {
        entity.setDataList(requestData.dataList());
        entity.setType(requestData.dataList().first().type());
    }
    entity.setTags(requestData.tags());
    entity.setTitle(requestData.title());
    TransactionHolder holder(Source);
    LabRepo->edit(entity, &ok);
    if (!ok)
        return Out(t.translate("LabService", "Failed to edit lab (internal)", "error"));
    entity = LabRepo->findOne(entity.id(), &ok);
    if (!ok || !entity.isValid())
        return Out(t.translate("LabService", "Failed to get lab (internal)", "error"));
    TEditLabReplyData replyData;
    TLabInfo info = labToLabInfo(entity, &ok);
    if (!ok)
        return Out(t.translate("LabService", "Failed to create lab info (internal)", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
    replyData.setLabInfo(info);
    return Out(replyData, entity.lastModificationDateTime());
}

RequestOut<TGetLabDataReplyData> LabService::getLabData(const RequestIn<TGetLabDataRequestData> &in)
{
    typedef RequestOut<TGetLabDataReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, &error))
        return Out(error);
    QDateTime dt = QDateTime::currentDateTime();
    bool ok = false;
    if (in.cachingEnabled() && in.lastRequestDateTime().isValid()) {
        QDateTime lastModDT = LabRepo->findLastModificationDateTime(in.data().labId(), &ok);
        if (!ok)
            return Out(t.translate("LabService", "Failed to get lab last modification date time (internal)", "error"));
        if (in.lastRequestDateTime() >= lastModDT)
            return Out(dt);
    }
    Lab entity = LabRepo->findOne(in.data().labId(), &ok);
    if (!ok)
        return Out(t.translate("LabService", "Failed to get lab (internal)", "error"));
    TGetLabDataReplyData replyData;
    replyData.setData(entity.labData(in.data().os()));
    return Out(replyData, dt);
}

RequestOut<TGetLabExtraFileReplyData> LabService::getLabExtraFile(const RequestIn<TGetLabExtraFileRequestData> &in)
{
    typedef RequestOut<TGetLabExtraFileReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, &error))
        return Out(error);
    QDateTime dt = QDateTime::currentDateTime();
    bool ok = false;
    if (in.cachingEnabled() && in.lastRequestDateTime().isValid()) {
        QDateTime lastModDT = LabRepo->findLastModificationDateTime(in.data().labId(), &ok);
        if (!ok)
            return Out(t.translate("LabService", "Failed to get lab last modification date time (internal)", "error"));
        if (in.lastRequestDateTime() >= lastModDT)
            return Out(dt);
    }
    Lab entity = LabRepo->findOne(in.data().labId(), &ok);
    if (!ok)
        return Out(t.translate("LabService", "Failed to get lab (internal)", "error"));
    TGetLabExtraFileReplyData replyData;
    replyData.setFile(entity.extraFile(in.data().fileName()));
    return Out(replyData, dt);
}

RequestOut<TGetLabInfoListReplyData> LabService::getLabInfoList(const RequestIn<TGetLabInfoListRequestData> &in,
                                                                quint64 userId)
{
    typedef RequestOut<TGetLabInfoListReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, &error))
        return Out(error);
    if (!checkUserId(t, userId, &error))
        return Out(error);
    QDateTime dt = QDateTime::currentDateTime();
    bool ok = false;
    TIdList groups;
    int lvlSelf = UserRepo->findAccessLevel(userId, &ok).level();
    if (!ok)
        return Out(t.translate("LabService", "Failed to get user access level (internal)", "error"));
    if (lvlSelf < TAccessLevel::ModeratorLevel) {
        groups = getGroups(userId, &ok);
        if (!ok)
            return Out(t.translate("LabService", "Failed to get user group list (internal)", "error"));
    }
    QList<Lab> entities = LabRepo->findAllNewerThan(in.lastRequestDateTime(), groups, &ok);
    if (!ok)
        return Out(t.translate("LabService", "Failed to get lab list (internal)", "error"));
    TLabInfoList newLabs;
    TIdList deletedLabs = LabRepo->findAllDeletedNewerThan(in.lastRequestDateTime(), groups, &ok);
    if (!ok)
        return Out(t.translate("LabService", "Failed to get deleted lab list (internal)", "error"));
    foreach (const Lab &entity, entities) {
        newLabs << labToLabInfo(entity, &ok);
        if (!ok)
            return Out(t.translate("LabService", "Failed to create lab info (internal)", "error"));
    }
    TGetLabInfoListReplyData replyData;
    replyData.setNewLabs(newLabs);
    replyData.setDeletedLabs(deletedLabs);
    return Out(replyData, dt);
}

bool LabService::isValid() const
{
    return Source && Source->isValid() && LabRepo->isValid();
}

/*============================== Private methods ===========================*/

bool LabService::checkUserId(const Translator &t, quint64 userId, QString *error)
{
    if (!userId)
        return bRet(error, t.translate("LabService", "Invalid user ID (internal)", "error"), false);
    return bRet(error, QString(), true);
}

bool LabService::commit(const Translator &t, TransactionHolder &holder, QString *error)
{
    if (!holder.doCommit())
        return bRet(error, t.translate("LabService", "Failed to commit (internal)", "error"), false);
    return bRet(error, QString(), true);
}

bool LabService::commonCheck(const Translator &t, QString *error) const
{
    if (!isValid())
        return bRet(error, t.translate("LabService", "Invalid LabService instance (internal)", "error"), false);
    return bRet(error, QString(), true);
}

TIdList LabService::getGroups(quint64 userId, bool *ok)
{
    TIdList list;
    if (!isValid() || !userId)
        return bRet(ok, false, list);
    bool b = false;
    QList<Group> entityList = GroupRepo->findAllByUserId(userId, QDateTime(), &b);
    if (!b)
        return bRet(ok, false, list);
    foreach (const Group &entity, entityList)
        list << entity.id();
    return bRet(ok, true, list);
}

TLabInfo LabService::labToLabInfo(const Lab &entity, bool *ok)
{
    TLabInfo info;
    if (!isValid() || !entity.isValid() || !entity.isCreatedByRepo())
        return bRet(ok, false, info);
    info.setAuthors(entity.authors());
    info.setCreationDateTime(entity.creationDateTime());
    info.setDataInfos(entity.labDataInfos());
    info.setDescription(entity.description());
    info.setExtraFiles(entity.extraFileInfos());
    info.setGroups(entity.groups());
    info.setId(entity.id());
    info.setLastModificationDateTime(entity.lastModificationDateTime());
    info.setSenderId(entity.senderId());
    bool b = false;
    info.setSenderLogin(UserRepo->findLogin(entity.senderId(), &b));
    if (!b)
        return bRet(ok, false, TLabInfo());
    info.setTags(entity.tags());
    info.setTitle(entity.title());
    return bRet(ok, true, info);
}
