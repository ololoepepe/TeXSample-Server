#ifndef INVITECODEREPOSITORY_H
#define INVITECODEREPOSITORY_H

class DataSource;

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
    bool deleteSome(const TIdList &ids);
    QList<InviteCode> findAllByOwnerId(quint64 ownerId);
    bool isValid() const;
private:
    friend class InviteCode;
    Q_DISABLE_COPY(InviteCodeRepository)
};

#endif // INVITECODEREPOSITORY_H
