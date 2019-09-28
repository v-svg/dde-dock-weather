#include "weathersettingdialog.h"
#include <QBoxLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QAbstractItemView>
#include <QApplication>
#include <QProxyStyle>
#include <QListView>
#include <QObject>
#include <QToolButton>
#include <QEnterEvent>
#include <QDesktopServices>
#include "countrycode.h"

LimitedHightComboBox::LimitedHightComboBox(int h, QWidget *parent):
    QComboBox(parent), height(h) {
    view()->setMaximumHeight(height);
    view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void LimitedHightComboBox::showPopup() {
    QComboBox::showPopup();
    QFrame *popup = findChild<QFrame*>();
    popup->setMaximumHeight(height);
    // Position vertically so the curently selected item lines up
    // with the combo box, inspired by QComboBox source code
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    QPoint above = mapToGlobal(style()->subControlRect(
                                   QStyle::CC_ComboBox, &opt,
                                   QStyle::SC_ComboBoxListBoxPopup,
                                   this).topLeft());
    const QRect currentItemRect = view()->visualRect(view()->currentIndex());
    int top = view()->mapToGlobal(currentItemRect.topLeft()).y();
    popup->move(popup->x(), popup->y() + above.y() - top);
}

bool AppidBox::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::Enter && this->text() == "") {
        qApp->postEvent(watched, new QHelpEvent(QEvent::ToolTip,
                            static_cast<QEnterEvent*>(event)->pos(),
                            static_cast<QEnterEvent*>(event)->globalPos()));
    }
    return false;
}

#define OPTIONS_WIDTH 100
WeatherSettingDialog::WeatherSettingDialog(PluginProxyInterface *proxyInter,
                                           WeatherPlugin *weatherplugin,
                                           QNetworkAccessManager &net,
                                           QTextStream &logStream,
                                           QWidget *parent) :
    QDialog(parent),
    m_proxyInter(proxyInter), m_weatherPlugin(weatherplugin),
    m_cityLookupClient(new CityLookup(net, logStream, this)),
    cityBox(new QComboBox(this)), countryBox(new LimitedHightComboBox(500, this)),
    themeBox(new QComboBox(this)), timeIntvBox(new QLineEdit(this)),
    appidBox(new AppidBox(this)), langBox(new QComboBox(this))
{
    setModal(true);
    setWindowTitle(tr("Weather"));
    QVBoxLayout *vLayout = new QVBoxLayout();
    vLayout->setSizeConstraint(QLayout::SetFixedSize);

    QGroupBox *geoGroup = new QGroupBox(tr("City Information"));
    QGridLayout *geoLayout = new QGridLayout();
    geoLayout->addWidget(new QLabel(tr("City")), 0, 0);
    cityBox->setEditable(true);
    cityBox->setCurrentText(m_weatherPlugin->m_client->cityName());
    cityBox->setMinimumWidth(OPTIONS_WIDTH*1.25);
    geoLayout->addWidget(cityBox, 0, 1);
    geoLayout->addWidget(new QLabel(tr("Country")), 0, 2);
    countryBox->setModel(new CountryModel(this));
    countryBox->setMaximumWidth(OPTIONS_WIDTH*1.25);
    cityBox->setMinimumHeight(countryBox->height());
    QString country = m_weatherPlugin->m_client->countryName();
    countryBox->setCurrentIndex(countryBox->findData(country));
    geoLayout->addWidget(countryBox, 0, 3);
    QLabel *geoComments =  new QLabel(tr("You can also input cityid from "
                                         "<a href=\"https://openweathermap.org/city\" style=\"color: yellow\">openweathermap.org</a> "
                                         "(the number in the URL of your city page) "
                                         "to avoid unambiguous result. "));
    geoComments->setWordWrap(true);
    geoComments->setTextFormat(Qt::RichText);
    geoComments->setTextInteractionFlags(Qt::TextBrowserInteraction);
    geoComments->setOpenExternalLinks(true);
    geoLayout->addWidget(geoComments, 1, 0, 1, -1);
    geoGroup->setLayout(geoLayout);
    // TODO: help icon
    vLayout->addWidget(geoGroup);

    QString appid = m_proxyInter->getValue(m_weatherPlugin, APPID_KEY, "").toString();

    QGroupBox *optionGroup = new QGroupBox(tr("Options"));
    QGridLayout *settingLayout = new QGridLayout();
    settingLayout->addWidget(new QLabel(tr("Interval")),
                             0, 0);
    timeIntvBox->setText(QString::number(m_proxyInter->getValue(
                              m_weatherPlugin, CHK_INTERVAL_KEY,
                              DEFAULT_INTERVAL).toInt()));
    timeIntvBox->setInputMask(tr("999 min"));
    timeIntvBox->setMaximumWidth(OPTIONS_WIDTH);
    timeIntvBox->setEnabled(appid != "");
    timeIntvBox->setToolTip(tr("Due to the limitations of openweathermap.org for free account, "
                               "you have to use your own appid to set shorter time interval."));
    timeIntvBox->installEventFilter(appidBox);
    settingLayout->addWidget(timeIntvBox, 0, 1);
    settingLayout->addWidget(new QLabel(tr("Units")), 1, 0);
    QHBoxLayout *unitButtons = new QHBoxLayout();
    metricButton = new QRadioButton(tr("metric"), optionGroup);
    imperialButton = new QRadioButton(tr("imperial"), optionGroup);
    unitButtons->addWidget(metricButton);
    unitButtons->addWidget(imperialButton);
    unitButtons->addStretch();
    if (m_proxyInter->getValue(m_weatherPlugin, UNIT_KEY, true).toBool()) {
        metricButton->setChecked(true);
    }
    else {
        imperialButton->setChecked(true);
    }
    settingLayout->addLayout(unitButtons, 1, 1);
    settingLayout->addWidget(new QLabel(tr("Theme")), 2, 0);
    themeBox->addItems(themeSet);
    themeBox->setCurrentText(m_proxyInter->getValue(m_weatherPlugin, THEME_KEY,
                                  themeSet[0]).toString());
    themeBox->setMaximumWidth(OPTIONS_WIDTH);
    settingLayout->addWidget(themeBox, 2, 1);

    settingLayout->addWidget(new QLabel(tr("Language")), 3, 0);
    QString defaultLang = tr("System Language");
    langBox->addItems(langSet.keys());
    langBox->setItemText(0, defaultLang);
    langBox->setMaximumWidth(OPTIONS_WIDTH);
    langBox->setToolTip(tr("You may need to restart the dock to make language setting effective."));
    QString lang = m_proxyInter->getValue(m_weatherPlugin, LANG_KEY,
                                          defaultLang).toString();
    if (lang == "")
        lang = defaultLang;
    langBox->setCurrentText(langSet.key(lang, defaultLang));
    settingLayout->addWidget(langBox, 3, 1);

    appidBox->setPlaceholderText(tr("appid from openweathermap.org"));
    if (appid != "") {
        appidBox->setText(appid);
    }
    connect(appidBox, &QLineEdit::textEdited,
            this, &WeatherSettingDialog::newAppidInput);
    settingLayout->addWidget(new QLabel(tr("AppID")), 4, 0);
    settingLayout->addWidget(appidBox, 4, 1);

    QToolButton* appidHelp = new QToolButton(this);
    appidHelp->setIcon(QIcon(":/question"));
    appidHelp->setToolTip(tr("The plugin default appid is proviede by the author. "
                             "Click to learn how to get your own appid."));
    appidHelp->installEventFilter(appidBox);
    connect(appidHelp, &QToolButton::released, [=](){
        QDesktopServices::openUrl(QUrl("https://openweathermap.org/appid"));});
    settingLayout->addWidget(appidHelp, 4, 2);
    optionGroup->setLayout(settingLayout);
    vLayout->addWidget(optionGroup);

    QDialogButtonBox *buttons = new QDialogButtonBox(
                QDialogButtonBox::Cancel|QDialogButtonBox::Ok, this);
    buttons->setStyleSheet("* { button-layout: 3 }");
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, this, &WeatherSettingDialog::accept);
    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->setContentsMargins(0, 10, 0, 0);
    hbox->addWidget(buttons);
    vLayout->addLayout(hbox);

    setLayout(vLayout);
}

void WeatherSettingDialog::accept() {
    m_proxyInter->saveValue(m_weatherPlugin, THEME_KEY,
                            themeBox->currentText());
    // TODO: cityid
    QString cityInput = cityBox->currentText();
    bool isCityid;
    int cityid = cityInput.toInt(&isCityid);
    if (isCityid) {
        m_proxyInter->saveValue(m_weatherPlugin, CITYID_KEY, cityid);
        m_proxyInter->saveValue(m_weatherPlugin, CITY_KEY, "");
        m_proxyInter->saveValue(m_weatherPlugin, COUNTRY_KEY, "");
    }
    else {
        QString countryInput = countryBox->currentData().toString();
        if (cityInput != m_weatherPlugin->m_client->cityName() ||
                countryInput != m_weatherPlugin->m_client->countryName()) {
            m_proxyInter->saveValue(m_weatherPlugin, CITYID_KEY, 0);
            m_proxyInter->saveValue(m_weatherPlugin, CITY_KEY, cityInput);
            m_proxyInter->saveValue(m_weatherPlugin, COUNTRY_KEY, countryInput);
        }
    }
    m_proxyInter->saveValue(m_weatherPlugin, UNIT_KEY,
                            metricButton->isChecked());
    m_proxyInter->saveValue(m_weatherPlugin, APPID_KEY,
                            appidBox->text());
    bool ok;
    int time = timeIntvBox->text().split(" ")[0].toInt(&ok);
    if (time <= 0 || !ok) {
        time = 30;
    }
    m_proxyInter->saveValue(m_weatherPlugin, CHK_INTERVAL_KEY, time);
    if (langBox->currentIndex() == 0) {
        m_proxyInter->saveValue(m_weatherPlugin, LANG_KEY, "");
    }
    else {
        m_proxyInter->saveValue(m_weatherPlugin, LANG_KEY,
                                langSet[langBox->currentText()]);
    }
    QDialog::accept();
}

void WeatherSettingDialog::newAppidInput(const QString &input) {
    timeIntvBox->setEnabled(input != "");
    // Make it effective immediately for event filter to start
    appidBox->setText(input);
}
