#include "videoencoderwidget.hpp"

#include <ffmpeg/ffmpegutils.hpp>

#include <QtWidgets>

class VideoEncoderWidget::VideoEncoderWidgetPrivate
{
public:
    explicit VideoEncoderWidgetPrivate(VideoEncoderWidget *q)
        : q_ptr(q)
    {
        const auto *const comboBoxStyleSheet{"QComboBox {combobox-popup:0;}"};
        videoEncoderCbx = new QComboBox(q_ptr);
        videoEncoderCbx->setView(new QListView(videoEncoderCbx));
        videoEncoderCbx->setMaxVisibleItems(10);
        videoEncoderCbx->setStyleSheet(comboBoxStyleSheet);
        auto videoCodecs = Ffmpeg::getCurrentSupportCodecs(AVMEDIA_TYPE_VIDEO, true);
        for (auto iter = videoCodecs.cbegin(); iter != videoCodecs.cend(); ++iter) {
            videoEncoderCbx->addItem(iter.value(), iter.key());
        }
        videoEncoderCbx->setCurrentIndex(videoEncoderCbx->findData(AV_CODEC_ID_H264));
        videoEncoderCbx->model()->sort(0);

        gpuEncodeCbx = new QCheckBox(QCoreApplication::translate("VideoEncoderWidgetPrivate",
                                                                 "Use GPU Encode"),
                                     q_ptr);
        gpuEncodeCbx->setChecked(true);
        gpuDecodeCbx = new QCheckBox(QCoreApplication::translate("VideoEncoderWidgetPrivate",
                                                                 "Use GPU Decode"),
                                     q_ptr);
        gpuDecodeCbx->setChecked(true);

        quailtySbx = new QSpinBox(q_ptr);
        quailtySbx->setRange(2, 31);
        quailtySbx->setToolTip(
            QCoreApplication::translate("VideoEncoderWidgetPrivate", "smaller -> better"));
        crfSbx = new QSpinBox(q_ptr);
        crfSbx->setRange(0, 51);
        crfSbx->setToolTip(
            QCoreApplication::translate("VideoEncoderWidgetPrivate", "smaller -> better"));
        crfSbx->setValue(18);
        presetCbx = new QComboBox(q_ptr);
        presetCbx->setView(new QListView(presetCbx));
        tuneCbx = new QComboBox(q_ptr);
        tuneCbx->setView(new QListView(tuneCbx));
        profileCbx = new QComboBox(q_ptr);
        profileCbx->setView(new QListView(profileCbx));

        widthSbx = new QSpinBox(q_ptr);
        widthSbx->setRange(0, INT_MAX);
        heightSbx = new QSpinBox(q_ptr);
        heightSbx->setRange(0, INT_MAX);
        aspectCheckBox = new QCheckBox(QCoreApplication::translate("VideoEncoderWidgetPrivate",
                                                                   "Keep aspect ratio:"),
                                       q_ptr);
        aspectCheckBox->setChecked(true);

        minBitrateSbx = new QSpinBox(q_ptr);
        minBitrateSbx->setRange(0, INT_MAX);
        maxBitrateSbx = new QSpinBox(q_ptr);
        maxBitrateSbx->setRange(0, INT_MAX);
    }

    void calBitrate() const
    {
        auto w = widthSbx->value();
        auto h = heightSbx->value();
        minBitrateSbx->setValue(w * h);
        maxBitrateSbx->setValue(w * h * 4);
    }

    VideoEncoderWidget *q_ptr;

    QComboBox *videoEncoderCbx;
    QCheckBox *gpuEncodeCbx;
    QCheckBox *gpuDecodeCbx;

    QSpinBox *quailtySbx;
    QSpinBox *crfSbx;
    QComboBox *presetCbx;
    QComboBox *tuneCbx;
    QComboBox *profileCbx;

    QSpinBox *widthSbx;
    QSpinBox *heightSbx;
    QCheckBox *aspectCheckBox;

    QSpinBox *minBitrateSbx;
    QSpinBox *maxBitrateSbx;

    QSize originalSize;
};

VideoEncoderWidget::VideoEncoderWidget(QWidget *parent)
    : QWidget{parent}
    , d_ptr(new VideoEncoderWidgetPrivate(this))
{
    setupUI();
    buildConnect();
}

VideoEncoderWidget::~VideoEncoderWidget() = default;

auto VideoEncoderWidget::setEncoder(AVCodecID codecId) -> bool
{
    auto index = d_ptr->videoEncoderCbx->findData(codecId);
    auto finded = (index >= 0);
    if (finded) {
        d_ptr->videoEncoderCbx->setCurrentIndex(index);
    }
    return finded;
}

auto VideoEncoderWidget::encoder() const -> QString
{
    return d_ptr->videoEncoderCbx->currentText();
}

void VideoEncoderWidget::setPreset(const QStringList &presets, const QString &current)
{
    d_ptr->presetCbx->clear();
    d_ptr->presetCbx->addItems(presets);
    d_ptr->presetCbx->setCurrentText(current);
}

auto VideoEncoderWidget::preset() const -> QString
{
    return d_ptr->presetCbx->currentText();
}

void VideoEncoderWidget::setTune(const QStringList &tunes, const QString &current)
{
    d_ptr->tuneCbx->clear();
    d_ptr->tuneCbx->addItems(tunes);
    d_ptr->tuneCbx->setCurrentText(current);
}

auto VideoEncoderWidget::tune() const -> QString
{
    return d_ptr->tuneCbx->currentText();
}

void VideoEncoderWidget::setProfile(const QStringList &profiles, const QString &current)
{
    d_ptr->profileCbx->clear();
    d_ptr->profileCbx->addItems(profiles);
    d_ptr->profileCbx->setCurrentText(current);
}

auto VideoEncoderWidget::profile() const -> QString
{
    return d_ptr->profileCbx->currentText();
}

void VideoEncoderWidget::setVideoSize(const QSize &size)
{
    d_ptr->widthSbx->blockSignals(true);
    d_ptr->heightSbx->blockSignals(true);
    d_ptr->widthSbx->setValue(size.width());
    d_ptr->heightSbx->setValue(size.height());
    d_ptr->widthSbx->blockSignals(false);
    d_ptr->heightSbx->blockSignals(false);

    d_ptr->originalSize = size;
    d_ptr->calBitrate();
}

auto VideoEncoderWidget::videoSize() const -> QSize
{
    return {d_ptr->widthSbx->value(), d_ptr->heightSbx->value()};
}

auto VideoEncoderWidget::quality() const -> int
{
    return d_ptr->quailtySbx->value();
}

auto VideoEncoderWidget::minBitrate() const -> int
{
    return d_ptr->minBitrateSbx->value();
}

auto VideoEncoderWidget::gpuEncode() const -> bool
{
    return d_ptr->gpuEncodeCbx->isChecked();
}

auto VideoEncoderWidget::gpuDecode() const -> bool
{
    return d_ptr->gpuDecodeCbx->isChecked();
}

auto VideoEncoderWidget::maxBitrate() const -> int
{
    return d_ptr->maxBitrateSbx->value();
}

auto VideoEncoderWidget::crf() const -> int
{
    return d_ptr->crfSbx->value();
}

void VideoEncoderWidget::onEncoderChanged()
{
    auto quantizer = Ffmpeg::getCodecQuantizer(d_ptr->videoEncoderCbx->currentText());
    if (quantizer.first < 0 || quantizer.second < 0) {
        return;
    }
    d_ptr->quailtySbx->setRange(quantizer.first, quantizer.second);
}

void VideoEncoderWidget::onVideoWidthChanged()
{
    if (!d_ptr->aspectCheckBox->isChecked() || !d_ptr->originalSize.isValid()) {
        return;
    }
    auto multiple = d_ptr->originalSize.width() * 1.0 / d_ptr->widthSbx->value();
    int height = d_ptr->originalSize.height() / multiple;
    d_ptr->heightSbx->blockSignals(true);
    d_ptr->heightSbx->setValue(height);
    d_ptr->heightSbx->blockSignals(false);
    d_ptr->calBitrate();
}

void VideoEncoderWidget::onVideoHeightChanged()
{
    if (!d_ptr->aspectCheckBox->isChecked() || !d_ptr->originalSize.isValid()) {
        return;
    }
    auto multiple = d_ptr->originalSize.height() * 1.0 / d_ptr->heightSbx->value();
    int width = d_ptr->originalSize.width() / multiple;
    d_ptr->widthSbx->blockSignals(true);
    d_ptr->widthSbx->setValue(width);
    d_ptr->widthSbx->blockSignals(false);
    d_ptr->calBitrate();
}

void VideoEncoderWidget::setupUI()
{
    auto *codecLayout = new QHBoxLayout;
    codecLayout->setSpacing(20);
    codecLayout->addWidget(new QLabel(tr("Encoder:"), this));
    codecLayout->addWidget(d_ptr->videoEncoderCbx);
    codecLayout->addStretch();
    codecLayout->addWidget(d_ptr->gpuEncodeCbx);
    codecLayout->addWidget(d_ptr->gpuDecodeCbx);

    auto *invailedGroupBox = new QGroupBox(tr("Invalid setting"), this);
    auto *invailedLayout = new QFormLayout(invailedGroupBox);
    invailedLayout->addRow(tr("Quality:"), d_ptr->quailtySbx);
    invailedLayout->addRow(tr("Crf:"), d_ptr->crfSbx);
    invailedLayout->addRow(tr("Preset:"), d_ptr->presetCbx);
    invailedLayout->addRow(tr("Tune:"), d_ptr->tuneCbx);
    invailedLayout->addRow(tr("Profile:"), d_ptr->profileCbx);

    auto *sizeGroupBox = new QGroupBox(tr("Size"), this);
    auto *sizeLayout = new QFormLayout(sizeGroupBox);
    sizeLayout->addRow(tr("Width:"), d_ptr->widthSbx);
    sizeLayout->addRow(tr("Height:"), d_ptr->heightSbx);
    sizeLayout->addRow(d_ptr->aspectCheckBox);

    auto *bitrateGroupBox = new QGroupBox(tr("Bitrate"), this);
    auto *bitrateLayout = new QFormLayout(bitrateGroupBox);
    bitrateLayout->addRow(tr("Min Bitrate:"), d_ptr->minBitrateSbx);
    bitrateLayout->addRow(tr("Max Bitrate:"), d_ptr->maxBitrateSbx);

    auto *layout = new QGridLayout(this);
    layout->addLayout(codecLayout, 0, 0, 1, 2);
    layout->addWidget(invailedGroupBox, 1, 0, 2, 1);
    layout->addWidget(sizeGroupBox, 1, 1, 1, 1);
    layout->addWidget(bitrateGroupBox, 2, 1, 1, 1);
}

void VideoEncoderWidget::buildConnect()
{
    connect(d_ptr->videoEncoderCbx,
            &QComboBox::currentTextChanged,
            this,
            &VideoEncoderWidget::onEncoderChanged);
    connect(d_ptr->widthSbx,
            &QSpinBox::valueChanged,
            this,
            &VideoEncoderWidget::onVideoWidthChanged);
    connect(d_ptr->heightSbx,
            &QSpinBox::valueChanged,
            this,
            &VideoEncoderWidget::onVideoHeightChanged);
    connect(d_ptr->aspectCheckBox,
            &QCheckBox::stateChanged,
            this,
            &VideoEncoderWidget::onVideoWidthChanged);
}
