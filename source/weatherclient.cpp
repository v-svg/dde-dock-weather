#include "weatherclient.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QTimeZone>

#define LOGTIMEFORMAT "yyyy/MM/dd HH:mm:ss"
#define DEFAULTAPPID "5cdf0b925cb0a95f1151ec3c2f3be33b"

/* OpenWeatherClient members */

QString OpenWeatherClient::appid = DEFAULTAPPID;
void OpenWeatherClient::setAppid(const QString &key) {
    if (key != "")
        appid = key;
    else
        appid = DEFAULTAPPID;
}
OpenWeatherClient::OpenWeatherClient(QNetworkAccessManager &net,
                                     QTextStream &logStream,
                                     QObject *parent):
    QObject (parent), log(logStream), checking(false), netmgr(net)
{
        netTimer.setSingleShot(true);
        connect(&netTimer, &QTimer::timeout, [this](){
            this->errorHandle(NetWorkTimeOut);
        });
        connect(this, SIGNAL(error(ErrorCode)),
                this, SLOT(errorHandle(ErrorCode)));
}

QNetworkReply *OpenWeatherClient::netRequest(const QString type,
                                             const QString &options)
{
    QString url = QString(
                "https://api.openweathermap.org/data/2.5/%1?%2&appid=%3"
                ).arg(type).arg(options).arg(appid);
    log << "Connect: " << url << endl;
    return netmgr.get(QNetworkRequest(QUrl(url)));
}

inline void cleanNetworkReply(QPointer<QNetworkReply> reply) {
    // Discard unfinished network requirement
    if (reply != nullptr) {
        reply->disconnect();
        reply->close();
        reply->deleteLater();
    }
}
#define LOGMAX 144
QByteArray OpenWeatherClient::netResult(QPointer<QNetworkReply> reply) {
    netTimer.stop();
    QByteArray result = reply->readAll();
    cleanNetworkReply(reply);
    if (result.length() > LOGMAX) {
        log << QString("Get reply: ...") <<
                (result.constData() + result.length() - LOGMAX) << endl;
    }
    else {
        log << QString("Get reply: ") << result << endl;
    }
    return result;
}

WeatherClient::WeatherClient(QNetworkAccessManager &net,
                             QTextStream &logStream,
                             int icityid,
                             const QString &icity,
                             const QString &icountry,
                             bool ismetric, QObject *parent) :
    OpenWeatherClient(net, logStream, parent),
    weatherReply(nullptr), forecastReply(nullptr),
    cityid(icityid), city(icity), country(icountry),
    isMetric(ismetric), lang(QLocale::system().name()),
    last(QDateTime::fromTime_t(0)),
    wnow(Weather({last, 800, "na", "na", "na", 0, 0, 0, 0, 0, 0, 0, 0})),
    status(NoneDone)
{
    forecasts.append(wnow);
    log << "Weather Client inited.. cityid: " << cityid <<
           " country: " << country << " city: " << city << endl;
}

WeatherClient::~WeatherClient() {
    cleanNetworkReply(forecastReply);
    cleanNetworkReply(weatherReply);
}

void WeatherClient::checkWeather(int timeout) {
    if (checking) {
        emit error(AlreadyChecking);
        return;
    }
    else if (cityid == 0 && city == "") {
        emit error(NoGeoInfo);
        return;
    }
    last = QDateTime::currentDateTime();
    log << "Check update: " << last.toString(LOGTIMEFORMAT) << endl;
    checking = true;
    status = NoneDone;
    QString options = QString("%1&lang=%2&units=%3").arg(
                cityid == 0 ?
                    QString("q=%1,%2").arg(city).arg(country) :
                    QString("id=%1").arg(cityid)
                    ).arg(lang).arg(isMetric? "metric" : "imperial");

    cleanNetworkReply(weatherReply);
    weatherReply = netRequest("weather", options);
    connect(weatherReply, &QNetworkReply::finished,
            this, &WeatherClient::parseWeather);

    cleanNetworkReply(forecastReply);
    forecastReply = netRequest("forecast", options);
    connect(forecastReply, &QNetworkReply::finished,
            this, &WeatherClient::parseForecast);


    netTimer.start(timeout);
}

void WeatherClient::errorHandle(ErrorCode e){
    switch (e) {
    case AlreadyChecking:
        return;
    case NoGeoInfo:
        log << "Geo information is not set!" << endl;
        return;
    case NetWorkTimeOut:
        log << "Network requirement time out!" << endl;
        break;
    case NoValidJson:
        break;
    case JsonNoList:
        log << "Forecast parser cannot find \"list\" in json reply" << endl;
        break;
    case JsonBadWeather:
        log << "Weather parser get bad json reply" << endl;
    }
    checking = false;
    cleanNetworkReply(forecastReply);
    cleanNetworkReply(weatherReply);
}

inline OpenWeatherClient::Weather jWeatherParser(const QJsonObject &w) {
    // See https://openweathermap.org/current for more info
    return OpenWeatherClient::Weather(
    {QDateTime::fromTime_t(w.value("dt").toInt()),        // datetime

     w.value("weather").toArray().at(0).toObject().value("id").toInt(),             // weather id
     w.value("weather").toArray().at(0).toObject().value("main").toString(),        // weather
     w.value("weather").toArray().at(0).toObject().value("description").toString(), // description
     w.value("weather").toArray().at(0).toObject().value("icon").toString(),        // icon
     w.value("main").toObject().value("temp").toDouble(),
     w.value("main").toObject().value("temp_min").toDouble(),
     w.value("main").toObject().value("temp_max").toDouble(),
     w.value("main").toObject().value("pressure").toDouble(),
     w.value("main").toObject().value("humidity").toInt(),
     w.value("wind").toObject().value("speed").toDouble(),
     w.value("wind").toObject().value("deg").toDouble(),
     w.value("clouds").toObject().value("all").toInt()});
}
void WeatherClient::parseWeather() {
    QByteArray result = netResult(weatherReply);
    QJsonParseError JPE;
    QJsonDocument JD = QJsonDocument::fromJson(result, &JPE);
    if (JPE.error != QJsonParseError::NoError) {
        log << "Weather json Parse Error" << JPE.error << endl;
        emit error(NoValidJson);
        return;
    } else {
        const QJsonObject o = JD.object();
        if (o.value("cod") == QJsonValue::Undefined ||
                o.value("cod").toInt() != 200) {
            // Bad return
            emit error(JsonBadWeather);
            return;
        }
        if (city == "") {
            city = o.value("name").toString();
            country = o.value("sys").toObject().value("country").toString();
        }
        if (cityid == 0)
            cityid = o.value("id").toInt();
        // sunrise and sunset
        sunrise = QDateTime::fromTime_t(o.value("sys").toObject().value("sunrise").toInt());
        sunset = QDateTime::fromTime_t(o.value("sys").toObject().value("sunset").toInt());

        log << QString("Parsing weather for %1, %2 (id: %3)"
                       ).arg(city).arg(country).arg(cityid) << endl;
        wnow = jWeatherParser(o);
        log << QString("\tDateTime: %1\n"
                       "\tWeather: %2, %3, icon %4\n"
                       "\tTemperature: %5").arg(
                   wnow.dateTime.toString(LOGTIMEFORMAT)).arg(
                   wnow.weather).arg(wnow.description).arg(
                   wnow.icon).arg(wnow.temp) << endl;
    }
    status |= WeatherDone;
    if (status == AllDone) {
        log << "Checking all done! Weather info arrived later" << endl;
        checking = false;
        emit forecastReady();
        emit allReady();
        return;
    }
    emit weatherReady();
}

void WeatherClient::parseForecast() {
    QByteArray result = netResult(forecastReply);
    QJsonParseError JPE;
    QJsonDocument JD = QJsonDocument::fromJson(result, &JPE);
    if (JPE.error != QJsonParseError::NoError) {
        log << "Forecast json Parse Error" << JPE.error << endl;
        emit error(NoValidJson);
        return;
    } else {
        const QJsonValue obj = JD.object().value("list");
        if (obj == QJsonValue::Undefined) {
            emit error(JsonNoList);
            return;
        }

        log << QString("Parsing forecast for %1, %2 (id: %3)"
                       ).arg(city).arg(country).arg(cityid) << endl;

        if (city == "") {
            const auto o = JD.object().value("city").toObject();
            city = o.value("name").toString();
            country = o.value("country").toString();
        }
        QJsonArray list = obj.toArray();
        forecasts.clear();
        for (auto item: list) {
            forecasts.append(jWeatherParser(item.toObject()));
        }
        const auto &o = forecasts.first();
        log << QString("Total %1 forcasts, first being:\n"
                       "\tDateTime: %2\n"
                       "\tWeather: %3, %4, icon %5\n"
                       "\tTemperature: %6").arg(
                   forecasts.count()).arg(
                   o.dateTime.toString(LOGTIMEFORMAT)).arg(
                   o.weather).arg(o.description).arg(
                   o.icon).arg(o.temp) << endl;
    }
    status |= ForecastDone;
    if (status == AllDone) {
        log << "Checking all done! Forecast info arrived later" << endl;
        checking = false;
        emit forecastReady();
        emit allReady();
        return;
    }
    emit forecastReady();
}

inline void unitTransform (bool isMetric, OpenWeatherClient::Weather &item) {
    if (isMetric) {
        item.temp = (item.temp - 32)/1.8;
        item.temp_max = (item.temp_max - 32)/1.8;
        item.temp_min = (item.temp_min - 32)/1.8;
        item.wind /= 2.23693629;
    }
    else {
        item.temp = item.temp * 1.8 + 32;
        item.temp_max = item.temp_max * 1.8 + 32;
        item.temp_min = item.temp_min * 1.8 + 32;
        item.wind *= 2.23693629;
    }
}
void WeatherClient::setMetric(bool is) {
    if(is == isMetric) {
        return;
    }
    isMetric = is;
    unitTransform(isMetric, wnow);
    for (auto &item: forecasts) {
        unitTransform(isMetric, item);
    }
    emit changed();
}



/* CityLookup members */

CityLookup::~CityLookup() {
    cleanNetworkReply(netReply);
}

void CityLookup::lookForCity(const QString &city, const QString &country,
                             int timeout){
    if (country == "")
        return;
    log << QString("Look for city: %1, %2 ").arg(city).arg(country);
    log << QDateTime::currentDateTime().toString(LOGTIMEFORMAT) << endl;
    cleanNetworkReply(netReply);
    netReply = netRequest("find", QString("q=%1").arg(city) +
                          country=="" ? "" : ","+country);
    connect(netReply, &QNetworkReply::finished,
            this, &CityLookup::parseCityInfo);
    netTimer.start(timeout);
    return;
}

void CityLookup::parseCityInfo() {
    QByteArray result = netResult(netReply);
    QJsonParseError JPE;
    QJsonDocument JD = QJsonDocument::fromJson(result, &JPE);
    if (JPE.error != QJsonParseError::NoError) {
        log << "CityInfo json Parse Error" << JPE.error << endl;
        emit error(NoValidJson);
    } else {
        QJsonValue obj = JD.object().value("list");
        if (obj == QJsonValue::Undefined) {
            emit error(JsonNoList);
            return;
        }

        QJsonArray list = obj.toArray();
        QList<CityInfo> cityList;
        for (const auto item: list) {
            auto o = item.toObject();
            cityList.append(CityInfo{
                                o.value("id").toInt(), o.value("name").toString(),
                                o.value("sys").toObject().value("country").toString(),
                                o.value("coord").toObject().value("lat").toDouble(),
                                o.value("coord").toObject().value("lon").toDouble()
                            });
        }
        emit foundCity(cityList);
    }
    return;
}

void CityLookup::errorHandle(ErrorCode e){
    switch (e) {
    case AlreadyChecking:
        return;
    case NoGeoInfo:
        log << "Shouldn't be here!" << endl;
        return;
    case NetWorkTimeOut:
        log << "Network requirement time out!" << endl;
        break;
    case NoValidJson:
        break;
    case JsonNoList:
        log << "CityInfo cannot find \"list\" in json reply" << endl;
        break;
    default:
        log << "Should not be here" << endl;
    }
    checking = false;
    cleanNetworkReply(netReply);
}
