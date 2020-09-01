#include "weatheritem.h"
#include "dde-dock/constants.h" // provide Dock::DisplayMode
#include <QApplication> // provide qApp
#include <QPainter>

WeatherItem::WeatherItem(const WeatherClient *wcli,
                         const ForecastApplet *popups,
                         QWidget *parent):
    QWidget(parent), client(wcli), fcstApplet(popups)
{ 
    connect(client, &WeatherClient::weatherReady, this, &WeatherItem::refreshIcon);
    connect(client, &WeatherClient::changed, this, &WeatherItem::refreshIcon);
}

QSize WeatherItem::sizeHint() const
{
    QFontMetrics FM(qApp->font());
    QString tempNow = client->tempNow();
    const Dock::DisplayMode displayMode = qApp->property(PROP_DISPLAY_MODE).value<Dock::DisplayMode>();
    const Dock::Position position = qApp->property(PROP_POSITION).value<Dock::Position>();
    if (displayMode == Dock::Efficient) {
        if (client->weatherNowIcon() == "na") {
            return QSize(26, 26);
        } else if (position == Dock::Top || position == Dock::Bottom) {
                return QSize(FM.boundingRect(tempNow).width() + 32, 26);
            } else
                return QSize(FM.boundingRect(tempNow).width() + 10, FM.boundingRect(tempNow).height() * 2 + 4);
    } else 
        return m_iconPixmap.size();
}

void WeatherItem::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QString tempNow = client->tempNow();

    if (tempCur != tempNow)
        emit requestUpdateGeometry();
    
    const Dock::DisplayMode displayMode = qApp->property(PROP_DISPLAY_MODE).value<Dock::DisplayMode>();
    if (displayMode == Dock::Efficient) {
        painter.setPen(Qt::white);
        QFont font = qApp->font();
        painter.setFont(font);
        QFontMetrics FM(font);
        int heightFM = FM.height();
        const Dock::Position position = qApp->property(PROP_POSITION).value<Dock::Position>();
        if (position == Dock::Top || position == Dock::Bottom) {
            if (height() < 40)
                painter.drawPixmap(5, height() / 2 - 8, m_iconPixmap);
            else
                painter.drawPixmap(5, height() / 2 - 7, m_iconPixmap);
            if (client->weatherNowIcon() != "na") {
                if (height() < 40)
                    painter.translate(9, height() / 2 - heightFM / 2);
                else
                    painter.translate(10, height() / 2 - heightFM / 2.2);
                painter.drawText(rect(), Qt::AlignHCenter, tempNow);
            }
        } else {
            if (width() < 70)
                painter.drawPixmap(width() / 2 - 8, heightFM / 4, m_iconPixmap);
            else
                painter.drawPixmap(width() / 2 - 6, heightFM / 4, m_iconPixmap);
            if (client->weatherNowIcon() != "na")
                painter.drawText(rect(), Qt::AlignCenter, " \n" + tempNow);
        } 
    } else {
        const QRectF rf = QRectF(rect());
        const QRectF rfp = QRectF(m_iconPixmap.rect());
        painter.drawPixmap(rf.center() - rfp.center() / m_iconPixmap.devicePixelRatioF(), m_iconPixmap);


        if (client->weatherNowIcon() != "na") {
            int fontSize = std::min(width(), height()) * 0.21;
            QFont font = qApp->font();
            font.setWeight(QFont::Black);
            font.setPixelSize(fontSize);
            painter.setFont(font);

            QFontMetrics FM(font);
            int widthFM = FM.width(tempNow);
            qreal offsetX = -widthFM / 2 - fontSize / 4;
            qreal widthPath = widthFM + fontSize / 2;
            qreal heightPath = fontSize + fontSize / 8;
            qreal radius = heightPath / 2;

            QPainterPath pathShadow;
            painter.translate(width() / 2, height() / 2 - heightPath / 2);
            pathShadow.addRoundedRect(offsetX - 1, - 1, widthPath + 2, heightPath + 3.5, radius, radius);
            painter.setPen(Qt::NoPen);
            painter.fillPath(pathShadow, QColor(0, 0, 0, 40));

            QPainterPath pathLabel;
            painter.translate(0, -0.3);
            pathLabel.addRoundedRect(offsetX, 0, widthPath, heightPath, radius, radius);
            painter.fillPath(pathLabel, QColor(255, 255, 255, 140));

            painter.setPen(QColor(255, 255, 255));
            painter.drawText(-widthFM / 2, 1.3, widthFM, fontSize, Qt::AlignCenter, tempNow);
            painter.setPen(QColor(50, 50, 50));
            painter.drawText(-widthFM / 2, 0, widthFM, fontSize, Qt::AlignCenter, tempNow);
        }
    }
    tempCur = tempNow;
}

void WeatherItem::refreshIcon()
{
    const Dock::DisplayMode displayMode = qApp->property(PROP_DISPLAY_MODE).value<Dock::DisplayMode>();
    QString weather = client->weatherNowIcon();
    if (displayMode == Dock::Fashion) {
        int iconSize = std::min(width(), height()) * 0.9;
        m_iconPixmap = fcstApplet->loadWIconNow(weather, iconSize);
    } else 
        m_iconPixmap = fcstApplet->loadWIconSymbolic(weather, 16);
    update();
}

