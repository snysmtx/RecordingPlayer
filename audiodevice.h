#ifndef AUDIODEVICE_H
#define AUDIODEVICE_H

#include <QIODevice>

class AudioDevice : public QIODevice
{
    Q_OBJECT

public:
    AudioDevice(const QByteArray &pcm);
    ~AudioDevice();

public:
    int getProgressRate();
    void setProgressRate(const int progress);

    // QIODevice interface
protected:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

private:
    QByteArray _dataPcm;    // 存放pcm数据
    int _writeLen;          // 记录已写入多少字节
};

#endif // AUDIODEVICE_H
