#ifndef DATABASEINTERACTOR_H
#define DATABASEINTERACTOR_H

#include <QString>

class DatabaseInteractor
{
public:
    static void setAdminInfo(const QString &login, const QString &password);
    static bool checkAdmin();
private:
    DatabaseInteractor();
    DatabaseInteractor(const DatabaseInteractor &other);
    ~DatabaseInteractor();
    //
    DatabaseInteractor &operator=(const DatabaseInteractor &other);
};

#endif // DATABASEINTERACTOR_H
