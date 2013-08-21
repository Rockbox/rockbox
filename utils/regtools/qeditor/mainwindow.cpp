#include <QWidget>
#include <QApplication>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QTabBar>
#include <QCloseEvent>
#include <QDebug>

#include "mainwindow.h"
#include "regtab.h"

MyTabWidget::MyTabWidget()
{
    setMovable(true);
    setTabsClosable(true);
    connect(this, SIGNAL(tabCloseRequested(int)), this, SLOT(OnCloseTab(int)));
}

void MyTabWidget::OnCloseTab(int index)
{
    removeTab(index);
}

MainWindow::MainWindow(Backend *backend)
    :m_backend(backend)
{
    QAction *new_regtab_act = new QAction(QIcon::fromTheme("document-new"), tr("&Register Tab"), this);
    QAction *load_desc_act = new QAction(QIcon::fromTheme("document-open"), tr("&Soc Description"), this);
    QAction *quit_act = new QAction(QIcon::fromTheme("application-exit"), tr("&Quit"), this);
    QAction *about_act = new QAction(QIcon::fromTheme("help-about"), tr("&About"), this);

    connect(new_regtab_act, SIGNAL(triggered()), this, SLOT(OnNewRegTab()));
    connect(load_desc_act, SIGNAL(triggered()), this, SLOT(OnLoadDesc()));
    connect(quit_act, SIGNAL(triggered()), this, SLOT(OnQuit()));
    connect(about_act, SIGNAL(triggered()), this, SLOT(OnAbout()));

    QMenu *file_menu = menuBar()->addMenu(tr("&File"));
    QMenu *new_submenu = file_menu->addMenu(QIcon::fromTheme("document-new"), "&New");
    QMenu *load_submenu = file_menu->addMenu(QIcon::fromTheme("document-open"), "&Load");
    file_menu->addAction(quit_act);

    new_submenu->addAction(new_regtab_act);

    load_submenu->addAction(load_desc_act);

    QMenu *about_menu = menuBar()->addMenu(tr("&About"));
    about_menu->addAction(about_act);

    m_tab = new MyTabWidget();

    setCentralWidget(m_tab);

    ReadSettings();
}

void MainWindow::ReadSettings()
{
    restoreGeometry(Settings::Get()->value("mainwindow/geometry").toByteArray());
}

void MainWindow::WriteSettings()
{
    Settings::Get()->setValue("mainwindow/geometry", saveGeometry());
}

void MainWindow::OnQuit()
{
    WriteSettings();
}

void MainWindow::OnAbout()
{
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    WriteSettings();
    event->accept();
}

void MainWindow::OnLoadDesc()
{
    QFileDialog *fd = new QFileDialog(this);
    fd->setFilter("XML files (*.xml);;All files (*)");
    fd->setFileMode(QFileDialog::ExistingFiles);
    fd->setDirectory(Settings::Get()->value("mainwindow/loaddescdir", QDir::currentPath()).toString());
    if(fd->exec())
    {
        QStringList filenames = fd->selectedFiles();
        for(int i = 0; i < filenames.size(); i++)
            if(!m_backend->LoadSocDesc(filenames[i]))
            {
                QMessageBox msg;
                msg.setText(QString("Cannot load ") + filenames[i]);
                msg.exec();
            }
        Settings::Get()->setValue("mainwindow/loaddescdir", fd->directory().absolutePath());
    }
}

void MainWindow::OnNewRegTab()
{
    new RegTab(m_backend, m_tab);
}
