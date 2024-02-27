#ifndef FFMPEGUTILS_HPP
#define FFMPEGUTILS_HPP

#include "ffmepg_global.h"

#include <QSize>

extern "C" {
#include <libavcodec/codec_id.h>
#include <libavutil/hwcontext.h>
}

struct AVCodec;

namespace Ffmpeg {

class Packet;
class Frame;
class AVContextInfo;
class FormatContext;

using Metadatas = QMap<QString, QString>;

void FFMPEG_EXPORT printFfmpegInfo();

void calculatePts(Frame *frame, AVContextInfo *contextInfo, FormatContext *formatContext);
void calculatePts(Packet *packet, AVContextInfo *contextInfo);

auto getCurrentHWDeviceTypes() -> QVector<AVHWDeviceType>;

auto getPixelFormat(const AVCodec *codec, AVHWDeviceType type) -> AVPixelFormat;

auto compareAVRational(const AVRational &a, const AVRational &b) -> bool;

auto getMetaDatas(AVDictionary *metadata) -> Metadatas;

auto FFMPEG_EXPORT getCodecQuantizer(const QString &codecname) -> QPair<int, int>;

auto FFMPEG_EXPORT getCurrentSupportCodecs(AVMediaType mediaType, bool encoder)
    -> QMap<AVCodecID, QString>;

} // namespace Ffmpeg

#endif // FFMPEGUTILS_HPP
