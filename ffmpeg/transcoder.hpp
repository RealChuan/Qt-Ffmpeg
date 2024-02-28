#ifndef TRANSCODER_H
#define TRANSCODER_H

#include "mediainfo.hpp"

#include <ffmpeg/event/event.hpp>

#include <QThread>

extern "C" {
#include <libavcodec/codec_id.h>
}

namespace Ffmpeg {

class AVError;
class Frame;
class FFMPEG_EXPORT Transcoder : public QThread
{
    Q_OBJECT
public:
    explicit Transcoder(QObject *parent = nullptr);
    ~Transcoder() override;

    void setUseGpuDecode(bool useGpu);

    void setInFilePath(const QString &filePath);
    auto parseInputFile() -> bool;

    [[nodiscard]] auto duration() const -> qint64; // microsecond
    auto mediaInfo() -> MediaInfo;
    void startPreviewFrames(int count);
    void setPreviewFrames(const std::vector<QSharedPointer<Frame>> &framePtrs);
    [[nodiscard]] auto previewFrames() const -> std::vector<QSharedPointer<Frame>>;

    void setOutFilePath(const QString &filepath);

    void setAudioEncodecID(AVCodecID codecID);
    void setVideoEnCodecID(AVCodecID codecID);
    void setAudioEncodecName(const QString &name);
    void setVideoEncodecName(const QString &name);

    void setSize(const QSize &size);
    void setSubtitleFilename(const QString &filename);

    void setQuailty(int quailty);
    void setMinBitrate(int64_t bitrate);
    void setMaxBitrate(int64_t bitrate);
    void setCrf(int crf);

    void setPreset(const QString &preset);
    [[nodiscard]] auto preset() const -> QString;
    [[nodiscard]] auto presets() const -> QStringList;

    void setTune(const QString &tune);
    [[nodiscard]] auto tune() const -> QString;
    [[nodiscard]] auto tunes() const -> QStringList;

    void setProfile(const QString &profile);
    [[nodiscard]] auto profile() const -> QString;
    [[nodiscard]] auto profiles() const -> QStringList;

    void startTranscode();
    void stopTranscode();

    auto fps() -> float;

    void setPropertyEventQueueMaxSize(size_t size);
    [[nodiscard]] auto propertEventyQueueMaxSize() const -> size_t;
    [[nodiscard]] auto propertyChangeEventSize() const -> size_t;
    auto takePropertyChangeEvent() -> PropertyChangeEventPtr;

signals:
    void progressChanged(qreal); // 0.XXXX
    void eventIncrease();

protected:
    void run() override;

private:
    class TranscoderPrivate;
    QScopedPointer<TranscoderPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // TRANSCODER_H
