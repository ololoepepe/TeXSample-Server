#include "application.h"
#include "storage.h"

#include <TInviteInfo>

#include <BCoreApplication>

#include <QMutex>
#include <QMutexLocker>
#include <QTimer>
#include <QDateTime>

/*============================================================================
================================ Application =================================
============================================================================*/

/*============================== Public constructors =======================*/

Application::Application() :
    BCoreApplication()
{
    mstorage = 0;
}

Application::~Application()
{
    delete mstorage;
}

/*============================== Public methods ============================*/

void Application::scheduleInvitesAutoTest(const TInviteInfo &info)
{
    if (!info.isValid())
        return;
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    if (!mstorage)
        mstorage = new Storage;
    int msecs = info.expirationDateTime().toMSecsSinceEpoch() - QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    msecs *= 1.1;
    QTimer::singleShot(msecs, this, SLOT(invitesTestTimeout()));
}

void Application::scheduleRecoveryCodesAutoTest(const QDateTime &expirationDT)
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    if (!mstorage)
        mstorage = new Storage;
    int msecs = expirationDT.toMSecsSinceEpoch() - QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    msecs *= 1.1;
    QTimer::singleShot(msecs, this, SLOT(recoveryCodesTestTimeout()));
}

/*============================== Public methods ============================*/

void Application::invitesTestTimeout()
{
    mstorage->testInvites();
}

void Application::recoveryCodesTestTimeout()
{
    mstorage->testRecoveryCodes();
}
