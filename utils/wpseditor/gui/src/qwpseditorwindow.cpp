#include <QFileDialog>
#include <QDebug>
#include <QInputDialog>
#include "api.h"
#include "qwpseditorwindow.h"
#include "utils.h"
#include "qsyntaxer.h"


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
    setCentralWidget(drawer);
    connectActions();
    m_propertyEditor->addObject(&trackState);
    m_propertyEditor->addObject(&wpsState);
    new QSyntaxer(plainWpsEdit->document());
}

void QWpsEditorWindow::connectActions() {
    DEBUGF3("connect actions");
    connect(actOpenWps,     SIGNAL(triggered()),     this,   SLOT(slotOpenWps()));
    connect(actSetVolume,   SIGNAL(triggered()),     drawer, SLOT(slotSetVolume()));
    connect(actSetProgress, SIGNAL(triggered()),     drawer, SLOT(slotSetProgress()));
    connect(actShowGrid,    SIGNAL(triggered(bool)), drawer, SLOT(slotShowGrid(bool)));

    connect(actUpdatePlainWps,       SIGNAL(triggered()),              SLOT(slotUpdatePlainWps()));
    connect(plainWpsEdit->document(),SIGNAL(modificationChanged(bool)),SLOT(slotPlainDocModChanged(bool)));

    connect(&wpsState,   SIGNAL(stateChanged(wpsstate)),   drawer, SLOT(slotWpsStateChanged(wpsstate)));
    connect(&trackState, SIGNAL(stateChanged(trackstate)), drawer, SLOT(slotTrackStateChanged(trackstate)));
    connect(&wpsState,   SIGNAL(stateChanged(wpsstate)),   this,   SLOT(slotWpsStateChanged(wpsstate)));
    connect(&trackState, SIGNAL(stateChanged(trackstate)), this,   SLOT(slotTrackStateChanged(trackstate)));

    connect(actClearLog,     SIGNAL(triggered()), logEdit, SLOT(clear()));
    connect(actVerboseLevel, SIGNAL(triggered()), SLOT(slotVerboseLevel()));

    actGroupAudios = new QActionGroup(this);
    audiosSignalMapper = new QSignalMapper(this);
    for (int i=0;i<PLAYMODES_NUM;i++) {
        QAction *act = new QAction(playmodeNames[i],this);
        act->setCheckable(true);
        actGroupAudios->addAction(act);
        connect(act,SIGNAL(triggered()),audiosSignalMapper,SLOT(map()));
        audiosSignalMapper->setMapping(act, i);
        menuPlay->addAction(act);
        actAudios[playmodes[i]] = act;
    }
    connect(audiosSignalMapper, SIGNAL(mapped(int)), SIGNAL(signalAudioStatusChanged(int)));
    connect(this, SIGNAL(signalAudioStatusChanged(int)), drawer, SLOT(slotSetAudioStatus(int)));
    actGroupAudios->setEnabled(false);

    QList<QString> targets = drawer->getTargets();
    actGroupTargets = new QActionGroup(this);
    targetsSignalMapper = new QSignalMapper(this);

    for (int i=0;i<targets.size();i++) {
        QAction *act = new QAction(targets[i],this);
        act->setCheckable(true);
        actGroupTargets->addAction(act);
        connect(act,SIGNAL(triggered()),targetsSignalMapper,SLOT(map()));
        targetsSignalMapper->setMapping(act, targets[i]);
        menuTarget->addAction(act);
        actTargets[targets[i]] = act;
    }
    connect(targetsSignalMapper, SIGNAL(mapped(const QString &)),this, SIGNAL(signalSetTarget(const QString &)));
    connect(this, SIGNAL(signalSetTarget(const QString &)),this, SLOT(slotSetTarget(const QString &)));
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
    trackState.setAlbum(trackState.album()); ////updating property editor
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
    m_propertyEditor->setEnabled(true);
    actGroupAudios->setEnabled(true);
    trackState.setAlbum(trackState.album()); //updating property editor
}

void QWpsEditorWindow::slotPlainDocModChanged(bool changed) {
    if (changed)
        dockPlainWps->setWindowTitle(tr("PlainWps*"));
    else
        dockPlainWps->setWindowTitle(tr("PlainWps"));
}
void QWpsEditorWindow::slotSetTarget(const QString & target) {
    if (drawer->setTarget(target)) {
        DEBUGF1(tr("New target <%1> switched").arg(target));
        actTargets[target]->setChecked(true);
    } else
        DEBUGF1(tr("ERR: Target <%1> failed!").arg(target));
    update();
    slotUpdatePlainWps();
}



