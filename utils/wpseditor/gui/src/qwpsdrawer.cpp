#include <stdarg.h>

#include <QtGui>
#include <QLibrary>

#include "qwpsdrawer.h"
#include "slider.h"
#include "utils.h"
#include "qtrackstate.h"
#include "qwpsstate.h"
#include "api.h"

QPointer<QWpsDrawer> drawer;
QPixmap   *QWpsDrawer::pix = NULL;
QString   QWpsDrawer::mTmpWpsString;
QImage    QWpsDrawer::backdrop;
proxy_api QWpsDrawer::api;

QWpsDrawer::QWpsDrawer( QWpsState *ws,QTrackState *ms, QWidget *parent )
        : QWidget(parent),wpsState(ws),trackState(ms),showGrid(false),mCurTarget(qApp->applicationDirPath()+"/libwps_IRIVER_H10_5GB") {

    tryResolve();
    newTempWps();
}

bool QWpsDrawer::tryResolve() {
    QLibrary lib(mCurTarget);
    lib_wps_init = (pfwps_init)lib.resolve("wps_init");
    lib_wps_display = (pfwps_display)lib.resolve("wps_display");
    lib_wps_refresh = (pfwps_refresh)lib.resolve("wps_refresh");
    lib_get_model_name = (pfget_model_name)lib.resolve("get_model_name");
    mResolved = lib_wps_init && lib_wps_display && lib_wps_refresh && lib_get_model_name;
    if (!mResolved)
        DEBUGF1(tr("ERR: Failed to resolve funcs!"));
    else {
        int v = api.verbose;
        memset(&api,0,sizeof(struct proxy_api));
        api.verbose = v;
        api.putsxy =                    &QWpsDrawer::putsxy;
        api.transparent_bitmap_part =   &QWpsDrawer::transparent_bitmap_part;
        api.bitmap_part =               &QWpsDrawer::bitmap_part;
        api.drawpixel =                 &QWpsDrawer::drawpixel;
        api.fillrect =                  &QWpsDrawer::fillrect;
        api.hline =                     &QWpsDrawer::hline;
        api.vline =                     &QWpsDrawer::vline;
        api.clear_viewport =            &QWpsDrawer::clear_viewport;
        api.load_wps_backdrop =         &QWpsDrawer::load_wps_backdrop;
        api.read_bmp_file =             &QWpsDrawer::read_bmp_file;
        api.debugf =                    &qlogger;
        qDebug()<<(mCurTarget+" resolved");
    }
    return mResolved;
}
QWpsDrawer::~QWpsDrawer() {
    qDebug()<<"QWpsDrawer::~QWpsDrawer()";
    cleanTemp();
}

void QWpsDrawer::mouseReleaseEvent ( QMouseEvent * event ) {
    Q_UNUSED(event);
    /*int x = event->x() - (this->width()-pix->width())/2,
            y = event->y() - (this->height()-pix->height())/2;
    DEBUGF1("x=%d,y=%d",x,y);*/
}
void QWpsDrawer::newTempWps() {
    QTemporaryFile tmpWps;
    tmpWps.setAutoRemove(false);
    tmpWps.setFileTemplate(QDir::tempPath()+"/XXXXXXXXXX.wps");
    if (tmpWps.open()) {
        QString tmpDir = tmpWps.fileName().left(tmpWps.fileName().length()-4);
        if (QDir::temp().mkpath(tmpDir)) {
            mTmpWpsString = tmpDir;
            DEBUGF3(QString("Created :"+mTmpWpsString).toAscii());
        }
    }
}

void QWpsDrawer::WpsInit(QString buffer, bool isFile) {
	DEBUGF3("QWpsDrawer::WpsInit");
    if (!mResolved)
        if (!tryResolve())
            return;
    if (isFile) {
        cleanTemp();
        DEBUGF1( tr("Loading %1").arg(buffer));
        QFile file(buffer);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
            mWpsString = file.readAll();
        newTempWps();
    } else
        mWpsString = buffer;
    {
        QFile tfile(mTmpWpsString+".wps");
        if (tfile.open(QIODevice::WriteOnly | QIODevice::Text))
            tfile.write(mWpsString.toAscii(),mWpsString.length());
    }
    backdrop.fill(Qt::black);
    DEBUGF3("clear backdrop");
    if (isFile)
        lib_wps_init(buffer.toAscii(), &api, isFile);
    else
        lib_wps_init(QString(mTmpWpsString+".wps").toAscii(), &api, true);
    pix = new QPixmap(api.getwidth(),api.getheight());
    pix->fill(Qt::black);

    drawBackdrop();

    setMinimumWidth(api.getwidth());
    setMinimumHeight(api.getheight());
    update();
}

void QWpsDrawer::paintEvent(QPaintEvent * event) {
    DEBUGF3("QWpsDrawer::paintEvent()");
    if (!mResolved)
        return;
    if (pix==NULL)
        return;
    QPainter p(this);
    QRect rect = event->rect();

    drawBackdrop();
    lib_wps_refresh();

    if (showGrid) {
        QPainter g(pix);
        viewport_api avp;
        api.get_current_vp(&avp);

        g.setPen(Qt::green);

        for (int i=0;i*avp.fontheight/1.5<avp.width ;i++) {
            g.drawLine(int(i*avp.fontheight/1.5), 0, int(i*avp.fontheight/1.5), avp.height);
        }
        for (int j=0;j*avp.fontheight<avp.height; j++) {
            g.drawLine(0,j*avp.fontheight,avp.width,j*avp.fontheight);
        }
    }

    p.drawPixmap((rect.width()-pix->width())/2,(rect.height()-pix->height())/2,*pix);

}

void QWpsDrawer::clear_viewport(int x,int y,int w,int h, int color) {
    DEBUGF2("clear_viewport(int x=%d,int y=%d,int w=%d,int h=%d, int color)",x,y,w,h);
    QPainter p(pix);
    //p.setOpacity(0.1);
    //QImage img = backdrop.copy(x,y,w,h);
    //p.drawImage(x,y,img);
}

void QWpsDrawer::slotSetVolume() {
    Slider *slider = new Slider(this, tr("Volume"),-74,10);
    slider->show();
    connect(slider, SIGNAL(valueChanged(int)), wpsState, SLOT(setVolume(int)));
    connect(this, SIGNAL(destroyed()),slider, SLOT(close()));
}

void QWpsDrawer::slotSetProgress() {
    Slider *slider = new Slider(this,tr("Progress"),0,100);
    slider->show();
    connect(slider, SIGNAL(valueChanged(int)), trackState, SLOT(setElapsed(int)));
    connect(this, SIGNAL(destroyed()),slider, SLOT(close()));
}

void QWpsDrawer::slotWpsStateChanged(wpsstate ws_) {
    if (api.set_wpsstate)
        api.set_wpsstate(ws_);
    update();
}

void QWpsDrawer::slotTrackStateChanged(trackstate ms_) {
    if (api.set_wpsstate)
        api.set_trackstate(ms_);
    update();
}

void QWpsDrawer::slotShowGrid(bool show) {
    showGrid = show;
    update();
}

void QWpsDrawer::drawBackdrop() {
    DEBUGF3("QWpsDrawer::drawBackdrop()");
    if (backdrop.isNull())
    	return;
    QPainter b(pix);
    QImage pink = backdrop.createMaskFromColor(qRgb(255,0,255),Qt::MaskOutColor);
    backdrop.setAlphaChannel(pink);
    b.drawImage(0,0,backdrop,0,0,pix->width(),pix->height());
}

void QWpsDrawer::slotSetAudioStatus(int status) {
    api.set_audio_status(status);
    update();
}

void QWpsDrawer::cleanTemp(bool fileToo) {
    if (fileToo)
        QFile::remove(mTmpWpsString+".wps");
    QDirIterator it(mTmpWpsString, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QFile::remove(it.next());
    }
    QDir(mTmpWpsString).rmdir(mTmpWpsString);
}

void QWpsDrawer::closeEvent(QCloseEvent *event) {
    qDebug()<<"QWpsDrawer::closeEvent()";
    cleanTemp();
    event->accept();
}

QString QWpsDrawer::getModelName(QString libraryName) {
    QLibrary lib(libraryName);
    if ((pfget_model_name)lib.resolve("get_model_name"))
        return ((pfget_model_name)lib.resolve("get_model_name"))();
    DEBUGF1("ERR: failed to resolve <get_model_name>");
    return "unknown";
}

QList<QString> QWpsDrawer::getTargets() {
    QList<QString> list ;
    QDir d = QDir(qApp->applicationDirPath());
    QFileInfoList libs = d.entryInfoList(QStringList("libwps_*"));
    qDebug() << libs.size()<<"libs found";
    for (int i = 0; i < libs.size(); i++) {
        QString modelName = getModelName(libs[i].absoluteFilePath());
        qDebug() << libs[i].fileName()<<modelName;
        if (modelName == "unknown")
            continue;
        list.append(modelName);
        libs_array[i].target_name = modelName;
        libs_array[i].lib = libs[i].absoluteFilePath();
    }
    return list;
}
bool QWpsDrawer::setTarget(QString target) {
    foreach(lib_t cur_lib, libs_array)
    {
        if(cur_lib.target_name == target)
        {
            QLibrary lib(cur_lib.lib);
            //lib.unload();
            if (getModelName(cur_lib.lib) != "unknown")
            {
                mCurTarget = cur_lib.lib;
                return tryResolve();
            }
        }
    }
    return false;
}
