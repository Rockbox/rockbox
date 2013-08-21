#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QSettings>
#include "backend.h"
#include "settings.h"

class MyTabWidget : public QTabWidget
{
    Q_OBJECT
public:
    MyTabWidget();

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

private slots:
    void OnQuit();
    void OnAbout();
    void OnLoadDesc();
    void OnNewRegTab();

private:
    QTabWidget *m_tab;
    Backend *m_backend;
};

#endif 
