#include "videoencoderwidget.hpp"

#include <ffmpeg/avcontextinfo.h>
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
        auto videoCodecs = Ffmpeg::getCodecsInfo(AVMEDIA_TYPE_VIDEO, true);
        for (const auto &codec : std::as_const(videoCodecs)) {
            auto text = QString("%1 (%2)").arg(codec.longName).arg(codec.name);
            videoEncoderCbx->addItem(text, QVariant::fromValue(codec));
            if (codec.codecId == AV_CODEC_ID_H264) {
                videoEncoderCbx->setCurrentText(text);
            }
        }
        videoEncoderCbx->model()->sort(0);

        gpuDecodeCbx = new QCheckBox(QCoreApplication::translate("VideoEncoderWidgetPrivate",
                                                                 "Use GPU Decode"),
                                     q_ptr);
        gpuDecodeCbx->setChecked(true);

        crfSbx = new QSpinBox(q_ptr);
        crfSbx->setToolTip(
            QCoreApplication::translate("VideoEncoderWidgetPrivate", "smaller -> better"));

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

        init();
    }

    void calBitrate() const
    {
        auto w = widthSbx->value();
        auto h = heightSbx->value();
        minBitrateSbx->setValue(w * h);
        maxBitrateSbx->setValue(w * h * 4);
    }

    void init() const
    {
        Ffmpeg::EncodeContext encodeParam;

        presetCbx->clear();
        presetCbx->addItems(Ffmpeg::EncodeLimit::presets);
        presetCbx->setCurrentText(encodeParam.preset);

        tuneCbx->clear();
        tuneCbx->addItems(Ffmpeg::EncodeLimit::tunes);
        tuneCbx->setCurrentText(encodeParam.tune);

        profileCbx->clear();

        crfSbx->setRange(Ffmpeg::EncodeLimit::crf_min, Ffmpeg::EncodeLimit::crf_max);
        crfSbx->setValue(encodeParam.crf);
    }

    [[nodiscard]] auto currentCodecName() const -> QString
    {
        return videoEncoderCbx->currentData().value<Ffmpeg::CodecInfo>().name;
    }

    VideoEncoderWidget *q_ptr;

    QComboBox *videoEncoderCbx;
    QCheckBox *gpuDecodeCbx;

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
    onEncoderChanged();
}

VideoEncoderWidget::~VideoEncoderWidget() = default;

auto VideoEncoderWidget::setEncoder(AVCodecID codecId) -> bool
{
    Ffmpeg::CodecInfo codec{"", "", codecId};
    auto index = d_ptr->videoEncoderCbx->findData(QVariant::fromValue(codec));
    auto finded = (index >= 0);
    if (finded) {
        d_ptr->videoEncoderCbx->setCurrentIndex(index);
    }
    return finded;
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

auto VideoEncoderWidget::encodeParam() const -> Ffmpeg::EncodeContext
{
    Ffmpeg::EncodeContext encodeParam;
    encodeParam.mediaType = AVMEDIA_TYPE_VIDEO;
    encodeParam.encoderName = d_ptr->currentCodecName();
    encodeParam.size = {d_ptr->widthSbx->value(), d_ptr->heightSbx->value()};
    encodeParam.gpuDecode = d_ptr->gpuDecodeCbx->isChecked();
    encodeParam.minBitrate = d_ptr->minBitrateSbx->value();
    encodeParam.maxBitrate = d_ptr->maxBitrateSbx->value();
    encodeParam.bitrate = d_ptr->maxBitrateSbx->value();
    encodeParam.crf = d_ptr->crfSbx->value();
    encodeParam.preset = d_ptr->presetCbx->currentText();
    encodeParam.tune = d_ptr->tuneCbx->currentText();
    // encodeParam.profile = d_ptr->profileCbx->currentText();

    return encodeParam;
}

void VideoEncoderWidget::onEncoderChanged()
{
    d_ptr->profileCbx->clear();

    QScopedPointer<Ffmpeg::AVContextInfo> contextInfoPtr(new Ffmpeg::AVContextInfo);
    if (!contextInfoPtr->initEncoder(d_ptr->currentCodecName())) {
        return;
    }
    auto profiles = contextInfoPtr->profiles();
    for (const auto &profile : std::as_const(profiles)) {
        d_ptr->profileCbx->addItem(profile.name, profile.profile);
    }
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
    codecLayout->addWidget(d_ptr->gpuDecodeCbx);

    auto *invailedGroupBox = new QGroupBox(tr("Invalid setting"), this);
    auto *invailedLayout = new QFormLayout(invailedGroupBox);
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
            &QComboBox::currentIndexChanged,
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
