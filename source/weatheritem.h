#ifndef WEATHERWIDGET_H
#define WEATHERWIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include "weatherclient.h"
#include "forecastapplet.h"

class WeatherItem : public QWidget {
    Q_OBJECT

public:
    explicit WeatherItem(const WeatherClient *wcli,
                         const ForecastApplet *popups,
                         QWidget *parent = nullptr);

signals:
    void requestUpdateGeometry() const;
    void mouseMidBtnClicked() const;

protected:
    QSize sizeHint() const;
    void resizeEvent(QResizeEvent *e) {
        QWidget::resizeEvent(e);
        refreshIcon();
    }
    void mouseReleaseEvent(QMouseEvent *e);
    void paintEvent(QPaintEvent *e);

public slots:
    void refreshIcon();

private:
    const WeatherClient *client;
    const ForecastApplet *fcstApplet;
    QPixmap m_iconPixmap;
    QString tempCur;
};

#endif // WEATHERWIDGET_H
