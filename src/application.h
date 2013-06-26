#ifndef APPLICATION_H
#define APPLICATION_H

class Storage;

class TInviteInfo;

#include <BCoreApplication>

#include <QObject>

#if defined(bApp)
#undef bApp
#endif
#define bApp (static_cast<Application *>(BCoreApplication::instance()))

/*============================================================================
================================ Application =================================
============================================================================*/

class Application : public BCoreApplication
{
    Q_OBJECT
public:
    explicit Application();
    ~Application();
public:
    void scheduleInvitesAutoTest(const TInviteInfo &info);
private slots:
    void timeout();
private:
    Storage *mstorage;
private:
    Q_DISABLE_COPY(Application)
};

#endif // APPLICATION_H
