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

inline void DataSlice::clear()
{
    dim_idx_.clear();
    dim_order_.clear();
    i0_.clear();
    data_.clear();
    x_.clear();
    y_.clear();
    dim_ = {0, 0};
    name_.clear();
    desc_.clear();
    dim_name_.clear();
    dim_desc_.clear();
    D_ = nullptr;
}

inline void DataSlice::assign(const AbstractDataStore *d, size_t dx, const dim_t &i0)
{
    clear();
    D_ = d;
    dim_idx_ = {dx, 0UL - 1};
    dim_ = {D_->dim()[dx], 1};
    size_t sz = dim_[0];
    data_.resize(sz);
    x_.resize(sz);
    y_.resize(1);
    D_->get_x(dx, sz, x_.data());
    y_[0] = 0;

    assign_(i0);

    // names etc
    name_ = d->name();
    desc_ = d->description();
    dim_name_.push_back(d->dim_name(dx));
    dim_desc_.push_back(d->dim_desc(dx));

    // build dim order
    dim_order_.clear();
    dim_order_.push_back(dx);
    for (int i = 0; i < D_->dim_.size(); ++i) {
        if (i != dx)
            dim_order_.push_back(i);
    }
}

inline void DataSlice::assign(const AbstractDataStore *d, size_t dx, size_t dy, const dim_t &i0)
{
    clear();
    D_ = d;
    dim_idx_ = {dx, dy};

    dim_ = {D_->dim()[dx], D_->dim()[dy]};
    size_t sz = dim_[0] * dim_[1];
    data_.resize(sz);
    x_.resize(dim_[0]);
    y_.resize(dim_[1]);
    D_->get_x(dx, dim_[0], x_.data());
    D_->get_x(dy, dim_[1], y_.data());

    assign_(i0);

    // names etc
    name_ = d->name();
    desc_ = d->description();
    dim_name_.push_back(d->dim_name(dx));
    dim_desc_.push_back(d->dim_desc(dx));
    dim_name_.push_back(d->dim_name(dy));
    dim_desc_.push_back(d->dim_desc(dy));

    // build dim order
    dim_order_.clear();
    dim_order_.push_back(dx);
    dim_order_.push_back(dy);
    for (int i = 0; i < D_->dim_.size(); ++i) {
        if (i != dx && i != dy)
            dim_order_.push_back(i);
    }
}

inline void DataSlice::assign(const AbstractDataStore *d, size_t dims)
{
    clear();
    D_ = d;
    if (!d || d->empty())
        return;
    if (dims < 1 || dims > 2)
        dims = 2;
    dims = std::min(dims, d->ndim());

    // get dimensions
    dim_t dim = D_->dim();
    dim_t i0(dim.size(), 0);

    // find max dim
    auto it = find_max(dim);
    size_t dx = it - dim.begin();

    if (dims == 1) {
        assign(d, dx, i0);
        return;
    }

    // find next max
    dim[dx] = 0;
    it = find_max(dim);
    size_t dy = it - dim.begin();
    assign(d, dx, dy, i0);
}

void DataSlice::assign(const dim_t &new_i0)
{
    assert(i0_.size() == new_i0.size());
    assign_(new_i0);
}

void DataSlice::export_csv(std::ostream &os)
{
    if (empty())
        return;

    if (ndim() == 1) {
        os << "x,y" << std::endl;
        for (size_t i = 0; i < dim_[0]; ++i)
            os << x_[i] << ", " << data_[i] << std::endl;
    } else {
        for (size_t i = 0; i < dim_[0]; ++i) {
            os << data_[i];
            for (size_t j = 1; j < dim_[1]; ++j) {
                os << ", " << data_[i + j * dim_[0]];
            }
            os << std::endl;
        }
    }
}

void DataSlice::assign_(const dim_t &new_i0)
{
    i0_ = new_i0;
    if (ndim() == 1) {
        i0_[dx()] = 0;
        D_->get_y(dx(), i0_, dim_[0], data_.data());
    } else {
        i0_[dx()] = 0;
        i0_[dy()] = 0;
        double *p = data_.data();
        dim_t j1(i0_);
        for (size_t i = 0; i < dim_[1]; ++i) {
            j1[dy()] = i;
            D_->get_y(dx(), j1, dim_[0], p);
            p += dim_[0];
        }
    }
}

inline size_t DataSlice::get_y(size_t d, const dim_t &i0, size_t n, double *v) const
{
    return 0;
}

inline size_t DataSlice::get_x(size_t d, size_t n, double *v) const
{
    const vec_t &z = (d) ? y_ : x_;
    for (size_t i = 0; i < n; ++i) {
        v[i] = z[i];
    }
    return n;
}

/************      **************/

QDataSliceSelector::QDataSliceSelector(QWidget *parent)
    : QWidget{parent}
{
    // layout
    QVBoxLayout *vbox = new QVBoxLayout;
    setLayout(vbox);
    vbox->setSpacing(12);

    /* top tool bar */
    {
        QHBoxLayout *hbox = new QHBoxLayout;
        hbox->setContentsMargins(0, 0, 0, 0);

        QLabel *lbl;
        QFrame *frm;

        lbl = new QLabel;
        lbl->setPixmap(QIcon(":/qdatabrowser/lucide/chart-pie.svg").pixmap(16, 16));
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
        btExchangeXY->setIcon(QIcon(":/qdatabrowser/lucide/arrow-left-right.svg"));
        btExchangeXY->setAutoRaise(true);
        //btExchangeXY->setEnabled(false);
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
        //gridPanel->setStyleSheet("border: 1px solid black");

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

void QDataSliceSelector::assign(AbstractDataStore *D, int dim)
{
    clear();
    slice_.assign(D, dim);
    initCtrls();
    updateCtrls(All);
    connectCtrls();
    emit sliceReset();
}

void QDataSliceSelector::clearCtrls()
{
    cbX->clear();
    cbY->clear();
    cbY->setEnabled(false);
    btExchangeXY->setEnabled(false);
    QLayoutItem *i;
    while ((i = grid->takeAt(0)) != nullptr) {
        delete i;
    }

    for (auto &e : gridElements) {
        e.label->deleteLater();
        e.slider->deleteLater();
        e.value->deleteLater();
    }
    gridElements.clear();

    gridPanel->hide();
}

void QDataSliceSelector::initCtrls()
{
    const AbstractDataStore *D = slice_.dataStore();
    if (D == nullptr)
        return;

    int ndim = D->ndim();
    int slice_dim = slice_.ndim();
    int nrows = std::max(ndim - slice_dim, 0);

    for (int i = 0; i < ndim; ++i) {
        cbX->addItem(dimLabel(i));
    }
    cbX->setCurrentIndex(slice_.dx());

    if (slice_dim == 2) {
        cbY->setEnabled(true);
        btExchangeXY->setEnabled(true);
        for (int i = 0; i < ndim; ++i) {
            cbY->addItem(dimLabel(i));
        }
        cbY->setCurrentIndex(slice_.dy());
    }

    if (!nrows)
        return;

    gridElements.resize(nrows);
    int k = ndim - nrows;
    for (int r = 0; r < nrows; ++r, ++k) {
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

    if (f == XYex) {
        cbX->setCurrentIndex(slice_.dx());
        cbY->setCurrentIndex(slice_.dy());
        return;
    }

    auto &i0 = slice_.i0();
    auto &dim_order = slice_.dim_order();
    int d0 = slice_.ndim();
    auto x = slice_.dataStore()->x(slice_.i0());

    switch (f) {
    case All:
        cbX->setCurrentIndex(slice_.dx());
        if (cbY->isEnabled())
            cbY->setCurrentIndex(slice_.dy());
        // remake sliders
        for (int i = 0; i < gridElements.size(); ++i) {
            auto &e = gridElements[i];
            e.d = dim_order[d0 + i];
            e.label->setText(dimLabel(e.d));
            size_t n = slice_.dataStore()->dim()[e.d];
            e.slider->setRange(0, n - 1);
            size_t page = 1;
            size_t nticks = n;
            while (nticks > maxTicks) {
                page++;
                nticks = n / page;
            }
            e.slider->setTickInterval(page);
            e.slider->setPageStep(page);
        }
    case SldrOnly:
        for (auto &e : gridElements) {
            e.slider->setValue(slice_.i0()[e.d]);
            e.value->setText(QString("%1: %2").arg(slice_.i0()[e.d]).arg(x[e.d]));
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
    for (auto &e : gridElements) {
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
    for (auto &e : gridElements) {
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
    const AbstractDataStore *D = slice_.dataStore();
    return QString("%1 [n=%2]").arg(D->dim_name(d).c_str()).arg(D->dim()[d]);
}

void QDataSliceSelector::onX(int new_dx)
{
    blockCtrls(true);

    updFlag f = All;

    auto i0 = slice_.i0();
    if (slice_.ndim() == 1) {
        slice_.assign(slice_.dataStore(), new_dx, i0);
    } else {
        int dy = slice_.dy();
        if (dy == new_dx) {
            int dx = slice_.dx();
            slice_.assign(slice_.dataStore(), new_dx, dx, i0);
            f = XYex;
        } else
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
    if (dx == new_dy) {
        int dy = slice_.dy();
        slice_.assign(slice_.dataStore(), dy, new_dy, i0);
        f = XYex;
    } else
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
    for (int i = 0; i < gridElements.size(); ++i) {
        gridElement &e = gridElements[i];
        if (sender() == e.slider) {
            i0[e.d] = v;
            slice_.assign(i0);
            break;
        }
    }

    updateCtrls(SldrOnly);

    blockCtrls(false);

    emit sliceChanged();
}
