#ifndef MAINWINDOWIMPL_H
#define MAINWINDOWIMPL_H
//
#include <QMainWindow>
#include <QActionGroup>
#include <QSignalMapper>
#include "ui_mainwindow.h"
#include "wpsstate.h"
#include "qwpsdrawer.h"
#include "qwpsstate.h"
#include "qtrackstate.h"
//
class QWpsEditorWindow : public QMainWindow, public Ui::MainWindow {
    Q_OBJECT
    QWpsState wpsState;
    QTrackState trackState;
    QPointer<QWpsDrawer> drawer;

    QHash<int, QAction*> actAudios;
    QActionGroup     *actGroupAudios;
    QSignalMapper     *signalMapper;

protected:
    void connectActions();
public:
    QWpsEditorWindow( QWidget * parent = 0, Qt::WFlags f = 0 );
    void logMsg(QString s);
private slots:
    void slotOpenWps();
    void slotVerboseLevel();
    void slotWpsStateChanged(wpsstate);
    void slotTrackStateChanged(trackstate);

    void slotUpdatePlainWps();
    void slotPlainDocModChanged(bool m);

signals:
    void signalAudioStatusChanged(int);

};
#endif




