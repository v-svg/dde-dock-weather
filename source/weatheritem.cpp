#include "weatheritem.h"
#include "dde-dock/constants.h" // provide Dock::DisplayMode
#include <QApplication> // provide qApp
#include <QPainter>

WeatherItem::WeatherItem(const WeatherClient *wcli,
                         const ForecastApplet *popups,
                         QWidget *parent):
    QWidget(parent), client(wcli), fcstApplet(popups)
{ 
    connect(client, &WeatherClient::weatherReady,
            this, &WeatherItem::refreshIcon);
    connect(client, &WeatherClient::changed,
            this, &WeatherItem::refreshIcon);
}

QSize WeatherItem::sizeHint() const {
    QFontMetrics FM(qApp->font());
    QString format = QString("%1").arg(client->tempNow());
    const Dock::DisplayMode displayMode = qApp->property(PROP_DISPLAY_MODE).value<Dock::DisplayMode>();
    const Dock::Position position = qApp->property(PROP_POSITION).value<Dock::Position>();
    if (displayMode == Dock::Efficient) {
        if (position == Dock::Top || position == Dock::Bottom) {
            return QSize(FM.boundingRect(format).width() + 34, 26);
        } else
            return QSize(FM.boundingRect(format).width() + 12, FM.boundingRect(format).height() * 2);
    } else 
        return m_iconPixmap.size();
}

void WeatherItem::paintEvent(QPaintEvent *e) {
    QWidget::paintEvent(e);

    QString format = QString("%1").arg(client->tempNow());
    const Dock::DisplayMode displayMode = qApp->property(
                PROP_DISPLAY_MODE).value<Dock::DisplayMode>();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (displayMode == Dock::Efficient) {
        painter.setPen(Qt::white);
        QFont font = qApp->font();
        painter.setFont(font);
        QFontMetrics FM(font);
        int heightFM = FM.height();
        const Dock::Position position = qApp->property(PROP_POSITION).value<Dock::Position>();
        if (position == Dock::Top || position == Dock::Bottom) {
            if (height() < 40)
                painter.drawPixmap(6, height() / 2 - 8, m_iconPixmap);
            else
                painter.drawPixmap(6, height() / 2 - 7, m_iconPixmap);
            if (height() < 40)
                painter.translate(9, height() / 2 - heightFM / 2);
            else
                painter.translate(10, height() / 2 - heightFM / 2.2);
            painter.drawText(rect(), Qt::AlignHCenter, format);
        } else {
            painter.drawPixmap(width() / 2 - 8, heightFM / 4, m_iconPixmap);
            painter.drawText(rect(), Qt::AlignCenter, " \n" + format);
        } 
    } else {
        const QRectF &rf = QRectF(rect());
        const QRectF &rfp = QRectF(m_iconPixmap.rect());
        painter.drawPixmap(rf.center() - rfp.center()
                           / m_iconPixmap.devicePixelRatioF(), m_iconPixmap);
        int fontSize = std::min(width(), height()) * 0.21;
        QFont font = qApp->font();
        font.setWeight(QFont::Black);
        font.setPixelSize(fontSize);
        painter.setFont(font);

        QFontMetrics FM(font);
        int widthFM = FM.width(format);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.translate(width() / 2, height() / 2 - fontSize / 2);
        QPainterPath pathShadow;
        pathShadow.addRoundedRect(QRectF(-widthFM / 2 - fontSize / 4 - 1, - 1, widthFM + fontSize / 2
                                  + 2, fontSize + height() / 30 + 2.5), fontSize / 2, fontSize / 2);
        painter.setPen(Qt::NoPen);
        painter.fillPath(pathShadow, QColor(0, 0, 0, 40));
        QPainterPath pathLabel;
        painter.translate(0, -0.3);
        pathLabel.addRoundedRect(QRectF(-widthFM / 2 - fontSize / 4, 0, widthFM + fontSize
                                 / 2, fontSize + fontSize / 8), fontSize / 2, fontSize / 2);
        painter.fillPath(pathLabel, QColor(255, 255, 255, 140));
        painter.setPen(QColor(255, 255, 255, 200));
        painter.drawText(QRectF(-widthFM / 2, 0, widthFM, fontSize).adjusted (0, fontSize / 5.4, 0, 0),
                         Qt::AlignCenter, format);
        painter.setPen(QColor(50, 50, 50));
        painter.drawText(QRectF(-widthFM / 2, 0, widthFM, fontSize), Qt::AlignCenter, format);
    }
}

void WeatherItem::refreshIcon() {
    const Dock::DisplayMode displayMode =
            qApp->property(PROP_DISPLAY_MODE).value<Dock::DisplayMode>();
    QString weather;
    weather = client->weatherNowIcon();
    int iconSize;
    if (displayMode == Dock::Fashion) {
    iconSize = std::min(width(), height()) * 0.9;
    m_iconPixmap = fcstApplet->loadWIconNow(weather, iconSize);
    } else {
        iconSize = 16;
        m_iconPixmap = fcstApplet->loadWIconSymbolic(weather, iconSize);
    }
    update();
}

