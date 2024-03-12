#include "encodecontext.hpp"
#include "avcontextinfo.h"
#include "codeccontext.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace Ffmpeg {

EncodeContext::EncodeContext(int streamIndex, AVContextInfo *info)
{
    auto *avCodecContext = info->codecCtx()->avCodecCtx();
    const auto *codec = avCodecContext->codec;
    setEncoderName(QString::fromUtf8(codec->name));
    setChannel(static_cast<AVChannel>(avCodecContext->ch_layout.u.mask));
    setProfile(avCodecContext->profile);
    this->streamIndex = streamIndex;
    mediaType = avCodecContext->codec_type;
    minBitrate = avCodecContext->rc_min_rate;
    maxBitrate = avCodecContext->rc_max_rate;
    bitrate = avCodecContext->bit_rate;
    if (bitrate <= 0) {
        bitrate = 512000;
    }
    size = {avCodecContext->width, avCodecContext->height};
}

void EncodeContext::setEncoderName(const QString &name)
{
    QScopedPointer<AVContextInfo> contextInfoPtr(new AVContextInfo);
    if (!contextInfoPtr->initEncoder(name)) {
        return;
    }

    const auto *codec = contextInfoPtr->codecCtx()->avCodecCtx()->codec;
    m_codecInfo.codecId = codec->id;
    m_codecInfo.name = QString::fromUtf8(codec->name);
    m_codecInfo.longName = QString::fromUtf8(codec->long_name);
    m_codecInfo.displayName = QString("%1 (%2)").arg(m_codecInfo.longName, m_codecInfo.name);

    profiles = contextInfoPtr->profiles();
    if (!profiles.isEmpty()) {
        m_profile = profiles.first();
    }

    chLayouts = Ffmpeg::getChLayouts(contextInfoPtr->chLayouts());
    auto index = chLayouts.indexOf({static_cast<AVChannel>(AV_CH_LAYOUT_STEREO), {}});
    if (index >= 0) {
        m_chLayout = chLayouts[index];
    } else if (!chLayouts.isEmpty()) {
        m_chLayout = chLayouts.first();
    }
}

void EncodeContext::setChannel(AVChannel channel)
{
    auto index = chLayouts.indexOf({channel, {}});
    if (index >= 0) {
        m_chLayout = chLayouts[index];
    }
}

void EncodeContext::setProfile(int profile)
{
    for (const auto &prof : std::as_const(profiles)) {
        if (prof.profile == profile) {
            m_profile = prof;
            return;
        }
    }
}

} // namespace Ffmpeg
