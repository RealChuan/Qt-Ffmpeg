#include "mainwindow.hpp"
#include "previewwidget.hpp"
#include "sourcewidget.hpp"

#include <ffmpeg/averror.h>
#include <ffmpeg/event/errorevent.hpp>
#include <ffmpeg/event/trackevent.hpp>
#include <ffmpeg/event/valueevent.hpp>
#include <ffmpeg/ffmpegutils.hpp>
#include <ffmpeg/transcoder.hpp>
#include <ffmpeg/widgets/mediainfodialog.hpp>

#include <QtWidgets>

#define BUTTON_SIZE QSize(100, 35)

class MainWindow::MainWindowPrivate
{
public:
    explicit MainWindowPrivate(MainWindow *q)
        : q_ptr(q)
    {
        Ffmpeg::printFfmpegInfo();

        transcoder = new Ffmpeg::Transcoder(q_ptr);

        sourceWidget = new SourceWidget(q_ptr);
        tabWidget = new QTabWidget(q_ptr);
        previewWidget = new PreviewWidget(q_ptr);
        tabWidget->addTab(previewWidget,
                          QCoreApplication::translate("MainWindowPrivate", "Preview"));

        subtitleTextEdit = new QTextEdit(q_ptr);
        outTextEdit = new QTextEdit(q_ptr);

        audioCodecCbx = new QComboBox(q_ptr);
        audioCodecCbx->setView(new QListView(audioCodecCbx));
        audioCodecCbx->setMaxVisibleItems(10);
        audioCodecCbx->setStyleSheet("QComboBox {combobox-popup:0;}");
        auto audioCodecs = Ffmpeg::getCurrentSupportCodecs(AVMEDIA_TYPE_AUDIO, true);
        for (auto iter = audioCodecs.cbegin(); iter != audioCodecs.cend(); ++iter) {
            audioCodecCbx->addItem(iter.value(), iter.key());
        }
        audioCodecCbx->setCurrentIndex(audioCodecCbx->findData(AV_CODEC_ID_AAC));

        videoCodecCbx = new QComboBox(q_ptr);
        videoCodecCbx->setView(new QListView(videoCodecCbx));
        videoCodecCbx->setMaxVisibleItems(10);
        videoCodecCbx->setStyleSheet("QComboBox {combobox-popup:0;}");
        auto videoCodecs = Ffmpeg::getCurrentSupportCodecs(AVMEDIA_TYPE_VIDEO, true);
        for (auto iter = videoCodecs.cbegin(); iter != videoCodecs.cend(); ++iter) {
            videoCodecCbx->addItem(iter.value(), iter.key());
        }
        videoCodecCbx->setCurrentIndex(videoCodecCbx->findData(AV_CODEC_ID_H264));

        quailtySbx = new QSpinBox(q_ptr);
        quailtySbx->setRange(2, 31);
        quailtySbx->setToolTip(tr("smaller -> better"));
        crfSbx = new QSpinBox(q_ptr);
        crfSbx->setRange(0, 51);
        crfSbx->setToolTip(tr("smaller -> better"));
        crfSbx->setValue(18);
        presetCbx = new QComboBox(q_ptr);
        presetCbx->setView(new QListView(presetCbx));
        presetCbx->addItems(transcoder->presets());
        presetCbx->setCurrentText(transcoder->preset());
        tuneCbx = new QComboBox(q_ptr);
        tuneCbx->setView(new QListView(tuneCbx));
        tuneCbx->addItems(transcoder->tunes());
        tuneCbx->setCurrentText(transcoder->tune());
        profileCbx = new QComboBox(q_ptr);
        profileCbx->setView(new QListView(profileCbx));
        profileCbx->addItems(transcoder->profiles());
        profileCbx->setCurrentText(transcoder->profile());

        widthLineEdit = new QLineEdit(q_ptr);
        widthLineEdit->setValidator(new QIntValidator(0, INT_MAX, widthLineEdit));
        heightLineEdit = new QLineEdit(q_ptr);
        heightLineEdit->setValidator(new QIntValidator(0, INT_MAX, heightLineEdit));
        keepAspectRatioCkb = new QCheckBox(tr("keepAspectRatio"), q_ptr);
        keepAspectRatioCkb->setChecked(true);

        videoMinBitrateLineEdit = new QLineEdit(q_ptr);
        videoMaxBitrateLineEdit = new QLineEdit(q_ptr);

        startButton = new QToolButton(q_ptr);
        startButton->setText(QObject::tr("Start"));
        startButton->setMinimumSize(BUTTON_SIZE);
        progressBar = new QProgressBar(q_ptr);
        progressBar->setRange(0, 100);
        fpsLabel = new QLabel(q_ptr);
        fpsLabel->setToolTip(QObject::tr("Video Encoder FPS."));
        fpsTimer = new QTimer(q_ptr);
    }

    [[nodiscard]] auto initVideoSetting() const -> QGroupBox *
    {
        auto *layout1 = new QHBoxLayout;
        layout1->addWidget(new QLabel(tr("Width:"), q_ptr));
        layout1->addWidget(widthLineEdit);
        layout1->addWidget(new QLabel(tr("height:"), q_ptr));
        layout1->addWidget(heightLineEdit);
        layout1->addWidget(keepAspectRatioCkb);
        auto *layout2 = new QHBoxLayout;
        layout2->addWidget(new QLabel(tr("Min Bitrate:"), q_ptr));
        layout2->addWidget(videoMinBitrateLineEdit);
        layout2->addWidget(new QLabel(tr("Max Bitrate:"), q_ptr));
        layout2->addWidget(videoMaxBitrateLineEdit);

        auto *groupBox = new QGroupBox(tr("Video"), q_ptr);
        auto *layout = new QVBoxLayout(groupBox);
        layout->addLayout(layout1);
        layout->addLayout(layout2);
        return groupBox;
    }

    [[nodiscard]] auto invalidSetting() const -> QGroupBox *
    {
        auto *groupBox = new QGroupBox(tr("Invalid setting"), q_ptr);
        auto *layout = new QHBoxLayout(groupBox);
        layout->addWidget(new QLabel(tr("Quality:"), q_ptr));
        layout->addWidget(quailtySbx);
        layout->addWidget(new QLabel(tr("Crf:"), q_ptr));
        layout->addWidget(crfSbx);
        layout->addWidget(new QLabel(tr("Preset:"), q_ptr));
        layout->addWidget(presetCbx);
        layout->addWidget(new QLabel(tr("Tune:"), q_ptr));
        layout->addWidget(tuneCbx);
        layout->addWidget(new QLabel(tr("Profile:"), q_ptr));
        layout->addWidget(profileCbx);
        return groupBox;
    }

    void initInputFileAttribute(const QString &filePath) const
    {
        transcoder->setInFilePath(filePath);
        transcoder->startPreviewFrames(10);
        transcoder->parseInputFile();

        QSize size;
        double frameRate = 0;
        QString format;
        int videoCount = 0;
        int audioCount = 0;
        int subtitleCount = 0;

        auto mediaInfo = transcoder->mediaInfo();
        auto streamInfos = mediaInfo.streamInfos;
        for (const auto &streamInfo : std::as_const(streamInfos)) {
            switch (streamInfo.mediaType) {
            case AVMEDIA_TYPE_VIDEO:
                videoCount++;
                size = streamInfo.size;
                frameRate = streamInfo.frameRate;
                format = streamInfo.format;
                break;
            case AVMEDIA_TYPE_AUDIO: audioCount++; break;
            case AVMEDIA_TYPE_SUBTITLE: subtitleCount++; break;
            default: break;
            }
        }

        auto info = QCoreApplication::translate(
                        "MainWindowPrivate",
                        "%1, Duration: %2, %3, %4x%5, %6FPS, %7 video, %8 audio, %9 subtitle")
                        .arg(QFileInfo(mediaInfo.url).fileName(),
                             mediaInfo.durationText,
                             format,
                             QString::number(size.width()),
                             QString::number(size.height()),
                             QString::number(frameRate),
                             QString::number(videoCount),
                             QString::number(audioCount),
                             QString::number(subtitleCount));
        sourceWidget->setFileInfo(info);
        sourceWidget->setDuration(mediaInfo.duration / 1000);
    }

    void calBitrate() const
    {
        auto w = widthLineEdit->text().toInt();
        auto h = heightLineEdit->text().toInt();
        videoMinBitrateLineEdit->setText(QString::number(w * h));
        videoMaxBitrateLineEdit->setText(QString::number(w * h * 4));
    }

    MainWindow *q_ptr;

    Ffmpeg::Transcoder *transcoder;

    SourceWidget *sourceWidget;
    QTabWidget *tabWidget;
    PreviewWidget *previewWidget;

    QTextEdit *subtitleTextEdit;
    QTextEdit *outTextEdit;

    QComboBox *audioCodecCbx;
    QComboBox *videoCodecCbx;
    QSpinBox *quailtySbx;
    QSpinBox *crfSbx;
    QComboBox *presetCbx;
    QComboBox *tuneCbx;
    QComboBox *profileCbx;

    QLineEdit *widthLineEdit;
    QLineEdit *heightLineEdit;
    QSize originalSize = QSize(-1, -1);
    QCheckBox *keepAspectRatioCkb;
    QLineEdit *videoMinBitrateLineEdit;
    QLineEdit *videoMaxBitrateLineEdit;

    QToolButton *startButton;
    QProgressBar *progressBar;
    QLabel *fpsLabel;
    QTimer *fpsTimer;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , d_ptr(new MainWindowPrivate(this))
{
    setupUI();
    buildConnect();

    resize(1000, 618);
}

MainWindow::~MainWindow() = default;

void MainWindow::onVideoEncoderChanged()
{
    auto quantizer = Ffmpeg::getCodecQuantizer(d_ptr->videoCodecCbx->currentText());
    d_ptr->quailtySbx->setRange(quantizer.first, quantizer.second);
}

void MainWindow::onOpenInputFile()
{
    const auto path = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)
                          .value(0, QDir::homePath());
    const auto filePath
        = QFileDialog::getOpenFileName(this,
                                       tr("Open File"),
                                       path,
                                       tr("Audio Video (*.mp3 *.mp4 *.mkv *.rmvb)"));
    if (filePath.isEmpty()) {
        return;
    }

    d_ptr->sourceWidget->setSource(filePath);

    d_ptr->initInputFileAttribute(filePath);
}

void MainWindow::onResetConfig()
{
    auto filePath = d_ptr->sourceWidget->source();
    if (filePath.isEmpty()) {
        qWarning() << "filePath.isEmpty()";
        return;
    }
    if (QFile::exists(filePath)) {
        d_ptr->initInputFileAttribute(filePath);
        return;
    }
    QUrl url(filePath);
    if (!url.isValid()) {
        qWarning() << "!url.isValid()";
        return;
    }
    d_ptr->initInputFileAttribute(url.toEncoded());
}

void MainWindow::onOpenSubtitle()
{
    const QString path = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)
                             .value(0, QDir::homePath());
    const QString filePath = QFileDialog::getOpenFileName(this,
                                                          tr("Open File"),
                                                          path,
                                                          tr("Audio Video (*.srt *.ass *.txt)"));
    if (filePath.isEmpty()) {
        return;
    }

    d_ptr->subtitleTextEdit->setPlainText(filePath);
}

void MainWindow::onOpenOutputFile()
{
    const QString path = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)
                             .value(0, QDir::homePath());
    const QString filePath
        = QFileDialog::getSaveFileName(this,
                                       tr("Save File"),
                                       path,
                                       tr("Audio Video (*.mp3 *.mp4 *.mkv *.rmvb)"));
    if (filePath.isEmpty()) {
        return;
    }

    d_ptr->outTextEdit->setPlainText(filePath);
}

void MainWindow::onStart()
{
    if (d_ptr->startButton->text() == tr("Start")) {
        auto inPath = d_ptr->sourceWidget->source();
        auto subtitlePath = d_ptr->subtitleTextEdit->toPlainText();
        auto outPath = d_ptr->outTextEdit->toPlainText();
        if (inPath.isEmpty() || outPath.isEmpty()) {
            return;
        }

        d_ptr->transcoder->setInFilePath(inPath);
        d_ptr->transcoder->setOutFilePath(outPath);
        d_ptr->transcoder->setAudioEncodecName(d_ptr->audioCodecCbx->currentText());
        d_ptr->transcoder->setVideoEncodecName(d_ptr->videoCodecCbx->currentText());
        d_ptr->transcoder->setSize(
            {d_ptr->widthLineEdit->text().toInt(), d_ptr->heightLineEdit->text().toInt()});
        if (QFile::exists(subtitlePath)) {
            d_ptr->transcoder->setSubtitleFilename(subtitlePath);
        }
        d_ptr->transcoder->setQuailty(d_ptr->quailtySbx->value());
        d_ptr->transcoder->setMinBitrate(d_ptr->videoMinBitrateLineEdit->text().toInt());
        d_ptr->transcoder->setMaxBitrate(d_ptr->videoMaxBitrateLineEdit->text().toInt());
        d_ptr->transcoder->setCrf(d_ptr->crfSbx->value());
        d_ptr->transcoder->setPreset(d_ptr->presetCbx->currentText());
        d_ptr->transcoder->setProfile(d_ptr->profileCbx->currentText());
        d_ptr->transcoder->startTranscode();
        d_ptr->startButton->setText(tr("Stop"));

        auto filename = QFile::exists(inPath) ? QFileInfo(inPath).fileName()
                                              : QFileInfo(QUrl(inPath).toString()).fileName();
        setWindowTitle(filename);

        d_ptr->fpsTimer->start(1000);

    } else if (d_ptr->startButton->text() == tr("Stop")) {
        d_ptr->transcoder->stopTranscode();
        d_ptr->progressBar->setValue(0);
        d_ptr->startButton->setText(tr("Start"));

        d_ptr->fpsTimer->stop();
    }
}

void MainWindow::onShowMediaInfo()
{
    Ffmpeg::MediaInfoDialog dialog(this);
    dialog.setMediaInfo(d_ptr->transcoder->mediaInfo());
    dialog.exec();
}

void MainWindow::onProcessEvents()
{
    while (d_ptr->transcoder->propertyChangeEventSize() > 0) {
        auto eventPtr = d_ptr->transcoder->takePropertyChangeEvent();
        switch (eventPtr->type()) {
        case Ffmpeg::PropertyChangeEvent::EventType::Position: {
            // auto *positionEvent = dynamic_cast<Ffmpeg::PositionEvent *>(eventPtr.data());
            // d_ptr->controlWidget->setPosition(positionEvent->position() / AV_TIME_BASE);
        } break;
        case Ffmpeg::PropertyChangeEvent::MediaTrack: {
            bool audioSet = false;
            bool videoSet = false;
            auto *mediaTrackEvent = dynamic_cast<Ffmpeg::MediaTrackEvent *>(eventPtr.data());
            auto tracks = mediaTrackEvent->tracks();
            for (const auto &track : std::as_const(tracks)) {
                switch (track.mediaType) {
                case AVMEDIA_TYPE_AUDIO:
                    if (!audioSet) {
                        auto index = d_ptr->audioCodecCbx->findData(track.codecId);
                        if (index > 0) {
                            d_ptr->audioCodecCbx->setCurrentIndex(index);
                            audioSet = true;
                        }
                    }
                    break;
                case AVMEDIA_TYPE_VIDEO:
                    if (!videoSet) {
                        auto index = d_ptr->videoCodecCbx->findData(track.codecId);
                        if (index > 0) {
                            d_ptr->videoCodecCbx->setCurrentIndex(index);
                        }
                        videoSet = true;
                        d_ptr->widthLineEdit->blockSignals(true);
                        d_ptr->heightLineEdit->blockSignals(true);
                        d_ptr->widthLineEdit->setText(QString::number(track.size.width()));
                        d_ptr->heightLineEdit->setText(QString::number(track.size.height()));
                        d_ptr->originalSize = track.size;
                        d_ptr->widthLineEdit->blockSignals(false);
                        d_ptr->heightLineEdit->blockSignals(false);
                        d_ptr->calBitrate();
                    }
                    break;
                default: break;
                }
            }
        } break;
        case Ffmpeg::PropertyChangeEvent::EventType::PreviewFramesChanged:
            d_ptr->previewWidget->setFrames(d_ptr->transcoder->previewFrames());
            break;
        case Ffmpeg::PropertyChangeEvent::Error: {
            auto *errorEvent = dynamic_cast<Ffmpeg::ErrorEvent *>(eventPtr.data());
            const auto text = tr("Error[%1]:%2.")
                                  .arg(QString::number(errorEvent->error().errorCode()),
                                       errorEvent->error().errorString());
            qWarning() << text;
        }
        default: break;
        }
    }
}

void MainWindow::setupUI()
{
    auto *fileMenu = new QMenu(tr("File"), this);
    fileMenu->addAction(tr("Open In"), this, &MainWindow::onOpenInputFile);
    fileMenu->addAction(tr("Reset Config"), this, &MainWindow::onResetConfig);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Exit"), qApp, &QApplication::quit, Qt::QueuedConnection);
    menuBar()->addMenu(fileMenu);

    auto *subtitleBtn = new QToolButton(this);
    subtitleBtn->setText(tr("Add Subtitle"));
    subtitleBtn->setMinimumSize(BUTTON_SIZE);
    connect(subtitleBtn, &QToolButton::clicked, this, &MainWindow::onOpenSubtitle);
    auto *outBtn = new QToolButton(this);
    outBtn->setText(tr("Open Out"));
    outBtn->setMinimumSize(BUTTON_SIZE);
    connect(outBtn, &QToolButton::clicked, this, &MainWindow::onOpenOutputFile);

    auto *editLayout = new QGridLayout;
    editLayout->addWidget(d_ptr->subtitleTextEdit, 1, 0, 1, 1);
    editLayout->addWidget(subtitleBtn, 1, 1, 1, 1);
    editLayout->addWidget(d_ptr->outTextEdit, 2, 0, 1, 1);
    editLayout->addWidget(outBtn, 2, 1, 1, 1);

    auto *groupLayout1 = new QHBoxLayout;
    groupLayout1->addWidget(new QLabel(tr("Audio Codec Name:"), this));
    groupLayout1->addWidget(d_ptr->audioCodecCbx);
    groupLayout1->addWidget(new QLabel(tr("Video Codec Name:"), this));
    groupLayout1->addWidget(d_ptr->videoCodecCbx);
    auto *groupBox = new QGroupBox(tr("Encoder Settings"), this);
    auto *groupLayout = new QVBoxLayout(groupBox);
    groupLayout->addLayout(groupLayout1);
    groupLayout->addWidget(d_ptr->invalidSetting());
    groupLayout->addWidget(d_ptr->initVideoSetting());

    auto *useGpuCheckBox = new QCheckBox(tr("GPU Decode"), this);
    useGpuCheckBox->setToolTip(tr("GPU Decode"));
    useGpuCheckBox->setChecked(true);
    connect(useGpuCheckBox, &QCheckBox::clicked, this, [this, useGpuCheckBox] {
        d_ptr->transcoder->setUseGpuDecode(useGpuCheckBox->isChecked());
    });
    d_ptr->transcoder->setUseGpuDecode(useGpuCheckBox->isChecked());
    auto *displayLayout = new QHBoxLayout;
    displayLayout->addWidget(d_ptr->startButton);
    displayLayout->addWidget(useGpuCheckBox);
    displayLayout->addWidget(d_ptr->progressBar);
    displayLayout->addWidget(d_ptr->fpsLabel);

    auto *widget = new QWidget(this);
    auto *layout = new QVBoxLayout(widget);
    layout->addWidget(d_ptr->sourceWidget);
    layout->addWidget(d_ptr->tabWidget);
    setCentralWidget(widget);

    auto *tempWidget = new QWidget(this);
    auto *tempLayout = new QVBoxLayout(tempWidget);
    tempLayout->addLayout(editLayout);
    tempLayout->addWidget(groupBox);
    tempLayout->addLayout(displayLayout);
    d_ptr->tabWidget->addTab(tempWidget, tr("Temp Widget"));
}

void MainWindow::buildConnect()
{
    connect(d_ptr->transcoder,
            &Ffmpeg::Transcoder::eventIncrease,
            this,
            &MainWindow::onProcessEvents);

    connect(d_ptr->sourceWidget, &SourceWidget::showMediaInfo, this, &MainWindow::onShowMediaInfo);

    connect(d_ptr->videoCodecCbx,
            &QComboBox::currentTextChanged,
            this,
            &MainWindow::onVideoEncoderChanged);

    connect(d_ptr->widthLineEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (!d_ptr->keepAspectRatioCkb->isChecked() || !d_ptr->originalSize.isValid()) {
            return;
        }
        auto multiple = d_ptr->originalSize.width() * 1.0 / text.toInt();
        int height = d_ptr->originalSize.height() / multiple;
        d_ptr->heightLineEdit->blockSignals(true);
        d_ptr->heightLineEdit->setText(QString::number(height));
        d_ptr->heightLineEdit->blockSignals(false);
        d_ptr->calBitrate();
    });
    connect(d_ptr->heightLineEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (!d_ptr->keepAspectRatioCkb->isChecked() || !d_ptr->originalSize.isValid()) {
            return;
        }
        auto multiple = d_ptr->originalSize.height() * 1.0 / text.toInt();
        int width = d_ptr->originalSize.width() / multiple;
        d_ptr->widthLineEdit->blockSignals(true);
        d_ptr->widthLineEdit->setText(QString::number(width));
        d_ptr->widthLineEdit->blockSignals(false);
        d_ptr->calBitrate();
    });
    connect(d_ptr->keepAspectRatioCkb, &QCheckBox::stateChanged, this, [this] {
        if (!d_ptr->keepAspectRatioCkb->isChecked() || !d_ptr->originalSize.isValid()) {
            return;
        }
        auto multiple = d_ptr->originalSize.width() * 1.0 / d_ptr->widthLineEdit->text().toInt();
        int height = d_ptr->originalSize.height() / multiple;
        d_ptr->heightLineEdit->blockSignals(true);
        d_ptr->heightLineEdit->setText(QString::number(height));
        d_ptr->heightLineEdit->blockSignals(false);
        d_ptr->calBitrate();
    });

    connect(d_ptr->startButton, &QToolButton::clicked, this, &MainWindow::onStart);
    connect(d_ptr->transcoder, &Ffmpeg::Transcoder::progressChanged, this, [this](qreal value) {
        d_ptr->progressBar->setValue(value * 100);
    });
    connect(d_ptr->transcoder, &Ffmpeg::Transcoder::finished, this, [this] {
        if (d_ptr->startButton->text() == tr("Stop")) {
            d_ptr->startButton->click();
        }
    });
    connect(d_ptr->fpsTimer, &QTimer::timeout, this, [this] {
        auto str = QString("FPS: %1").arg(QString::number(d_ptr->transcoder->fps(), 'f', 2));
        d_ptr->fpsLabel->setText(str);
    });
}
