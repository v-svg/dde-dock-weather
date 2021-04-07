#ifndef FORECASTWIDGET_H
#define FORECASTWIDGET_H

#include "weatherclient.h"
#include "constants.h"

#include <QWidget>
#include <QLabel>

#define MAXDAYS 5

class ForecastApplet : public QWidget
{
    Q_OBJECT

public:
    explicit ForecastApplet(const WeatherClient *wcli,
                           QString theme=themeSet[0],
                           QWidget *parent = nullptr);
    virtual ~ForecastApplet();

    void setTheme(const QString& theme) {
        themeName = theme; reloadForecast();}
    const QString& theme() const {return themeName;}
    QPixmap loadWIcon(const QString &name="na", int size=50) const;
    QPixmap loadWIconNow(const QString &name="na", int size=50) const;
    QPixmap loadWIconSymbolic(const QString &name="na", int size=16) const;

public slots:
    void reloadForecast();
    void updateError(OpenWeatherClient::ErrorCode);

protected:
    void paintEvent(QPaintEvent *e);

private:
    const WeatherClient *client;
    QString themeName;
    QPointer<QLabel> WImgNow, dateNow, tempNow;
    struct ForecastColumn {
        QPointer<QLabel> Date, WImg, Temp;
    } fcstLabels[MAXDAYS];

    QVector<WeatherClient::Weather>::const_iterator getDayStatic(
            const WeatherClient::Weather *start,
            double &temp_min, double &temp_max,
            const WeatherClient::Weather **p_primaryWeather) const;

};

#endif // FORECASTWIDGET_H
