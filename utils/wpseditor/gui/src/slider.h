#ifndef SLIDERIMPL_H
#define SLIDERIMPL_H
//
#include <QWidget>
#include <QDialog>
#include "ui_slider.h"
//
class Slider : public QDialog , Ui::slider {
    Q_OBJECT
public slots:
    void slotValueChanged(int step);
signals:
    void valueChanged(int);
public:
    Slider(QWidget *parent, QString caption, int min, int max );
    int value();



};
#endif
