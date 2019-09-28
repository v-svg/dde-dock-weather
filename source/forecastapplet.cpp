#include "forecastapplet.h"
#include <QApplication> // provide qApp
#include <QPainter>

#define PRIMARYICONSIZE 74
#define DATEFORMAT tr(" MM/dd \nddd")

ForecastApplet::ForecastApplet(const WeatherClient *wcli,
                             QString thm, QWidget *parent)
    : QWidget(parent), client(wcli), themeName(thm)
{
    setFixedWidth(298);
    setFixedHeight(440);
    QDate date = date.currentDate();
    QString dateString = date.toString(DATEFORMAT);
    QString styleSheet = QString("font-weight: bold; font-size: 20px; margin: 0px 0px 20px 0px;");
    WImgNow = new QLabel;
    WImgNow->setPixmap(loadWIconNow("na", PRIMARYICONSIZE));
    WImgNow->setAlignment(Qt::AlignCenter);
    WImgNow->setStyleSheet("margin: 0px 0px 20px 0px;");
    WImgNow->setFixedSize(70, 95);
    defaultLayout.addWidget(WImgNow, 0, 0);
    tempNow = new QLabel("-25 ~ 25 °C\nClear");
    tempNow->setAlignment(Qt::AlignCenter);
    tempNow->setStyleSheet(styleSheet);
    tempNow->setWordWrap(true);
    tempNow->setFixedHeight(95);
    defaultLayout.addWidget(tempNow, 0, 1);
    dateNow = new QLabel(dateString);
    dateNow->setAlignment(Qt::AlignCenter);
    dateNow->setStyleSheet(styleSheet);
    dateNow->setMaximumWidth(70);
    dateNow->setFixedHeight(95);
    defaultLayout.addWidget(dateNow, 0, 2);

    const QDate today = QDate::currentDate();
    for (int i=0; i<MAXDAYS; i++) {
        fcstLabels[i].WImg = new QLabel;
        fcstLabels[i].WImg->setPixmap(loadWIcon());
        fcstLabels[i].WImg->setAlignment(Qt::AlignCenter);
        defaultLayout.addWidget(fcstLabels[i].WImg, i+1, 0);
        fcstLabels[i].Temp = new QLabel("-25 ~ 25 °C\nClear");
        fcstLabels[i].Temp->setAlignment(Qt::AlignCenter);
        fcstLabels[i].Temp->setWordWrap(true);
        defaultLayout.addWidget(fcstLabels[i].Temp, i+1, 1);
        fcstLabels[i].Date = new QLabel(
                    today.addDays(i+1).toString(DATEFORMAT));
        fcstLabels[i].Date->setAlignment(Qt::AlignCenter);
        defaultLayout.addWidget(fcstLabels[i].Date, i+1, 2);
    }

    setLayout(&defaultLayout);
    connect(client, &WeatherClient::allReady,
            this, &ForecastApplet::reloadForecast);
    connect(client, &WeatherClient::changed,
            this, &ForecastApplet::reloadForecast);
    connect(client, &OpenWeatherClient::error,
            this, &ForecastApplet::updateError);
}

ForecastApplet::~ForecastApplet() {
}

void ForecastApplet::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 0.12 * 255));
    painter.drawRect(0, 97, 298, 1);
}

QPixmap ForecastApplet::loadWIconNow(const QString &name, int size) const {
    const auto ratio = qApp->devicePixelRatio();
    const int iconSize = static_cast<int> (size * ratio);
    QPixmap iconPixmap = QPixmap(QString(":/%1/%2").arg(themeName).arg(name)).scaled(
                iconSize, iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    iconPixmap.setDevicePixelRatio(ratio);
    return iconPixmap;
}

QPixmap ForecastApplet::loadWIconSymbolic(const QString &name, int size) const {
    const auto ratio = qApp->devicePixelRatio();
    const int iconSize = static_cast<int> (size * ratio);
    QPixmap iconPixmap = QPixmap(QString(":/White/%1").arg(name)).scaled(
                iconSize, iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    iconPixmap.setDevicePixelRatio(ratio);
    return iconPixmap;
}

QPixmap ForecastApplet::loadWIcon(const QString &name, int size) const {
    const auto ratio = qApp->devicePixelRatio();
    const int iconSize = static_cast<int> (size * ratio);
    QString iconName = name;
    if (iconName != "na")
    iconName.replace(QString("n"), QString("d"));
    QPixmap iconPixmap = QPixmap(QString(":/%1/%2").arg(themeName).arg(iconName)).scaled(
                iconSize, iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    iconPixmap.setDevicePixelRatio(ratio);
    return iconPixmap;
}

QVector<WeatherClient::Weather>::const_iterator ForecastApplet::getDayStatic(
        QVector<WeatherClient::Weather>::const_iterator iter,
        double &temp_min, double &temp_max,
        const WeatherClient::Weather **p_primary) const {
    //TODO: secondary weather?
    QDate theDay = iter->dateTime.date();
    temp_min = iter->temp_min;
    temp_max = iter->temp_max;
    *p_primary = iter;
    for(iter++; iter != client->getForecast().end() &&
        iter->dateTime.date() == theDay; iter++) {
        if (iter->temp_max > temp_max) {
            temp_max = iter->temp_max;
        }
        if (iter->temp_min < temp_min) {
            temp_min = iter->temp_min;
        }
        if (iter->weatherID / 100 < (*p_primary)->weatherID / 100 &&
                iter->weatherID % 100 > (*p_primary)->weatherID % 100) {
            // Get the most serious condition
            *p_primary = iter;
        }
    }
    return iter;
}

void ForecastApplet::reloadForecast() {
    const QVector<WeatherClient::Weather> &forecasts = client->getForecast();
    double temp_min, temp_max;
    const WeatherClient::Weather *primary;
    QVector<WeatherClient::Weather>::const_iterator next = forecasts.begin();
    if (next->dateTime.date() == QDate::currentDate()) {
        // Today is included in the forecast
        next = getDayStatic(forecasts.begin(), temp_min, temp_max, &primary);
    }
    else {
        primary = &client->weatherNow();
        temp_min = primary->temp_min;
        temp_max = primary->temp_max;
    }
    WImgNow->setPixmap(loadWIconNow(primary->icon, PRIMARYICONSIZE));
    tempNow->setText(QString("%1 ~ %2 %3\n%4").arg(qRound(temp_min)).arg(
                         qRound(temp_max)).arg(client->tempUnit()).arg(primary->description));

    int n = 0;
    while (next != forecasts.end() && n < MAXDAYS) {
        next = getDayStatic(next, temp_min, temp_max, &primary);
        fcstLabels[n].WImg->setPixmap(loadWIcon(primary->icon));
        fcstLabels[n].Temp->setText(QString("%1 ~ %2 %3\n%4").arg(
                                        qRound(temp_min)).arg(qRound(temp_max)).arg(
                                        client->tempUnit()).arg(primary->description));
        n++;
    }
    for (; n < MAXDAYS; n++) {
        fcstLabels[n].WImg->setPixmap(loadWIcon("na"));
        fcstLabels[n].Temp->setText(tr("N/A"));
    }
}

void ForecastApplet::updateError(OpenWeatherClient::ErrorCode e) {
    WImgNow->setPixmap(loadWIcon("na", PRIMARYICONSIZE));
    tempNow->setText(tr("Error: %1").arg(e));
    for (auto & f: fcstLabels) {
        f.WImg->setPixmap(loadWIcon("na"));
        f.Temp->setText(tr("N/A"));
    }
    return;
}
