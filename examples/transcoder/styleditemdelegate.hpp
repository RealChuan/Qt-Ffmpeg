#ifndef STYLEDITEMDELEGATE_HPP
#define STYLEDITEMDELEGATE_HPP

#include <QStyledItemDelegate>

class AudioEncoderDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit AudioEncoderDelegate(QObject *parent = nullptr);

    auto createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const
        -> QWidget *Q_DECL_OVERRIDE;
    void setEditorData(QWidget *editor, const QModelIndex &index) const Q_DECL_OVERRIDE;
    void setModelData(QWidget *editor,
                      QAbstractItemModel *model,
                      const QModelIndex &index) const Q_DECL_OVERRIDE;
};

class ChannelLayoutDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ChannelLayoutDelegate(QObject *parent = nullptr);

    auto createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const
        -> QWidget *Q_DECL_OVERRIDE;
    void setEditorData(QWidget *editor, const QModelIndex &index) const Q_DECL_OVERRIDE;
    void setModelData(QWidget *editor,
                      QAbstractItemModel *model,
                      const QModelIndex &index) const Q_DECL_OVERRIDE;
};

class ProfileDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ProfileDelegate(QObject *parent = nullptr);

    auto createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const
        -> QWidget *Q_DECL_OVERRIDE;
    void setEditorData(QWidget *editor, const QModelIndex &index) const Q_DECL_OVERRIDE;
    void setModelData(QWidget *editor,
                      QAbstractItemModel *model,
                      const QModelIndex &index) const Q_DECL_OVERRIDE;
};

#endif // STYLEDITEMDELEGATE_HPP
