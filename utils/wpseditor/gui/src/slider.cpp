#include "slider.h"
#include <QDebug>
//
Slider::Slider(QWidget *parent, QString caption, int min, int max ):QDialog(parent),mCaption(caption) {
    setupUi ( this );
    connect(horslider, SIGNAL(valueChanged(int)), this, SIGNAL(valueChanged(int)));
    connect(this, SIGNAL(valueChanged(int)), this, SLOT(slotValueChanged(int)));
    setWindowTitle(mCaption);
    horslider->setMinimum(min);
    horslider->setMaximum(max);
}
//
int Slider::value() {
    return horslider->value();
}
void Slider::slotValueChanged(int step) {
    setWindowTitle(tr("%1 = %2 ").arg(mCaption).arg(step));
}



