#ifndef CONFIGDOCUMENT_H
#define CONFIGDOCUMENT_H

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>
#include <QMap>

#include "tabcontent.h"

namespace Ui {
    class ConfigDocument;
}

class ConfigDocument : public TabContent {
    Q_OBJECT
public:
    ConfigDocument(QMap<QString, QString>& settings, QString file,
                   QWidget *parent = 0);
    virtual ~ConfigDocument();

    TabType type() const{ return TabContent::Config; }
    QString file() const{ return filePath; }
    QString title() const;

    QString toPlainText() const;

    void save();
    void saveAs();

    bool requestClose();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::ConfigDocument *ui;
    QList<QHBoxLayout*> containers;
    QList<QLineEdit*> keys;
    QList<QLineEdit*> values;
    QList<QPushButton*> deleteButtons;

    QString filePath;
    QString saved;

    void addRow(QString key, QString value);

private slots:
    void deleteClicked();
    void addClicked();
};

#endif // CONFIGDOCUMENT_H
