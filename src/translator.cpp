/****************************************************************************
**
** Copyright (C) 2012-2014 Andrey Bogdanov
**
** This file is part of TeXSample Server.
**
** TeXSample Server is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** TeXSample Server is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with TeXSample Server.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

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
