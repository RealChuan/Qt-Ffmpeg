#include "outputwidget.hpp"

#include <QtWidgets>

class OutPutWidget::OutPutWidgetPrivate
{
public:
    explicit OutPutWidgetPrivate(OutPutWidget *q)
        : q_ptr(q)
    {
        outLineEdit = new QLineEdit(q_ptr);
    }

    OutPutWidget *q_ptr;

    QLineEdit *outLineEdit;
};

OutPutWidget::OutPutWidget(QWidget *parent)
    : QWidget{parent}
    , d_ptr(new OutPutWidgetPrivate(this))
{
    setupUI();
    buildConnect();
}

OutPutWidget::~OutPutWidget() {}

void OutPutWidget::setOutputFileName(const QString &fileName)
{
    const auto path = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)
                          .value(0, QDir::homePath());
    d_ptr->outLineEdit->setText(QDir(path).filePath(fileName));
}

auto OutPutWidget::outputFilePath() const -> QString
{
    return d_ptr->outLineEdit->text().trimmed();
}

void OutPutWidget::onBrowse()
{
    const auto filePath = QFileDialog::getSaveFileName(this,
                                                       tr("Save File"),
                                                       d_ptr->outLineEdit->text().trimmed());
    if (filePath.isEmpty()) {
        return;
    }
    d_ptr->outLineEdit->setText(filePath);
}

void OutPutWidget::setupUI()
{
    auto *button = new QToolButton(this);
    button->setText(tr("Browse"));
    connect(button, &QToolButton::clicked, this, &OutPutWidget::onBrowse);

    auto *layout = new QHBoxLayout(this);
    layout->addWidget(new QLabel(tr("Save File:"), this));
    layout->addWidget(d_ptr->outLineEdit);
    layout->addWidget(button);
}

void OutPutWidget::buildConnect() {}
