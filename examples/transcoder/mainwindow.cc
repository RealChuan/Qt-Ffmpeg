#include "mainwindow.hpp"

#include <ffmpeg/avversion.hpp>
#include <ffmpeg/transcode.hpp>

#include <QtWidgets>

#define BUTTON_SIZE QSize(100, 35)

class MainWindow::MainWindowPrivate
{
public:
    MainWindowPrivate(QWidget *parent)
        : owner(parent)
    {
        transcode = new Ffmpeg::Transcode(owner);

        inTextEdit = new QTextEdit(owner);
        outTextEdit = new QTextEdit(owner);
        startButton = new QToolButton(owner);
        startButton->setText(QObject::tr("Start"));
        startButton->setMinimumSize(BUTTON_SIZE);
        progressBar = new QProgressBar(owner);
        progressBar->setRange(0, 100);
    }

    QWidget *owner;

    Ffmpeg::Transcode *transcode;

    QTextEdit *inTextEdit;
    QTextEdit *outTextEdit;
    QToolButton *startButton;
    QProgressBar *progressBar;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , d_ptr(new MainWindowPrivate(this))
{
    Ffmpeg::printFfmpegInfo();

    setupUI();
    buildConnect();
    resize(700, 430);
}

MainWindow::~MainWindow() {}

void MainWindow::onOpenInputFile()
{
    const QString path = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)
                             .value(0, QDir::homePath());
    const QString filePath
        = QFileDialog::getOpenFileName(this,
                                       tr("Open File"),
                                       path,
                                       tr("Audio Video (*.mp3 *.mp4 *.mkv *.rmvb)"));
    if (filePath.isEmpty()) {
        return;
    }

    d_ptr->inTextEdit->setPlainText(filePath);
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
    auto inPath = d_ptr->inTextEdit->toPlainText();
    if (!QFile::exists(inPath)) {
        QMessageBox::warning(this, tr("Not exist"), tr("Input file path does not exist!"));
        return;
    }
    auto outPath = d_ptr->outTextEdit->toPlainText();

    d_ptr->transcode->setInFilePath(inPath);
    d_ptr->transcode->setOutFilePath(outPath);
    d_ptr->transcode->setVideoEnCodecID(AV_CODEC_ID_MJPEG);
    d_ptr->transcode->startTranscode();
    d_ptr->startButton->setEnabled(false);
}

void MainWindow::setupUI()
{
    auto inBtn = new QToolButton(this);
    inBtn->setText(tr("Open In"));
    inBtn->setMinimumSize(BUTTON_SIZE);
    connect(inBtn, &QToolButton::clicked, this, &MainWindow::onOpenInputFile);
    auto outBtn = new QToolButton(this);
    outBtn->setText(tr("Open Out"));
    outBtn->setMinimumSize(BUTTON_SIZE);
    connect(outBtn, &QToolButton::clicked, this, &MainWindow::onOpenOutputFile);

    auto editLayout = new QGridLayout;
    editLayout->addWidget(d_ptr->inTextEdit, 0, 0, 1, 1);
    editLayout->addWidget(inBtn, 0, 1, 1, 1);
    editLayout->addWidget(d_ptr->outTextEdit, 1, 0, 1, 1);
    editLayout->addWidget(outBtn, 1, 1, 1, 1);

    auto displayLayout = new QHBoxLayout;
    displayLayout->addWidget(d_ptr->startButton);
    displayLayout->addWidget(d_ptr->progressBar);

    auto widget = new QWidget(this);
    auto layout = new QVBoxLayout(widget);
    layout->addLayout(editLayout);
    layout->addLayout(displayLayout);
    setCentralWidget(widget);
}

void MainWindow::buildConnect()
{
    connect(d_ptr->startButton, &QToolButton::clicked, this, &MainWindow::onStart);
    connect(d_ptr->transcode, &Ffmpeg::Transcode::progressChanged, this, [this](qreal value) {
        d_ptr->progressBar->setValue(value * 100);
    });
    connect(d_ptr->transcode, &Ffmpeg::Transcode::finished, this, [this] {
        d_ptr->startButton->setEnabled(true);
    });
}
