#include "authorityinfo.h"

#include <QFlags>

/*============================================================================
================================ AuthorityInfo ===============================
============================================================================*/

/*============================== Public constructors =======================*/

AuthorityInfo::AuthorityInfo()
{
    accessLevel = SelfLevel;
    basicState = No;
    groupsMatch = false;
    sampleTypes = SampleTypes(Unverified | Approved | Rejected);
    self = false;
    selfGroupsMatch = false;
    services = NoService;
    servicesMach = false;
    targetAccessLevel = SelfLevel;
    targetAccessLevelMode = NoMode;
}
