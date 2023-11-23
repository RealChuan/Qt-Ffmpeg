#include "filter.hpp"
#include "filtercontext.hpp"
#include "filtergraph.hpp"
#include "filterinout.hpp"

#include <ffmpeg/frame.hpp>

#include <QDebug>

extern "C" {
#include <libavfilter/avfilter.h>
}

namespace Ffmpeg {

class Filter::FilterPrivate
{
public:
    explicit FilterPrivate(Filter *q)
        : q_ptr(q)
    {
        filterGraph = new FilterGraph(q_ptr);
    }

    void initVideoFilter(Frame *frame)
    {
        buffersrcCtx = new FilterContext("buffer", q_ptr);
        buffersinkCtx = new FilterContext("buffersink", q_ptr);
        auto *avFrame = frame->avFrame();
        auto timeBase = avFrame->time_base;
        auto sampleAspectRatio = avFrame->sample_aspect_ratio;
        auto args
            = QString::asprintf("video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
                                avFrame->width,
                                avFrame->height,
                                avFrame->format,
                                timeBase.num,
                                timeBase.den,
                                sampleAspectRatio.num,
                                sampleAspectRatio.den);
        qDebug() << "Video filter in args:" << args;

        create(args);
    }

    void initAudioFilter(Frame *frame)
    {
        buffersrcCtx = new FilterContext("abuffer", q_ptr);
        buffersinkCtx = new FilterContext("abuffersink", q_ptr);
        auto *avFrame = frame->avFrame();
        if (avFrame->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC) {
            av_channel_layout_default(&avFrame->ch_layout, avFrame->ch_layout.nb_channels);
        }
        char buf[64];
        av_channel_layout_describe(&avFrame->ch_layout, buf, sizeof(buf));
        auto args = QString::asprintf(
            "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%s" PRIx64,
            1,
            avFrame->sample_rate,
            avFrame->sample_rate,
            av_get_sample_fmt_name(static_cast<AVSampleFormat>(avFrame->format)),
            buf);
        qDebug() << "Audio filter in args:" << args;

        create(args);
    }

    void create(const QString &args)
    {
        buffersrcCtx->create("in", args, filterGraph);
        buffersinkCtx->create("out", "", filterGraph);
    }

    void config(const QString &filterSpec)
    {
        QScopedPointer<FilterInOut> fliterOutPtr(new FilterInOut);
        QScopedPointer<FilterInOut> fliterInPtr(new FilterInOut);
        auto *outputs = fliterOutPtr->avFilterInOut();
        auto *inputs = fliterInPtr->avFilterInOut();
        /* Endpoints for the filter graph. */
        outputs->name = av_strdup("in");
        outputs->filter_ctx = buffersrcCtx->avFilterContext();
        outputs->pad_idx = 0;
        outputs->next = nullptr;

        inputs->name = av_strdup("out");
        inputs->filter_ctx = buffersinkCtx->avFilterContext();
        inputs->pad_idx = 0;
        inputs->next = nullptr;

        filterGraph->parse(filterSpec, fliterInPtr.data(), fliterOutPtr.data());
        filterGraph->config();

        qDebug() << "Filter config:" << filterSpec;

        //    if (!(enc_ctx->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)) {
        //        buffersink_ctx->buffersink_setFrameSize(enc_ctx->frame_size);
        //    }
    }

    Filter *q_ptr;

    FilterContext *buffersrcCtx = nullptr;
    FilterContext *buffersinkCtx = nullptr;
    FilterGraph *filterGraph;
};

Filter::Filter(QObject *parent)
    : QObject{parent}
    , d_ptr(new FilterPrivate(this))
{}

Filter::~Filter() = default;

auto Filter::isInitialized() const -> bool
{
    return d_ptr->buffersrcCtx != nullptr;
}

auto Filter::init(AVMediaType type, Frame *frame) -> bool
{
    switch (type) {
    case AVMEDIA_TYPE_AUDIO: d_ptr->initAudioFilter(frame); break;
    case AVMEDIA_TYPE_VIDEO: d_ptr->initVideoFilter(frame); break;
    default: return false;
    }
    return true;
}

void Filter::config(const QString &filterSpec)
{
    Q_ASSERT(!filterSpec.isEmpty());
    d_ptr->config(filterSpec);
}

auto Filter::filterFrame(Frame *frame) -> QVector<FramePtr>
{
    QVector<FramePtr> framepPtrs{};
    if (!d_ptr->buffersrcCtx->buffersrcAddFrameFlags(frame)) {
        return framepPtrs;
    }
    std::unique_ptr<Frame> framePtr(new Frame);
    while (d_ptr->buffersinkCtx->buffersinkGetFrame(framePtr.get())) {
        framePtr->setPictType(AV_PICTURE_TYPE_NONE);
        framepPtrs.emplace_back(framePtr.release());
        framePtr.reset(new Frame);
    }
    return framepPtrs;
}

auto Filter::buffersinkCtx() -> FilterContext *
{
    Q_ASSERT(d_ptr->buffersinkCtx);
    return d_ptr->buffersinkCtx;
}

} // namespace Ffmpeg