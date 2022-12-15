#include "widgetrender.hpp"

#include <ffmpeg/frame.hpp>
#include <ffmpeg/subtitle.h>
#include <ffmpeg/videoformat.hpp>
#include <ffmpeg/videoframeconverter.hpp>

#include <QElapsedTimer>
#include <QPainter>

extern "C" {
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

class WidgetRender::WidgetRenderPrivate
{
public:
    WidgetRenderPrivate(QWidget *parent)
        : owner(parent)
    {}
    ~WidgetRenderPrivate() {}

    QWidget *owner;

    QSizeF size;
    QRectF frameRect;
    QSharedPointer<Frame> framePtr;
    QList<AVPixelFormat> supportFormats = VideoFormat::qFormatMaps.keys();
    QScopedPointer<VideoFrameConverter> frameConverterPtr;
    QSharedPointer<Subtitle> subTitleFramePtr;
    QImage videoImage;
    QImage subTitleImage;

    QColor backgroundColor = Qt::black;
};

WidgetRender::WidgetRender(QWidget *parent)
    : QWidget{parent}
    , d_ptr(new WidgetRenderPrivate(this))
{}

WidgetRender::~WidgetRender() {}

bool WidgetRender::isSupportedOutput_pix_fmt(AVPixelFormat pix_fmt)
{
    return d_ptr->supportFormats.contains(pix_fmt);
}

QSharedPointer<Frame> WidgetRender::convertSupported_pix_fmt(QSharedPointer<Frame> frame)
{
    auto avframe = frame->avFrame();
    auto size = QSize(avframe->width, avframe->height);
    if (d_ptr->frameConverterPtr.isNull()) {
        d_ptr->frameConverterPtr.reset(new VideoFrameConverter(frame.data(), size, AV_PIX_FMT_RGBA));
    } else {
        d_ptr->frameConverterPtr->flush(frame.data(), size, AV_PIX_FMT_RGBA);
    }
    QSharedPointer<Frame> frameRgbPtr(new Frame);
    frameRgbPtr->imageAlloc(size, AV_PIX_FMT_RGBA);
    d_ptr->frameConverterPtr->scale(frame.data(), frameRgbPtr.data());
    //    qDebug() << frameRgbPtr->avFrame()->width << frameRgbPtr->avFrame()->height
    //             << frameRgbPtr->avFrame()->format;
    return frameRgbPtr;
}

QVector<AVPixelFormat> WidgetRender::supportedOutput_pix_fmt()
{
    return d_ptr->supportFormats;
}

void WidgetRender::resetAllFrame()
{
    d_ptr->videoImage = QImage();
    d_ptr->subTitleImage = QImage();
    d_ptr->framePtr.reset();
    d_ptr->subTitleFramePtr.reset();
}

void WidgetRender::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    // draw BackGround
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);
    painter.drawRect(rect());

    if (d_ptr->videoImage.isNull()) {
        return;
    }

    auto size = d_ptr->videoImage.size().scaled(this->size(), Qt::KeepAspectRatio);
    auto rect = QRect((width() - size.width()) / 2,
                      (height() - size.height()) / 2,
                      size.width(),
                      size.height());
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    // draw VideoImage
    painter.drawImage(rect, d_ptr->videoImage); // 15% cpu , i5-7300
    // draw SubTitlImage
    paintSubTitleFrame(rect, &painter);
}

void WidgetRender::updateFrame(QSharedPointer<Frame> frame)
{
    QMetaObject::invokeMethod(
        this, [=] { displayFrame(frame); }, Qt::QueuedConnection);
}

void WidgetRender::updateSubTitleFrame(QSharedPointer<Subtitle> frame)
{
    QMetaObject::invokeMethod(
        this,
        [=] {
            d_ptr->subTitleFramePtr = frame;
            d_ptr->subTitleImage = d_ptr->subTitleFramePtr->image();
            // need update?
            //update();
        },
        Qt::QueuedConnection);
}

void WidgetRender::displayFrame(QSharedPointer<Frame> framePtr)
{
    d_ptr->framePtr = framePtr;
    d_ptr->videoImage = framePtr->convertToImage();
    update();
}

void WidgetRender::paintSubTitleFrame(const QRect &rect, QPainter *painter)
{
    if (d_ptr->subTitleImage.isNull()) {
        return;
    }
    if (d_ptr->subTitleFramePtr->pts() > d_ptr->framePtr->pts()
        || (d_ptr->subTitleFramePtr->pts() + d_ptr->subTitleFramePtr->duration())
               < d_ptr->framePtr->pts()) {
        return;
    }
    painter->drawImage(rect, d_ptr->subTitleImage);
}

} // namespace Ffmpeg
