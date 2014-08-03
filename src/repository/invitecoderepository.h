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
public:
    explicit InviteCodeRepository(DataSource *source);
    ~InviteCodeRepository();
public:
    long count();
    DataSource *dataSource() const;
    bool deleteAll();
    bool deleteOne(quint64 id);
    bool deleteSome(const TIdList &ids);
    bool exists(quint64 id);
    QList<InviteCode> findAll();
    QList<InviteCode> findAll(const TIdList &ids);
    InviteCode findOne(quint64 id);
    bool isValid() const;
    quint64 save(const InviteCode &entity);
    TIdList save(const QList<InviteCode> &entities);
private:
    DataSource * const Source;
private:
    friend class InviteCode;
    Q_DISABLE_COPY(InviteCodeRepository)
};

#endif // INVITECODEREPOSITORY_H
