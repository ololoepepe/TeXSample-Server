#ifndef AUTHORITYINFO_H
#define AUTHORITYINFO_H

#include <QFlags>

/*============================================================================
================================ AuthorityInfo ===============================
============================================================================*/

struct AuthorityInfo
{
public:
    enum AccessLevel
    {
        SelfLevel = 0,
        UserLevel,
        ModerLevel,
        AdminLevel,
        SuperLevel
    };
    enum AccessLevelMode
    {
        NoMode = 0,
        LessMode,
        LessOrEqualMode
    };
    enum BasicState
    {
        No = 0,
        Any,
        Authorized,
        Active
    };
    enum SampleType
    {
        NoType,
        Unverified,
        Approved,
        Rejected
    };
    Q_DECLARE_FLAGS(SampleTypes, SampleType)
    enum Service
    {
        NoService = 0,
        CloudlabService,
        TexsampleService
    };
    Q_DECLARE_FLAGS(Services, Service)
public:
    AccessLevel accessLevel;
    BasicState basicState;
    bool groupsMatch;
    SampleTypes sampleTypes;
    bool self;
    bool selfGroupsMatch;
    Services services;
    bool servicesMach;
    AccessLevel targetAccessLevel;
    AccessLevelMode targetAccessLevelMode;
public:
    explicit AuthorityInfo();
};

#endif // AUTHORITYINFO_H
