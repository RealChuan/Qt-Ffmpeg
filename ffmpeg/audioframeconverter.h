#pragma once

#include <QAudioFormat>

extern "C" {
#include <libavutil/channel_layout.h>
}

namespace Ffmpeg {

class CodecContext;
class Frame;
class AudioFrameConverter : public QObject
{
public:
    explicit AudioFrameConverter(CodecContext *codecCtx,
                                 const QAudioFormat &format,
                                 QObject *parent = nullptr);
    ~AudioFrameConverter() override;

    auto convert(Frame *frame) -> QByteArray;

private:
    class AudioFrameConverterPrivate;
    QScopedPointer<AudioFrameConverterPrivate> d_ptr;
};

auto getAudioFormatFromCodecCtx(CodecContext *codecCtx, int &sampleSize) -> QAudioFormat;

auto getAVChannelLayoutDescribe(const AVChannelLayout &chLayout) -> QString;

} // namespace Ffmpeg
