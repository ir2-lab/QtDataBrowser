#include "qdatasliceselector.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSlider>
#include <QToolButton>

QDataSliceSelector::QDataSliceSelector(QWidget *parent)
    : QWidget{parent}
{
    // setStyleSheet("border: 1px solid black");

    // layout
    QVBoxLayout *vbox = new QVBoxLayout;
    setLayout(vbox);
    vbox->setContentsMargins(0, 0, 0, 0);

    /* top tool bar */
    {
        QHBoxLayout *hbox = new QHBoxLayout;
        hbox->setContentsMargins(0, 0, 0, 0);

        QLabel *lbl;
        QFrame *frm;

        lbl = new QLabel;
        lbl->setPixmap(QIcon(":/qdatabrowser/icons/lucide/chart-pie.svg").pixmap(16, 16));
        hbox->addWidget(lbl);

        lbl = new QLabel("Slice Selector");
        hbox->addWidget(lbl);

        frm = new QFrame;
        frm->setFrameStyle(QFrame::VLine | QFrame::Sunken);
        hbox->addWidget(frm);

        lbl = new QLabel("X: ");
        hbox->addWidget(lbl);

        cbX = new QComboBox;
        cbX->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        hbox->addWidget(cbX);

        // frm = new QFrame;
        // frm->setFrameStyle(QFrame::VLine | QFrame::Sunken);
        // hbox->addWidget(frm);

        btExchangeXY = new QToolButton;
        btExchangeXY->setIcon(QIcon(":/qdatabrowser/icons/lucide/arrow-left-right.svg"));
        btExchangeXY->setAutoRaise(true);
        // btExchangeXY->setEnabled(false);
        btExchangeXY->setToolTip("X ↔ Y");
        hbox->addWidget(btExchangeXY);

        lbl = new QLabel("Y: ");
        hbox->addWidget(lbl);

        cbY = new QComboBox;
        cbY->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        hbox->addWidget(cbY);
        cbY->setEnabled(false);

        hbox->addStretch();

        vbox->addLayout(hbox);
    }

    /* create grid widget*/
    {
        gridPanel = new QWidget;
        // gridPanel->setStyleSheet("border: 1px solid black");

        grid = new QGridLayout;
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setSpacing(12);
        gridPanel->setLayout(grid);

        vbox->addWidget(gridPanel);
    }

    vbox->addStretch();
}

void QDataSliceSelector::clear()
{
    disconnectCtrls();
    clearCtrls();
    slice_.clear();
    emit sliceReset();
}

void QDataSliceSelector::assign(DataStorePtr D, int dim)
{
    clear();
    slice_.assign(D, dim);
    initCtrls();
    updateCtrls(All);
    connectCtrls();
    emit sliceReset();
}

void QDataSliceSelector::updateData()
{
    slice_.update();
    emit sliceChanged();
}

void QDataSliceSelector::clearCtrls()
{
    cbX->clear();
    cbY->clear();
    cbY->setEnabled(false);
    btExchangeXY->setEnabled(false);
    QLayoutItem *i;
    while ((i = grid->takeAt(0)) != nullptr)
    {
        delete i;
    }

    for (auto &e : gridElements)
    {
        e.label->deleteLater();
        e.slider->deleteLater();
        e.value->deleteLater();
    }
    gridElements.clear();

    gridPanel->hide();
}

void QDataSliceSelector::initCtrls()
{
    DataStorePtr D = slice_.dataStore();
    if (D == nullptr)
        return;

    int ndim = D->ndim();
    int slice_dim = slice_.ndim();
    int nrows = std::max(ndim - slice_dim, 0);

    for (int i = 0; i < ndim; ++i)
    {
        cbX->addItem(dimLabel(i));
    }
    cbX->setCurrentIndex(slice_.dx());

    if (slice_dim == 2)
    {
        cbY->setEnabled(true);
        btExchangeXY->setEnabled(true);
        for (int i = 0; i < ndim; ++i)
        {
            cbY->addItem(dimLabel(i));
        }
        cbY->setCurrentIndex(slice_.dy());
    }

    if (!nrows)
        return;

    gridElements.resize(nrows);
    int k = ndim - nrows;
    for (int r = 0; r < nrows; ++r, ++k)
    {
        gridElement &e = gridElements[r];
        e.d = slice_.dim_order()[k];
        e.label = new QLabel(dimLabel(e.d));
        e.slider = new QSlider(Qt::Horizontal);
        e.slider->setTickPosition(QSlider::TicksBothSides);
        e.value = new QLineEdit;
        e.value->setReadOnly(true);

        grid->addWidget(e.label, r, 0);
        grid->addWidget(e.slider, r, 1);
        grid->addWidget(e.value, r, 2);
    }

    grid->setColumnStretch(1, 1);

    gridPanel->show();
}

void QDataSliceSelector::updateCtrls(updFlag f)
{
    if (slice_.empty())
        return;

    if (f == XYex)
    {
        cbX->setCurrentIndex(slice_.dx());
        cbY->setCurrentIndex(slice_.dy());
        return;
    }

    auto &i0 = slice_.i0();
    auto &dim_order = slice_.dim_order();
    int d0 = slice_.ndim();

    switch (f)
    {
    case All:
        cbX->setCurrentIndex(slice_.dx());
        if (cbY->isEnabled())
            cbY->setCurrentIndex(slice_.dy());
        // remake sliders
        for (int i = 0; i < gridElements.size(); ++i)
        {
            auto &e = gridElements[i];
            e.d = dim_order[d0 + i];
            e.label->setText(dimLabel(e.d));
            size_t n = slice_.dataStore()->dim()[e.d];
            if (n > 1)
            {
                e.slider->setEnabled(true);
                e.slider->setRange(0, n - 1);
                size_t page = 1;
                size_t nticks = n;
                while (nticks > maxTicks)
                {
                    page++;
                    nticks = n / page;
                }
                e.slider->setTickPosition(QSlider::TicksBothSides);
                e.slider->setTickInterval(page);
                e.slider->setPageStep(page);
            }
            else
            {
                e.slider->setRange(0, 0);
                e.slider->setTickPosition(QSlider::NoTicks);
                e.slider->setEnabled(false);
            }
        }
        setSliderLabels();
    case SldrOnly:
        for (int i = 0; i < gridElements.size(); ++i)
        {
            auto &e = gridElements[i];
            if (e.slider->isEnabled())
            {
                size_t k = slice_.i0()[e.d];
                e.slider->setValue(k);
                e.value->setText(e.valueLbls.at(k));
            }
            else
            {
                e.value->setText(e.valueLbls.at(0));
            }
        }
    default:
        break;
    }
}

void QDataSliceSelector::connectCtrls()
{
    connect(cbX,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &QDataSliceSelector::onX);
    connect(cbY,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &QDataSliceSelector::onY);
    connect(btExchangeXY, &QToolButton::clicked, this, &QDataSliceSelector::onExchangeXY);
    for (auto &e : gridElements)
    {
        connect(e.slider, &QSlider::valueChanged, this, &QDataSliceSelector::onI0);
    }
}

void QDataSliceSelector::disconnectCtrls()
{
    disconnect(cbX,
               QOverload<int>::of(&QComboBox::currentIndexChanged),
               this,
               &QDataSliceSelector::onX);
    disconnect(cbY,
               QOverload<int>::of(&QComboBox::currentIndexChanged),
               this,
               &QDataSliceSelector::onY);
    disconnect(btExchangeXY, &QToolButton::clicked, this, &QDataSliceSelector::onExchangeXY);
    for (auto &e : gridElements)
    {
        disconnect(e.slider, &QSlider::valueChanged, this, &QDataSliceSelector::onI0);
    }
}

void QDataSliceSelector::blockCtrls(bool b)
{
    cbX->blockSignals(b);
    cbY->blockSignals(b);
    btExchangeXY->blockSignals(b);
    for (auto &e : gridElements)
        e.slider->blockSignals(b);
}

QString QDataSliceSelector::dimLabel(int d)
{
    DataStorePtr D = slice_.dataStore();
    if (!D)
        return QString();
    return QString("%1 [n=%2]").arg(D->dim_name(d).c_str()).arg(D->dim()[d]);
}

void QDataSliceSelector::setSliderLabels()
{
    DataStorePtr D = slice_.dataStore();
    if (!D)
        return;

    for (int i = 0; i < gridElements.size(); ++i)
    {
        auto &e = gridElements[i];
        e.valueLbls.clear();
        size_t n = D->dim()[e.d];
        if (D->is_x_categorical(e.d))
        {
            AbstractDataStore::strvec_t x(n);
            D->get_x_categorical(e.d, x);
            for (size_t i = 0; i < n; ++i)
                e.valueLbls.push_back(QString("%1: %2").arg(i).arg(x[i].c_str()));
        }
        else
        {
            AbstractDataStore::vec_t x(n);
            D->get_x(e.d, x);
            for (size_t i = 0; i < n; ++i)
                e.valueLbls.push_back(QString("%1: %2").arg(i).arg(x[i]));
        }
    }
}

void QDataSliceSelector::onX(int new_dx)
{
    blockCtrls(true);

    updFlag f = All;

    auto i0 = slice_.i0();
    if (slice_.ndim() == 1)
    {
        slice_.assign(slice_.dataStore(), new_dx, i0);
    }
    else
    {
        int dy = slice_.dy();
        if (dy == new_dx)
        {
            int dx = slice_.dx();
            slice_.assign(slice_.dataStore(), new_dx, dx, i0);
            f = XYex;
        }
        else
            slice_.assign(slice_.dataStore(), new_dx, dy, i0);
    }

    updateCtrls(f);

    blockCtrls(false);

    emit sliceChanged();
}

void QDataSliceSelector::onY(int new_dy)
{
    blockCtrls(true);

    updFlag f = All;

    auto i0 = slice_.i0();
    int dx = slice_.dx();
    if (dx == new_dy)
    {
        int dy = slice_.dy();
        slice_.assign(slice_.dataStore(), dy, new_dy, i0);
        f = XYex;
    }
    else
        slice_.assign(slice_.dataStore(), dx, new_dy, i0);

    updateCtrls(f);

    blockCtrls(false);

    emit sliceChanged();
}

void QDataSliceSelector::onExchangeXY(bool)
{
    blockCtrls(true);

    auto i0 = slice_.i0();
    int dx = slice_.dx();
    int dy = slice_.dy();
    slice_.assign(slice_.dataStore(), dy, dx, i0);

    updateCtrls(XYex);

    blockCtrls(false);

    emit sliceChanged();
}

void QDataSliceSelector::onI0(int v)
{
    blockCtrls(true);

    auto i0 = slice_.i0();
    for (int i = 0; i < gridElements.size(); ++i)
    {
        gridElement &e = gridElements[i];
        if (sender() == e.slider)
        {
            i0[e.d] = v;
            slice_.assign(i0);
            break;
        }
    }

    updateCtrls(SldrOnly);

    blockCtrls(false);

    emit sliceChanged();
}
