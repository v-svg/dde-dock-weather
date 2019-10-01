QT              += widgets svg network gui
TEMPLATE         = lib
CONFIG          += plugin c++11 lrelease embed_translations link_pkgconfig
PKGCONFIG       += dtkwidget

TARGET           = $$qtLibraryTarget(weather)
DESTDIR          = $$_PRO_FILE_PWD_
DISTFILES       += \
    weather.json

HEADERS += \
    weatherplugin.h \
    weatheritem.h \
    weatherclient.h \
    forecastapplet.h \
    weathersettingdialog.h \
    countrycode.h \
    constants.h

SOURCES += \
    weatherplugin.cpp \
    weatheritem.cpp \
    weatherclient.cpp \
    forecastapplet.cpp \
    weathersettingdialog.cpp \
    countrycode.cpp

isEmpty(PREFIX) {
    PREFIX = /usr
}

target.path = $${PREFIX}/lib/dde-dock/plugins/
INSTALLS += target

RESOURCES += res.qrc
lessThan(QT_MAJOR_VERSION, 5.8): RESOURCES += i18n.qrc
lessThan(QT_MAJOR_VERSION, 5.8): !system($$PWD/genqm.sh): error("Failed to generate translation")


TRANSLATIONS = \
    $$_PRO_FILE_PWD_/i18n/weather-es.ts \
    $$_PRO_FILE_PWD_/i18n/weather-pt_BR.ts \
    $$_PRO_FILE_PWD_/i18n/weather-ru.ts \
    $$_PRO_FILE_PWD_/i18n/weather-tr.ts \
    $$_PRO_FILE_PWD_/i18n/weather-uk.ts \
    $$_PRO_FILE_PWD_/i18n/weather-zh_CN.ts
