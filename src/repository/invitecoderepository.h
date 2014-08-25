#ifndef INVITECODEREPOSITORY_H
#define INVITECODEREPOSITORY_H

class DataSource;

class BUuid;

#include "entity/invitecode.h"

#include <TIdList>

#include <QList>

/*============================================================================
================================ InviteCodeRepository ========================
============================================================================*/

class InviteCodeRepository
{
private:
    DataSource * const Source;
public:
    explicit InviteCodeRepository(DataSource *source);
    ~InviteCodeRepository();
public:
    quint64 add(const InviteCode &entity);
    DataSource *dataSource() const;
    bool deleteExpired();
    bool deleteOne(quint64 id);
    bool deleteSome(const TIdList &ids);
    QList<InviteCode> findAll();
    QList<InviteCode> findAllByOwnerId(quint64 ownerId);
    InviteCode findOne(quint64 id);
    InviteCode findOneByCode(const BUuid &code);
    bool isValid() const;
private:
    friend class InviteCode;
    Q_DISABLE_COPY(InviteCodeRepository)
};

#endif // INVITECODEREPOSITORY_H
