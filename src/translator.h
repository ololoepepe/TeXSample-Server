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
    explicit Translator(const QLocale &locale = QLocale("en"));
    ~Translator();
public:
    void setLocale(const QLocale &locale);
    QLocale locale() const;
    QString translate(const char *context, const char *sourceText, const char *disambiguation = 0, int n = -1) const;
private:
    QLocale mlocale;
    QList<QTranslator *> mtranslators;
private:
    Q_DISABLE_COPY(Translator)
};

#endif // TRANSLATOR_H
