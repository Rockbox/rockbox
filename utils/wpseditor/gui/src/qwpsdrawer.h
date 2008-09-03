#ifndef WPSDRAWER_H
#define WPSDRAWER_H

#include <QWidget>
#include <QPixmap>
#include <QPointer>
#include <QTemporaryFile>

#include "wpsstate.h"

struct proxy_api;

class QWpsState;
class QTrackState;

typedef int (*pfwps_init)(const char* buff,struct proxy_api *api, bool isfile);
typedef int (*pfwps_display)();
typedef int (*pfwps_refresh)();
typedef const char* (*pfget_model_name)();

class QWpsDrawer : public QWidget {
    Q_OBJECT

    pfwps_init       lib_wps_init;
    pfwps_display    lib_wps_display;
    pfwps_refresh    lib_wps_refresh;
    pfget_model_name lib_get_model_name;

    static QPixmap    *pix;
    static QImage     backdrop;

    QWpsState         *wpsState;
    QTrackState       *trackState;

    bool              showGrid;
    bool              mResolved;
    QString           mWpsString;
    QString           mCurTarget;
    static QString    mTmpWpsString;


protected:
    virtual void paintEvent(QPaintEvent * event);
    virtual void closeEvent(QCloseEvent *event);
    virtual void mouseReleaseEvent ( QMouseEvent * event ) ;
    void drawBackdrop();
    void newTempWps();
    void cleanTemp(bool fileToo=true);
    bool tryResolve();
    QString getModelName(QString libraryName);
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
    QList<QString> getTargets();
    bool setTarget(QString target);


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
