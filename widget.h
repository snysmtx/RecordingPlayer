#ifndef WIDGET_H
#define WIDGET_H

#include "audiodevice.h"

#include <QWidget>

#include <QAudioOutput>
#include <QSharedPointer>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

public slots:
    void onPlayBtnClick();
    void onStopPlay();

private:
    void toPlayFile(const QString &filePath);
    void toCreateWaveform(const QString &filePath);
    void playOver();

    void paintWaveform(const QString &filePath);

private:
    Ui::Widget *ui;

    QAudioOutput *_pAudioOut;
    AudioDevice *_pDev;

    QImage _image;

    QString _dirPath;
};

#endif // WIDGET_H
