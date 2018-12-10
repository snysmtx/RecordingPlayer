#include "widget.h"
#include "ui_widget.h"

#include "waveform/wav-def.h"
#include "waveform/wav-file.h"

#include <QAudioFormat>
#include <QFileDialog>
#include <QPainter>
#include <QDebug>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , _pAudioOut(NULL)
    , _pDev(NULL)
{
    ui->setupUi(this);

    // create graph
    ui->customPlot->addGraph();

    connect(ui->playBtn, SIGNAL(clicked(bool)), this, SLOT(onPlayBtnClick()));
    connect(ui->stopBtn, SIGNAL(clicked(bool)), this, SLOT(onStopPlay()));

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

    QString dirPath;
    dirPath.append(QDir::homePath()).append("/Download");

    //设置默认文件路径
    fileDialog->setDirectory(dirPath);

    //设置文件过滤器
    fileDialog->setNameFilter(tr("Audio(*.wav *.mp3 *.pcm)"));

    //设置可以选择多个文件,默认为只能选择一个文件 QFileDialog::ExistingFiles
    // fileDialog->setFileMode(QFileDialog::ExistingFiles);

    //设置视图模式
    fileDialog->setViewMode(QFileDialog::Detail);

    //打印所有选择的文件的路径
    QStringList filePaths;

    if(fileDialog->exec())
        filePaths = fileDialog->selectedFiles();

    qDebug() << filePaths;

    Q_ASSERT(filePaths.size() <= 1);

    foreach (const QString &filePath, filePaths) {
        toCreateWaveform(filePath);
        toPlayFile(filePath);
        break;
    }
}

void Widget::onStopPlay()
{
    playOver();
}

void Widget::toPlayFile(const QString &filePath)
{
    //先把文件的pcm数据弄到内存数组里
    QByteArray barray;

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
        return;

    barray = f.readAll();
    f.close();

    WAV_HDR *pHDR = reinterpret_cast<WAV_HDR *>(barray.data());

    qDebug() << "ChunkID : " << pHDR->rID << endl
        << "ChunkSize : " << pHDR->rLen << endl
        << "Format : " << pHDR->wID << endl
        << "Subchunk1ID : " << pHDR->fId << endl
        << "Subchunk1Size: " << pHDR->pcmHeaderLength << endl
        << "AudioFormat: " << pHDR->wFormatTag << endl
        << "NumChannels: " << pHDR->numChannels << endl
        << "SampleRate: " << pHDR->nSamplesPerSec << endl
        << "ByteRate: " << pHDR->nAvgBytesPerSec << endl
        << "BlockAign: " << pHDR->numBlockAlingn << endl
        << "BitsPerSample: " << pHDR->numBitsPerSample;

    qDebug() << "------------------------------------";

    QAudioFormat fmt;

    fmt.setSampleRate(pHDR->nSamplesPerSec);
    fmt.setChannelCount(pHDR->numChannels);
    fmt.setSampleSize(pHDR->numBitsPerSample);
    fmt.setCodec("audio/pcm");

    if (_pAudioOut) {
        delete _pAudioOut;
        _pAudioOut = NULL;
    }

    if (_pDev) {
        delete _pDev;
        _pDev = NULL;
    }

    //创建声音输出对象并初始化
    _pAudioOut = new QAudioOutput(fmt, this);

    //创建自定义的IO设备
    _pDev = new AudioDevice(barray);
    _pAudioOut->start(_pDev);
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
    ui->customPlot->xAxis->setLabel("Time");
    ui->customPlot->yAxis->setLabel("Vol");

    // set axes ranges, so we see all data:
    ui->customPlot->xAxis->setRange(0, x.at(x.size() - 1));
    ui->customPlot->yAxis->setRange(minY - 0.1, maxY + 0.1);
    ui->customPlot->replot();
}
