// LegendView.cpp

#include "legendview.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QPixmap>
#include <QStringList>
#include <QToolButton>
#include <QVBoxLayout>

// Container for the legend's search/list controls.
// Overrides sizeHint() to halve the natural layout width so the splitter
// allocates a compact initial size rather than whatever QListWidget prefers.
class LegendContent : public QWidget
{
public:
    using QWidget::QWidget;
    QSize sizeHint() const override
    {
        QSize sh = QWidget::sizeHint();
        //sh.setWidth(sh.width() / 2);
        sh.setWidth(128);
        return sh;
    }
};

LegendView::LegendView(QWidget *parent)
    : QWidget(parent)
{
    // Top row: "Legend" label + toggle button
    titleLabel = new QLabel(tr("Legend"), this);
    QFont f = titleLabel->font();
    f.setBold(true);
    titleLabel->setFont(f);

    panelButton = new QToolButton(this);
    panelButton->setIcon(QIcon(":/qdatabrowser/icons/lucide/panel-right-close.svg"));
    panelButton->setCheckable(true);
    panelButton->setChecked(true);
    panelButton->setAutoRaise(true);
    panelButton->setToolTip(tr("Toggle Legend Panel"));

    auto *topRow = new QHBoxLayout;
    topRow->setContentsMargins(0, 0, 0, 0);
    topRow->addWidget(titleLabel, 1);
    topRow->addWidget(panelButton);

    // Content area: search + select-all + list
    searchEdit = new QLineEdit;
    searchEdit->setPlaceholderText(tr("Search..."));
    searchEdit->setClearButtonEnabled(true);

    selectAllBox = new QCheckBox(tr("(Select All)"));
    selectAllBox->setTristate(true);
    selectAllBox->setCheckState(Qt::Checked);

    listWidget = new QListWidget;
    listWidget->setUniformItemSizes(true);
    listWidget->setSelectionMode(QAbstractItemView::NoSelection);
    listWidget->setIconSize({16, 16});

    contentWidget = new LegendContent;
    auto *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(4);
    contentLayout->addWidget(searchEdit);
    contentLayout->addWidget(selectAllBox);
    contentLayout->addWidget(listWidget, 1);

    // Main layout
    auto *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(4);
    vbox->addLayout(topRow);
    vbox->addWidget(contentWidget, 1);
    vbox->addStretch(0);

    // setMaximumWidth(220);

    // Default colors matching FilterView
    QVector<QRgb> defaults;
    defaults << qRgb(0, 0, 125) << qRgb(125, 0, 0) << qRgb(0, 125, 0);
    setColorOrder(defaults);

    connect(panelButton, &QToolButton::toggled, this, &LegendView::onPanelToggled);
    connect(searchEdit, &QLineEdit::textChanged, this, &LegendView::applySearch);
    connect(selectAllBox, &QCheckBox::stateChanged, this, &LegendView::onSelectAllChanged);
    connect(listWidget, &QListWidget::itemChanged, this, &LegendView::onItemChanged);
}

void LegendView::setValues(const QStringList &values, const QVector<uint> &checked)
{
    updating = true;
    listWidget->clear();

    const QSet<uint> checkedSet(checked.begin(), checked.end());
    const bool checkAll = checked.empty();

    const int nIcons = iconOrder.size();
    for (int k = 0; k < values.size(); ++k)
    {
        QIcon ico = nIcons > 0 ? iconOrder[k % nIcons] : QIcon();
        auto *item = new QListWidgetItem(ico, values[k], listWidget);
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        const bool on = checkAll || checkedSet.contains(k);
        item->setCheckState(on ? Qt::Checked : Qt::Unchecked);
    }

    updating = false;
    updateSelectAllState();
}

void LegendView::setColorOrder(const QVector<QRgb> &v)
{
    colorOrder = v;
    iconOrder.clear();

    QIcon baseIcon(":/qdatabrowser/icons/lucide/chart-spline.svg");
    for (int i = 0; i < colorOrder.size(); ++i)
    {
        QPixmap pix = baseIcon.pixmap(24, 24);
        if (pix.isNull())
            continue;
        QPixmap colored(pix.size());
        colored.fill(Qt::transparent);
        QPainter p(&colored);
        p.drawPixmap(0, 0, pix);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(colored.rect(), QColor(colorOrder[i]));
        p.end();
        iconOrder.push_back(QIcon(colored));
    }
}

int LegendView::count() const
{
    return listWidget->count();
}

int LegendView::collapsedWidth() const
{
    const auto m = contentsMargins();
    //auto btWidth = panelButton->width();
    auto btWidth = panelButton->sizeHint().width();
    return m.left() + btWidth + m.right();
}

bool LegendView::isCollapsed() const
{
    return !(panelButton->isChecked());
}

void LegendView::setCollapsed(bool collapsed)
{
    panelButton->setChecked(!collapsed);
}

QVector<uint> LegendView::checkedValues() const
{
    QVector<uint> result;
    for (int i = 0; i < listWidget->count(); ++i)
        if (listWidget->item(i)->checkState() == Qt::Checked)
            result << static_cast<uint>(i);
    return result;
}

void LegendView::setCheckedValues(const QVector<uint> &v)
{
    int n = listWidget->count();
    for (int i = 0; i < n; ++i)
        listWidget->item(i)->setCheckState(Qt::Unchecked);
    for (int i = 0; i < v.size(); ++i) {
        if (v[i] < (uint) n)
            listWidget->item(v[i])->setCheckState(Qt::Checked);
    }
}

void LegendView::applySearch(const QString &text)
{
    for (int i = 0; i < listWidget->count(); ++i)
    {
        QListWidgetItem *item = listWidget->item(i);
        item->setHidden(!item->text().contains(text, Qt::CaseInsensitive));
    }
}

void LegendView::onSelectAllChanged(int state)
{
    if (updating)
        return;
    if (state == Qt::PartiallyChecked)
    {
        updating = true;
        selectAllBox->setCheckState(Qt::Checked);
        updating = false;
        state = Qt::Checked;
    }

    updating = true;
    const Qt::CheckState target = (state == Qt::Checked) ? Qt::Checked : Qt::Unchecked;
    for (int i = 0; i < listWidget->count(); ++i)
    {
        QListWidgetItem *item = listWidget->item(i);
        if (!item->isHidden())
            item->setCheckState(target);
    }
    updating = false;
    emit selectionChanged(checkedValues());
}

void LegendView::onItemChanged(QListWidgetItem *)
{
    if (updating)
        return;
    updateSelectAllState();
    emit selectionChanged(checkedValues());
}

void LegendView::updateSelectAllState()
{
    int checked = 0, total = 0;
    for (int i = 0; i < listWidget->count(); ++i)
    {
        ++total;
        if (listWidget->item(i)->checkState() == Qt::Checked)
            ++checked;
    }
    updating = true;
    if (checked == 0)
        selectAllBox->setCheckState(Qt::Unchecked);
    else if (checked == total)
        selectAllBox->setCheckState(Qt::Checked);
    else
        selectAllBox->setCheckState(Qt::PartiallyChecked);
    updating = false;
}

void LegendView::onPanelToggled(bool open)
{
    titleLabel->setVisible(open);
    contentWidget->setVisible(open);
    panelButton->setIcon(QIcon(open ? ":/qdatabrowser/icons/lucide/panel-right-close.svg"
                                    : ":/qdatabrowser/icons/lucide/panel-right-open.svg"));
    // Force the splitter to honour the collapsed width; remove the cap when reopening.
    if (open)
        setMaximumWidth(QWIDGETSIZE_MAX);
    else
        setMaximumWidth(collapsedWidth());
    emit panelToggled(open);
}
