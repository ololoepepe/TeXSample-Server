#ifndef APPLICATION_H
#define APPLICATION_H

class Storage;
class Server;

class QTimerEvent;

#include <BCoreApplication>

#include <QObject>

#if defined(bApp)
#undef bApp
#endif
#define bApp static_cast<Application *>(BCoreApplication::instance())

/*============================================================================
================================ Application =================================
============================================================================*/

class Application : public BCoreApplication
{
    Q_OBJECT
public:
    static Server *server();
public:
    explicit Application();
    ~Application();
protected:
    void timerEvent(QTimerEvent *e);
private:
    Storage *mstorage;
    Server *mserver;
    int timerId;
private:
    Q_DISABLE_COPY(Application)
};

#endif // APPLICATION_H
