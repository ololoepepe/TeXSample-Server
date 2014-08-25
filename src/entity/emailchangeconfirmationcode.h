#ifndef EMAILCHANGECONFIRMATIONCODE_H
#define EMAILCHANGECONFIRMATIONCODE_H

class EmailChangeConfirmationCodeRepository;

#include <BUuid>

#include <QDateTime>
#include <QString>

/*============================================================================
================================ EmailChangeConfirmationCode =================
============================================================================*/

class EmailChangeConfirmationCode
{
private:
    quint64 muserId;
    BUuid mcode;
    QString memail;
    QDateTime mexpirationDateTime;
    bool createdByRepo;
    EmailChangeConfirmationCodeRepository *repo;
    bool valid;
public:
    explicit EmailChangeConfirmationCode();
    EmailChangeConfirmationCode(const EmailChangeConfirmationCode &other);
    ~EmailChangeConfirmationCode();
protected:
    explicit EmailChangeConfirmationCode(EmailChangeConfirmationCodeRepository *repo);
public:
    BUuid code() const;
    void convertToCreatedByUser();
    QString email() const;
    QDateTime expirationDateTime() const;
    bool isCreatedByRepo() const;
    bool isValid() const;
    EmailChangeConfirmationCodeRepository *repository() const;
    void setCode(const BUuid &code);
    void setEmail(const QString &email);
    void setExpirationDateTime(const QDateTime &dt);
    void setUserId(quint64 userId);
    quint64 userId() const;
public:
    EmailChangeConfirmationCode &operator =(const EmailChangeConfirmationCode &other);
private:
    void init();
private:
    friend class EmailChangeConfirmationCodeRepository;
};

#endif // EMAILCHANGECONFIRMATIONCODE_H
