#ifndef VIDEOENCODERWIDGET_HPP
#define VIDEOENCODERWIDGET_HPP

#include <ffmpeg/encodecontext.hpp>

#include <QWidget>

extern "C" {
#include <libavcodec/codec_id.h>
}

class VideoEncoderWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoEncoderWidget(QWidget *parent = nullptr);
    ~VideoEncoderWidget() override;

    auto setEncoder(AVCodecID codecId) -> bool;

    void setVideoSize(const QSize &size);

    [[nodiscard]] auto encodeParam() const -> Ffmpeg::EncodeContext;

private slots:
    void onEncoderChanged();
    void onVideoWidthChanged();
    void onVideoHeightChanged();

private:
    void setupUI();
    void buildConnect();

    class VideoEncoderWidgetPrivate;
    QScopedPointer<VideoEncoderWidgetPrivate> d_ptr;
};

#endif // VIDEOENCODERWIDGET_HPP
