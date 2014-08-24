#include "translator.h"

#include "application.h"

#include <BTranslator>

#include <QDebug>
#include <QList>
#include <QLocale>
#include <QMap>
#include <QString>

/*============================================================================
================================ Translator ==================================
============================================================================*/

/*============================== Public constructors =======================*/

Translator::Translator()
{
    mlocale = QLocale("en");
    mtranslator = 0;
}

Translator::Translator(const QLocale &locale)
{
    mlocale = locale;
    mtranslator = translator(locale);
}

Translator::Translator(const Translator &other)
{
    *this = other;
}

/*============================== Public methods ============================*/

QLocale Translator::locale() const
{
    return mlocale;
}

QString Translator::translate(const char *context, const char *sourceText, const char *disambiguation, int n) const
{
    if (mtranslator)
        return mtranslator->translate(context, sourceText, disambiguation, n);
    else
        return QString::fromLatin1(sourceText);
}

/*============================== Public operators ==========================*/

Translator &Translator::operator =(const Translator &other)
{
    mlocale = other.mlocale;
    mtranslator = other.mtranslator;
    return *this;
}

QString Translator::operator ()(const char *context, const char *sourceText, const char *disambiguation, int n) const
{
    return translate(context, sourceText, disambiguation, n);
}

/*============================== Static private methods ====================*/

BTranslator *Translator::translator(const QLocale &locale)
{
    typedef QMap<QString, BTranslator *> TranslatorMap;
    init_once(TranslatorMap, translators, TranslatorMap()) {
        BTranslator tt("texsample-server");
        foreach (const QLocale &l, tt.availableLocales()) {
            BTranslator *tt = new BTranslator(l, "texsample-server");
            if (!tt->load() || !tt->isValid()) {
                delete tt;
                continue;
            }
            translators.insert(l.name(), tt);
        }
    }
    return translators.value(locale.name());
}
