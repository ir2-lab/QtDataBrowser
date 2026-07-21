// AxisValueSelect.cpp

#include "axisvalueselect.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QSlider>
#include <QHBoxLayout>
#include <QToolButton>
#include <QIcon>

#define ICON_SIZE 16
#define BUTTON_SIZE 20
#define ROW_HEIGHT 20

// ── AxisValueSelect ────────────────────────────────────────────────────────────

AxisValueSelect::AxisValueSelect(QWidget *parent)
    : QWidget(parent)
{
    m_combo = new QComboBox(this);
    m_combo->setFocusPolicy(Qt::StrongFocus);
    m_combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_combo->setToolTip("Other dimensions");

    m_sliderBar = new HSliderWithButtons(this);
    m_sliderBar->setSingleStep(1);
    m_sliderBar->setPageStep(1);
    //m_sliderBar->setFixedHeight(ROW_HEIGHT);

    m_valueLabel = new QLineEdit(this);
    m_valueLabel->setReadOnly(true);
    m_valueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_valueLabel->setToolTip("Datapoint index: value");
    //m_valueLabel->setFixedHeight(ROW_HEIGHT);
    m_valueLabel->setFixedWidth(120);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    layout->addWidget(m_combo);
    layout->addWidget(m_sliderBar, 1);
    layout->addWidget(m_valueLabel);

    connect(m_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int ci) { loadAxis(ci); });

    connect(m_sliderBar, &HSliderWithButtons::valueChanged, this, [this](int value) {
        if (m_loading)
            return;
        int ci = m_combo->currentIndex();
        if (ci < 0)
            return;
        int id = m_combo->itemData(ci).toInt();
        int ai = axisIndexById(id);
        if (ai < 0)
            return;
        m_axes[ai].currentIndex = value;
        updateDisplay();
        emit valueChanged(id, value);
    });

    hide();
}

void AxisValueSelect::insertAxis(int id, const QString &name,
                                const QStringList &values, int page)
{
    int ai = axisIndexById(id);
    if (ai >= 0) {
        AxisData &a = m_axes[ai];
        a.name = name;
        a.values = values;
        a.pageStep = page;
        a.currentIndex = qBound(0, a.currentIndex, qMax(0, values.size() - 1));

        for (int i = 0; i < m_combo->count(); ++i) {
            if (m_combo->itemData(i).toInt() == id) {
                m_combo->setItemText(i, name);
                break;
            }
        }
        if (m_combo->currentData().toInt() == id)
            loadAxis(m_combo->currentIndex());
        return;
    }

    AxisData a;
    a.id = id;
    a.name = name;
    a.values = values;
    a.pageStep = page;
    a.currentIndex = 0;
    m_axes.append(a);

    bool wasEmpty = (m_combo->count() == 0);
    m_combo->addItem(name, id);

    if (wasEmpty) {
        show();
        loadAxis(0);
    }
}

void AxisValueSelect::removeAxis(int id)
{
    int ai = axisIndexById(id);
    if (ai < 0)
        return;

    m_axes.removeAt(ai);

    for (int i = 0; i < m_combo->count(); ++i) {
        if (m_combo->itemData(i).toInt() == id) {
            m_combo->removeItem(i);
            break;
        }
    }

    if (m_combo->count() == 0) {
        m_sliderBar->setRange(0, 0);
        m_valueLabel->clear();
        hide();
    }
}

void AxisValueSelect::clearAxes()
{
    m_axes.clear();
    m_combo->blockSignals(true);
    m_combo->clear();
    m_combo->blockSignals(false);
    m_sliderBar->setRange(0, 0);
    m_valueLabel->clear();
    hide();
}

int AxisValueSelect::axisCount() const
{
    return m_axes.size();
}

int AxisValueSelect::currentIndex(int id) const
{
    int ai = axisIndexById(id);
    return ai >= 0 ? m_axes.at(ai).currentIndex : -1;
}

void AxisValueSelect::setValue(int id, int index)
{
    int ai = axisIndexById(id);
    if (ai >= 0) {
        m_axes[ai].currentIndex = index;
        if (ai == m_combo->currentIndex()) {
            m_sliderBar->setValue(index);
            updateDisplay();
        }
    }
}

// ── Private ───────────────────────────────────────────────────────────────────

int AxisValueSelect::axisIndexById(int id) const
{
    for (int i = 0; i < m_axes.size(); ++i)
        if (m_axes.at(i).id == id)
            return i;
    return -1;
}

void AxisValueSelect::loadAxis(int comboIndex)
{
    if (comboIndex < 0 || comboIndex >= m_combo->count())
        return;

    int id = m_combo->itemData(comboIndex).toInt();
    int ai = axisIndexById(id);
    if (ai < 0)
        return;

    const AxisData &a = m_axes.at(ai);

    m_loading = true;

    if (a.values.isEmpty()) {
        m_sliderBar->setRange(0, 0);
        m_sliderBar->setEnabled(false);
        m_valueLabel->clear();
    } else {
        m_sliderBar->setEnabled(true);
        m_sliderBar->setPageStep(a.pageStep);
        m_sliderBar->setTickInterval(a.pageStep);
        m_sliderBar->setTickPosition(QSlider::TicksBothSides);
        m_sliderBar->setRange(0, a.values.size() - 1);
        m_sliderBar->setValue(a.currentIndex);
        updateDisplay();
    }

    m_loading = false;
}

void AxisValueSelect::updateDisplay()
{
    int ci = m_combo->currentIndex();
    if (ci < 0)
        return;
    int id = m_combo->itemData(ci).toInt();
    int ai = axisIndexById(id);
    if (ai < 0)
        return;
    const AxisData &a = m_axes.at(ai);
    int i = m_sliderBar->value();
    m_valueLabel->setText((i >= 0 && i < a.values.size()) ? a.values.at(i) : QString{});
}

// ── HSliderWithButtons ────────────────────────────────────────────────────────

static QToolButton *makeButton(const QString &iconPath, QWidget *parent)
{
    auto *btn = new QToolButton(parent);
    btn->setIcon(QIcon(iconPath));
    btn->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
    btn->setFocusPolicy(Qt::NoFocus);
    btn->setAutoRepeat(true);
    btn->setAutoRepeatDelay(400);
    btn->setAutoRepeatInterval(80);
    //btn->setFixedSize(BUTTON_SIZE, BUTTON_SIZE);
    return btn;
}

HSliderWithButtons::HSliderWithButtons(QWidget *parent)
    : QWidget(parent)
{
    m_slider = new QSlider(Qt::Horizontal, this);
    m_slider->setToolTip("Datapoint selector");
    m_prevPage = makeButton(QStringLiteral(":/qdatabrowser/icons/lucide/chevrons-left.svg"), this);
    m_prevPage->setToolTip("Previous page");
    m_nextPage = makeButton(QStringLiteral(":/qdatabrowser/icons/lucide/chevrons-right.svg"), this);
    m_nextPage->setToolTip("Next page");
    m_prevStep = makeButton(QStringLiteral(":/qdatabrowser/icons/lucide/chevron-left.svg"), this);
    m_prevStep->setToolTip("Previous datapoint");
    m_nextStep = makeButton(QStringLiteral(":/qdatabrowser/icons/lucide/chevron-right.svg"), this);
    m_nextStep->setToolTip("Next datapoint");

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(1);
    layout->addWidget(m_prevPage);
    layout->addWidget(m_prevStep);
    layout->addWidget(m_slider, 1);
    layout->addWidget(m_nextStep);
    layout->addWidget(m_nextPage);

    connect(m_prevPage, &QToolButton::clicked, this, [this] { page(false); });
    connect(m_prevStep, &QToolButton::clicked, this, [this] {
        m_slider->setValue(m_slider->value() - m_slider->singleStep());
    });
    connect(m_nextStep, &QToolButton::clicked, this,
            [this] { m_slider->setValue(m_slider->value() + m_slider->singleStep()); });
    connect(m_nextPage, &QToolButton::clicked, this, [this] { page(true); });

    connect(m_slider, &QSlider::valueChanged, this, &HSliderWithButtons::valueChanged);
    connect(m_slider, &QSlider::sliderPressed, this, &HSliderWithButtons::sliderPressed);
    connect(m_slider, &QSlider::sliderMoved, this, &HSliderWithButtons::sliderMoved);
    connect(m_slider, &QSlider::sliderReleased, this, &HSliderWithButtons::sliderReleased);
    connect(m_slider, &QSlider::rangeChanged, this, &HSliderWithButtons::rangeChanged);
    connect(m_slider, &QSlider::actionTriggered, this, &HSliderWithButtons::actionTriggered);
}

int HSliderWithButtons::minimum() const { return m_slider->minimum(); }
int HSliderWithButtons::maximum() const { return m_slider->maximum(); }
int HSliderWithButtons::value() const { return m_slider->value(); }
int HSliderWithButtons::singleStep() const { return m_slider->singleStep(); }
int HSliderWithButtons::pageStep() const { return m_slider->pageStep(); }

void HSliderWithButtons::setMinimum(int min) { m_slider->setMinimum(min); }
void HSliderWithButtons::setMaximum(int max) { m_slider->setMaximum(max); }
void HSliderWithButtons::setRange(int min, int max) { m_slider->setRange(min, max); }
void HSliderWithButtons::setSingleStep(int step) { m_slider->setSingleStep(step); }
void HSliderWithButtons::setPageStep(int step) { m_slider->setPageStep(step); }
void HSliderWithButtons::setValue(int value) { m_slider->setValue(value); }

void HSliderWithButtons::page(bool up)
{
    int pos = m_slider->value();
    int step = m_slider->pageStep();

    int rem = pos % step;

    if (up)
        pos = pos - rem + step;
    else
        pos = rem ? pos - rem : pos - step;

    m_slider->setValue(pos);
}

void HSliderWithButtons::setTickPosition(QSlider::TickPosition pos) { m_slider->setTickPosition(pos); }
void HSliderWithButtons::setTickInterval(int ti) { m_slider->setTickInterval(ti); }

QSlider *HSliderWithButtons::slider() const { return m_slider; }
