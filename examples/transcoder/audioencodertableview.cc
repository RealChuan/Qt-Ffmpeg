#include "audioencodertableview.hpp"
#include "audioencodermodel.hpp"
#include "styleditemdelegate.hpp"

#include <QHeaderView>

class AudioEncoderTableView::AudioEncoderTableViewPrivate
{
public:
    explicit AudioEncoderTableViewPrivate(AudioEncoderTableView *q)
        : q_ptr(q)
    {
        model = new AudioEncoderModel(q_ptr);
    }

    AudioEncoderTableView *q_ptr;

    AudioEncoderModel *model;
};

AudioEncoderTableView::AudioEncoderTableView(QWidget *parent)
    : QTableView{parent}
    , d_ptr(new AudioEncoderTableViewPrivate(this))
{
    setupUI();
}

AudioEncoderTableView::~AudioEncoderTableView() {}

void AudioEncoderTableView::setDatas(const Ffmpeg::EncodeContexts &encodeContexts)
{
    d_ptr->model->setDatas(encodeContexts);
}

Ffmpeg::EncodeContexts AudioEncoderTableView::datas() const
{
    return d_ptr->model->datas();
}

void AudioEncoderTableView::setupUI()
{
    setModel(d_ptr->model);

    setShowGrid(true);
    setWordWrap(false);
    setAlternatingRowColors(true);
    verticalHeader()->setVisible(false);
    verticalHeader()->setDefaultSectionSize(35);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setDefaultSectionSize(120);
    horizontalHeader()->setMinimumSectionSize(60);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    horizontalHeader()->setSectionResizeMode(AudioEncoderModel::Property::Encoder,
                                             QHeaderView::Stretch);
    setIconSize(QSize(20, 20));

    setItemDelegateForColumn(AudioEncoderModel::Property::Encoder, new AudioEncoderDelegate(this));
    setItemDelegateForColumn(AudioEncoderModel::Property::ChannelLayout,
                             new ChannelLayoutDelegate(this));
    setItemDelegateForColumn(AudioEncoderModel::Property::Profile, new ProfileDelegate(this));

    setColumnWidth(AudioEncoderModel::Property::ID, 50);
    setColumnWidth(AudioEncoderModel::Property::ChannelLayout, 100);
    setColumnWidth(AudioEncoderModel::Property::Bitrate, 100);
    setColumnWidth(AudioEncoderModel::Property::Crf, 50);
    setColumnWidth(AudioEncoderModel::Property::Profile, 100);
}
