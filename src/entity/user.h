#ifndef USER_H
#define USER_H

class UserRepository;

#include <TAccessLevel>
#include <TGroupInfoList>
#include <TIdList>
#include <TServiceList>

#include <QByteArray>
#include <QDateTime>
#include <QImage>
#include <QString>

/*============================================================================
================================ User ========================================
============================================================================*/

class User
{
private:
    quint64 mid;
    TAccessLevel maccessLevel;
    bool mactive;
    TIdList mavailableGroupIds;
    TGroupInfoList mavailableGroups;
    TServiceList mavailableServices;
    QImage mavatar;
    QString memail;
    TIdList mgroupIds;
    TGroupInfoList mgroups;
    QDateTime mlastModificationDateTime;
    QString mlogin;
    QString mname;
    QByteArray mpassword;
    QString mpatronymic;
    QDateTime mregistrationDateTime;
    bool msaveAvatar;
    QString msurname;
    bool avatarFetched;
    bool createdByRepo;
    UserRepository *repo;
    bool valid;
public:
    explicit User(bool saveAvatar = true);
    User(const User &other);
    ~User();
protected:
    explicit User(UserRepository *repo);
public:
    TAccessLevel accessLevel() const;
    bool active() const;
    TIdList availableGroupIds() const;
    TGroupInfoList availableGroups() const;
    TServiceList availableServices() const;
    QImage avatar() const;
    QString email() const;
    TIdList groupIds() const;
    TGroupInfoList groups() const;
    quint64 id() const;
    bool isCreatedByRepo() const;
    bool isValid() const;
    QDateTime lastModificationDateTime() const;
    QString login() const;
    QString name() const;
    QByteArray password() const;
    QString patronymic() const;
    QDateTime registrationDateTime() const;
    UserRepository *repository() const;
    bool saveAvatar() const;
    void setAccessLevel(const TAccessLevel &accessLevel);
    void setActive(bool active);
    void setAvailableGroupIds(const TIdList &ids);
    void setAvailableServices(const TServiceList &services);
    void setAvatar(const QImage &avatar);
    void setEmail(const QString &email);
    void setGroupIds(const TIdList &ids);
    void setId(quint64 id);
    void setLastModificationDateTime(const QDateTime &dt);
    void setLogin(const QString &login);
    void setName(const QString &name);
    void setPassword(const QByteArray &password);
    void setPatronymic(const QString &partonymic);
    void setRegistrationDateTime(const QDateTime &dt);
    void setSurname(const QString &surname);
    QString surname() const;
public:
    User &operator =(const User &other);
private:
    void init();
private:
    friend class UserRepository;
};

#endif // USER_H
