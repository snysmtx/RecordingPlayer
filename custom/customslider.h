#ifndef CUSTOMSLIDER_H
#define CUSTOMSLIDER_H

#include <QSlider>

class CustomSlider : public QSlider
{
    Q_OBJECT
public:
    CustomSlider(QWidget *parent = 0);

protected:
    void mousePressEvent(QMouseEvent *ev);

signals:
    void costomSliderClicked();
};

#endif // CUSTOMSLIDER_H
