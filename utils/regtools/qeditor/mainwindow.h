#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QSettings>
#include "backend.h"
#include "settings.h"

class DocumentTab
{
public:
    virtual bool Quit() = 0;
    virtual void OnModified(bool modified) = 0;
};

class MyTabWidget : public QTabWidget
{
    Q_OBJECT
public:
    MyTabWidget();
    bool CloseTab(int index);

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
    void AddTab(QWidget *tab, const QString& title);
    bool Quit();

private slots:
    void OnQuit();
    void OnAbout();
    void OnAboutQt();
    void OnLoadDesc();
    void OnNewRegTab();
    void OnNewRegEdit();
    void OnTabModified(bool modified);

private:
    MyTabWidget *m_tab;
    Backend *m_backend;
};

#endif 
