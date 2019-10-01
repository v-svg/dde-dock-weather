#ifndef THEMESET_H
#define THEMESET_H
#include <QStringList>
#include <QMap>

/** \brief themeSet is a list describing the possible choices of icon themes.
 * The first String is the default choice.
 * The theme names should be consistent with the corresponding definition of
 * the prefix in `res.qrc` file.
 * In the `res.qrc` file it's expected to set alias of the icon codes:
 * `01d`, `01n`, ..., `04n`, `09d`, ..., `13d`, `13n`, `50d`, `50n`, `na`
 */
const QStringList themeSet({"Color", "Flat", "Pure", "Simple", "Sticker", "White"});

/** \brief langSet is a list describing the possible choices of languages.
 * The first String is the default choice (system Language).
 */
const QMap<QString, QString> langSet = {{"", ""},
                                        {"Español", "es"},
                                        {"Português do Brasil", "pt_BR"},                                        
                                        {"Русский", "ru"},
                                        {"Türkçe", "tr"},
                                        {"Українська", "uk"},
                                        {"简体中文", "zh_CN"}};

#endif // THEMESET_H
