#include "translator.h"

#include <BCoreApplication>

#include <QLocale>
#include <QString>
#include <QList>
#include <QTranslator>
#include <QString>
#include <QCoreApplication>
#include <QStringList>
#include <QDir>

/*============================================================================
================================ Translator ==================================
============================================================================*/

/*============================== Public constructors =======================*/

Translator::Translator(const QLocale &locale)
{
    setLocale(locale);
}

Translator::~Translator()
{
    foreach (QTranslator *t, mtranslators)
        delete t;
}

/*============================== Public methods ============================*/

void Translator::setLocale(const QLocale &locale)
{
    if (locale == mlocale)
        return;
    foreach (QTranslator *t, mtranslators)
        delete t;
    mtranslators.clear();
    mlocale = locale;
    QStringList dirs = BCoreApplication::locations(BCoreApplication::TranslationsPath);
    foreach (int i, bRangeR(dirs.size() - 1, 0)) //User translators come last, having higher priority
    {
        QTranslator *t = new QTranslator;
        if (t->load(mlocale, "texsample-server", "_", dirs.at(i), ".qm"))
            mtranslators << t;
        else
            delete t;
    }
}

QLocale Translator::locale() const
{
    return mlocale;
}

QString Translator::translate(const char *context, const char *sourceText, const char *disambiguation, int n) const
{
    foreach (QTranslator *t, mtranslators)
    {
        QString s = t->translate(context, sourceText, disambiguation, n);
        if (!s.isEmpty())
            return s;
    }
    return sourceText;
}
