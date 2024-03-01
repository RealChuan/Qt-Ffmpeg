#ifndef TRANSCODERCONTEXT_HPP
#define TRANSCODERCONTEXT_HPP

#include <QSharedPointer>

namespace Ffmpeg {

class Frame;
class AVContextInfo;
class Filter;
class AudioFifo;

struct TranscoderContext
{
    auto initFilter(const QString &filter_spec, Frame *frame) const -> bool;

    QSharedPointer<AVContextInfo> decContextInfoPtr;
    QSharedPointer<AVContextInfo> encContextInfoPtr;

    QSharedPointer<Filter> filterPtr;

    QSharedPointer<AudioFifo> audioFifoPtr;
    qint64 audioPts = 0;
};

} // namespace Ffmpeg

#endif // TRANSCODERCONTEXT_HPP
