#include "qwpsdrawer.h"
#include "slider.h"
#include "utils.h"
#include <QtGui>
#include <QLibrary>
#include <stdarg.h>
//


QPointer<QWpsDrawer> drawer;
QPixmap   *QWpsDrawer::pix = NULL;
QString   QWpsDrawer::mTmpWpsString;
QImage    QWpsDrawer::backdrop;
proxy_api QWpsDrawer::api;

QWpsDrawer::QWpsDrawer( QWpsState *ws,QTrackState *ms, QWidget *parent )
        : QWidget(parent),wpsState(ws),trackState(ms),showGrid(false),mTargetLibName("libwps") {

    tryResolve();
    memset(&api,0,sizeof(struct proxy_api));

    api.verbose = 2;
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
    newTempWps();
}

bool QWpsDrawer::tryResolve() {
    QLibrary lib(qApp->applicationDirPath()+"/"+mTargetLibName);
    wps_init = (pfwps_init)lib.resolve("wps_init");
    wps_display = (pfwps_display)lib.resolve("wps_display");
    wps_refresh = (pfwps_refresh)lib.resolve("wps_refresh");
    mResolved = wps_init && wps_display && wps_refresh;
    if (!mResolved)
        DEBUGF1(tr("ERR: Failed to resolve funcs!"));
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
    QTemporaryFile    tmpWps;
    tmpWps.setAutoRemove(false);
    tmpWps.setFileTemplate(QDir::tempPath()+"/XXXXXXXXXX.wps");
    if (tmpWps.open()) {
        QString tmpDir = tmpWps.fileName().left(tmpWps.fileName().length()-4);
        if (QDir::temp().mkpath(tmpDir)) {
            mTmpWpsString = tmpDir;
            DEBUGF1(mTmpWpsString);
        }
    }
}

void QWpsDrawer::WpsInit(QString buffer, bool isFile) {

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

    if (isFile)
        wps_init(buffer.toAscii(), &api, isFile);
    else
        wps_init(QString(mTmpWpsString+".wps").toAscii(), &api, true);
    pix = new QPixmap(api.getwidth(),api.getheight());

    drawBackdrop();

    setMinimumWidth(api.getwidth());
    setMinimumHeight(api.getheight());

    update();
}

void QWpsDrawer::paintEvent(QPaintEvent * event) {
    if (!mResolved)
        return;
    if (pix==NULL)
        return;
    QPainter p(this);
    QRect rect = event->rect();

    drawBackdrop();
    wps_refresh();

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
    QPainter b(pix);
    QImage pink = backdrop.createMaskFromColor(qRgb(255,0,255),Qt::MaskOutColor);
    backdrop.setAlphaChannel(pink);
    b.drawImage(0,0,backdrop);
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
