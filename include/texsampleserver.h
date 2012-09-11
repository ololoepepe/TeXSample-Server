#ifndef TEXSAMPLESERVER_H
#define TEXSAMPLESERVER_H

#include <QString>
#include <QDataStream>
#include <QRegExp>
#include <QPair>
#include <QByteArray>
#include <QList>

namespace TexSampleServer
{

typedef QPair<QString, QByteArray> FilePair;
typedef QList<FilePair> FilePairList;
//
const QString DefaultHostName = "90.157.27.188";
const quint16 ServerPort = 9022;
const int DBPort = 3306;
const QString DBType = "QMYSQL";
const QString DBName = "texsample";
const QString DBTable = "samples";
//
enum SampleType
{
    Unverified = 0,
    Approved = 1
};
enum TableTexSampleColumn
{
    IdColumn = 0, //BIGINT(20) (Primary, Not NULL, Auto Increment, UNSIGNED, ZEROFILL)
    TypeColumn = 1, //TINYINT(3) (Not NULL, UNSIGNED) [SampleType: 0-2]
    TitleColumn = 2, //TEXT (Not NULL, Character Set: UTF-8)
    AuthorColumn = 3, //TEXT (Not NULL, Character Set: UTF-8)
    TagsColumn = 4, //TEXT (Not NULL, Character Set: UTF-8)
    CommentColumn = 5, //TEXT (Not NULL, Character Set: UTF-8)
    RatingColumn = 6, //TINYINT(3) (Not NULL, UNSIGNED) [0-100]
    UserColumn = 7, //TINYTEXT (Not NULL, Character set: UTF-8)
    AddressColumn = 8 //TINYTEXT (Not NULL, Character set: UTF-8)
};
const QRegExp TagSeparator = QRegExp("\\,\\s*");
//
const QString SendVersionOperation = "send version";
const QString GetSampleOperation = "get sample";
const QString GetPdfOperation = "get pdf";
const QString SendSampleOperation = "send sample";
const QString UpdateNotifyOperation = "update samples";
//
const QDataStream::Version DataStreamVersion = QDataStream::Qt_4_7;

}

#endif // TEXSAMPLESERVER_H
