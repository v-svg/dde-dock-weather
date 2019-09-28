#ifndef WEATHEQRSETTINGDIALOG_H
#define WEATHERSETTINGDIALOG_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QRadioButton>
#include <QDialog>
#include "dde-dock/pluginsiteminterface.h"
#include "weatherclient.h"
#include "weatherplugin.h"
#include "constants.h"

class WeatherSettingDialog : public QDialog
{
    Q_OBJECT
public:
    explicit WeatherSettingDialog(PluginProxyInterface *proxyInter,
                                  WeatherPlugin *weatherplugin,
                                  QNetworkAccessManager &net,
                                  QTextStream &logStream,
                                  QWidget *parent = nullptr);
//    virtual ~WeatherSettingDialog() override;

public slots:
    virtual void accept() override; // Also delete self lateron

private:
    ///
    /// \brief m_proxyInter
    /// To get and save setting options.
    /// the WeatherPlugin class is responsible to reload setting
    /// when accepted SIGNAL is sent
    ///
    PluginProxyInterface *m_proxyInter;
    WeatherPlugin *m_weatherPlugin;
    QPointer<CityLookup> m_cityLookupClient;
    QPointer<QComboBox> cityBox;
    QPointer<QComboBox> countryBox;
    QPointer<QComboBox> themeBox;
    QPointer<QLineEdit> timeIntvBox;
    QPointer<QRadioButton> metricButton;
    QPointer<QRadioButton> imperialButton;
    QPointer<QLineEdit> appidBox;
    QPointer<QComboBox> langBox;

    void loadSettings();

private slots:
    void newAppidInput(const QString &input);
};


/**
 * @brief The LimitedHightComboBox class
 * This class implement a QComboBox with limited height
 * under popup mode
 */
class LimitedHightComboBox: public QComboBox {
    Q_OBJECT
public:
    LimitedHightComboBox(int h, QWidget *parent=nullptr);
    virtual void showPopup () override;

private:
    int height;
};

class AppidBox : public QLineEdit
{
    Q_OBJECT
public:
    AppidBox(QWidget *parent = nullptr): QLineEdit(parent) {}

    /** This filter watches QEvent::Enter,
     * and triggers QEvent:ToolTip if this->text()=="" */
    virtual bool eventFilter(QObject *watched, QEvent *event) override;
};

#endif // WEATHERSETTINGDIALOG_H
