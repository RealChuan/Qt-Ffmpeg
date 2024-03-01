#include "audioencoderwidget.hpp"

#include <ffmpeg/avcontextinfo.h>
#include <ffmpeg/ffmpegutils.hpp>

#include <QtWidgets>

class AudioEncoderWidget::AudioEncoderWidgetPrivate
{
public:
    explicit AudioEncoderWidgetPrivate(AudioEncoderWidget *q)
        : q_ptr(q)
    {
        const auto *comboBoxStyleSheet = "QComboBox {combobox-popup:0;}";

        audioEncoderCbx = new QComboBox(q_ptr);
        audioEncoderCbx->setView(new QListView(audioEncoderCbx));
        audioEncoderCbx->setMaxVisibleItems(10);
        audioEncoderCbx->setStyleSheet(comboBoxStyleSheet);
        auto audioEncodercs = Ffmpeg::getCodecsInfo(AVMEDIA_TYPE_AUDIO, true);
        for (const auto &codec : std::as_const(audioEncodercs)) {
            auto text = QString("%1 (%2)").arg(codec.longName).arg(codec.name);
            audioEncoderCbx->addItem(text, QVariant::fromValue(codec));
            if (codec.codecId == AV_CODEC_ID_AAC) {
                audioEncoderCbx->setCurrentText(text);
            }
        }
        audioEncoderCbx->model()->sort(0);

        chLayoutCbx = new QComboBox(q_ptr);
        chLayoutCbx->setView(new QListView(chLayoutCbx));
        chLayoutCbx->setMaxVisibleItems(10);
        chLayoutCbx->setStyleSheet(comboBoxStyleSheet);

        crfSbx = new QSpinBox(q_ptr);
        crfSbx->setToolTip(
            QCoreApplication::translate("VideoEncoderWidgetPrivate", "smaller -> better"));

        profileCbx = new QComboBox(q_ptr);
        profileCbx->setView(new QListView(profileCbx));
        profileCbx->setMaxVisibleItems(10);
        profileCbx->setStyleSheet(comboBoxStyleSheet);

        const int defaultBitrate = 512 * 1000;
        minBitrateSbx = new QSpinBox(q_ptr);
        minBitrateSbx->setRange(0, INT_MAX);
        minBitrateSbx->setValue(defaultBitrate);
        maxBitrateSbx = new QSpinBox(q_ptr);
        maxBitrateSbx->setRange(0, INT_MAX);
        maxBitrateSbx->setValue(defaultBitrate);

        init();
    }

    void init()
    {
        Ffmpeg::EncodeContext encodeParam;

        profileCbx->clear();

        crfSbx->setRange(Ffmpeg::EncodeLimit::crf_min, Ffmpeg::EncodeLimit::crf_max);
        crfSbx->setValue(encodeParam.crf);
    }

    [[nodiscard]] auto currentCodecName() const -> QString
    {
        return audioEncoderCbx->currentData().value<Ffmpeg::CodecInfo>().name;
    }

    AudioEncoderWidget *q_ptr;

    QComboBox *audioEncoderCbx;
    QComboBox *chLayoutCbx;
    QSpinBox *crfSbx;
    QComboBox *profileCbx;

    QSpinBox *minBitrateSbx;
    QSpinBox *maxBitrateSbx;
};

AudioEncoderWidget::AudioEncoderWidget(QWidget *parent)
    : QWidget{parent}
    , d_ptr(new AudioEncoderWidgetPrivate(this))
{
    setupUI();
    buildConnect();
    onEncoderChanged();
}

AudioEncoderWidget::~AudioEncoderWidget() = default;

auto AudioEncoderWidget::setEncoder(AVCodecID codecId) -> bool
{
    Ffmpeg::CodecInfo codec{"", "", codecId};
    auto index = d_ptr->audioEncoderCbx->findData(QVariant::fromValue(codec));
    auto finded = (index >= 0);
    if (finded) {
        d_ptr->audioEncoderCbx->setCurrentIndex(index);
    }
    return finded;
}

auto AudioEncoderWidget::encodeParam() const -> Ffmpeg::EncodeContext
{
    Ffmpeg::EncodeContext encodeParam;
    encodeParam.mediaType = AVMEDIA_TYPE_AUDIO;
    encodeParam.encoderName = d_ptr->currentCodecName();
    encodeParam.channel = static_cast<AVChannel>(d_ptr->chLayoutCbx->currentData().toLongLong());
    encodeParam.crf = d_ptr->crfSbx->value();
    encodeParam.minBitrate = d_ptr->minBitrateSbx->value();
    encodeParam.maxBitrate = d_ptr->maxBitrateSbx->value();
    encodeParam.bitrate = d_ptr->maxBitrateSbx->value();

    return encodeParam;
}

void AudioEncoderWidget::onEncoderChanged()
{
    d_ptr->profileCbx->clear();
    d_ptr->chLayoutCbx->clear();

    QScopedPointer<Ffmpeg::AVContextInfo> contextInfoPtr(new Ffmpeg::AVContextInfo);
    if (!contextInfoPtr->initEncoder(d_ptr->currentCodecName())) {
        return;
    }
    auto profiles = contextInfoPtr->profiles();
    for (const auto &profile : std::as_const(profiles)) {
        d_ptr->profileCbx->addItem(profile.name, profile.profile);
    }
    auto chLayouts = Ffmpeg::getChLayouts(contextInfoPtr->chLayouts());
    for (const auto &chLayout : std::as_const(chLayouts)) {
        d_ptr->chLayoutCbx->addItem(chLayout.channelName, chLayout.channel);
    }
    auto index = d_ptr->chLayoutCbx->findData(AV_CH_LAYOUT_STEREO);
    d_ptr->chLayoutCbx->setCurrentIndex(index >= 0 ? index : 0);
}

void AudioEncoderWidget::setupUI()
{
    auto *invailedGroupBox = new QGroupBox(tr("Invalid setting"), this);
    auto *invailedLayout = new QFormLayout(invailedGroupBox);
    invailedLayout->addRow(tr("Crf:"), d_ptr->crfSbx);
    invailedLayout->addRow(tr("Profile:"), d_ptr->profileCbx);

    auto *bitrateGroupBox = new QGroupBox(tr("Bitrate"), this);
    auto *bitrateLayout = new QFormLayout(bitrateGroupBox);
    bitrateLayout->addRow(tr("Min Bitrate:"), d_ptr->minBitrateSbx);
    bitrateLayout->addRow(tr("Max Bitrate:"), d_ptr->maxBitrateSbx);

    auto *hLayout = new QHBoxLayout;
    hLayout->addWidget(invailedGroupBox);
    hLayout->addWidget(bitrateGroupBox);

    auto *layout = new QFormLayout(this);
    layout->addRow(tr("Encoder:"), d_ptr->audioEncoderCbx);
    layout->addRow(tr("Channel Layout:"), d_ptr->chLayoutCbx);
    layout->addRow(hLayout);
}

void AudioEncoderWidget::buildConnect()
{
    connect(d_ptr->audioEncoderCbx,
            &QComboBox::currentIndexChanged,
            this,
            &AudioEncoderWidget::onEncoderChanged);
}
