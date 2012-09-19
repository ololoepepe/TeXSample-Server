#ifndef DATABASEINTERACTOR_H
#define DATABASEINTERACTOR_H

#include <QString>

class DatabaseInteractor
{
public:
    static void setAdminInfo(const QString &login, const QString &password);
    static bool checkAdmin();
    static bool checkUser(const QString &login, const QString &password);
    static QString insertSample(const QString &title, const QString &author,
                                const QString &tags, const QString &comment);
    static bool deleteSample(const QString &id);
private:
    DatabaseInteractor();
    DatabaseInteractor(const DatabaseInteractor &other);
    ~DatabaseInteractor();
    //
    DatabaseInteractor &operator=(const DatabaseInteractor &other);
};

#endif // DATABASEINTERACTOR_H
