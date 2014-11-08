#ifndef AUTHORITYRESOLVER_H
#define AUTHORITYRESOLVER_H

class DataSource;
class GroupRepository;
class InviteCodeRepository;
class LabRepository;
class SampleRepository;
class Translator;
class UserRepository;

class TAccessLevel;
class TGroupInfoList;
class TRequest;
class TServiceList;
class TUserInfo;

class QString;

#include "authorityinfo.h"
#include "authorityinfoprovider.h"

/*============================================================================
================================ AuthorityResolver ===========================
============================================================================*/

class AuthorityResolver
{
private:
    GroupRepository * const GroupRepo;
    InviteCodeRepository * const InviteCodeRepo;
    LabRepository * const LabRepo;
    SampleRepository * const SampleRepo;
    DataSource * const Source;
    UserRepository * const UserRepo;
private:
    const AuthorityInfoProvider *mdefaultProvider;
    const AuthorityInfoProvider *mprovider;
public:
    explicit AuthorityResolver(DataSource *source, const AuthorityInfoProvider *provider = 0,
                               const AuthorityInfoProvider *defaultProvider = 0);
    ~AuthorityResolver();
public:
    const AuthorityInfoProvider *defaultProvider() const;
    bool isValid() const;
    bool mayPerformOperation(const TUserInfo &info, AuthorityInfoProvider::OperationType type, const TRequest &request,
                             QString *error = 0) const;
    const AuthorityInfoProvider *provider() const;
    void setDefaultProvider(const AuthorityInfoProvider *provider);
    void setProvider(const AuthorityInfoProvider *provider);
private:
    static bool predicate(const TAccessLevel &lvl1, const TAccessLevel &lvl2, AuthorityInfo::AccessLevelMode mode);
private:
    bool testAccessLevel(const TAccessLevel &userAccessLevel, AuthorityInfo::AccessLevel requiredAccessLevel,
                         TAccessLevel *level = 0) const;
    bool testAuth(const TUserInfo &info, AuthorityInfoProvider::OperationType type, const TRequest &request,
                  const AuthorityInfo &auth, Translator &t, QString *error = 0) const;
    bool testGroups(const TGroupInfoList &groups, AuthorityInfoProvider::OperationType type,
                    const TRequest &request) const;
    bool testOwnership(quint64 userId, AuthorityInfoProvider::OperationType type, const TRequest &request) const;
    bool testSampleTypes(AuthorityInfo::SampleTypes sampleTypes, AuthorityInfoProvider::OperationType type,
                         const TRequest &request) const;
    bool testServices(const TServiceList &services, AuthorityInfoProvider::OperationType type,
                      const TRequest &request) const;
    bool testTargetAccessLevel(const TAccessLevel &userAccessLevel, AuthorityInfoProvider::OperationType type,
                               AuthorityInfo::AccessLevel targetAccesslevel,
                               AuthorityInfo::AccessLevelMode accessLevelMode, const TRequest &request,
                               TAccessLevel *level = 0) const;
};

#endif // AUTHORITYRESOLVER_H
