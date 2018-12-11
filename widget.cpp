#include "widget.h"
#include "ui_widget.h"

#include "waveform/wav-def.h"
#include "waveform/wav-file.h"

#include <QAudioFormat>
#include <QFileDialog>
#include <QPainter>
#include <QDebug>

#define PLAY_FINISH_PROGRESS 100

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , _pAudioOut(NULL)
    , _pDev(NULL)
{
    ui->setupUi(this);

    // create graph
    ui->customPlot->addGraph();

    ui->slider->setRange(0, 100);
    ui->slider->setEnabled(false);
    ui->slider->setFocusPolicy(Qt::NoFocus);

    _timer = new QTimer();
    _timer->setInterval(1000);

    _format.setSampleRate(8000);
    _format.setChannelCount(1);
    _format.setSampleSize(8);
    _format.setCodec("audio/pcm");
    _format.setByteOrder(QAudioFormat::LittleEndian);
    _format.setSampleType(QAudioFormat::SignedInt);

    connect(ui->playBtn, SIGNAL(clicked(bool)), this, SLOT(onPlayBtnClick()));
    connect(ui->stopBtn, SIGNAL(clicked(bool)), this, SLOT(onStopPlay()));

    connect(_timer, SIGNAL(timeout()), this, SLOT(onTimeOut()));

    connect(ui->slider, SIGNAL(costomSliderClicked()), this, SLOT(onSliderProgressClicked()));
    connect(ui->slider, SIGNAL(sliderMoved(int)), this, SLOT(onSliderProgressMoved()));
    connect(ui->slider, SIGNAL(sliderReleased()), this, SLOT(onSliderProgressReleased()));

    resize(800, 400);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::onPlayBtnClick()
{
    //定义文件对话框类
    QFileDialog *fileDialog = new QFileDialog(this);
    //定义文件对话框标题
    fileDialog->setWindowTitle(tr("Open File"));

    if (_dirPath.isEmpty())
        _dirPath.append(QDir::homePath()).append("/tmp/AudioSource");

    //设置默认文件路径
    fileDialog->setDirectory(_dirPath);

    //设置文件过滤器
    fileDialog->setNameFilter(tr("Audio(*.wav *.mp3 *.pcm *.au)"));

    //设置可以选择多个文件,默认为只能选择一个文件 QFileDialog::ExistingFiles
    // fileDialog->setFileMode(QFileDialog::ExistingFiles);

    //设置视图模式
    fileDialog->setViewMode(QFileDialog::Detail);

    //打印所有选择的文件的路径
    QStringList filePaths;

    if(fileDialog->exec()) {
        _dirPath = fileDialog->directory().path();
        filePaths = fileDialog->selectedFiles();
    }

    qDebug() << filePaths;

    Q_ASSERT(filePaths.size() <= 1);

    foreach (const QString &filePath, filePaths) {
        // toCreateWaveform(filePath);
        toPlayFile(filePath);
        break;
    }
}

void Widget::onStopPlay()
{
    playOver();
}

void Widget::onTimeOut()
{
    qDebug() << "time out, progress rate:" << _pDev->getProgressRate();

    ui->slider->setValue(_pDev->getProgressRate());

    if (_pDev->getProgressRate() == PLAY_FINISH_PROGRESS) {
        _timer->stop();
        ui->slider->setValue(0);
        ui->slider->setEnabled(false);
    }
}

void Widget::onSliderProgressClicked()
{
    qDebug() << ui->slider->value();

    _pAudioOut->suspend();
    _pDev->setProgressRate(ui->slider->value());
    _pAudioOut->resume();
}

void Widget::onSliderProgressMoved()
{
    if (_timer->isActive())
        _timer->stop();

    _pAudioOut->suspend();

    qDebug() << ui->slider->value();
}

void Widget::onSliderProgressReleased()
{
    qDebug() << ui->slider->value();
    _timer->start();

    _pAudioOut->resume();
}

typedef struct
{
    uint32_t magic;       /* magic number */
    uint32_t hdr_size;    /* size of this header */
    uint32_t data_size;   /* length of data (optional) */
    uint32_t encoding;    /* data encoding format */
    uint32_t sample_rate; /* samples per second */
    uint32_t channels;    /* number of interleaved channels */
} Audio_filehdr;

#include "g711.h"

void Widget::toPlayFile(const QString &filePath)
{
    //先把文件的pcm数据弄到内存数组里
    QByteArray bary;

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
        return;

    bary = f.readAll();
    f.close();

    QDataStream stream(&bary, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    Audio_filehdr hdr;

    stream >> hdr.magic >> hdr.hdr_size >> hdr.data_size >> hdr.encoding >> hdr.sample_rate >> hdr.channels;

    qDebug() << hdr.hdr_size << endl
             << hdr.sample_rate << endl
             << hdr.channels << endl
             << hdr.encoding;
//    qDebug() << barray.toHex();

    // char *buffer = new char[];

    QByteArray buffer;

    char buf[2048];

    int dataSize = 0;
    while (dataSize < bary.size()) {

        G711Decode(buf, bary.data() + dataSize, 1024, 0);

        buffer.append(QByteArray(buf, 2048));

        dataSize += 1024;
    }

    qDebug() << bary.size() << "," << buffer.size();

//    return;

    if (_pAudioOut) {
        delete _pAudioOut;
        _pAudioOut = NULL;
    }

    if (_pDev) {
        delete _pDev;
        _pDev = NULL;
    }

    QAudioDeviceInfo info (QAudioDeviceInfo::defaultOutputDevice());
    if(! info.isFormatSupported(_format))
    {
        _format = info.nearestFormat(_format);
    }

    //创建声音输出对象并初始化
    _pAudioOut = new QAudioOutput(info, _format, this);

    //创建自定义的IO设备
    _pDev = new AudioDevice(buffer);
    _pAudioOut->start(_pDev);

    _timer->start();
    ui->slider->setEnabled(true);
}

void Widget::toCreateWaveform(const QString &filePath)
{
    WavFile* pObjWavFile;
    pObjWavFile = new WavFile;
    if (EXIT_SUCCESS != pObjWavFile->openWavFile(const_cast<char *>(filePath.toStdString().c_str()))) {
        std::cout << "\nCan't load wav file.";
        return;
    }

    QString outPath = filePath;
    outPath.append(".csv");

    pObjWavFile->writeDataToFile(const_cast<char *>(outPath.toStdString().c_str()));

    paintWaveform(outPath);
}

void Widget::playOver()
{
    if (_pAudioOut)
        _pAudioOut->stop();

    _timer->stop();
}

void Widget::paintWaveform(const QString &filePath)
{
//    ui->customPlot->yAxis->grid()->setSubGridVisible(true);

    QVector<double> x, y; // initialize with entries

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly)) {
        qDebug()<<"OPEN FILE FAILED";
        return;
    }

    QTextStream * out = new QTextStream(&file); //文本流

    QString tmpX, tmpY;
    double minY = 0.0, maxY = 0.0;
    bool begin = true;

    while (!out->atEnd()) {
        QStringList tempbar = out->readLine().split(",");
        // qDebug() << "x:" << tempbar.at(0) << "y:" << tempbar.at(1);

        tmpX = tempbar.at(0);
        tmpY = tempbar.at(1);

        if (begin) {
            minY = tmpY.toDouble();
            begin = false;
        }

        if (minY > tmpY.toDouble())
            minY = tmpY.toDouble();

        if (maxY < tmpY.toDouble())
            maxY = tmpY.toDouble();

        x.push_back(tmpX.toDouble());
        y.push_back(tmpY.toDouble());
    }

    file.close();

    // assign data to it:
    ui->customPlot->graph(0)->setData(x, y);

    // give the axes some labels:
    // ui->customPlot->xAxis->setLabel("Time");
    // ui->customPlot->yAxis->setLabel("Vol");

    // set axes ranges, so we see all data:
    ui->customPlot->xAxis->setRange(0, x.at(x.size() - 1));
    ui->customPlot->yAxis->setRange(minY - 0.1, maxY + 0.1);
    ui->customPlot->replot();
}
