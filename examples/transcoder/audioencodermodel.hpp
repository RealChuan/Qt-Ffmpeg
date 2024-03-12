#ifndef AUDIOENCODERMODEL_HPP
#define AUDIOENCODERMODEL_HPP

#include <ffmpeg/encodecontext.hpp>

#include <QAbstractTableModel>

class AudioEncoderModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Property { ID, Encoder, ChannelLayout, Bitrate, Crf, Profile };
    Q_ENUM(Property);

    explicit AudioEncoderModel(QObject *parent = nullptr);
    ~AudioEncoderModel() override;

    [[nodiscard]] auto rowCount(const QModelIndex &parent = QModelIndex()) const -> int override;
    [[nodiscard]] auto columnCount(const QModelIndex &parent = QModelIndex()) const -> int override;

    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    auto setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole)
        -> bool override;

    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;

    [[nodiscard]] QVariant headerData(int section,
                                      Qt::Orientation orientation,
                                      int role = Qt::DisplayRole) const override;

    void setDatas(const Ffmpeg::EncodeContexts &encodeContexts);
    Ffmpeg::EncodeContexts datas() const;

private:
    class AudioEncoderModelPrivate;
    QScopedPointer<AudioEncoderModelPrivate> d_ptr;
};

#endif // AUDIOENCODERMODEL_HPP
