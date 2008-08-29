#ifndef __WPSSTATE_H__
#define __WPSSTATE_H__

#include <QObject>
#include "wpsstate.h"

class QWpsState : public QObject {
    Q_OBJECT


    Q_CLASSINFO("QWpsState", "WPS State");
    Q_PROPERTY(int FontHeight READ fontHeight WRITE setFontHeight DESIGNABLE true USER true)
    Q_CLASSINFO("FontHeight", "minimum=6;maximum=20;value=10");
    Q_PROPERTY(int FontWidth READ fontWidth WRITE setFontWidth DESIGNABLE true USER true)
    Q_CLASSINFO("FontWidth", "minimum=4;maximum=20;value=8");
    Q_PROPERTY(int Volume READ volume WRITE setVolume DESIGNABLE true USER true)
    Q_CLASSINFO("Volume", "minimum=-74;maximum=24;value=-15");
    Q_PROPERTY(int Battery READ battery WRITE setBattery DESIGNABLE true USER true)
    Q_CLASSINFO("Battery", "minimum=0;maximum=100;value=50");

    wpsstate state;

public:
    QWpsState();

    int fontHeight() const {
        return state.fontheight;
    }
    void setFontHeight(int val);

    int fontWidth() const {
        return state.fontwidth;
    }
    void setFontWidth(int val);

    int battery() const {
        return state.battery_level;
    }
    void setBattery(int val);

    int volume() const {
        return state.volume;
    }
public slots:
    void setVolume(int val);





signals:
    void stateChanged ( wpsstate state );
};
#endif // __WPSSTATE_H__
