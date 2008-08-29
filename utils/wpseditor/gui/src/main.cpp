#include <QApplication>
#include "qwpseditorwindow.h"
#include "utils.h"
#include <QPointer>

QPointer<QWpsEditorWindow> win;

int main(int argc, char ** argv) {
    QApplication app( argc, argv );

    win = new QWpsEditorWindow;
    win->show();
    app.connect( &app, SIGNAL( lastWindowClosed() ), &app, SLOT( quit() ) );

    return app.exec();
}
