#include "application.h"
#include "storage.h"
#include "server.h"
#include "connection.h"

#include <TInviteInfo>

#include <BCoreApplication>

#include <QTimerEvent>

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
    mstorage = new Storage;
    mserver = new Server(this);
    timerId = startTimer(BeQt::Hour);
}

Application::~Application()
{
    delete mstorage;
    delete mserver;
}

/*============================== Protected =================================*/

void Application::timerEvent(QTimerEvent *e)
{
    if (!e || e->timerId() != timerId)
        return;
    mstorage->testInvites();
    mstorage->testRecoveryCodes();
}
