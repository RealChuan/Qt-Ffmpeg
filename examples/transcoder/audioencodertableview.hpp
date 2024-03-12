#ifndef AUDIOENCODERTABLEVIEW_HPP
#define AUDIOENCODERTABLEVIEW_HPP

#include <ffmpeg/encodecontext.hpp>

#include <QTableView>

class AudioEncoderTableView : public QTableView
{
public:
    explicit AudioEncoderTableView(QWidget *parent = nullptr);
    ~AudioEncoderTableView() override;

    void setDatas(const Ffmpeg::EncodeContexts &encodeContexts);
    Ffmpeg::EncodeContexts datas() const;

private:
    void setupUI();

    class AudioEncoderTableViewPrivate;
    QScopedPointer<AudioEncoderTableViewPrivate> d_ptr;
};

#endif // AUDIOENCODERTABLEVIEW_HPP
