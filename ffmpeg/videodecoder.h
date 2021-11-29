#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include "decoder.h"
#include "packet.h"

struct AVFrame;

namespace Ffmpeg {

class VideoDecoder : public Decoder<Packet *>
{
    Q_OBJECT
public:
    explicit VideoDecoder(QObject *parent = nullptr);
    ~VideoDecoder();

    void pause(bool state) override;

    void setSpeed(double speed) override;

signals:
    void readyRead(const QImage &image);

protected:
    void runDecoder() override;

private:
    class VideoDecoderPrivate;
    QScopedPointer<VideoDecoderPrivate> d_ptr;
};

}

#endif // VIDEODECODER_H
