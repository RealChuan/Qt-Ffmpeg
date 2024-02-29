#include "transcoder.hpp"
#include "audiofifo.hpp"
#include "audioframeconverter.h"
#include "avcontextinfo.h"
#include "averrormanager.hpp"
#include "codeccontext.h"
#include "ffmpegutils.hpp"
#include "formatcontext.h"
#include "frame.hpp"
#include "packet.h"
#include "previewtask.hpp"
#include "transcodercontext.hpp"

#include <event/errorevent.hpp>
#include <event/trackevent.hpp>
#include <event/valueevent.hpp>
#include <filter/filter.hpp>
#include <filter/filtercontext.hpp>
#include <utils/fps.hpp>
#include <utils/threadsafequeue.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavutil/channel_layout.h>
}

namespace Ffmpeg {

class Transcoder::TranscoderPrivate
{
public:
    explicit TranscoderPrivate(Transcoder *q)
        : q_ptr(q)
        , inFormatContext(new FormatContext(q_ptr))
        , outFormatContext(new FormatContext(q_ptr))
        , fpsPtr(new Utils::Fps)
    {
        threadPool = new QThreadPool(q_ptr);
        threadPool->setMaxThreadCount(2);

        QObject::connect(AVErrorManager::instance(),
                         &AVErrorManager::error,
                         q_ptr,
                         [this](const AVError &error) {
                             addPropertyChangeEvent(new ErrorEvent(error));
                         });
    }

    ~TranscoderPrivate() { reset(); }

    auto openInputFile() -> bool
    {
        Q_ASSERT(!inFilePath.isEmpty());
        auto ret = inFormatContext->openFilePath(inFilePath);
        if (!ret) {
            return ret;
        }
        inFormatContext->findStream();
        auto stream_num = inFormatContext->streams();
        for (int i = 0; i < stream_num; i++) {
            auto *transContext = new TranscoderContext;
            transContext->filterPtr.reset(new Filter);
            transcodeContexts.append(transContext);
            auto *stream = inFormatContext->stream(i);
            auto codec_type = stream->codecpar->codec_type;
            if ((stream->disposition & AV_DISPOSITION_ATTACHED_PIC) != 0) {
                continue;
            }
            QSharedPointer<AVContextInfo> contextInfoPtr;
            switch (codec_type) {
            case AVMEDIA_TYPE_AUDIO:
            case AVMEDIA_TYPE_VIDEO:
            case AVMEDIA_TYPE_SUBTITLE: contextInfoPtr.reset(new AVContextInfo); break;
            default:
                qWarning() << "Unsupported codec type: " << av_get_media_type_string(codec_type);
                return false;
            }
            if (!setInMediaIndex(contextInfoPtr.data(), i)) {
                return false;
            }
            if (codec_type == AVMEDIA_TYPE_VIDEO || codec_type == AVMEDIA_TYPE_AUDIO) {
                contextInfoPtr->openCodec(useGpuDecode ? AVContextInfo::GpuDecode
                                                       : AVContextInfo::NotUseGpu);
            }
            transContext->decContextInfoPtr = contextInfoPtr;
        }
        inFormatContext->dumpFormat();

        auto tracks = inFormatContext->audioTracks();
        tracks.append(inFormatContext->videoTracks());
        tracks.append(inFormatContext->subtitleTracks());
        tracks.append(inFormatContext->attachmentTracks());
        addPropertyChangeEvent(new MediaTrackEvent(tracks));
        addPropertyChangeEvent(new DurationEvent(inFormatContext->duration()));
        return true;
    }

    auto openOutputFile() -> bool
    {
        Q_ASSERT(!outFilepath.isEmpty());
        auto ret = outFormatContext->openFilePath(outFilepath, FormatContext::WriteOnly);
        if (!ret) {
            return ret;
        }
        outFormatContext->copyChapterFrom(inFormatContext);
        auto stream_num = inFormatContext->streams();
        for (int i = 0; i < stream_num; i++) {
            auto *inStream = inFormatContext->stream(i);
            auto *stream = outFormatContext->createStream();
            if (stream == nullptr) {
                return false;
            }
            av_dict_copy(&stream->metadata, inStream->metadata, 0);
            stream->disposition = inStream->disposition;
            stream->discard = inStream->discard;
            stream->sample_aspect_ratio = inStream->sample_aspect_ratio;
            stream->avg_frame_rate = inStream->avg_frame_rate;
            stream->event_flags = inStream->event_flags;
            auto *transContext = transcodeContexts.at(i);
            auto decContextInfo = transContext->decContextInfoPtr;
            if ((inStream->disposition & AV_DISPOSITION_ATTACHED_PIC) != 0) {
                auto ret = avcodec_parameters_copy(stream->codecpar, inStream->codecpar);
                if (ret < 0) {
                    qErrnoWarning("Copying parameters for stream #%u failed", i);
                    return ret != 0;
                }
                stream->time_base = inStream->time_base;
                stream->codecpar->width = inStream->codecpar->width > 0 ? inStream->codecpar->width
                                                                        : size.width();
                stream->codecpar->height = inStream->codecpar->height > 0
                                               ? inStream->codecpar->height
                                               : size.height();
                continue;
            }
            stream->codecpar->codec_type = decContextInfo->mediaType();
            switch (decContextInfo->mediaType()) {
            case AVMEDIA_TYPE_AUDIO:
            case AVMEDIA_TYPE_VIDEO: {
                QSharedPointer<AVContextInfo> contextInfoPtr(new AVContextInfo);
                contextInfoPtr->setIndex(i);
                contextInfoPtr->setStream(stream);
                contextInfoPtr->initEncoder(decContextInfo->mediaType() == AVMEDIA_TYPE_AUDIO
                                                ? audioEncoderName
                                                : videoEncoderName);
                //contextInfoPtr->initEncoder(decContextInfo->codecCtx()->avCodecCtx()->codec_id);
                auto *codecCtx = contextInfoPtr->codecCtx();
                auto *avCodecCtx = codecCtx->avCodecCtx();
                decContextInfo->codecCtx()->copyToCodecParameters(codecCtx);
                // ffmpeg example transcoding.c ? framerate, sample_rate
                codecCtx->avCodecCtx()->time_base = decContextInfo->timebase();
                codecCtx->setQuailty(quailty);
                codecCtx->setCrf(crf);
                codecCtx->setPreset(preset);
                codecCtx->setTune(tune);
                if (decContextInfo->mediaType() == AVMEDIA_TYPE_VIDEO) {
                    codecCtx->setSize(size);
                    codecCtx->setMinBitrate(minBitrate);
                    codecCtx->setMaxBitrate(maxBitrate);
                    codecCtx->setProfile(profile);
                }
                if ((outFormatContext->avFormatContext()->oformat->flags & AVFMT_GLOBALHEADER)
                    != 0) {
                    avCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
                }
                contextInfoPtr->openCodec(useGpuEncode ? AVContextInfo::GpuEncode
                                                       : AVContextInfo::NotUseGpu);
                auto ret = avcodec_parameters_from_context(stream->codecpar,
                                                           contextInfoPtr->codecCtx()->avCodecCtx());
                if (ret < 0) {
                    SET_ERROR_CODE(ret);
                    return ret != 0;
                }
                stream->time_base = decContextInfo->timebase();
                transContext->encContextInfoPtr = contextInfoPtr;
            } break;
            case AVMEDIA_TYPE_UNKNOWN:
                qFatal("Elementary stream #%d is of unknown type, cannot proceed", i);
                return false;
            default: {
                auto ret = avcodec_parameters_copy(stream->codecpar, inStream->codecpar);
                if (ret < 0) {
                    SET_ERROR_CODE(ret);
                    return ret != 0;
                }
                stream->time_base = inStream->time_base;
            } break;
            }
        }
        outFormatContext->dumpFormat();
        outFormatContext->avioOpen();
        return outFormatContext->writeHeader();
    }

    void initFilters(int stream_index, Frame *frame)
    {
        auto *transcodeCtx = transcodeContexts.at(stream_index);
        if (transcodeCtx->decContextInfoPtr.isNull()) {
            return;
        }
        auto codec_type = inFormatContext->stream(stream_index)->codecpar->codec_type;
        if (codec_type != AVMEDIA_TYPE_AUDIO && codec_type != AVMEDIA_TYPE_VIDEO) {
            return;
        }
        QString filter_spec;
        if (codec_type == AVMEDIA_TYPE_VIDEO) {
            if (size.isValid()) { // "scale=320:240"
                filter_spec = QString("scale=%1:%2")
                                  .arg(QString::number(size.width()),
                                       QString::number(size.height()));
            }
            if (!subtitleFilename.isEmpty()) {
                // "subtitles=filename=..." burn subtitle into video
                if (!filter_spec.isEmpty()) {
                    filter_spec += ",";
                }
                filter_spec += QString("subtitles=filename='%1':original_size=%2x%3")
                                   .arg(subtitleFilename,
                                        QString::number(size.width()),
                                        QString::number(size.height()));
            }
            if (filter_spec.isEmpty()) {
                filter_spec = "null";
            }
            qInfo() << "Video Filter: " << filter_spec;
        } else {
            filter_spec = "anull";
        }
        transcodeCtx->init_filter(filter_spec, frame);
    }

    void initAudioFifo() const
    {
        auto stream_num = inFormatContext->streams();
        for (int i = 0; i < stream_num; i++) {
            auto *transCtx = transcodeContexts.at(i);
            if (transCtx->decContextInfoPtr.isNull()) {
                continue;
            }
            if (transCtx->decContextInfoPtr->mediaType() == AVMEDIA_TYPE_AUDIO) {
                transCtx->audioFifoPtr.reset(new AudioFifo(transCtx->encContextInfoPtr->codecCtx()));
            }
        }
    }

    void cleanup()
    {
        auto stream_num = inFormatContext->streams();
        for (int i = 0; i < stream_num; i++) {
            if (!transcodeContexts.at(i)->filterPtr->isInitialized()) {
                continue;
            }
            if (!transcodeContexts.at(i)->audioFifoPtr.isNull()) {
                fliterAudioFifo(i, nullptr, true);
            }
            QScopedPointer<Frame> framePtr(new Frame);
            framePtr->destroyFrame();
            filterEncodeWriteframe(framePtr.data(), i);
            flushEncoder(i);
        }
        outFormatContext->writeTrailer();
        reset();
    }

    auto filterEncodeWriteframe(Frame *frame, uint stream_index) const -> bool
    {
        auto *transcodeCtx = transcodeContexts.at(stream_index);
        auto framePtrs = transcodeCtx->filterPtr->filterFrame(frame);
        for (const auto &framePtr : framePtrs) {
            framePtr->setPictType(AV_PICTURE_TYPE_NONE);
            if (transcodeCtx->audioFifoPtr.isNull()) {
                encodeWriteFrame(stream_index, 0, framePtr);
            } else {
                fliterAudioFifo(stream_index, framePtr);
            }
        }
        return true;
    }

    void fliterAudioFifo(uint stream_index,
                         const QSharedPointer<Frame> &framePtr,
                         bool finish = false) const
    {
        //qDebug() << "old: " << stream_index << frame->avFrame()->pts;
        if (!framePtr.isNull()) {
            addSamplesToFifo(framePtr.data(), stream_index);
        }
        while (1) {
            auto framePtr = takeSamplesFromFifo(stream_index, finish);
            if (framePtr.isNull()) {
                return;
            }
            encodeWriteFrame(stream_index, 0, framePtr);
        }
    }

    auto addSamplesToFifo(Frame *frame, uint stream_index) const -> bool
    {
        auto *transcodeCtx = transcodeContexts.at(stream_index);
        auto audioFifoPtr = transcodeCtx->audioFifoPtr;
        if (audioFifoPtr.isNull()) {
            return false;
        }
        const int output_frame_size
            = transcodeCtx->encContextInfoPtr->codecCtx()->avCodecCtx()->frame_size;
        if (audioFifoPtr->size() >= output_frame_size) {
            return true;
        }
        if (!audioFifoPtr->realloc(audioFifoPtr->size() + frame->avFrame()->nb_samples)) {
            return false;
        }
        return audioFifoPtr->write(reinterpret_cast<void **>(frame->avFrame()->data),
                                   frame->avFrame()->nb_samples);
    }

    auto takeSamplesFromFifo(uint stream_index, bool finished = false) const
        -> QSharedPointer<Frame>
    {
        auto *transcodeCtx = transcodeContexts.at(stream_index);
        auto audioFifoPtr = transcodeCtx->audioFifoPtr;
        if (audioFifoPtr.isNull()) {
            return nullptr;
        }
        auto *enc_ctx = transcodeCtx->encContextInfoPtr->codecCtx()->avCodecCtx();
        if (audioFifoPtr->size() < enc_ctx->frame_size && !finished) {
            return nullptr;
        }
        const int frame_size = FFMIN(audioFifoPtr->size(), enc_ctx->frame_size);
        QSharedPointer<Frame> framePtr(new Frame);
        auto *frame = framePtr->avFrame();
        frame->nb_samples = frame_size;
        av_channel_layout_copy(&frame->ch_layout, &enc_ctx->ch_layout);
        frame->format = enc_ctx->sample_fmt;
        frame->sample_rate = enc_ctx->sample_rate;
        if (!framePtr->getBuffer()) {
            return nullptr;
        }
        if (!audioFifoPtr->read(reinterpret_cast<void **>(framePtr->avFrame()->data), frame_size)) {
            return nullptr;
        }
        // fix me?
        frame->pts = transcodeCtx->audioPts / av_q2d(transcodeCtx->decContextInfoPtr->timebase())
                     / transcodeCtx->decContextInfoPtr->codecCtx()->avCodecCtx()->sample_rate;
        transcodeCtx->audioPts += frame->nb_samples;
        //qDebug() << "new: " << stream_index << frame->pts;

        return framePtr;
    }

    auto encodeWriteFrame(uint stream_index, int flush, const QSharedPointer<Frame> &framePtr) const
        -> bool
    {
        auto *transcodeCtx = transcodeContexts.at(stream_index);
        std::vector<PacketPtr> packetPtrs{};
        if (flush != 0) {
            QSharedPointer<Frame> frame_tmp_ptr(new Frame);
            frame_tmp_ptr->destroyFrame();
            packetPtrs = transcodeCtx->encContextInfoPtr->encodeFrame(frame_tmp_ptr);
        } else {
            packetPtrs = transcodeCtx->encContextInfoPtr->encodeFrame(framePtr);
        }
        for (const auto &packetPtr : packetPtrs) {
            packetPtr->setStreamIndex(stream_index);
            packetPtr->rescaleTs(transcodeCtx->encContextInfoPtr->timebase(),
                                 outFormatContext->stream(stream_index)->time_base);
            outFormatContext->writePacket(packetPtr.data());
        }
        return true;
    }

    auto flushEncoder(uint stream_index) const -> bool
    {
        auto *codecCtx
            = transcodeContexts.at(stream_index)->encContextInfoPtr->codecCtx()->avCodecCtx();
        if ((codecCtx->codec->capabilities & AV_CODEC_CAP_DELAY) == 0) {
            return true;
        }
        return encodeWriteFrame(stream_index, 1, nullptr);
    }

    auto setInMediaIndex(AVContextInfo *contextInfo, int index) const -> bool
    {
        contextInfo->setIndex(index);
        contextInfo->setStream(inFormatContext->stream(index));
        return contextInfo->initDecoder(inFormatContext->guessFrameRate(index));
    }

    void loop()
    {
        auto duration = inFormatContext->duration();
        while (runing.load()) {
            PacketPtr packetPtr(new Packet);
            if (!inFormatContext->readFrame(packetPtr.get())) {
                break;
            }
            auto stream_index = packetPtr->streamIndex();
            auto *transcodeCtx = transcodeContexts.at(stream_index);
            auto encContextInfoPtr = transcodeCtx->encContextInfoPtr;
            if (encContextInfoPtr.isNull()) {
                packetPtr->rescaleTs(inFormatContext->stream(stream_index)->time_base,
                                     outFormatContext->stream(stream_index)->time_base);
                outFormatContext->writePacket(packetPtr.data());
            } else {
                packetPtr->rescaleTs(inFormatContext->stream(stream_index)->time_base,
                                     transcodeCtx->decContextInfoPtr->timebase());
                auto framePtrs = transcodeCtx->decContextInfoPtr->decodeFrame(packetPtr);
                for (const auto &framePtr : framePtrs) {
                    if (!transcodeCtx->filterPtr->isInitialized()) {
                        initFilters(stream_index, framePtr.data());
                    }
                    filterEncodeWriteframe(framePtr.data(), stream_index);
                }

                calculatePts(packetPtr.data(),
                             transcodeContexts.at(stream_index)->decContextInfoPtr.data());
                addPropertyChangeEvent(new PositionEvent(packetPtr->pts()));
                if (transcodeCtx->decContextInfoPtr->mediaType() == AVMEDIA_TYPE_VIDEO) {
                    fpsPtr->update();
                }
            }
        }
    }

    void addPropertyChangeEvent(PropertyChangeEvent *event)
    {
        propertyChangeEventQueue.append(PropertyChangeEventPtr(event));
        while (propertyChangeEventQueue.size() > maxPropertyEventQueueSize.load()) {
            propertyChangeEventQueue.take();
        }
        emit q_ptr->eventIncrease();
    }

    void reset()
    {
        if (!transcodeContexts.isEmpty()) {
            qDeleteAll(transcodeContexts);
            transcodeContexts.clear();
        }
        inFormatContext->close();
        outFormatContext->close();

        subtitleFilename.clear();

        fpsPtr->reset();
    }

    static auto getKeyFrame(FormatContext *formatContext,
                            AVContextInfo *videoInfo,
                            qint64 timestamp,
                            FramePtr &outPtr) -> bool
    {
        PacketPtr packetPtr(new Packet);
        if (!formatContext->readFrame(packetPtr.get())) {
            return false;
        }
        if (!formatContext->checkPktPlayRange(packetPtr.get())) {
        } else if (packetPtr->streamIndex() == videoInfo->index()
                   && ((videoInfo->stream()->disposition & AV_DISPOSITION_ATTACHED_PIC) == 0)
                   && packetPtr->isKey()) {
            auto framePtrs = videoInfo->decodeFrame(packetPtr);
            for (const auto &framePtr : std::as_const(framePtrs)) {
                if (!framePtr->isKey() && framePtr.isNull()) {
                    continue;
                }
                calculatePts(framePtr.data(), videoInfo, formatContext);
                auto pts = framePtr->pts();
                if (timestamp > pts) {
                    continue;
                }
                outPtr = framePtr;
            }
        }
        return true;
    }

    Transcoder *q_ptr;

    QString inFilePath;
    QString outFilepath;
    FormatContext *inFormatContext;
    FormatContext *outFormatContext;
    QVector<TranscoderContext *> transcodeContexts{};
    QString audioEncoderName;
    QString videoEncoderName;
    QSize size = QSize(-1, -1);
    QString subtitleFilename;
    int quailty = -1;
    int64_t minBitrate = -1;
    int64_t maxBitrate = -1;
    int crf = 18;
    QStringList presets{"ultrafast",
                        "superfast",
                        "veryfast",
                        "faster",
                        "fast",
                        "medium",
                        "slow",
                        "slower",
                        "veryslow",
                        "placebo"};
    QString preset = "medium";
    QStringList tunes{"film",
                      "animation",
                      "grain",
                      "stillimage",
                      "psnr",
                      "ssim",
                      "fastdecode",
                      "zerolatency"};
    QString tune = "film";
    QStringList profiles{"baseline", "extended", "main", "high"};
    QString profile = "main";

    bool useGpuDecode = false;
    bool useGpuEncode = false;

    std::atomic_bool runing = true;
    QScopedPointer<Utils::Fps> fpsPtr;

    Utils::ThreadSafeQueue<PropertyChangeEventPtr> propertyChangeEventQueue;
    std::atomic<size_t> maxPropertyEventQueueSize = 100;

    std::vector<FramePtr> previewFrames;
    QThreadPool *threadPool;
};

Transcoder::Transcoder(QObject *parent)
    : QThread{parent}
    , d_ptr(new TranscoderPrivate(this))
{
    av_log_set_level(AV_LOG_INFO);
}

Transcoder::~Transcoder()
{
    d_ptr->threadPool->clear();
    stopTranscode();
}

void Transcoder::setUseGpuDecode(bool use)
{
    d_ptr->useGpuDecode = use;
}

void Transcoder::setUseGpuEncode(bool use)
{
    d_ptr->useGpuEncode = use;
}

void Transcoder::setInFilePath(const QString &filePath)
{
    d_ptr->inFilePath = filePath;
}

auto Transcoder::parseInputFile() -> bool
{
    d_ptr->reset();
    return d_ptr->openInputFile();
}

auto Transcoder::duration() const -> qint64
{
    return d_ptr->inFormatContext->duration();
}

auto Transcoder::mediaInfo() -> MediaInfo
{
    return d_ptr->inFormatContext->mediaInfo();
}

void Transcoder::startPreviewFrames(int count)
{
    d_ptr->threadPool->start(new PreviewCountTask(d_ptr->inFilePath, count, this));
}

void Transcoder::setPreviewFrames(const std::vector<QSharedPointer<Frame>> &framePtrs)
{
    QMetaObject::invokeMethod(
        this,
        [=] {
            d_ptr->previewFrames = framePtrs;
            auto *propertChangeEvent = new PropertyChangeEvent;
            propertChangeEvent->setType(PropertyChangeEvent::PreviewFramesChanged);
            d_ptr->addPropertyChangeEvent(propertChangeEvent);
        },
        Qt::QueuedConnection);
}

auto Transcoder::previewFrames() const -> std::vector<QSharedPointer<Frame>>
{
    return d_ptr->previewFrames;
}

void Transcoder::setOutFilePath(const QString &filepath)
{
    d_ptr->outFilepath = filepath;
}

void Transcoder::setAudioEncodecID(AVCodecID codecID)
{
    d_ptr->audioEncoderName = avcodec_get_name(codecID);
}

void Transcoder::setVideoEnCodecID(AVCodecID codecID)
{
    d_ptr->videoEncoderName = avcodec_get_name(codecID);
}

void Transcoder::setAudioEncodecName(const QString &name)
{
    d_ptr->audioEncoderName = name;
}

void Transcoder::setVideoEncodecName(const QString &name)
{
    d_ptr->videoEncoderName = name;
}

void Transcoder::setSize(const QSize &size)
{
    d_ptr->size = size;
}

void Transcoder::setSubtitleFilename(const QString &filename)
{
    Q_ASSERT(QFile::exists(filename));
    d_ptr->subtitleFilename = filename;
    // only for windwos
    d_ptr->subtitleFilename.replace('/', "\\\\");
    auto index = d_ptr->subtitleFilename.indexOf(":\\");
    if (index > 0) {
        d_ptr->subtitleFilename.insert(index, ('\\'));
    }
}

void Transcoder::setQuailty(int quailty)
{
    d_ptr->quailty = quailty;
}

void Transcoder::setMinBitrate(int64_t bitrate)
{
    d_ptr->minBitrate = bitrate;
}

void Transcoder::setMaxBitrate(int64_t bitrate)
{
    d_ptr->maxBitrate = bitrate;
}

void Transcoder::setCrf(int crf)
{
    d_ptr->crf = crf;
}

void Transcoder::setPreset(const QString &preset)
{
    Q_ASSERT(d_ptr->presets.contains(preset));
    d_ptr->preset = preset;
}

auto Transcoder::preset() const -> QString
{
    return d_ptr->preset;
}

auto Transcoder::presets() const -> QStringList
{
    return d_ptr->presets;
}

void Transcoder::setTune(const QString &tune)
{
    Q_ASSERT(d_ptr->tunes.contains(tune));
    d_ptr->tune = tune;
}

auto Transcoder::tune() const -> QString
{
    return d_ptr->tune;
}

auto Transcoder::tunes() const -> QStringList
{
    return d_ptr->tunes;
}

void Transcoder::setProfile(const QString &profile)
{
    Q_ASSERT(d_ptr->profiles.contains(profile));
    d_ptr->profile = profile;
}

auto Transcoder::profile() const -> QString
{
    return d_ptr->profile;
}

auto Transcoder::profiles() const -> QStringList
{
    return d_ptr->profiles;
}

void Transcoder::startTranscode()
{
    stopTranscode();
    d_ptr->runing = true;
    start();
}

void Transcoder::stopTranscode()
{
    d_ptr->runing = false;
    if (isRunning()) {
        quit();
        wait();
    }
    d_ptr->reset();
}

auto Transcoder::fps() -> float
{
    return d_ptr->fpsPtr->getFps();
}

void Transcoder::setPropertyEventQueueMaxSize(size_t size)
{
    d_ptr->maxPropertyEventQueueSize.store(size);
}

auto Transcoder::propertEventyQueueMaxSize() const -> size_t
{
    return d_ptr->maxPropertyEventQueueSize.load();
}

auto Transcoder::propertyChangeEventSize() const -> size_t
{
    return d_ptr->propertyChangeEventQueue.size();
}

auto Transcoder::takePropertyChangeEvent() -> PropertyChangeEventPtr
{
    return d_ptr->propertyChangeEventQueue.take();
}

void Transcoder::run()
{
    QElapsedTimer timer;
    timer.start();
    qInfo() << "Start Transcoding";
    d_ptr->reset();
    if (!d_ptr->openInputFile()) {
        qWarning() << "Open input file failed!";
        return;
    }
    if (!d_ptr->openOutputFile()) {
        qWarning() << "Open ouput file failed!";
        return;
    }
    d_ptr->initAudioFifo();
    d_ptr->loop();
    d_ptr->cleanup();
    qInfo() << "Finish Transcoding: "
            << QTime::fromMSecsSinceStartOfDay(timer.elapsed()).toString("hh:mm:ss.zzz");
}

} // namespace Ffmpeg
