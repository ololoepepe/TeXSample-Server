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
