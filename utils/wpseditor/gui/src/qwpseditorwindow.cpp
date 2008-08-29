#include "qwpseditorwindow.h"
#include "qwpsdrawer.h"
#include "utils.h"
#include <QFileDialog>
#include <QDebug>
#include <QInputDialog>

enum api_playmode playmodes[PLAYMODES_NUM] = {
            API_STATUS_PLAY,
            API_STATUS_STOP,
            API_STATUS_PAUSE,
            API_STATUS_FASTFORWARD,
            API_STATUS_FASTBACKWARD
        };

const char *playmodeNames[] = {
                                  "Play",
                                  "Stop",
                                  "Pause",
                                  "FastForward",
                                  "FastBackward"
                              };

QWpsEditorWindow::QWpsEditorWindow( QWidget * parent, Qt::WFlags f)
        : QMainWindow(parent, f) {
    logEdit = 0;
    setupUi(this);
    drawer = new QWpsDrawer(&wpsState,&trackState, this);
    QWpsDrawer::api.verbose = 1;
    //drawer->WpsInit("iCatcher.wps");
    setCentralWidget(drawer);
    connectActions();
    m_propertyEditor->addObject(&trackState);
    m_propertyEditor->addObject(&wpsState);
}

void QWpsEditorWindow::connectActions() {
    qDebug()<<"connect actions";
    connect(actOpenWps,     SIGNAL(triggered()),     this,   SLOT(slotOpenWps()));
    connect(actSetVolume,   SIGNAL(triggered()),     drawer, SLOT(slotSetVolume()));
    connect(actSetProgress, SIGNAL(triggered()),     drawer, SLOT(slotSetProgress()));
    connect(actShowGrid,    SIGNAL(triggered(bool)), drawer, SLOT(slotShowGrid(bool)));

    connect(actUpdatePlainWps,             SIGNAL(triggered()),                 SLOT(slotUpdatePlainWps()));
    connect(plainWpsEdit->document(),     SIGNAL(modificationChanged(bool)),    SLOT(slotPlainDocModChanged(bool)));

    connect(&wpsState, SIGNAL(stateChanged(wpsstate)), drawer,   SLOT(slotWpsStateChanged(wpsstate)));
    connect(&trackState, SIGNAL(stateChanged(trackstate)), drawer,   SLOT(slotTrackStateChanged(trackstate)));
    connect(&wpsState, SIGNAL(stateChanged(wpsstate)), this,     SLOT(slotWpsStateChanged(wpsstate)));
    connect(&trackState, SIGNAL(stateChanged(trackstate)), this,     SLOT(slotTrackStateChanged(trackstate)));

    connect(actClearLog, SIGNAL(triggered()), logEdit, SLOT(clear()));
    connect(actVerboseLevel, SIGNAL(triggered()), SLOT(slotVerboseLevel()));

    actGroupAudios = new QActionGroup(this);
    signalMapper = new QSignalMapper(this);
    for (int i=0;i<PLAYMODES_NUM;i++) {
        QAction *act = new QAction(playmodeNames[i],this);
        act->setCheckable(true);
        actGroupAudios->addAction(act);
        connect(act,SIGNAL(triggered()),signalMapper,SLOT(map()));
        signalMapper->setMapping(act, i);
        menuPlay->addAction(act);
        actAudios[playmodes[i]] = act;
    }
    connect(signalMapper, SIGNAL(mapped(int)), SIGNAL(signalAudioStatusChanged(int)));
    connect(this,         SIGNAL(signalAudioStatusChanged(int)), drawer, SLOT(slotSetAudioStatus(int)));
    actGroupAudios->setEnabled(false);
}

void QWpsEditorWindow::slotWpsStateChanged(wpsstate) {
    m_propertyEditor->updateObject(&wpsState);
    m_propertyEditor->update();
}

void QWpsEditorWindow::slotTrackStateChanged(trackstate) {
    m_propertyEditor->updateObject(&trackState);
    m_propertyEditor->update();
}

void QWpsEditorWindow::slotOpenWps() {
    QString wpsfile = QFileDialog::getOpenFileName(this,
                      tr("Open WPS"), "", tr("WPS Files (*.wps);; All Files (*.*)"));
    if (wpsfile == "") {
        DEBUGF1(tr("File wasn't chosen"));
        return;
    }
    m_propertyEditor->setEnabled(true);
    drawer->WpsInit(wpsfile);
    plainWpsEdit->clear();
    plainWpsEdit->append(drawer->wpsString());
    trackState.setAlbum(trackState.album());
    actGroupAudios->setEnabled(true);
}

void QWpsEditorWindow::logMsg(QString s) {
    logEdit->append(s);
}

void QWpsEditorWindow::slotVerboseLevel() {
    bool ok;
    int i = QInputDialog::getInteger(this, tr("Set Verbose Level"),tr("Level:"), QWpsDrawer::api.verbose, 0, 3, 1, &ok);
    if (ok)
        QWpsDrawer::api.verbose = i;
}

void QWpsEditorWindow::slotUpdatePlainWps() {
    DEBUGF1(tr("Updating WPS"));
    plainWpsEdit->document()->setModified(false);
    drawer->WpsInit(plainWpsEdit->toPlainText(),false);

}

void QWpsEditorWindow::slotPlainDocModChanged(bool changed) {
    if (changed)
        dockPlainWps->setWindowTitle(tr("PlainWps*"));
    else
        dockPlainWps->setWindowTitle(tr("PlainWps"));
}

