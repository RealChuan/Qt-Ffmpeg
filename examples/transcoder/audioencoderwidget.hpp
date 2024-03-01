#ifndef AUDIOENCODERWIDGET_HPP
#define AUDIOENCODERWIDGET_HPP

#include <ffmpeg/encodecontext.hpp>

#include <QWidget>

extern "C" {
#include <libavcodec/codec_id.h>
}

class AudioEncoderWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AudioEncoderWidget(QWidget *parent = nullptr);
    ~AudioEncoderWidget() override;

    auto setEncoder(AVCodecID codecId) -> bool;

    [[nodiscard]] auto encodeParam() const -> Ffmpeg::EncodeContext;

private slots:
    void onEncoderChanged();

private:
    void setupUI();
    void buildConnect();

    class AudioEncoderWidgetPrivate;
    QScopedPointer<AudioEncoderWidgetPrivate> d_ptr;
};

#endif // AUDIOENCODERWIDGET_HPP
