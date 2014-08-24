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
