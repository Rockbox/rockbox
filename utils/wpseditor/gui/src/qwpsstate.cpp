#include "qwpsstate.h"

QWpsState::QWpsState(): QObject() {
    state.fontheight = 8;
    state.fontwidth = 5;
    state.volume = -30;
    state.battery_level = 50;

}

void QWpsState::setFontHeight(int val) {
    state.fontheight = val;
    emit stateChanged(state);
}

void QWpsState::setFontWidth(int val) {
    state.fontwidth = val;
    emit stateChanged(state);
}

void QWpsState::setVolume(int val) {
    state.volume = val;
    emit stateChanged(state);
}

void QWpsState::setBattery(int val) {
    state.battery_level = val;
    emit stateChanged(state);
}
