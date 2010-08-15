#ifndef TABCONTENT_H
#define TABCONTENT_H

#include <QWidget>

class PreferencesDialog;

class TabContent : public QWidget
{
Q_OBJECT
public:
    enum TabType
    {
        Skin,
        Config
    };

    TabContent(QWidget *parent = 0): QWidget(parent){ }

    virtual TabType type() const = 0;
    virtual QString title() const = 0;
    virtual QString file() const = 0;

    virtual void save() = 0;
    virtual void saveAs() = 0;

    virtual bool requestClose() = 0;

    virtual void connectPrefs(PreferencesDialog* prefs) = 0;

signals:
    void titleChanged(QString);
    void lineChanged(int);

public slots:
    virtual void settingsChanged() = 0;

};

#endif // TABCONTENT_H
