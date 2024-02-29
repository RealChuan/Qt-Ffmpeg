#ifndef VIDEOENCODERWIDGET_HPP
#define VIDEOENCODERWIDGET_HPP

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
    [[nodiscard]] auto encoder() const -> QString;

    void setPreset(const QStringList &presets, const QString &current);
    [[nodiscard]] auto preset() const -> QString;

    void setTune(const QStringList &tunes, const QString &current);
    [[nodiscard]] auto tune() const -> QString;

    void setProfile(const QStringList &profiles, const QString &current);
    [[nodiscard]] auto profile() const -> QString;

    void setVideoSize(const QSize &size);
    [[nodiscard]] auto videoSize() const -> QSize;

    [[nodiscard]] auto quality() const -> int;
    [[nodiscard]] auto crf() const -> int;

    [[nodiscard]] auto minBitrate() const -> int;
    [[nodiscard]] auto maxBitrate() const -> int;

    [[nodiscard]] auto gpuEncode() const -> bool;
    [[nodiscard]] auto gpuDecode() const -> bool;

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
