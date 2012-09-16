#include "src/userconnection.h"
#include "src/databaseinteractor.h"
#include "include/texsampleserver.h"

#include <bnetworkconnection.h>
#include <bnetworkoperation.h>
#include <bcore.h>

#include <QTimer>
#include <QByteArray>
#include <QMap>
#include <QString>
#include <QDataStream>
#include <QMutex>
#include <QMutexLocker>
#include <QFile>
#include <QTextStream>
#include <QIODevice>

const int AuthorizationTimeout = 15 * BCore::Second;
const int IdLength = 20;
const QString SampleBaseName = "___sample___";
const QString SamplePrefix = "\\input texsample.tex\n\n";
const QString SamplePostfix = "\n\n\\end{document}";
const QStringList ExtraFiles = QStringList() << SampleBaseName + ".aux" << SampleBaseName + ".idx"
    << SampleBaseName + ".log" << SampleBaseName + ".out" << SampleBaseName + ".pdf" << SampleBaseName + ".tex";

//

bool checkId(const QString &id)
{
    bool ok = false;
    int iid = id.toInt(&ok);
    return ok && iid > 0 && id.length() <= IdLength;
}

QString expandId(const QString &id)
{
    QString eid = id;
    int len = eid.length();
    if (len < IdLength)
        eid.prepend( QString().fill('0', IdLength - len) );
    return eid;
}

bool readFiles(const QString &dir, QString &sample, TexSampleServer::FilePairList &list)
{
    //Reading sample
    QFile f(dir + "/" + SampleBaseName + ".tex");
    if ( !f.open(QFile::ReadOnly) )
        return false;
    QTextStream fin(&f);
    fin.setCodec("UTF-8");
    sample = fin.readAll().remove(SamplePrefix).remove(SamplePostfix);
    f.close();
    //Reading auxiliary files
    QStringList sl = QDir(dir).entryList(QDir::Files);
    for (int i = 0; i < ExtraFiles.size(); ++i)
        sl.removeAll( ExtraFiles.at(i) );
    for (int i = 0; i < sl.size(); ++i)
    {
        TexSampleServer::FilePair fp;
        fp.first = sl.at(i);
        QFile f(dir + "/" + fp.first);
        if ( !f.open(QFile::ReadOnly) )
            return false;
        fp.second = f.readAll();
        f.close();
        if ( fp.second.isEmpty() )
            continue;
        list << fp;
    }
    return true;
}

//

UserConnection::UserConnection(BGenericSocket *socket, QObject *parent) :
    BNetworkConnection(socket, parent)
{
    if ( !isConnected() )
        return;
    //initializing members
    mauthorized = false;
    //adding reply handlers
    mreplyHandlers.insert(TexSampleServer::AuthorizeOperation, &UserConnection::handleReplyAuthorization);
    //adding request handlers
    mrequestHandlers.insert(TexSampleServer::GetPdfOperation, &UserConnection::handleRequestGetPdf);
    mrequestHandlers.insert(TexSampleServer::GetSampleOperation, &UserConnection::handleRequestGetSample);
    //other
    connect( this, SIGNAL( replyReceived(BNetworkOperation *) ),
             this, SLOT( replyReceivedSlot(BNetworkOperation *) ) );
    connect( this, SIGNAL( requestReceived(BNetworkOperation *) ),
             this, SLOT( requestReceivedSlot(BNetworkOperation *) ) );
    QTimer::singleShot( AuthorizationTimeout, this, SLOT( checkAuthorization() ) );
    sendRequest(TexSampleServer::AuthorizeOperation);
}

//reply handlers

void UserConnection::handleReplyAuthorization(BNetworkOperation *operation)
{
    if (!operation)
        return;
    QDataStream in( operation->data() );
    in.setVersion(TexSampleServer::DataStreamVersion);
    QString login;
    QString password;
    in >> login;
    in >> password;
    mauthorized = DatabaseInteractor::checkUser(login, password);
    if (!mauthorized)
    {
        close();
        return;
    }
    sendRequest(TexSampleServer::GetVersionOperation);
}

//request handlers

void UserConnection::handleRequestGetPdf(BNetworkOperation *operation)
{
    sendSample(operation, true);
}

void UserConnection::handleRequestGetSample(BNetworkOperation *operation)
{
    sendSample(operation, false);
}

//other

bool UserConnection::standardCheck(const QString &id, QDataStream &out)
{
    if (!mauthorized)
    {
        out << false;
        out << tr("Not authorized", "reply text");
        return false;
    }
    if ( !checkId(id) )
    {
        out << false;
        out << tr("Invalid id", "reply text");
        return false;
    }
    return true;
}

void UserConnection::sendSample(BNetworkOperation *operation, bool pdf)
{
    QDataStream in( operation->data() );
    in.setVersion(TexSampleServer::DataStreamVersion);
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(TexSampleServer::DataStreamVersion);
    QString id;
    in >> id;
    if ( !standardCheck(id, out) )
        return mySendReply(operation, data);
    QString dir = BCore::user("samples") + "/" + expandId(id);
    if ( !QDir(dir).exists() )
    {
        out << false;
        out << tr("No such sample", "reply text");
        return mySendReply(operation, data);
    }
    if (pdf)
    {
        QFile f(dir + "/" + SampleBaseName + ".pdf");
        if ( !f.open(QFile::ReadOnly) )
        {
            out << false;
            out << tr("Internal error", "reply text");
            return mySendReply(operation, data);
        }
        out << true;
        out << f.readAll();
        f.close();
    }
    else
    {
        QString sample;
        TexSampleServer::FilePairList fpl;
        if ( !readFiles(dir, sample, fpl) )
        {
            out << false;
            out << tr("Internal error", "reply text");
            return mySendReply(operation, data);
        }
        out << true;
        out << sample;
        for (int i = 0; i < fpl.size(); ++i)
        {
            const TexSampleServer::FilePair &fp = fpl.at(i);
            out << fp.first;
            out << fp.second;
        }
    }
    sendReply(operation, data);
}

void UserConnection::mySendReply(BNetworkOperation *operation, const QByteArray &data)
{
    sendReply(operation, data);
}

//

void UserConnection::replyReceivedSlot(BNetworkOperation *operation)
{
    Handler h = operation ? mreplyHandlers.value( operation->metaData().operation() ) : 0;
    if (h)
        (this->*h)(operation);
    if (operation)
        operation->deleteLater();
}

void UserConnection::requestReceivedSlot(BNetworkOperation *operation)
{
    Handler h = operation ? mrequestHandlers.value( operation->metaData().operation() ) : 0;
    if (h && mauthorized)
        (this->*h)(operation);
    else if (operation)
        operation->deleteLater();
}

void UserConnection::checkAuthorization()
{
    if (!mauthorized)
        close();
}

/*
const int UpdateNotifyTimeout = 1 * BCore::Second;
const int CriticalBufferSize = 100 * BCore::Megabyte;
//


const QString CompilerCommand = "pdflatex";
const QStringList CompilerArguments = QStringList() << "-interaction=nonstopmode";
const int CompilerStartTimeout = 10 * BCore::Second;
const int CompilerFinishTimeout = 2 * BCore::Minute;

//

//

bool readStream(QDataStream &in, QString &sample, QString &title, QString &author,
                QString &tags, QString &comment, TexSampleServer::FilePairList &list)
{
    if ( !in.device() || !in.device()->isReadable() )
        return false;
    in >> sample;
    in >> title;
    in >> author;
    in >> tags;
    in >> comment;
    TexSampleServer::FilePair fp;
    in >> fp.first;
    in >> fp.second;
    while ( !fp.first.isEmpty() && !fp.second.isEmpty() )
    {
        fp.first = fp.first.toLower();
        list << fp;
        fp.first.clear();
        fp.second.clear();
        in >> fp.first;
        in >> fp.second;
    }
    return !sample.isEmpty() && !title.isEmpty() && !author.isEmpty();
}

bool writeFiles(const QString &dir, const QString &sample, const TexSampleServer::FilePairList &list)
{
    QFile f(dir + "/" + SampleBaseName + ".tex");
    if ( !f.open(QFile::WriteOnly) )
        return false;
    QTextStream fout(&f);
    fout.setCodec("UTF-8");
    fout << SamplePrefix + sample + SamplePostfix;
    f.close();
    for (int i = 0; i < list.size(); ++i)
    {
        const TexSampleServer::FilePair &fp = list.at(i);
        QFile af(dir + "/" + fp.first);
        if ( !af.open(QFile::WriteOnly) )
            return false;
        af.write(fp.second);
        af.close();
    }
    return true;
}

bool checkSample(const QString &dir, QString &resultString, QString &logText)
{
    QString fn = dir + "/" + SampleBaseName + ".tex";
    if ( !QFile(fn).exists() )
    {
        resultString = Connection::tr("Internal error", "reply text");
        return false;
    }
    QProcess p;
    QStringList args;
    args << CompilerArguments;
    args << "\"" + fn + "\"";
    p.setWorkingDirectory(dir);
    p.start(CompilerCommand, args);
    bool b = true;
    if ( !p.waitForStarted(CompilerStartTimeout) || !p.waitForFinished(CompilerFinishTimeout) )
    {
        p.kill();
        b = false;
    }
    if (b)
    {
        QFile lf(dir + "/" + SampleBaseName + ".log");
        if ( lf.open(QFile::ReadOnly) )
        {
            QTextStream out(&lf);
            logText = out.readAll();
            lf.close();
        }
        else
        {
            QTextStream out(&p);
            logText = out.readAll();
        }
    }
    if (!b)
    {
        resultString = Connection::tr("Internal error", "reply text");
        return false;
    }
    if ( p.exitCode() )
    {
        resultString = Connection::tr("Compilation failed", "reply text");
        return false;
    }
    resultString = Connection::tr("Compilation was successful", "reply text");
    return true;
}

bool checkUser(const QString &uniqueId, const QString &login, const QString &password)
{
    QSqlDatabase *db = Connection::createDatabase(uniqueId);
    if (!db)
        return false;
    QSqlQuery *q = new QSqlQuery(*db);
    bool b = q->exec("CALL pCheckUser(\"" + login + "\", \"" + password + "\")") && q->next();
    delete q;
    Connection::removeDatabase(db);
    return b;
}

QString insertSample(const QString &uniqueId, const QString &title, const QString &author,
                     const QString &tags, const QString &comment, const QString &user, const QString &address)
{
    //TODO: Needs to be replaced by a stored procedure execution, but procedures seem not to work with Unicode
    QString id;
    QSqlDatabase *db = Connection::createDatabase(uniqueId);
    if (!db)
        return id;
    QSqlQuery *q = new QSqlQuery(*db);
    q->prepare("INSERT INTO samples (title, author, tags, comment, user, address) "
               "VALUES (:title, :author, :tags, :comment, :user, :address)");
    q->bindValue(":title", title);
    q->bindValue(":author", author);
    q->bindValue(":tags", tags);
    q->bindValue(":comment", comment);
    q->bindValue(":user", user);
    q->bindValue(":address", address);
    q->exec();
    id = q->lastInsertId().toString();
    delete q;
    Connection::removeDatabase(db);
    return id;
}

void deleteSample(const QString &uniqueId, const QString &id)
{
    if ( id.isEmpty() )
        return;
    QSqlDatabase *db = Connection::createDatabase(uniqueId);
    if (!db)
        return;
    QSqlQuery *q = new QSqlQuery(*db);
    q->exec("DELETE FROM " + TexSampleServer::DBTable + " WHERE id=\"" + id + "\"");
    delete q;
    Connection::removeDatabase(db);
}

//

void Connection::addToBanList(const QString &value, bool address)
{
    if ( value.isEmpty() )
        return;
    if (address)
    {
        mbanListAddressLock.lockForWrite();
        if ( !mbanListAddress.contains(value) )
            mbanListAddress << value;
        mbanListAddressLock.lockForWrite();
    }
    else
    {
        mbanListLoginLock.unlock();
        if ( !mbanListLogin.contains(value) )
            mbanListLogin << value;
        mbanListLoginLock.unlock();
    }
}

QStringList Connection::banList(bool address)
{
    QStringList sl;
    if (address)
    {
        mbanListAddressLock.lockForRead();
        sl = mbanListAddress;
        mbanListAddressLock.unlock();
    }
    else
    {
        mbanListLoginLock.lockForRead();
        sl = mbanListLogin;
        mbanListLoginLock.unlock();
    }
    return sl;
}

//

void Connection::requestUpdateNotify()
{
    if ( !isAuthorized() )
        return;
    QByteArray ba;
    QDataStream out(&ba, QIODevice::WriteOnly);
    out << (int) Request;
    out << TexSampleServer::UpdateNotifyOperation;
    sendData(Request, TexSampleServer::UpdateNotifyOperation, ba);
}

//

void Connection::handleSendSampleRequest(QDataStream &in, QDataStream &out)
{
    if ( !checkInOutDataStreams(in, out) )
        return;
    if ( !isAuthorized() )
    {
        out << false;
        out << tr("Not authorized", "reply text");
        return;
    }
    //Reading data
    QString sample;
    QString title;
    QString author;
    QString tags;
    QString comment;
    TexSampleServer::FilePairList fpl;
    if ( !readStream(in, sample, title, author, tags, comment, fpl) )
    {
        out << false;
        out << tr("Invalid data", "reply text");
        return;
    }
    //Creating temporary dir and writing files
    QString tdir = BCore::user("samples") + "/" + mCUniqueId;
    if ( !QDir(tdir).mkpath(tdir) || !writeFiles(tdir, sample, fpl) )
    {
        out << false;
        out << tr("Internal error", "reply text");
        BCore::removeDir(tdir);
        return;
    }
    //Checking sample
    QString resultString;
    QString logText;
    if ( !checkSample(tdir, resultString, logText) )
    {
        out << false;
        out << resultString;
        out << logText;
        BCore::removeDir(tdir);
        return;
    }
    //Executing query
    QString id = insertSample( mCUniqueId, title, author, tags, comment, mlogin, address() );
    QString dir = BCore::user("samples") + "/" + expandId(id);
    //Checking id and creating directory
    if ( !checkId(id) || !QDir(tdir).rename(tdir, dir) )
    {
        out << false;
        out << tr("Internal error", "reply text");
        deleteSample(mCUniqueId, id);
        BCore::removeDir(tdir);
        return;
    }
    //Writing reply
    out << true;
    out << logText;
    QTimer::singleShot( UpdateNotifyTimeout, this, SLOT( emitUpdateNotify() ) );
}

void Connection::handleUpdateNotifyReply(bool success, QDataStream &in)
{
    if (success)
        log( tr("Accepted update", "log text") );
    else
        log( tr("Rejected update", "log text") );
}

void Connection::handleSendVersionReply(bool success, QDataStream &in)
{
    if ( !checkInDataStream(in) )
        return;
    if (!success)
    {
        log( tr("Client rejected to send version.", "log text") );
        return;
    }
    QString beqt;
    QString plugin;
    QString editor;
    in >> beqt;
    in >> plugin;
    in >> editor;
    log("BeQt: " + beqt + ". " + tr("Plugin:", "log text") + " " + plugin + ". " +
        tr("Editor:", "log text") + " " + editor);
}

void Connection::logger(const QString &text)
{
    QString ntext = QDateTime::currentDateTime().toString("MM.dd-hh:mm:ss") + " -> ";
    ntext += address() + mCUniqueId + ": " + text + "\n";
    BStdIO::write(ntext);
}

//

void Connection::replySentSlot(const QString &operation)
{
    if (!isAuthorized() || AuthorizeOperation != operation)
        return;
    QByteArray ba;
    QDataStream out(&ba, QIODevice::WriteOnly);
    out << (int) Request;
    out << TexSampleServer::SendVersionOperation;
    sendData(Request, TexSampleServer::SendVersionOperation, ba);
}

void Connection::emitUpdateNotify()
{
    emit updateNotify();
}
*/
