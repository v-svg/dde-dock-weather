#ifndef WEATHERCLIENT_H
#define WEATHERCLIENT_H

#include <QObject>
#include <QPointer>
#include <QDateTime>
// #include <QFile>
// #include <QStandardPaths>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QFlags>

/**
 * \brief An abstract Qt interface for openweathermap.org API
 *
 * \details This class Provides a Qt interface for managing the network to
 * openweathermap request using the provied QNetworkAccessManager pointer
 * during consctruction.
 *
 * The class relies on reference to external QNetworkAccessManager and a
 * QTextStream as a log output. Parent of the class should promise both exist
 * during the class' life time.
 *
 * The class has a network timer but timeout should be handled by its heirs.
 *
 */
class OpenWeatherClient : public QObject
{
    Q_OBJECT
public:
    OpenWeatherClient(QNetworkAccessManager &net,
                      QTextStream &logStream,
                      QObject *parent = nullptr);
    //    static QString logPath() {
    //        return QStandardPaths::standardLocations(
    //                    QStandardPaths::CacheLocation).first()
    //                + "/dock_plugin_weather.log";}

    bool ischecking() const {return checking;}
    /** Set appid (globally). Empty string to reset. */
    static void setAppid(const QString &key);
    struct CityInfo {
        int id; QString name; QString country;
        double lat; double lon; // longitude and latitude
    };
    struct Weather {
        // See https://openweathermap.org/current for more info
        QDateTime dateTime; int weatherID;
        QString weather; QString description; QString icon;
        double temp; double temp_min; double temp_max;
        double pressure; int humidity; double wind; double windDeg;
        int clouds;
    };
    enum ErrorCode {
        NoValidJson,
        JsonNoList,
        JsonBadWeather,
        NetWorkTimeOut,
        AlreadyChecking,
        NoGeoInfo
    };

signals:
    /** error() can mean timeout (NetWorkTimeOut),
     * network error (NoValidJson)
     * or jsonparse error (NoValidJson or NoJsonList) */
    void error(ErrorCode);

protected slots:
    virtual void errorHandle(ErrorCode) = 0;

protected:
    QTextStream &log;
    static QString appid;
    bool checking;
    QTimer netTimer;

    QNetworkReply *netRequest(const QString type, const QString &options);
    QByteArray netResult(QPointer<QNetworkReply> reply);

private:
    QNetworkAccessManager &netmgr;
};

/**
 * \brief A Qt interface for openweathermap.org API weather and forecast
 *
 *
 * \details
 * This class Provides a Qt interface and managing the network request
 * using the provied QNetworkAccessManager pointer when consctructing.
 * See OpenWeatherCliet for network management and logging details.
 *
 * The asynchronous I/O for network request is based on SIGNAL of
 * QNetowrkReply, so it's not necessary for multi-threading or create an
 * EventLoop. During network requirement, the class has its own timeout
 * clock, with default time 1min. error() is emitted when timeout.
 * When checkWeather() is waiting for reply, ischecking() = true
 * and update SLOT will not respond to SIGNALs. When it finishes,
 * weatherReady() and forecastReady() SIGNAL is emitted if parsing of the
 * reply is sucessful, otherwise error() is emitted.
 *
 * The request for openweathermap API is done by cityid unless it's 0,
 * but cached city and country name information is always saved.
 * The class doens't promise their consistency until checkWeather() is
 * called and weatherReady() is emitted.
 *
 * For i18n, the language is decided by QLocale::system().name() when the
 * class is constructed and supported by openweathermap API.
 *
 * \note
 * logStream and net is external and they have to be alive during the class's
 * lifetime.
 *
 */
class WeatherClient : public OpenWeatherClient
{
    Q_OBJECT
public:
    explicit WeatherClient(QNetworkAccessManager &net,
                           QTextStream &logStream,
                           int cityid=0,
                           const QString &city="",
                           const QString &country="",
                           bool ismetric=true,
                           QObject *parent = nullptr);
    virtual ~WeatherClient() override;

//    static QMap<QString, QString> WeatherDict;
//    static void setupWeatherDict();

    int cityID() const {return cityid;}
    const QString &cityName() const {return city;}
    const QString &countryName() const {return country;}
//    const QDateTime &lastUpdate() const {return last;}
//    const QString weatherNowText() const {
//        return wnow.description;
//        return wnow.weather;}
//    const QString &weatherNowDesc() const {return wnow.description;}
    const QString &weatherNowIcon() const {return wnow.icon;}
    const Weather &weatherNow() const {return wnow;}
    inline QString tempUnit() const {return isMetric? "°C" : "°F";}
    inline QString windUnit() const {return isMetric? tr("m/s") : "mi/h";}
    QString tempNow() const {
        return QString::number(wnow.temp, 'f', 1).replace(".0", "") + " " + tempUnit();}
    QString windDir() const {
    QStringList labels({tr("N"), tr("NE"), tr("E"), tr("SE"),
                        tr("S"), tr("SW"), tr("W"), tr("NW")});
        return labels[(qRound(wnow.windDeg + 180.0 / labels.size()) *
                        labels.size() / 360) % labels.size()];
    }
    QString tipNow() const {
        return QString("%1, %2\n%3\n%4\n").arg(city).arg(
                    country).arg(tempNow()).arg(wnow.description.at(0).toUpper() + wnow.description.mid(1)) +
                tr("Humidity %1%\n").arg(wnow.humidity) +
                tr("Clouds %1%\n").arg(wnow.clouds) +
                tr("Wind %1 %2, %3\n").arg(wnow.wind).arg(windUnit()).arg(windDir()) +
                tr("Sunrise at %1\n").arg(sunrise.toString("HH:mm")) +
                tr("Sunset at %1\n").arg(sunset.toString("HH:mm")) +
                tr("Updated at %1").arg(last.toString("HH:mm"));
    }
    const QVector<Weather> &getForecast() const {return forecasts;}

    /** set unit and update cached weather info */
    void setMetric(bool is);
    /** set city and update cached weather */
    void setCity(const QString &icity, const QString &icountry) {
        cityid = 0; city = icity; country = icountry;}
    void setCity(int id) {cityid = id;}
    void setCity(const CityInfo &info) {
        cityid = info.id; city = info.name; country = info.country;}
    void setLang(const QString &l) {lang = l;}

signals:
    /** Asynchronous signal after calling checkWeather(), with weatherNow ready */
    void weatherReady();
    /** Asynchronous signal after calling checkWeather(), with forecasts ready */
    void forecastReady();
    /** Asynchronous signal after calling checkWeather(), with both weatherNow and forecasts ready */
    void allReady();
    /** changed() means sth. other than weatherReady or forwcastReady, e.g. unit */
    void changed();

public slots:
    /** Start a request for update forecast information,
     * and according to #cityid, update #city and #country. */
    void checkWeather(int timeout);
    void checkWeather() {return checkWeather(60000);}  //Overload 1min for timeout
    void parseWeather();
    void parseForecast();
    void errorHandle(ErrorCode) override;

private:
    QPointer<QNetworkReply> weatherReply;
    QPointer<QNetworkReply> forecastReply;
    int cityid;
    QString city;
    QString country;
    bool isMetric; // imperial or metric
    QString lang;
    QDateTime sunrise;
    QDateTime sunset;
    QDateTime last;
    QVector<Weather> forecasts;
    Weather wnow;
    enum Status {
        NoneDone = 0x0000,
        WeatherDone = 0x0001,
        ForecastDone = 0x0002,
        AllDone = WeatherDone | ForecastDone
    } ;
    QFlags<Status> status;

};

/**
 * \brief A Qt interface for openweathermap.org API find
 *
 *
 * \details
 * This class Provides a Qt interface and managing the network request
 * using the provied QNetworkAccessManager pointer when consctructing.
 * See OpenWeatherCliet for network management and logging details.
 *
 * Same as WeatherClient the asynchronous I/O for network request is based
 * on SIGNAL of QNetowrkReply. When it finishes, foundCity(const QList<CityInfo> &)
 * SIGNAL is emitted if parsing of the reply is sucessful, otherwise error() is
 * emitted.
 *
 * \note
 * logStream and net is external and they have to be alive during the class's
 * lifetime.
 *
 */
class CityLookup : public OpenWeatherClient
{
    Q_OBJECT
public:
    explicit CityLookup(QNetworkAccessManager &net,
                        QTextStream &logStream,
                        QObject *parent = nullptr):
        OpenWeatherClient(net, logStream, parent) {}
    virtual ~CityLookup() override;

signals:
    /** Asynchronous result signal after calling lookForCity() */
    void foundCity(const QList<CityInfo> &);

public slots:
    void lookForCity(const QString &city, const QString &country, int timeout = 60000) ;

private slots:
    void parseCityInfo();
    void errorHandle(ErrorCode) override;

private:
    QPointer<QNetworkReply> netReply;
};

#endif // WEATHERCLIENT_H
