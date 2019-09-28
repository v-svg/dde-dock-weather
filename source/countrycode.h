#ifndef COUNTRYCODE_H
#define COUNTRYCODE_H
#include <QAbstractListModel>
#include <QMap>

class CountryModel: public QAbstractListModel {
    Q_OBJECT

public:
    explicit CountryModel(QObject *parent = nullptr);
//    virtual ~CountryModel() override;

    virtual int rowCount(
            const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const override;

    static const QMap<QString, QString> CodeMap;
    static const QStringList Code;

    inline QString displayCountry(const QString &code) const {
        return (CodeMap.contains(code) && code != "") ?
                    QString("%1 (%2)").arg(code).arg(CodeMap[code]) : "";
    }


};

#endif // COUNTRYCODE_H
