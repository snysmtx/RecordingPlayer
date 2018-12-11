#include "audiodevice.h"

#include <QDebug>

#define T_UNUSED(c)            static_cast<void>(c)

AudioDevice::AudioDevice(const QByteArray &pcm)
    : _dataPcm(pcm)
{
    open(QIODevice::ReadOnly); // 为了解决QIODevice::read (QIODevice): device not open
    _writeLen = 0;
}

AudioDevice::~AudioDevice()
{
    close();
}

int AudioDevice::getProgressRate()
{
    return (static_cast<double>(_writeLen) / static_cast<double>(_dataPcm.size()) * 100);
}

void AudioDevice::setProgressRate(const int progress)
{
    double progressRate = static_cast<double>(progress) / 100;
    qDebug() << "progressRate" << progressRate;

    _writeLen = _dataPcm.size() * progressRate;
}

qint64 AudioDevice::readData(char *data, qint64 maxlen)
{
    if (_writeLen >= _dataPcm.size())
        return 0;

    int len;

    //计算未播放的数据的长度
    len = (_writeLen + maxlen) > _dataPcm.size() ? (_dataPcm.size() - _writeLen) : maxlen;

    //把要播放的pcm数据存入声卡缓冲区里
    memcpy(data, _dataPcm.data() + _writeLen, len);

    // 更新已播放的数据长度
    _writeLen += len;

    qDebug() << "wriLen:" << _writeLen << " dataLen:" << _dataPcm.size();

    return len;
}

qint64 AudioDevice::writeData(const char *data, qint64 len)
{
    T_UNUSED(data);
    T_UNUSED(len);

    return 0;
}
