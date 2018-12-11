#include "customslider.h"

#include <QMouseEvent>

CustomSlider::CustomSlider(QWidget *parent)
    : QSlider(parent)
{

}

void CustomSlider::mousePressEvent(QMouseEvent *ev)
{
    QSlider::mousePressEvent(ev);

    double pos = ev->pos().x() / (double)width();
    int progress = pos * (maximum() - minimum()) + minimum();
    setValue(progress);

    emit costomSliderClicked();
}
