#ifndef ENCODECONTEXT_HPP
#define ENCODECONTEXT_HPP

#include "ffmpegutils.hpp"

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
struct FFMPEG_EXPORT EncodeContext
{
    EncodeContext() = default;
    explicit EncodeContext(int streamIndex, AVContextInfo *info);

    void setEncoderName(const QString &name);
    [[nodiscard]] auto codecInfo() const -> CodecInfo { return m_codecInfo; }

    void setChannel(AVChannel channel);
    [[nodiscard]] auto chLayout() const -> ChLayout { return m_chLayout; }

    void setProfile(int profile);
    [[nodiscard]] auto profile() const -> AVProfile { return m_profile; }

    int streamIndex = -1;
    AVMediaType mediaType;

    qint64 minBitrate = -1;
    qint64 maxBitrate = -1;
    qint64 bitrate = -1;

    int threadCount = -1;

    bool gpuDecode = true;

    int crf = 18;

    QString preset = "slow";
    QString tune = "film";

    QSize size = {-1, -1};

    ChLayouts chLayouts;
    QVector<AVProfile> profiles;

private:
    CodecInfo m_codecInfo;
    ChLayout m_chLayout;
    AVProfile m_profile;
};

using EncodeContexts = QVector<EncodeContext>;

} // namespace Ffmpeg

Q_DECLARE_METATYPE(Ffmpeg::EncodeContext);

#endif // ENCODECONTEXT_HPP
