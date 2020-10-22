#ifndef WEATHEQRSETTINGDIALOG_H
#define WEATHERSETTINGDIALOG_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QListView>
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
    QPointer<QLineEdit> cityBox;
    QPointer<QComboBox> countryBox;
    QPointer<QComboBox> themeBox;
    QPointer<QLineEdit> timeIntvBox;
    QPointer<QRadioButton> metricButton;
    QPointer<QRadioButton> imperialButton;
    QPointer<QLineEdit> appidBox;
    QPointer<QComboBox> langBox;
    QString currentLang;
    void loadSettings();

private slots:
    void newAppidInput(const QString &input);
    void reloadLang();
};


/**
 * @brief The LimitedHightComboBox class
 * This class implement a QComboBox with limited height
 * under popup mode
 */
class LimitedHightComboBox: public QComboBox
{
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

class ComboView : public QListView
{
    Q_OBJECT
protected:
    QStyleOptionViewItem viewOptions() const
    {
        // Set icon on the top and center of combo box item.
        QStyleOptionViewItem option = QListView::viewOptions();
        option.decorationAlignment = Qt::AlignLeft | Qt::AlignVCenter;
        option.decorationPosition = QStyleOptionViewItem::Left;
        option.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;   
        return option;
    }
};
#endif // WEATHERSETTINGDIALOG_H
