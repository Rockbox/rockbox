#include "qwpsdrawer.h"
#include <QPainter>
#include <QFile>
#include <QFileInfo>
#include "utils.h"
#include "api.h"

void QWpsDrawer::putsxy(int x, int y, const unsigned char *str) {
    DEBUGF3("putsxy(int x=%d, int y=%d, *str=%s)",x,y,str);
    QPainter p(pix);
    viewport_api avp;
    api.get_current_vp(&avp);
    p.setPen(Qt::gray);
    QFont font("times",avp.fontheight,QFont::Bold);
    p.setFont(font);
    p.drawText(x+avp.x,y + avp.fontheight + avp.y,(char*)str);
}
void QWpsDrawer::transparent_bitmap_part(const void *src, int src_x, int src_y,
        int stride, int x, int y, int width, int height) {
    QImage img;
    img.load((char*)src);
    DEBUGF2("transparent_bitmap_part(const void *src=%s, int src_x=%d, int src_y=%d,int stride=%d, int x=%d, int y=%d, int width=%d, int height=%d",(char*)src,src_x, src_y,stride, x, y, width, height);
    QPainter p(pix);
    QPoint target(x,y);
    QRectF source(src_x, src_y, width, height);

    QImage pink = img.createMaskFromColor(qRgb(255,0,255),Qt::MaskOutColor);
    img.setAlphaChannel(pink);

    p.drawImage(target, img, source);
}
void QWpsDrawer::bitmap_part(const void *src, int src_x, int src_y,
                             int stride, int x, int y, int width, int height) {
    transparent_bitmap_part(src,src_x,src_y,stride,x,y,width,height);
}
void QWpsDrawer::drawpixel(int x, int y) {
    QPainter p(pix);
    p.setPen(Qt::blue);
    p.drawPoint(x,y);
}
void QWpsDrawer::fillrect(int x, int y, int width, int height) {
    QPainter p(pix);
    DEBUGF2("fillrect(int x=%d, int y=%d, int width=%d, int height=%d)\n",x, y, width, height);
    p.setPen(Qt::green);
}
void QWpsDrawer::hline(int x1, int x2, int y) {
    QPainter p(pix);
    p.setPen(Qt::black);
    p.drawLine(x1,y,x2,y);
}
void QWpsDrawer::vline(int x, int y1, int y2) {
    QPainter p(pix);
    p.setPen(Qt::black);
    p.drawLine(x,y1,x,y2);
}
bool QWpsDrawer::load_wps_backdrop(char* filename) {
    DEBUGF3("load backdrop: %s", filename);
    QFile file(filename);
    QFileInfo info(file);
    file.copy(mTmpWpsString+"/"+info.fileName());
    backdrop.load(filename);
    return true;
}

int QWpsDrawer::read_bmp_file(const char* filename,int *width, int *height) {
    QImage img;

    QFile file(filename);
    QFileInfo info(file);
    file.copy(mTmpWpsString+"/"+info.fileName());

    img.load(filename);
    *width = img.width();
    *height = img.height();
    return 1;
}
