#ifndef WPSDRAWER_H
#define WPSDRAWER_H
//
#include <QWidget>
#include <QPixmap>
#include <QPointer>
#include <QTemporaryFile>
#include "api.h"
#include "qtrackstate.h"
#include "qwpsstate.h"
//

typedef int (*pfwps_init)(const char* buff,struct proxy_api *api, bool isfile);
typedef int (*pfwps_display)();
typedef int (*pfwps_refresh)();

class QWpsDrawer : public QWidget {
    Q_OBJECT

    pfwps_init    wps_init;
    pfwps_display wps_display;
    pfwps_refresh wps_refresh;

    static QPixmap    *pix;
    static QImage     backdrop;

    QWpsState         *wpsState;
    QTrackState       *trackState;

    bool              showGrid;
    bool              mResolved;
    QString           mWpsString;
    QString           mTargetLibName;
    static QString    mTmpWpsString;


protected:
    virtual void paintEvent(QPaintEvent * event);
    virtual void closeEvent(QCloseEvent *event);
    virtual void mouseReleaseEvent ( QMouseEvent * event ) ;
    void drawBackdrop();
    void newTempWps();
    void cleanTemp(bool fileToo=true);
    bool tryResolve();
public:
    QWpsDrawer(QWpsState *ws,QTrackState *ms, QWidget *parent=0);
    ~QWpsDrawer();
    void WpsInit(QString buffer, bool isFile = true);

    QString wpsString() const {
        return mWpsString;
    };
    QString tempWps() const {
        return mTmpWpsString;
    };


    static proxy_api api;
    /***********Drawing api******************/
    static void putsxy(int x, int y, const unsigned char *str);
    static void transparent_bitmap_part(const void *src, int src_x, int src_y,
                                        int stride, int x, int y, int width, int height);
    static void bitmap_part(const void *src, int src_x, int src_y,
                            int stride, int x, int y, int width, int height);
    static void drawpixel(int x, int y);
    static void fillrect(int x, int y, int width, int height);
    static void hline(int x1, int x2, int y);
    static void vline(int x, int y1, int y2);
    static void clear_viewport(int x,int y,int w,int h, int color);
    static bool load_wps_backdrop(char* filename);
    static int  read_bmp_file(const char* filename,int *width, int *height);
    /****************************************/
public slots:
    void slotSetVolume();
    void slotSetProgress();

    void slotShowGrid(bool);
    void slotWpsStateChanged(wpsstate);
    void slotTrackStateChanged(trackstate);
    void slotSetAudioStatus(int);
};
#endif
