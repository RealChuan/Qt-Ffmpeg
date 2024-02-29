#include "audioencoderwidget.hpp"

#include <ffmpeg/ffmpegutils.hpp>

#include <QtWidgets>

class AudioEncoderWidget::AudioEncoderWidgetPrivate
{
public:
    explicit AudioEncoderWidgetPrivate(AudioEncoderWidget *q)
        : q_ptr(q)
    {
        audioEncoderCbx = new QComboBox(q_ptr);
        audioEncoderCbx->setView(new QListView(audioEncoderCbx));
        audioEncoderCbx->setMaxVisibleItems(10);
        audioEncoderCbx->setStyleSheet("QComboBox {combobox-popup:0;}");

        auto audioEncodercs = Ffmpeg::getCurrentSupportCodecs(AVMEDIA_TYPE_AUDIO, true);
        for (auto iter = audioEncodercs.cbegin(); iter != audioEncodercs.cend(); ++iter) {
            audioEncoderCbx->addItem(iter.value(), iter.key());
        }
        audioEncoderCbx->setCurrentIndex(audioEncoderCbx->findData(AV_CODEC_ID_AAC));
        audioEncoderCbx->model()->sort(0);
    }

    AudioEncoderWidget *q_ptr;

    QComboBox *audioEncoderCbx;
};

AudioEncoderWidget::AudioEncoderWidget(QWidget *parent)
    : QWidget{parent}
    , d_ptr(new AudioEncoderWidgetPrivate(this))
{
    setupUI();
    buildConnect();
}

AudioEncoderWidget::~AudioEncoderWidget() = default;

auto AudioEncoderWidget::setEncoder(AVCodecID codecId) -> bool
{
    auto index = d_ptr->audioEncoderCbx->findData(codecId);
    auto finded = (index >= 0);
    if (finded) {
        d_ptr->audioEncoderCbx->setCurrentIndex(index);
    }
    return finded;
}

auto AudioEncoderWidget::encoder() const -> QString
{
    return d_ptr->audioEncoderCbx->currentText();
}

void AudioEncoderWidget::setupUI()
{
    auto *layout = new QHBoxLayout(this);
    layout->addWidget(new QLabel(tr("Encoder:")));
    layout->addWidget(d_ptr->audioEncoderCbx);
    layout->addStretch();
}

void AudioEncoderWidget::buildConnect() {}
