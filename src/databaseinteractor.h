#ifndef DATABASEINTERACTOR_H
#define DATABASEINTERACTOR_H

class QSqlDatabase;

#include <QString>
#include <QStringList>
#include <QVariantList>

class DatabaseInteractor
{
public:
    static void setAdminInfo(const QString &login, const QString &password);
    static bool checkAdmin();
    static bool checkUser(const QString &login, const QString &password);
    static bool checkSampleExistance(const QString &title, const QString &author);
    static bool checkSampleAuthorship(const QString &author, const QString &id);
    static QString insertSample(const QString &title, const QString &author,
                                const QString &tags, const QString &comment);
    static bool deleteSample(const QString &id);
private:
    static QSqlDatabase *createDatabase();
    static void removeDatabase(QSqlDatabase *db);
    static bool callStoredProcedure(const QString &procedure, const QStringList &parameters, QVariantList &results);
    //
    DatabaseInteractor();
    DatabaseInteractor(const DatabaseInteractor &other);
    ~DatabaseInteractor();
    //
    DatabaseInteractor &operator=(const DatabaseInteractor &other);
};

#endif // DATABASEINTERACTOR_H
