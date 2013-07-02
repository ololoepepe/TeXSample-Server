#ifndef TRANSLATOR_H
#define TRANSLATOR_H

class QString;
class QTranslator;

#include <QLocale>
#include <QString>
#include <QList>

/*============================================================================
================================ Translator ==================================
============================================================================*/

class Translator
{
public:
    explicit Translator(const QLocale &locale);
    ~Translator();
public:
    QLocale locale() const;
    QString translate(const char *context, const char *sourceText, const char *disambiguation = 0, int n = -1) const;
private:
    const QLocale mLocale;
    QList<QTranslator *> mtranslators;
private:
    Q_DISABLE_COPY(Translator)
};

#endif // TRANSLATOR_H
