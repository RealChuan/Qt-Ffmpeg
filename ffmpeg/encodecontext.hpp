#ifndef ENCODECONTEXT_HPP
#define ENCODECONTEXT_HPP

#include "ffmepg_global.h"

#include <QtCore>

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
}

namespace Ffmpeg {

namespace EncodeLimit {

static const int crf_min = 0;
static const int crf_max = 51;

static const QStringList presets = QStringList{"ultrafast",
                                               "superfast",
                                               "veryfast",
                                               "faster",
                                               "fast",
                                               "medium",
                                               "slow",
                                               "slower",
                                               "veryslow",
                                               "placebo"};
static const QStringList tunes = QStringList{"film",
                                             "animation",
                                             "grain",
                                             "stillimage",
                                             "psnr",
                                             "ssim",
                                             "fastdecode",
                                             "zerolatency"};

} // namespace EncodeLimit
struct EncodeContext
{
    AVMediaType mediaType;

    QString encoderName;

    qint64 minBitrate = -1;
    qint64 maxBitrate = -1;
    qint64 bitrate = -1;

    int threadCount = -1;

    bool gpuDecode = true;

    int crf = 18;

    QString preset = "slow";
    QString tune = "film";
    int profile = 0;

    QSize size = {-1, -1};

    AVChannel channel;
};

} // namespace Ffmpeg

#endif // ENCODECONTEXT_HPP
