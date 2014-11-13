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

#include "authorityresolver.h"

#include "application.h"
#include "authorityinfo.h"
#include "datasource.h"
#include "entity/group.h"
#include "entity/invitecode.h"
#include "entity/lab.h"
#include "entity/sample.h"
#include "entity/user.h"
#include "repository/grouprepository.h"
#include "repository/invitecoderepository.h"
#include "repository/labrepository.h"
#include "repository/samplerepository.h"
#include "repository/userrepository.h"
#include "translator.h"

#include <TeXSample/TeXSampleCore>

#include <QDebug>
#include <QList>
#include <QLocale>
#include <QMap>
#include <QRegExp>
#include <QString>
#include <QStringList>

/*============================================================================
================================ AuthorityResolver ===========================
============================================================================*/

/*============================== Public constructors =======================*/

AuthorityResolver::AuthorityResolver(DataSource *source, const AuthorityInfoProvider *provider,
                                     const AuthorityInfoProvider *defaultProvider) :
    GroupRepo(new GroupRepository(source)), InviteCodeRepo(new InviteCodeRepository(source)),
    LabRepo(new LabRepository(source)), SampleRepo(new SampleRepository(source)), Source(source),
    UserRepo(new UserRepository(source))
{
    mprovider = provider;
    mdefaultProvider = defaultProvider;
}

AuthorityResolver::~AuthorityResolver()
{
    delete GroupRepo;
    delete InviteCodeRepo;
    delete LabRepo;
    delete SampleRepo;
    delete UserRepo;
    delete Source;
}

/*============================== Public methods ============================*/

const AuthorityInfoProvider *AuthorityResolver::defaultProvider() const
{
    return mdefaultProvider;
}

bool AuthorityResolver::isValid() const
{
    return (mprovider || mdefaultProvider) && Source && Source->isValid() && GroupRepo->isValid() && LabRepo->isValid()
            && SampleRepo->isValid() && UserRepo->isValid();
}

bool AuthorityResolver::mayPerformOperation(const TUserInfo &info, AuthorityInfoProvider::OperationType type,
                                            const TRequest &request, QString *error) const
{
    Translator t(request.locale());
    if (!isValid()) {
        bRet(error, t.translate("AuthorityResolver", "Invalid AuthorityResolver instance (internal)", "error"),
             false);
    }
    QList<AuthorityInfo> list = mprovider ? mprovider->authorityInfoList(type) :
                                            mdefaultProvider->authorityInfoList(type);
    if (list.isEmpty() && mprovider)
        list = mdefaultProvider->authorityInfoList(type);
    if (list.isEmpty())
        bRet(error, t.translate("AuthorityInfoResolver", "No rule for operation (internal)", "error"), false);
    QStringList errList;
    QString err;
    foreach (const AuthorityInfo &auth, list) {
        if (testAuth(info, type, request, auth, t, &err))
            return bRet(error, QString(), true);
        else
            errList << err;
    }
    return bRet(error, errList.join("\n"), false);
}

const AuthorityInfoProvider *AuthorityResolver::provider() const
{
    return mprovider;
}

void AuthorityResolver::setDefaultProvider(const AuthorityInfoProvider *provider)
{
    mdefaultProvider = provider;
}

void AuthorityResolver::setProvider(const AuthorityInfoProvider *provider)
{
    mprovider = provider;
}

/*============================== Static private methods ====================*/

bool AuthorityResolver::predicate(const TAccessLevel &lvl1, const TAccessLevel &lvl2,
                                  AuthorityInfo::AccessLevelMode mode)
{
    if (AuthorityInfo::LessMode == mode)
        return lvl1 < lvl2;
    else if (AuthorityInfo::LessOrEqualMode)
        return lvl2 <= lvl2;
    else
        return false;
}

/*============================== Private methods ===========================*/

bool AuthorityResolver::testAccessLevel(const TAccessLevel &userAccessLevel,
                                            AuthorityInfo::AccessLevel requiredAccessLevel, TAccessLevel *level) const
{
    switch (requiredAccessLevel) {
    case AuthorityInfo::UserLevel:
        if (userAccessLevel.level() < TAccessLevel::UserLevel)
            return bRet(level, TAccessLevel(TAccessLevel::UserLevel), false);
        break;
    case AuthorityInfo::ModerLevel:
        if (userAccessLevel.level() < TAccessLevel::ModeratorLevel)
            return bRet(level, TAccessLevel(TAccessLevel::ModeratorLevel), false);
        break;
    case AuthorityInfo::AdminLevel:
        if (userAccessLevel.level() < TAccessLevel::AdminLevel)
            return bRet(level, TAccessLevel(TAccessLevel::AdminLevel), false);
        break;
    case AuthorityInfo::SuperLevel:
        if (userAccessLevel.level() < TAccessLevel::SuperuserLevel)
            return bRet(level, TAccessLevel(TAccessLevel::SuperuserLevel), false);
        break;
    case AuthorityInfo::SelfLevel:
    default:
        break;
    }
    return bRet(level, TAccessLevel(TAccessLevel::NoLevel), true);
}

bool AuthorityResolver::testAuth(const TUserInfo &info, AuthorityInfoProvider::OperationType type,
                                 const TRequest &request, const AuthorityInfo &auth, Translator &t,
                                 QString *error) const
{
    switch (auth.basicState) {
    case AuthorityInfo::No:
        return bRet(error, t.translate("AuthorityInfoResolver", "This operation is not allowed to anyone", "error"),
                    false);
    case AuthorityInfo::Any:
        return bRet(error, QString(), true);
    case AuthorityInfo::Authorized:
        if (!info.id())
            return bRet(error, t.translate("AuthorityInfoResolver", "Not authorized", "error"), false);
        break;
    case AuthorityInfo::Active:
        if (!info.active())
            return bRet(error, t.translate("AuthorityInfoResolver", "Account is inactive", "error"), false);
        break;
    default:
        break;
    }
    bool clab = ((auth.services & AuthorityInfo::CloudlabService)
                 && !info.availableServices().contains(TService::CloudlabService));
    bool tsmp = ((auth.services & AuthorityInfo::TexsampleService)
                 && !info.availableServices().contains(TService::TexsampleService));
    if (clab || tsmp) {
        QString err = t.translate("AuthorityInfoResolver", "No access to %1 service", "error");
        TService::Service srv = clab ? TService::CloudlabService : TService::TexsampleService;
        return bRet(error, err.arg(TService::serviceToString(srv)), false);
    }
    TAccessLevel level;
    if (!testAccessLevel(info.accessLevel(), auth.accessLevel, &level)) {
        return bRet(error, t.translate("Connection", "%1 access level is required", "error").arg(level.toString()),
                    false);
    }
    if (info.accessLevel().level() >= TAccessLevel::SuperuserLevel)
        return bRet(error, QString(), true);
    if (auth.self && !testOwnership(info.id(), type, request)) {
        return bRet(error, t.translate("AuthorityInfoResolver", "Operation is only allowed on own objects", "error"),
                    false);
    }
    if (auth.groupsMatch && !testGroups(info.groups(), type, request)) {
        return bRet(error, t.translate("AuthorityInfoResolver", "Accessing/creating objects that belong to groups you "
                                       "do not belong to is not allowed", "error"), false);
    }
    if (auth.selfGroupsMatch && !testGroups(info.availableGroups(), type, request)) {
        return bRet(error, t.translate("AuthorityInfoResolver",
                                       "Accessing/creating objects that belong to groups you are not an owner of is "
                                       "not allowed", "error"), false);
    }
    if (auth.servicesMach && !testServices(info.availableServices(), type, request)) {
        return bRet(error, t.translate("AuthorityInfoResolver",
                                       "Accessing/creating objects that belong to services you do not have access to "
                                       "is not allowed", "error"), false);
    }
    if (AuthorityInfo::NoMode != auth.targetAccessLevelMode
            || !testTargetAccessLevel(info.accessLevel(), type, auth.targetAccessLevel, auth.targetAccessLevelMode,
                                      request, &level)) {
        QString err = t.translate("AuthorityInfoResolver", "Accessing/creating objects with access level highter than "
                                  "yours is not allowed (%1 access level is required)", "error");
        return bRet(error, err.arg(level.toString()), false);
    }
    if (!testSampleTypes(auth.sampleTypes, type, request)) {
        return bRet(error, t.translate("AuthorityInfoResolver", "Operation is not allowed on this type of samples",
                                       "error"), false);
    }
    return bRet(error, QString(), true);
}

bool AuthorityResolver::testGroups(const TGroupInfoList &groups, AuthorityInfoProvider::OperationType type,
                                   const TRequest &request) const
{
    quint64 labId = 0;
    TUserIdentifier identifier;
    switch (type) {
    case AuthorityInfoProvider::AddLab:
        foreach (quint64 id, request.data().value<TAddLabRequestData>().groups()) {
            if (!groups.contains(id))
                return false;
        }
        break;
    case AuthorityInfoProvider::AddUser:
        foreach (quint64 id, request.data().value<TAddUserRequestData>().groups()) {
            if (!groups.contains(id))
                return false;
        }
        break;
    case AuthorityInfoProvider::DeleteGroup:
        if (!groups.contains(request.data().value<TDeleteGroupRequestData>().id()))
            return false;
        break;
    case AuthorityInfoProvider::DeleteInvites:
        foreach (const quint64 id, request.data().value<TDeleteInvitesRequestData>().ids()) {
            bool ok = false;
            InviteCode entity = InviteCodeRepo->findOne(id, &ok);
            if (!ok)
                return false;
            if (!entity.isValid())
                continue;
            foreach (quint64 id, entity.groups()) {
                if (!groups.contains(id))
                    return false;
            }
        }
        break;
    case AuthorityInfoProvider::DeleteLab:
        labId = request.data().value<TDeleteLabRequestData>().id();
        break;
    case AuthorityInfoProvider::DeleteUser:
        identifier = request.data().value<TDeleteUserRequestData>().identifier();
        break;
    case AuthorityInfoProvider::EditGroup:
        if (!groups.contains(request.data().value<TEditGroupRequestData>().id()))
            return false;
        break;
    case AuthorityInfoProvider::EditLab:
        labId = request.data().value<TEditLabRequestData>().id();
        break;
    case AuthorityInfoProvider::EditUser:
        identifier = request.data().value<TEditUserRequestData>().identifier();
        break;
    case AuthorityInfoProvider::GenerateInvites:
        foreach (quint64 id, request.data().value<TGenerateInvitesRequestData>().groups()) {
            if (!groups.contains(id))
                return false;
        }
        break;
    case AuthorityInfoProvider::GetLabData:
        labId = request.data().value<TGetLabDataRequestData>().labId();
        break;
    case AuthorityInfoProvider::GetLabExtraFile:
        labId = request.data().value<TGetLabExtraFileRequestData>().labId();
        break;
    case AuthorityInfoProvider::GetUserInfo:
        identifier = request.data().value<TGetUserInfoRequestData>().identifier();
        break;
    case AuthorityInfoProvider::GetUserInfoAdmin:
        identifier = request.data().value<TGetUserInfoAdminRequestData>().identifier();
        break;
    default:
        break;
    }
    if (labId) {
        bool ok = false;
        Lab entity = LabRepo->findOne(labId, &ok);
        if (!ok)
            return false;
        if (!entity.isValid())
            return true;
        foreach (quint64 id, entity.groups()) {
            if (!groups.contains(id))
                return false;
        }
    } else if (identifier.isValid()) {
        bool ok = false;
        User entity = UserRepo->findOne(identifier, &ok);
        if (!ok)
            return false;
        if (!entity.isValid())
            return true;
        foreach (quint64 id, entity.groups()) {
            if (!groups.contains(id))
                return false;
        }
    }
    return true;
}

bool AuthorityResolver::testOwnership(quint64 userId, AuthorityInfoProvider::OperationType type,
                                      const TRequest &request) const
{
    if (!userId)
        return false;
    quint64 groupId = 0;
    quint64 labId = 0;
    quint64 sampleId = 0;
    TUserIdentifier identifier;
    switch (type) {
    case AuthorityInfoProvider::DeleteGroup:
        groupId = request.data().value<TDeleteGroupRequestData>().id();
        break;
    case AuthorityInfoProvider::DeleteInvites:
        foreach (const quint64 id, request.data().value<TDeleteInvitesRequestData>().ids()) {
            bool ok = false;
            InviteCode entity = InviteCodeRepo->findOne(id, &ok);
            if (!ok)
                return false;
            if (!entity.isValid())
                continue;
            if (entity.ownerId() != userId)
                return false;
        }
        break;
    case AuthorityInfoProvider::DeleteLab:
        labId = request.data().value<TDeleteLabRequestData>().id();
        break;
    case AuthorityInfoProvider::DeleteSample:
        sampleId = request.data().value<TDeleteSampleRequestData>().id();
        break;
    case AuthorityInfoProvider::DeleteUser:
        identifier = request.data().value<TDeleteUserRequestData>().identifier();
        break;
    case AuthorityInfoProvider::EditGroup:
        groupId = request.data().value<TEditGroupRequestData>().id();
        break;
    case AuthorityInfoProvider::EditLab:
        labId = request.data().value<TEditLabRequestData>().id();
        break;
    case AuthorityInfoProvider::EditSample:
        sampleId = request.data().value<TEditSampleRequestData>().id();
        break;
    case AuthorityInfoProvider::EditSampleAdmin:
        sampleId = request.data().value<TEditSampleAdminRequestData>().id();
        break;
    case AuthorityInfoProvider::GetLabData:
        labId = request.data().value<TGetLabDataRequestData>().labId();
        break;
    case AuthorityInfoProvider::GetLabExtraFile:
        labId = request.data().value<TGetLabExtraFileRequestData>().labId();
        break;
    case AuthorityInfoProvider::GetSamplePreview:
        sampleId = request.data().value<TGetSamplePreviewRequestData>().id();
        break;
    case AuthorityInfoProvider::GetSampleSource:
        sampleId = request.data().value<TGetSampleSourceRequestData>().id();
        break;
    case AuthorityInfoProvider::GetUserInfo:
        identifier = request.data().value<TGetUserInfoRequestData>().identifier();
        break;
    case AuthorityInfoProvider::GetUserInfoAdmin:
        identifier = request.data().value<TGetUserInfoAdminRequestData>().identifier();
        break;
    default:
        break;
    }
    if (groupId) {
        bool ok = false;
        Group entity = GroupRepo->findOne(groupId, &ok);
        if (!ok)
            return false;
        if (!entity.isValid())
            return true;
        if (entity.ownerId() != userId)
            return false;
    } else if (labId) {
        bool ok = false;
        Lab entity = LabRepo->findOne(labId, &ok);
        if (!ok)
            return false;
        if (!entity.isValid())
            return true;
        if (entity.senderId() != userId)
            return false;
    } else if (sampleId) {
        bool ok = false;
        Sample entity = SampleRepo->findOne(sampleId, &ok);
        if (!ok)
            return false;
        if (!entity.isValid())
            return true;
        if (entity.senderId() != userId)
            return false;
    } else if (identifier.isValid()) {
        bool ok = false;
        User entity = UserRepo->findOne(identifier, &ok);
        if (!ok)
            return false;
        if (!entity.isValid())
            return true;
        if (entity.id() != userId)
            return false;
    }
    return true;
}

bool AuthorityResolver::testSampleTypes(AuthorityInfo::SampleTypes sampleTypes,
                                        AuthorityInfoProvider::OperationType type, const TRequest &request) const
{
    quint64 sampleId = 0;
    switch (type) {
    case AuthorityInfoProvider::AddSample:
        if (!(sampleTypes & AuthorityInfo::Unverified))
            return false;
        break;
    case AuthorityInfoProvider::DeleteSample:
        sampleId = request.data().value<TDeleteSampleRequestData>().id();
        break;
    case AuthorityInfoProvider::EditSample:
        sampleId = request.data().value<TDeleteSampleRequestData>().id();
        break;
    case AuthorityInfoProvider::EditSampleAdmin:
        sampleId = request.data().value<TDeleteSampleRequestData>().id();
        break;
    case AuthorityInfoProvider::GetSamplePreview:
        sampleId = request.data().value<TDeleteSampleRequestData>().id();
        break;
    case AuthorityInfoProvider::GetSampleSource:
        sampleId = request.data().value<TDeleteSampleRequestData>().id();
        break;
    default:
        break;
    }
    if (!sampleId)
        return true;
    bool ok = false;
    Sample entity = SampleRepo->findOne(sampleId, &ok);
    if (!ok)
        return false;
    if (!entity.isValid())
        return true;
    switch (entity.type().type()) {
    case TSampleType::Unverified:
        if (!(sampleTypes & AuthorityInfo::Unverified))
            return false;
        break;
    case TSampleType::Approved:
        if (!(sampleTypes & AuthorityInfo::Approved))
            return false;
        break;
    case TSampleType::Rejected:
        if (!(sampleTypes & AuthorityInfo::Rejected))
            return false;
        break;
    default:
        break;
    }
    return true;
}

bool AuthorityResolver::testServices(const TServiceList &services, AuthorityInfoProvider::OperationType type,
                                     const TRequest &request) const
{
    TUserIdentifier userId;
    switch (type) {
    case AuthorityInfoProvider::AddUser:
        foreach (const TService &service, request.data().value<TAddUserRequestData>().availableServices()) {
            if (!services.contains(service))
                return false;
        }
        break;
    case AuthorityInfoProvider::DeleteInvites:
        foreach (quint64 id, request.data().value<TDeleteInvitesRequestData>().ids()) {
            bool ok = false;
            InviteCode entity = InviteCodeRepo->findOne(id, &ok);
            if (!ok)
                return false;
            if (!entity.isValid())
                continue;
            foreach (const TService &service, entity.availableServices()) {
                if (!services.contains(service))
                    return false;
            }
        }
        break;
    case AuthorityInfoProvider::DeleteUser:
        userId = request.data().value<TDeleteUserRequestData>().identifier();
        break;
    case AuthorityInfoProvider::EditUser:
        userId = request.data().value<TEditUserRequestData>().identifier();
        break;
    case AuthorityInfoProvider::GenerateInvites:
        foreach (const TService &service, request.data().value<TGenerateInvitesRequestData>().services()) {
            if (!services.contains(service))
                return false;
        }
        break;
    case AuthorityInfoProvider::GetUserInfo:
        userId = request.data().value<TGetUserInfoRequestData>().identifier();
        break;
    case AuthorityInfoProvider::GetUserInfoAdmin:
        userId = request.data().value<TGetUserInfoAdminRequestData>().identifier();
        break;
    default:
        break;
    }
    if (!userId.isValid())
        return true;
    bool ok = false;
    User entity = UserRepo->findOne(userId, &ok);
    if (!ok)
        return false;
    if (!entity.isValid())
        return true;
    foreach (const TService &service, entity.availableServices()) {
        if (!services.contains(service))
            return false;
    }
    return true;
}

bool AuthorityResolver::testTargetAccessLevel(
        const TAccessLevel &userAccessLevel, AuthorityInfoProvider::OperationType type,
        AuthorityInfo::AccessLevel targetAccessLevel, AuthorityInfo::AccessLevelMode accessLevelMode,
        const TRequest &request, TAccessLevel *level) const
{
    if (AuthorityInfo::SelfLevel == targetAccessLevel) {
        switch (userAccessLevel.level()) {
        case TAccessLevel::UserLevel:
            targetAccessLevel = AuthorityInfo::UserLevel;
            break;
        case TAccessLevel::ModeratorLevel:
            targetAccessLevel = AuthorityInfo::ModerLevel;
            break;
        case TAccessLevel::AdminLevel:
            targetAccessLevel = AuthorityInfo::AdminLevel;
            break;
        case TAccessLevel::SuperuserLevel:
            targetAccessLevel = AuthorityInfo::SuperLevel;
            break;
        default:
            break;
        }
    }
    TAccessLevel actualLevel;
    TUserIdentifier userId;
    switch (type) {
    case AuthorityInfoProvider::AddUser:
        actualLevel = request.data().value<TAddUserRequestData>().accessLevel();
        break;
    case AuthorityInfoProvider::DeleteInvites:
        foreach (quint64 id, request.data().value<TDeleteInvitesRequestData>().ids()) {
            bool ok = false;
            InviteCode entity = InviteCodeRepo->findOne(id, &ok);
            if (!ok)
                return bRet(level, TAccessLevel(), false);
            if (!entity.isValid())
                continue;
            if (entity.accessLevel() > actualLevel)
                actualLevel = entity.accessLevel();
        }
        break;
    case AuthorityInfoProvider::DeleteUser:
        userId = request.data().value<TDeleteUserRequestData>().identifier();
        break;
    case AuthorityInfoProvider::EditUser:
        actualLevel = request.data().value<TEditUserRequestData>().accessLevel();
        break;
    case AuthorityInfoProvider::GenerateInvites:
        actualLevel = request.data().value<TGenerateInvitesRequestData>().accessLevel();
        break;
    case AuthorityInfoProvider::GetUserInfo:
        userId = request.data().value<TGetUserInfoRequestData>().identifier();
        break;
    case AuthorityInfoProvider::GetUserInfoAdmin:
        userId = request.data().value<TGetUserInfoAdminRequestData>().identifier();
        break;
    default:
        break;
    }
    if (userId.isValid()) {
        bool ok = false;
        User entity = UserRepo->findOne(userId, &ok);
        if (!ok)
            return bRet(level, TAccessLevel(), false);
        if (!entity.isValid())
            return bRet(level, TAccessLevel(), true);
        actualLevel = entity.accessLevel();
    }
    if (!actualLevel.isValid())
        return bRet(level, TAccessLevel(), true);
    switch (targetAccessLevel) {
    case AuthorityInfo::UserLevel:
        if (predicate(userAccessLevel, TAccessLevel::UserLevel, accessLevelMode))
            return bRet(level, TAccessLevel(TAccessLevel::UserLevel), false);
        break;
    case AuthorityInfo::ModerLevel:
        if (predicate(userAccessLevel, TAccessLevel::ModeratorLevel, accessLevelMode))
            return bRet(level, TAccessLevel(TAccessLevel::ModeratorLevel), false);
        break;
    case AuthorityInfo::AdminLevel:
        if (predicate(userAccessLevel, TAccessLevel::AdminLevel, accessLevelMode))
            return bRet(level, TAccessLevel(TAccessLevel::AdminLevel), false);
        break;
    case AuthorityInfo::SuperLevel:
        if (predicate(userAccessLevel, TAccessLevel::SuperuserLevel, accessLevelMode))
            return bRet(level, TAccessLevel(TAccessLevel::SuperuserLevel), false);
        break;
    default:
        break;
    }
    return bRet(level, TAccessLevel(), true);
}
