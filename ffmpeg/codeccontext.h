#ifndef CODECCONTEXT_H
#define CODECCONTEXT_H

#include <QObject>

extern "C" {
#include <libavcodec/codec.h>
}

struct AVCodecParameters;
struct AVCodecContext;

namespace Ffmpeg {

struct EncodeContext;
class Subtitle;
class Packet;
class Frame;
class CodecContext : public QObject
{
public:
    explicit CodecContext(const AVCodec *codec, QObject *parent = nullptr);
    ~CodecContext() override;

    void copyToCodecParameters(CodecContext *dst);

    auto setParameters(const AVCodecParameters *par) -> bool;

    [[nodiscard]] auto supportFrameRates() const -> QVector<AVRational>;
    void setFrameRate(const AVRational &frameRate);

    [[nodiscard]] auto supportPixFmts() const -> QVector<AVPixelFormat>;
    void setPixfmt(AVPixelFormat pixfmt);

    [[nodiscard]] auto supportSampleRates() const -> QVector<int>;
    void setSampleRate(int sampleRate);

    [[nodiscard]] auto supportSampleFmts() const -> QVector<AVSampleFormat>;
    void setSampleFmt(AVSampleFormat sampleFmt);

    [[nodiscard]] auto supportedProfiles() const -> QVector<AVProfile>;
    void setProfile(int profile);

    [[nodiscard]] auto supportedChLayouts() const -> QVector<AVChannelLayout>;
    [[nodiscard]] auto chLayout() const -> AVChannelLayout;
    void setChLayout(const AVChannelLayout &chLayout);

    void setEncodeParameters(const EncodeContext &encodeContext);

    void setSize(const QSize &size);
    [[nodiscard]] auto size() const -> QSize;

    [[nodiscard]] auto quantizer() const -> QPair<int, int>;

    // Set before open, Soft solution is effective
    void setThreadCount(int threadCount);
    auto open() -> bool;

    auto sendPacket(Packet *packet) -> bool;
    auto receiveFrame(Frame *frame) -> bool;
    auto decodeSubtitle2(Subtitle *subtitle, Packet *packet) -> bool;

    auto sendFrame(Frame *frame) -> bool;
    auto receivePacket(Packet *packet) -> bool;

    [[nodiscard]] auto mediaTypeString() const -> QString;
    [[nodiscard]] auto isDecoder() const -> bool;

    void flush();

    auto codec() -> const AVCodec *;
    auto avCodecCtx() -> AVCodecContext *;

private:
    class CodecContextPrivate;
    QScopedPointer<CodecContextPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // CODECCONTEXT_H
