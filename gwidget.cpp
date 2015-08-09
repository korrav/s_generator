#include "gwidget.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <QDebug>
#include <QApplication>
#include "fcntl.h"
#include <limits>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QFileDialog>
#include <QSizePolicy>
#include <QValidator>
#include <functional>

void addButtonRunning(QPushButton &but, const QString& image)
{
    QPixmap pix(image);
    but.setIcon(pix);
    but.setIconSize(pix.size()/2);
    but.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    but.setCheckable(true);
}


void GWidget::writeUnit()
{
    struct {
        qint64 atime;
        qint64 dtime;
        qint16 data[4];
    } dataBuf;

    dataBuf.atime = getAtime();
    dataBuf.dtime = 1e6 * getDtime();
    Data curData = pgsampl_->getDataSamples();
    std::copy(curData.begin(), curData.end(), dataBuf.data);

    qint64 countByteToTransfer = sizeof(dataBuf);
    char drain[countByteToTransfer];
    qint64 countBytePerOneTransfer = 0;
    while(countByteToTransfer) {
        countBytePerOneTransfer = fifo_.write(reinterpret_cast<char*>(&dataBuf), countByteToTransfer);
        if(countBytePerOneTransfer >= 0 ) {
            if(countBytePerOneTransfer != countByteToTransfer)
                fifo_.read(drain, sizeof(dataBuf));
            countByteToTransfer -= countBytePerOneTransfer;
        }
        else if(errno == EAGAIN) {
            fifo_.read(drain, sizeof(dataBuf));
        } else {
            qDebug() << "Запись в именованный канала завершился ошибкой: " << errno;
            qApp->exit(1);
            break;
        }

    }
    return;
}

void GWidget::setAtime(qint64 a)
{
    atime_ = a;
}

void GWidget::setGeneratorSamples(std::shared_ptr<Samples> pg)
{
    pgsampl_ = pg;
    //    qDebug() << "Values fix samples: ";
    //    for (int i = 1; i< 10; i++) {
    //        qDebug() << i << " sampl";
    //        for(auto val: pgsampl_->getDataSamples())
    //            qDebug() << val;
    //    }
}

void GWidget::stopGeneratorSamples()
{
    killTimer(idTimer_);
}

void GWidget::activateControls(bool status)
{
    gSelectDtime_.setEnabled(status);
    gModeGenerateSamples_.setEnabled(status);
    gSnapSamples_.setEnabled(status);
    gRunning_.setEnabled(status);
}

void GWidget::runGeneratorSamples(bool status)
{
    gNameFifo_.setEnabled(!status);
    gSelectDtime_.setEnabled(!status);
    gModeGenerateSamples_.setEnabled(!status);

    if(status)
        idTimer_ = startTimer(dtime_);
    else
        stopGeneratorSamples();
}

void GWidget::selectEditorFixSamplesModes()
{
    for(int i = 0; i < 4; i++)
        edFixSamples_[i].setEnabled(true);
    butEnterFixSamples_.setEnabled(true);

    labMeanRandomSamples_.setEnabled(false);
    labSigmaRandomSamples_.setEnabled(false);
    edMeanRandomSamples_.setEnabled(false);
    edSigmaRandomSamples_.setEnabled(false);
    butEnterRandomSamples_.setEnabled(false);
}

void GWidget::setIntervalRandomParameter(QObject *pobj)
{
    int minValue = 0, maxValue = 0;
    if(pobj == &edMeanRandomSamples_) {
        if(edSigmaRandomSamples_.text().isEmpty()) {
            minValue = std::numeric_limits<qint16>::min();
            maxValue = std::numeric_limits<qint16>::max();
        } else {
            int sigma = edSigmaRandomSamples_.text().toInt();
            int amplitude = 6 * sigma;
            minValue = std::numeric_limits<qint16>::min() + amplitude;
            maxValue = std::numeric_limits<qint16>::max() - amplitude;
        }
        //        qDebug() << "mean: min = " << minValue << " max = " << maxValue;
        edMeanRandomSamples_.setValidator(new QIntValidator(minValue, maxValue, this));
    } else if(pobj == &edSigmaRandomSamples_) {
        minValue = 0;
        if(edMeanRandomSamples_.text().isEmpty())
            maxValue = std::numeric_limits<qint16>::max();
        else {
            int mean = edMeanRandomSamples_.text().toInt();
            int amplitude = std::min(std::numeric_limits<qint16>::max() - mean, mean - std::numeric_limits<qint16>::min());
            maxValue = amplitude/6;
        }
        //        qDebug() << "sigma: min = " << minValue << " max = " << maxValue;
        edSigmaRandomSamples_.setValidator(new QIntValidator(minValue, maxValue, this));
    }
}

void GWidget::selectEditorRandomSamplesModes()
{
    labMeanRandomSamples_.setEnabled(true);
    labSigmaRandomSamples_.setEnabled(true);
    edMeanRandomSamples_.setEnabled(true);
    edSigmaRandomSamples_.setEnabled(true);
    butEnterRandomSamples_.setEnabled(true);

    for(int i = 0; i < 4; i++)
        edFixSamples_[i].setEnabled(false);
    butEnterFixSamples_.setEnabled(false);
}

void GWidget::timerEvent(QTimerEvent *event)
{
    if(event->timerId() == idTimer_)
        writeUnit();
}

qint64 GWidget::getAtime(void)
{
    return atime_++;
}

qint64 GWidget::getDtime()
{
    return dtime_;
}

void GWidget::createFifo(QString name)
{
    if(fifo_.exists())
        fifo_.remove();
    if (mkfifo(name.toStdString().c_str(), 0777)) {
        qDebug() << "Именнованный канал создать не удалось: " << errno;
        emit statusCompleteCreateFifo(false);
        edNameFifo_.clear();
        return;
    }
    fifo_.setFileName(name);
    fifo_.open(QIODevice::ReadWrite);
    int fdesc = fifo_.handle();
    if(fcntl(fdesc, F_SETFL, O_NONBLOCK)) {
        qDebug() << "Не удалось ввести дескриптор именованного канала в неблокирующийся режим: " << errno;
        emit statusCompleteCreateFifo(false);
        edNameFifo_.clear();
        return;
    }
    stream_.setDevice(&fifo_);
    edNameFifo_.setText(name);
    emit statusCompleteCreateFifo(true);
    return;
}

void GWidget::changeNameFifoInEditor()
{
    createFifo(edNameFifo_.text());
}

void GWidget::selectFullNameFifo()
{
    QString curDirName = "/tmp";
    if(fifo_.exists())
        curDirName = QFileInfo(fifo_).path();
    QString fileName = QFileDialog::getSaveFileName(this, "Create Fifo", curDirName);
    if(!fileName.isEmpty())
        edNameFifo_.setText(fileName);
}

void GWidget::setSnapUnit(Samples::SnapData ps)
{
    pgsampl_->setSnap(ps);
}

void GWidget::setDtime(qint64 t)
{
    int temp = std::min<qint64>(std::numeric_limits<int>::max(), t);
    temp = std::max(0, temp);
    dtime_ = temp;
    //    qDebug() << "dtime = " << dtime_;
}

void GWidget::createControlsSelectDtime()
{
    edSelectDtime_.setAlignment(Qt::AlignRight);
    edSelectDtime_.setFixedSize(edSelectDtime_.sizeHint());
    QIntValidator *pValidator = new QIntValidator(0, std::numeric_limits<int>::max(), this);
    edSelectDtime_.setValidator(pValidator);
    labSelectDtime_.setText("sampling &period (ms)");
    labSelectDtime_.setBuddy(&edSelectDtime_);
    butEnterDtime_.setText("Enter");
    QHBoxLayout *phlDtime = new QHBoxLayout;
    phlDtime->addStretch();
    phlDtime->addWidget(&labSelectDtime_);
    phlDtime->addWidget(&edSelectDtime_);
    phlDtime->addWidget(&butEnterDtime_);
    gSelectDtime_.setTitle("Select sampling period");
    gSelectDtime_.setLayout(phlDtime);

    connect(&butEnterDtime_, &QPushButton::clicked,
            [this]() {
        if(!edSelectDtime_.text().isEmpty()) {
            qint64 dtime = edSelectDtime_.text().toLongLong();
            setDtime(dtime);
        }
        return;
    });
}

void GWidget::createControlsFifo(void)
{
    butSelectFifo_.setText("Select...");
    butEnterNameFifo_.setText("Enter");
    QHBoxLayout *phlNameFifo = new QHBoxLayout;
    phlNameFifo->addWidget(&edNameFifo_);
    phlNameFifo->addWidget(&butSelectFifo_);
    phlNameFifo->addWidget(&butEnterNameFifo_);
    gNameFifo_.setTitle("&Create fifo channel");
    gNameFifo_.setLayout(phlNameFifo);
    connect(&edNameFifo_, SIGNAL(returnPressed()), SLOT(changeNameFifoInEditor()));
    connect(&butSelectFifo_, SIGNAL(clicked()), SLOT(selectFullNameFifo()));
    connect(&butEnterNameFifo_, SIGNAL(clicked()), SLOT(changeNameFifoInEditor()));
    connect(this, SIGNAL(statusCompleteCreateFifo(bool)), SLOT(activateControls(bool)));
}

void GWidget::createControlsModeGenerateSamples(void)
{
    QHBoxLayout *phlFixSample = new QHBoxLayout;
    butFixSamples_.setText("&Fix");
    QVBoxLayout *pvlEditFixSample = new QVBoxLayout;
    QIntValidator* pValidator = new QIntValidator(std::numeric_limits<qint16>::min(),
                                                  std::numeric_limits<qint16>::max(), this);
    for(int i = 0; i < 4; i++) {
        edFixSamples_[i].setAlignment(Qt::AlignRight);
        edFixSamples_[i].setFixedSize(edFixSamples_[i].sizeHint());
        edFixSamples_[i].setValidator(pValidator);
        pvlEditFixSample->addWidget(&edFixSamples_[i]);
    }
    butEnterFixSamples_.setText("Enter");
    phlFixSample->addWidget(&butFixSamples_);
    phlFixSample->addStretch();
    phlFixSample->addLayout(pvlEditFixSample);
    phlFixSample->addWidget(&butEnterFixSamples_);
    QHBoxLayout *phlRandomSample = new QHBoxLayout;
    butRandomSamples_.setText("&Random");
    labMeanRandomSamples_.setText("&mean");
    labMeanRandomSamples_.setBuddy(&edMeanRandomSamples_);
    labSigmaRandomSamples_.setText("&sigma");
    labSigmaRandomSamples_.setBuddy(&edSigmaRandomSamples_);
    butEnterRandomSamples_.setText("Enter");
    edMeanRandomSamples_.setAlignment(Qt::AlignRight);
    edMeanRandomSamples_.setFixedSize(edMeanRandomSamples_.sizeHint());
    edSigmaRandomSamples_.setAlignment(Qt::AlignRight);
    edSigmaRandomSamples_.setFixedSize(edSigmaRandomSamples_.sizeHint());
    phlRandomSample->addWidget(&butRandomSamples_);
    phlRandomSample->addStretch();
    phlRandomSample->addWidget(&labMeanRandomSamples_);
    phlRandomSample->addWidget(&edMeanRandomSamples_);
    phlRandomSample->addWidget(&labSigmaRandomSamples_);
    phlRandomSample->addWidget(&edSigmaRandomSamples_);
    phlRandomSample->addWidget(&butEnterRandomSamples_);
    QVBoxLayout *pvlModeGenerateSamples = new QVBoxLayout;
    pvlModeGenerateSamples->addLayout(phlFixSample);
    pvlModeGenerateSamples->addLayout(phlRandomSample);
    gModeGenerateSamples_.setTitle("Mode &generate");
    gModeGenerateSamples_.setLayout(pvlModeGenerateSamples);
    connect(&butFixSamples_, SIGNAL(clicked()), SLOT(selectEditorFixSamplesModes()));
    connect(&butRandomSamples_, SIGNAL(clicked()), SLOT(selectEditorRandomSamplesModes()));
    butFixSamples_.click();

    FocusFilter* fFilter = new FocusFilter();
    edMeanRandomSamples_.installEventFilter(fFilter);
    edSigmaRandomSamples_.installEventFilter(fFilter);
    connect(fFilter, SIGNAL(selectFocus(QObject*)), this, SLOT(setIntervalRandomParameter(QObject*)));

    connect(&butEnterFixSamples_, &QPushButton::clicked, [this]() {
        for(auto& ed : edFixSamples_)
            if(ed.text().isEmpty())
                return;
        Data samples;
        std::transform(edFixSamples_, edFixSamples_ + 4, samples.begin(), [](QLineEdit& e){
            return e.text().toShort();});
        setGeneratorSamples(std::shared_ptr<Samples>(new FixSamples(samples)));
    });
    connect(&butEnterRandomSamples_, &QPushButton::clicked, [this]() {
        if(!edMeanRandomSamples_.text().isEmpty() && !edSigmaRandomSamples_.text().isEmpty()) {
            qint16 sigma = edSigmaRandomSamples_.text().toShort();
            qint16 mean = edMeanRandomSamples_.text().toShort();
            //            RandomSamples* pgenerator = new RandomSamples(mean, sigma);
            //            setGeneratorSamples(std::shared_ptr<Samples>(pgenerator));
            setGeneratorSamples(std::shared_ptr<Samples>(new RandomSamples(mean,sigma)));
        }
    });
}

void GWidget::createControlsSnapSamples()
{
    QVBoxLayout *pvlEditSnapSamples = new QVBoxLayout;
    butEnterSnapSamples_.setText("Enter");
    QIntValidator* pValidator = new QIntValidator(std::numeric_limits<qint16>::min(),
                                                  std::numeric_limits<qint16>::max(), this);
    for(int i = 0; i < 4; i++) {
        edSnapSamples_[i].setAlignment(Qt::AlignRight);
        edSnapSamples_[i].setFixedSize(edSnapSamples_[i].sizeHint());
        edSnapSamples_[i].setValidator(pValidator);
        pvlEditSnapSamples->addWidget(&edSnapSamples_[i]);
    }
    QHBoxLayout *phlSnapSamples = new QHBoxLayout;
    phlSnapSamples->addStretch();
    phlSnapSamples->addLayout(pvlEditSnapSamples);
    phlSnapSamples->addWidget(&butEnterSnapSamples_);
    gSnapSamples_.setTitle("&Launch snappet samples");
    gSnapSamples_.setLayout(phlSnapSamples);

    connect(&butEnterSnapSamples_, &QPushButton::clicked, [this]() {
        Samples::SnapData data(new Samples::SnapData::element_type);
        for(int i = 0; i < 4; i++) {
            if(!edSnapSamples_[i].text().isEmpty())
                (*data)[i] =  edSnapSamples_[i].text().toShort();
        }
        setSnapUnit(data);
    });
}

void GWidget::createControlsRunning()
{
    QHBoxLayout *phlRunning = new QHBoxLayout;
    addButtonRunning(butRun_, ":/media-playback-start.png");
    addButtonRunning(butStop_, ":/media-playback-stop.png");
    butStop_.setChecked(true);
    phlRunning->addWidget(&butRun_);
    phlRunning->addWidget(&butStop_);
    gRunning_.setTitle("R&unning");
    gRunning_.setLayout(phlRunning);
    bgRunning_.addButton(&butRun_);
    bgRunning_.addButton(&butStop_);
    connect(&butRun_, SIGNAL(toggled(bool)), SLOT(runGeneratorSamples(bool)));
}

GWidget::GWidget(QWidget *parent)
    : QWidget(parent), atime_(0), dtime_(1)
{
    createControlsFifo();
    createControlsSelectDtime();
    createControlsModeGenerateSamples();
    createControlsSnapSamples();
    createControlsRunning();

    QVBoxLayout *pvl = new QVBoxLayout;
    pvl->addWidget(&gNameFifo_);
    pvl->addWidget(&gSelectDtime_);
    pvl->addWidget(&gModeGenerateSamples_);
    pvl->addWidget(&gSnapSamples_);
    pvl->addWidget(&gRunning_);
    setLayout(pvl);

    setFixedSize(minimumSizeHint());

    Data d;
    d.fill(DEFAULT_SAMPLE_PERIOD);
    pgsampl_.reset(new FixSamples(d));
    createFifo(DEFAULT_NAME_FIFO);

}

GWidget::~GWidget()
{
    fifo_.remove();
}
