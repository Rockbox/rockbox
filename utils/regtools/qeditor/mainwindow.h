#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QSettings>
#include "backend.h"
#include "settings.h"

class MyTabWidget;

class DocumentTab
{
public:
    DocumentTab() { m_tab = 0; }
    virtual bool Quit() = 0;
    virtual QWidget *GetWidget() = 0;
    void SetTabWidget(MyTabWidget *tab) { m_tab = tab; }

protected:
    void OnModified(bool modified);
    void SetTabName(const QString& name);
    MyTabWidget *m_tab;
};

class MyTabWidget : public QTabWidget
{
    Q_OBJECT
public:
    MyTabWidget();
    bool CloseTab(int index);
    void SetTabModified(DocumentTab *tab, bool mod);
    void SetTabName(DocumentTab *tab, const QString& name);

private slots:
    void OnCloseTab(int index);
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(Backend *backend);
    void center();
    void ReadSettings();
    void WriteSettings();

private:
    void closeEvent(QCloseEvent *event);

protected:
    void AddTab(DocumentTab *tab, const QString& title);
    bool Quit();

private slots:
    void OnQuit();
    void OnAbout();
    void OnAboutQt();
    void OnLoadDesc();
    void OnNewRegTab();
    void OnNewRegEdit();

private:
    MyTabWidget *m_tab;
    Backend *m_backend;
};

#endif 
