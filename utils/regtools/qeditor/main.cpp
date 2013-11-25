#include <QApplication>
#include <QDir>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Backend *backend = new Backend;
    QDir dir(QCoreApplication::applicationDirPath());
    dir.cdUp();
    dir.cd("desc");
    dir.setFilter(QDir::Files);
    printf("%s\n", dir.absolutePath().toStdString().c_str());
    QFileInfoList list = dir.entryInfoList();
    for(int i = 0; i < list.size(); i++)
    {
        QFileInfo fileInfo = list.at(i);
        if(fileInfo.fileName().right(4) != ".xml" || fileInfo.fileName().left(5) != "regs-")
            continue;
        backend->LoadSocDesc(fileInfo.absoluteFilePath());
    }

    QCoreApplication::setOrganizationName("Rockbox");
    QCoreApplication::setApplicationName("Register Editor");
    QCoreApplication::setOrganizationDomain("rockbox.org");
    MainWindow win(backend);
    win.show();
    return app.exec();
} 
