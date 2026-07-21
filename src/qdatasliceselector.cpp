#include "qdatasliceselector.h"
#include "axisvalueselect.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

QDataSliceSelector::QDataSliceSelector(QWidget *parent)
    : QWidget(parent)
{
    // x-y slice selector
    xy_ = new QWidget;
    QHBoxLayout *xyLayout_ = new QHBoxLayout;
    xyLayout_->setContentsMargins(0, 0, 0, 0);
    xy_->setLayout(xyLayout_);

    lblX_ = new QLabel("X:", this);
    lblX_->setToolTip("x-axis (rows) dimension");
    xyLayout_->addWidget(lblX_);

    cbX_ = new QComboBox(this);
    cbX_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    cbX_->setToolTip("x-axis (rows) dimension");
    xyLayout_->addWidget(cbX_);

    btExchangeXY_ = new QToolButton(this);
    btExchangeXY_->setIcon(QIcon(":/qdatabrowser/icons/lucide/arrow-left-right.svg"));
    btExchangeXY_->setAutoRaise(true);
    btExchangeXY_->setToolTip("X ↔ Y");
    xyLayout_->addWidget(btExchangeXY_);

    lblY_ = new QLabel("Y:", this);
    lblY_->setToolTip("y-axis (cols) dimension");
    xyLayout_->addWidget(lblY_);

    cbY_ = new QComboBox(this);
    cbY_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    cbY_->setToolTip("y-axis (cols) dimension");
    cbY_->setEnabled(false);
    xyLayout_->addWidget(cbY_);

    // other dims selector
    dimSelect_ = new AxisValueSelect(this);

    auto *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(2);
    vbox->addWidget(xy_);
    vbox->addWidget(dimSelect_);

    //setStyleSheet("border: 1px solid black;");
}

// ── Public API ────────────────────────────────────────────────────────────────

void QDataSliceSelector::setXLabel(const QString &text) { lblX_->setText(text); }
void QDataSliceSelector::setYLabel(const QString &text) { lblY_->setText(text); }

void QDataSliceSelector::getAxisValueLabels(int axisId, QStringList &labels)
{
    if (slice_.empty())
        return;
    DataSetPtr D = slice_.dataStore();
    size_t n = D->dim()[axisId];
    if (n < 1)
        return;
    if (D->is_x_categorical(axisId)) {
        AbstractDataSet::strvec_t x(n);
        D->get_x_categorical(axisId, x);
        for (size_t j = 0; j < n; ++j)
            labels.push_back(QString("%1: %2").arg(j).arg(x[j].c_str()));
    } else {
        AbstractDataSet::vec_t x(n);
        D->get_x(axisId, x);
        for (size_t j = 0; j < n; ++j)
            labels.push_back(QString("%1: %2").arg(j).arg(x[j]));
    }
}

void QDataSliceSelector::clear()
{
    disconnectCtrls();
    clearCtrls();
    slice_.clear();
    xy_->setVisible(false);
    dimSelect_->setVisible(false);
    emit sliceReset();
}

void QDataSliceSelector::assign(DataSetPtr D, int dim)
{
    clear();
    slice_.assign(D, dim);
    finishAssign(D);
}

void QDataSliceSelector::assign(DataSetPtr D, const State &prev)
{
    clear();

    bool restorable = D && !D->empty() && prev.i0.size() == D->ndim() && prev.dx >= 0
        && (size_t) prev.dx < D->ndim() && (prev.dy < 0 || (size_t) prev.dy < D->ndim());
    if (restorable)
    {
        for (size_t k = 0; k < prev.i0.size(); ++k)
        {
            if (prev.i0[k] >= D->dim()[k])
            {
                restorable = false;
                break;
            }
        }
    }

    if (restorable)
    {
        if (prev.dy >= 0 && prev.dy != prev.dx)
            slice_.assign(D, prev.dx, prev.dy, prev.i0);
        else
            slice_.assign(D, prev.dx, prev.i0);
    }
    else
    {
        slice_.assign(D, 2);
    }

    finishAssign(D);
}

void QDataSliceSelector::finishAssign(DataSetPtr D)
{
    if (!D)
        return;
    initCtrls();
    updateCtrls(All);
    connectCtrls();
    int ndim = D->ndim();
    int sdim = slice_.ndim();
    xy_->setVisible(sdim == 2);
    dimSelect_->setVisible(ndim > 2);
    emit sliceReset();
}

void QDataSliceSelector::updateData()
{
    slice_.update();
    emit sliceDataChanged();
}

QDataSliceSelector::State QDataSliceSelector::state() const
{
    State s;
    if (!slice_.empty())
    {
        s.dx = slice_.dx();
        s.dy = slice_.dy();
        s.i0 = slice_.i0();
    }
    return s;
}

// ── Private ───────────────────────────────────────────────────────────────────

void QDataSliceSelector::clearCtrls()
{
    cbX_->clear();
    cbY_->clear();
    cbY_->setEnabled(false);
    btExchangeXY_->setEnabled(false);
    dimSelect_->clearAxes();
}

void QDataSliceSelector::initCtrls()
{
    DataSetPtr D = slice_.dataStore();
    if (D == nullptr)
        return;

    int ndim = D->ndim();
    int slice_dim = slice_.ndim();

    for (int i = 0; i < ndim; ++i)
        cbX_->addItem(dimLabel(i));
    cbX_->setCurrentIndex(slice_.dx());

    if (slice_dim == 2)
    {
        cbY_->setEnabled(true);
        btExchangeXY_->setEnabled(true);
        for (int i = 0; i < ndim; ++i)
            cbY_->addItem(dimLabel(i));
        cbY_->setCurrentIndex(slice_.dy());
    }
}

void QDataSliceSelector::updateCtrls(updFlag f)
{
    if (slice_.empty())
        return;

    if (f == XYex)
    {
        cbX_->setCurrentIndex(slice_.dx());
        cbY_->setCurrentIndex(slice_.dy());
        return;
    }

    auto &dim_order = slice_.dim_order();
    int d0 = slice_.ndim();

    switch (f)
    {
    case All:

        cbX_->setCurrentIndex(slice_.dx());
        if (cbY_->isEnabled())
        {
            cbY_->setCurrentIndex(slice_.dy());
        }

        dimSelect_->clearAxes();
        {
            DataSetPtr D = slice_.dataStore();
            for (int i = d0; i < (int)dim_order.size(); ++i)
            {
                int d = dim_order[i];
                size_t n = D->dim()[d];
                //if (n <= 1)
                //    continue;
                QStringList values;
                getAxisValueLabels(d, values);
                size_t page = 1;
                size_t nticks = n;
                while (nticks > maxTicks)
                {
                    page++;
                    nticks = n / page;
                }
                dimSelect_->insertAxis(d, dimLabel(d), values, (int)page);
            }
            for (int i = d0; i < (int) dim_order.size(); ++i) {
                int d = dim_order[i];
                dimSelect_->setValue(d, slice_.i0()[d]);
            }
        }
        break;

    case SldrOnly:
    default:
        break;
    }
}

void QDataSliceSelector::connectCtrls()
{
    connect(cbX_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &QDataSliceSelector::onX);
    connect(cbY_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &QDataSliceSelector::onY);
    connect(btExchangeXY_, &QToolButton::clicked, this, &QDataSliceSelector::onExchangeXY);
    connect(dimSelect_, &AxisValueSelect::valueChanged, this, &QDataSliceSelector::onI0);
}

void QDataSliceSelector::disconnectCtrls()
{
    disconnect(cbX_, QOverload<int>::of(&QComboBox::currentIndexChanged),
               this, &QDataSliceSelector::onX);
    disconnect(cbY_, QOverload<int>::of(&QComboBox::currentIndexChanged),
               this, &QDataSliceSelector::onY);
    disconnect(btExchangeXY_, &QToolButton::clicked, this, &QDataSliceSelector::onExchangeXY);
    disconnect(dimSelect_, &AxisValueSelect::valueChanged, this, &QDataSliceSelector::onI0);
}

void QDataSliceSelector::blockCtrls(bool b)
{
    cbX_->blockSignals(b);
    cbY_->blockSignals(b);
    btExchangeXY_->blockSignals(b);
    dimSelect_->blockSignals(b);
}

QString QDataSliceSelector::dimLabel(int d)
{
    DataSetPtr D = slice_.dataStore();
    if (!D)
        return QString();
    return QString("D%1: %2 [n=%3]").arg(d).arg(D->dim_name(d).c_str()).arg(D->dim()[d]);
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

void QDataSliceSelector::onI0(int axisId, int v)
{
    blockCtrls(true);

    auto i0 = slice_.i0();
    i0[axisId] = v;
    slice_.assign(i0);

    updateCtrls(SldrOnly);
    blockCtrls(false);
    emit sliceChanged();
}
