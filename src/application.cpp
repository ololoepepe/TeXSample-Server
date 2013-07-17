#include "application.h"
#include "storage.h"
#include "server.h"
#include "connection.h"

#include <TInviteInfo>

#include <BCoreApplication>

#include <QMutex>
#include <QMutexLocker>
#include <QTimer>
#include <QDateTime>

/*============================================================================
================================ Application =================================
============================================================================*/

/*============================== Static public methods =====================*/

Server *Application::server()
{
    return bApp ? bApp->mserver : 0;
}

/*============================== Public constructors =======================*/

Application::Application() :
    BCoreApplication()
{
    mstorage = 0;
    mserver = new Server(this);
}

Application::~Application()
{
    delete mstorage;
    delete mserver;
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
