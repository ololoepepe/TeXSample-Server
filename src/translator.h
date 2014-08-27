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

#ifndef TRANSLATOR_H
#define TRANSLATOR_H

class QString;

class BTranslator;

#include <QLocale>

/*============================================================================
================================ Translator ==================================
============================================================================*/

class Translator
{
private:
    QLocale mlocale;
    BTranslator *mtranslator;
public:
    explicit Translator();
    explicit Translator(const QLocale &locale);
    Translator(const Translator &other);
public:
    QLocale locale() const;
    QString translate(const char *context, const char *sourceText, const char *disambiguation = 0, int n = -1) const;
public:
    Translator &operator =(const Translator &other);
    QString operator ()(const char *context, const char *sourceText, const char *disambiguation = 0, int n = -1) const;
private:
    static BTranslator *translator(const QLocale &locale);
};

#endif // TRANSLATOR_H
