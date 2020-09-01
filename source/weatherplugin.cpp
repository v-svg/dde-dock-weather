#include "weatherplugin.h"
#include <QDesktopServices>
#include <QApplication>
#include "weathersettingdialog.h"

QTranslator WeatherPlugin::qtTranslator;
QTranslator *WeatherPlugin::loadTranslator(const QLocale &locale, QObject *parent) {
    // This is a dirty trick to load translator before init of config
    QTranslator* ts = new QTranslator(parent);
#ifdef QT_DEBUG
    qDebug() << "---- Weather Plugin: Load Language: " << locale.name();
#endif
    ts->load(locale, "weather", "-", ":/i18n", ".qm");
    qtTranslator.load(locale, "qtbase", "_",
                      QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    QApplication::instance()->installTranslator(ts);
    QApplication::instance()->installTranslator(&qtTranslator);
    return ts;
}

WeatherPlugin::WeatherPlugin (QObject *parent):
    QObject (parent), m_client(nullptr), m_items(nullptr),
    m_popups(nullptr), m_tips(nullptr), netmgr(this),
    logFile(logPath()), m_refershTimer(this),
    translator(loadTranslator(QLocale::system(), this))
{
}

WeatherPlugin::~WeatherPlugin() {
    log.flush();
    logFile.close();
    delete m_popups;
    delete m_tips;
    delete m_items;
    delete m_client;
    QApplication::instance()->removeTranslator(translator);
    QApplication::instance()->removeTranslator(&qtTranslator);
    delete translator;
}

void WeatherPlugin::reloadLang() {
    QString lang = m_proxyInter->getValue(this, LANG_KEY, "").toString();
    if (lang != "") {
        QApplication::instance()->removeTranslator(translator);
        translator->load(lang, "weather", "-", ":/i18n", ".qm");
        QApplication::instance()->installTranslator(translator);
        m_client->setLang(lang);
    }
}

void WeatherPlugin::init(PluginProxyInterface *proxyInter) {
    this->m_proxyInter = proxyInter;
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        qDebug() << "Cannot open log file";
    log.setDevice(&logFile);
    log << "Start!" << endl;
    m_client = new WeatherClient(
                netmgr, log,
                m_proxyInter->getValue(this, CITYID_KEY, 0).toInt(),
                m_proxyInter->getValue(this, CITY_KEY, "").toString(),
                m_proxyInter->getValue(this, COUNTRY_KEY, "").toString(),
                m_proxyInter->getValue(this, UNIT_KEY, true).toBool());
    reloadLang();
    QString appid =  m_proxyInter->getValue(this, APPID_KEY, "").toString();
    if (appid != "") {
        OpenWeatherClient::setAppid(appid);
    }
    m_popups = new ForecastApplet(m_client, m_proxyInter->getValue(this, THEME_KEY,
                                      themeSet[0]).toString());
    m_items = new WeatherItem(m_client, m_popups);
    m_tips = new QLabel(tr(" Updating... "));
    m_tips->setStyleSheet("color:white; padding:0px 2px;");
    m_refershTimer.setInterval(MINUTE * m_proxyInter->getValue(
                                   this, CHK_INTERVAL_KEY, DEFAULT_INTERVAL
                                   ).toInt());
    connect(m_client, &WeatherClient::weatherReady,
            this, &WeatherPlugin::refreshTips);
// TODO: how to handle error result?
    connect(m_client, &WeatherClient::error,
            [this](){this->m_refershTimer.start(MINUTE/2);}); // If Error, try every 30s
    connect(this, &WeatherPlugin::checkUpdate,
            &m_refershTimer, QOverload<>::of(&QTimer::start)); // restart timer
    connect(this, &WeatherPlugin::checkUpdate,
            m_client, QOverload<>::of(&WeatherClient::checkWeather));
    connect(m_items, &WeatherItem::requestUpdateGeometry, [this] {
            m_proxyInter->itemUpdate(this, pluginName());
    });

    if(!pluginIsDisable()) {
        this->m_proxyInter->itemAdded(this, WEATHER_KEY);
        connect(&m_refershTimer, &QTimer::timeout,
                m_client, QOverload<>::of(&WeatherClient::checkWeather));
        emit checkUpdate();
    }
}

void WeatherPlugin::reloadSettings() {
    m_popups->setTheme(m_proxyInter->getValue(this, THEME_KEY,
                           themeSet[0]).toString());
    m_client->setMetric(m_proxyInter->getValue(this, UNIT_KEY, true).toBool());
    QString appid = m_proxyInter->getValue(this, APPID_KEY, "").toString();
    m_client->setAppid(appid);
    m_refershTimer.setInterval(MINUTE * m_proxyInter->getValue(
                                   this, CHK_INTERVAL_KEY, DEFAULT_INTERVAL
                                   ).toInt());
    QString city = m_proxyInter->getValue(this, CITY_KEY, "").toString();
    QString country = m_proxyInter->getValue(this, COUNTRY_KEY, "").toString();
    int cityid = m_proxyInter->getValue(this, CITYID_KEY, 0).toInt();
    if (city != m_client->cityName() || country != m_client->countryName()
            || cityid != m_client->cityID()) {
        m_client->setCity(city, country);
        m_client->setCity(cityid);
        emit checkUpdate();
    }
}

void WeatherPlugin::pluginStateSwitched() {
    m_proxyInter->saveValue(this, STATE_KEY, pluginIsDisable());
    if (pluginIsDisable()) {
        m_refershTimer.stop();
        m_proxyInter->itemRemoved(this, WEATHER_KEY);
    }
    else {
        m_refershTimer.start();
        m_proxyInter->itemAdded(this, WEATHER_KEY);
        m_client->checkWeather();
    }
}

QWidget *WeatherPlugin::itemWidget(const QString &itemKey) {
    if (itemKey == WEATHER_KEY) {
        return m_items;
    }
    return nullptr;
}

QWidget *WeatherPlugin::itemTipsWidget(const QString &itemKey) {
    if (itemKey == WEATHER_KEY) {
        return m_tips;
    }
    return nullptr;
}

QWidget *WeatherPlugin::itemPopupApplet(const QString &itemKey) {
    if (itemKey == WEATHER_KEY) {
        return m_popups;
    }
    return nullptr;
}

int WeatherPlugin::itemSortKey(const QString &itemKey) {
    const QString key = QString("pos_%1_%2").arg(itemKey).arg(displayMode());
    return m_proxyInter->getValue(this, key,
            displayMode() == Dock::DisplayMode::Fashion ? 2 : 3).toInt();
}

void WeatherPlugin::setSortKey(const QString &itemKey, const int order) {
    const QString key = QString("pos_%1_%2").arg(itemKey).arg(displayMode());
    m_proxyInter->saveValue(this, key, order);
}

void WeatherPlugin::refreshIcon(const QString &itemKey) {
    if (itemKey == WEATHER_KEY) {
        m_items->refreshIcon();
    }
}

void WeatherPlugin::refreshTips() {
    m_tips->setText(m_client->tipNow());
}

const QString WeatherPlugin::itemContextMenu(const QString &itemKey) {
    if (itemKey == WEATHER_KEY) {
        QList<QVariant> items;
        items.reserve(2);

        QMap<QString, QVariant> refresh;
        refresh["itemId"] = REFRESH;
        refresh["itemText"] = tr("Refresh");
        refresh["isActive"] = true;
        items.push_back(refresh);

        QMap<QString, QVariant> set;
        set["itemId"] = SETTINGS;
        set["itemText"] = tr("Settings");
        set["isActive"] = true;
        items.push_back(set);

        QMap<QString, QVariant> menu;
        menu["items"] = items;
        menu["checkableMenu"] = false;
        menu["singleCheck"] = false;

        return QJsonDocument::fromVariant(menu).toJson();
    }
    return QString();
}

void WeatherPlugin::invokedMenuItem(const QString &itemKey,
                                    const QString &menuId,
                                    const bool checked)
{
    Q_UNUSED(checked)
    if (itemKey == WEATHER_KEY) {
        if (menuId == REFRESH) {
            emit checkUpdate();
        }
        else if (menuId == SETTINGS) {
            WeatherSettingDialog* setDialog = new WeatherSettingDialog(
                        m_proxyInter, this, netmgr, log);
            setDialog->setAttribute(Qt::WA_DeleteOnClose);
            connect(setDialog, &WeatherSettingDialog::accepted,
                    this, &WeatherPlugin::reloadSettings);
            setDialog->show();
        }

    }

}
