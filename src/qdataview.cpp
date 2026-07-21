#include "qdataview.h"
#include "legendview.h"
#include "qdatasliceselector.h"
#include "value_with_error.h"

#include <QComboBox>
#include <QFont>
#include <QFontDatabase>
#include <QLabel>
#include <QMatPlotWidget>
#include <QMenu>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableView>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>

QAbstractDataView::QAbstractDataView(QWidget *parent)
    : QWidget{parent}
{
}

void QAbstractDataView::setSliceSelector(QDataSliceSelector *s)
{
    sliceSelector_ = s;
    slice_ = sliceSelector_->slice();
    connect(sliceSelector_, &QDataSliceSelector::sliceChanged, this, &QAbstractDataView::updateView);
    connect(sliceSelector_,
            &QDataSliceSelector::sliceDataChanged,
            this,
            &QAbstractDataView::updateData);
}

void QAbstractDataView::updateView()
{
    updateView_();
    emit viewUpdated();
}

void QAbstractDataView::updateData()
{
    int i = 0;
}

/************* QTabularDataView *******************/

class QDataTableModel : public QAbstractTableModel
{
public:
    QDataTableModel(QObject *parent = 0)
        : QAbstractTableModel(parent), slice_(nullptr)
    {
    }

    typedef value_with_error<double> value_error_t;
    typedef value_error_t::iosfmt iosfmt;
    iosfmt fmt_{std::defaultfloat};
    bool withErrors_{false};
    int precision_{6};

    QString formatValue(double v) const
    {
        std::ostringstream ss;
        ss << fmt_ << std::setprecision(precision_) << v;
        return QString::fromUtf8(ss.str().c_str());
    }

    QString formatValue(double v, double e) const
    {
        value_error_t ve(v, e, 1, fmt_, true);
        return QString::fromUtf8(ve.to_string().c_str());
    }

    void setFormat(iosfmt fmt, bool withErrors = false)
    {
        fmt_ = fmt;
        withErrors_ = withErrors;
        dataUpdated();
    }

    void setDataSlice(DataSlice *s)
    {
        beginResetModel();
        slice_ = s;
        endResetModel();
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if (!validSlice())
            return 0;
        return slice_->dim()[0];
    }
    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if (!validSlice())
            return 0;
        return slice_->ndim() == 1 ? 1 : slice_->dim()[1];
    }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || role != Qt::DisplayRole || slice_ == nullptr || slice_->empty())
            return QVariant();

        QString s;
        int r = index.row();
        int c = index.column();
        int k = r + slice_->dim()[0] * c;
        if (slice_->is_numeric()) {
            double v = slice_->data()[k];
            if (withErrors_ && slice_->hasErrors()) {
                double e = slice_->errors()[k];
                s = formatValue(v, e);
            } else
                s = formatValue(v);
        } else {
            s = QString::fromUtf8(slice_->text_data(r, c).c_str());
        }
        return s;
    }
    QVariant headerData(int i,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override
    {
        if (role != Qt::DisplayRole || slice_ == nullptr || slice_->empty())
            return QVariant();
        if (orientation == Qt::Vertical)
        {
            if (i < 0 || i >= rowCount())
                return QVariant();

            return (slice_->is_x_categorical(0)) ? QString(slice_->x_category()[i].c_str())
                                                 : QString::number(slice_->x(i));
        }
        else if (orientation == Qt::Horizontal)
        {
            if (i < 0 || i >= columnCount())
                return QVariant();

            return (slice_->is_x_categorical(1)) ? QVariant(slice_->y_category()[i].c_str())
                                                 : QString::number(slice_->y(i));
        }
        return QVariant();
    }
    void dataUpdated()
    {
        if (!validSlice())
            return;
        QModelIndex topLeft = index(0, 0);
        QModelIndex bottomRight = index(rowCount() - 1, columnCount() - 1);
        emit dataChanged(topLeft, bottomRight);
    }

private:
    DataSlice *slice_{nullptr};

    bool validSlice() const { return slice_ && !slice_->empty(); }
};

QTabularDataView::QTabularDataView(QWidget *parent)
    : QAbstractDataView(parent)
{
    /* Create table model & view */
    model_ = new QDataTableModel(this);
    view_ = new QTableView;
    view_->setModel(model_);

    title_ = new QLabel;
    title_->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    title_->setStyleSheet("font-weight: bold");

    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    view_->setFont(mono);

    scalarView_ = new QPlainTextEdit;
    scalarView_->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    scalarView_->setReadOnly(true);
    scalarView_->setFont(mono);

    stack_ = new QStackedWidget;
    stack_->addWidget(scalarView_);
    stack_->addWidget(view_);

    /* create layout */
    QVBoxLayout *vbox = new QVBoxLayout;
    setLayout(vbox);
    vbox->addWidget(title_);
    vbox->addWidget(stack_);

    createOptionsMenu();
    connect(optionsMenu_, &QMenu::aboutToShow, this, &QTabularDataView::updateOptionsMenu);
}

QIcon QTabularDataView::icon() const
{
    return QIcon(":/qdatabrowser/icons/lucide/sheet.svg");
}

void QTabularDataView::updateData()
{
    model_->dataUpdated();
}

void QTabularDataView::updateView_()
{
    model_->setDataSlice(slice_);
    if (!slice_->description().empty())
        title_->setText(slice_->description().c_str());
    else
        title_->setText(slice_->name().c_str());

    if (slice_->is_scalar())
    {
        if (slice_->is_numeric())
            scalarView_->setPlainText(QString::number((*slice_)(0, 0)));
        else
            scalarView_->setPlainText(slice_->text_data(0, 0).c_str());
        stack_->setCurrentIndex(0);
    }
    else
    {
        stack_->setCurrentIndex(1);
    }
}

QTabularDataView::State QTabularDataView::state() const
{
    State s;
    s.format = model_->fmt_;
    s.withErrors = model_->withErrors_;
    s.precision = model_->precision_;
    return s;
}

void QTabularDataView::setState(const State &s)
{
    model_->precision_ = s.precision;
    model_->setFormat(s.format, s.withErrors);
}

void QTabularDataView::createOptionsMenu()
{
    optionsMenu_ = new QMenu((QWidget *) this);

    QMenu *m = optionsMenu_;
    QAction *a;

    formatGroup = new QActionGroup(this);
    a = m->addAction("Fixed", this, [this]() {
        bool err = model_->withErrors_;
        model_->setFormat(std::fixed, err);
    });
    a->setCheckable(true);
    a->setChecked(model_->fmt_ == std::fixed);
    formatGroup->addAction(a);
    a = m->addAction("Scientific", this, [this]() {
        bool err = model_->withErrors_;
        model_->setFormat(std::scientific, err);
    });
    a->setCheckable(true);
    a->setChecked(model_->fmt_ == std::scientific);
    formatGroup->addAction(a);
    a = m->addAction("Auto", this, [this]() {
        bool err = model_->withErrors_;
        model_->setFormat(std::defaultfloat, err);
    });
    a->setCheckable(true);
    a->setChecked(model_->fmt_ == std::defaultfloat);
    formatGroup->addAction(a);

    optionsMenu_->addSeparator();

    auto *wa = new QWidgetAction(m);
    QWidget *w = new QWidget(m);
    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->setContentsMargins(6, 6, 6, 6);
    w->setLayout(hbox);
    QLabel *lbl = new QLabel("Precision");
    QSpinBox *spin = new QSpinBox;
    spin->setRange(1, 18);
    spin->setValue(model_->precision_);
    hbox->addWidget(lbl);
    hbox->addWidget(spin);

    wa->setDefaultWidget(w);
    m->addAction(wa);
    connect(spin, SIGNAL(valueChanged(int)), this, SLOT(setPrecision(int)));

    optionsMenu_->addSeparator();

    errorAct = m->addAction("Show Errors", this, [this](bool b) {
        QDataTableModel::iosfmt f = this->model_->fmt_;
        this->model_->setFormat(f, b);
    });
    errorAct->setCheckable(true);
    errorAct->setChecked(this->model_->withErrors_);
    errorAct->setEnabled(slice_ && !slice_->empty() && slice_->hasErrors());
}

void QTabularDataView::updateOptionsMenu()
{
    formatGroup->actions().at(0)->setChecked(this->model_->fmt_ == std::fixed);
    formatGroup->actions().at(1)->setChecked(this->model_->fmt_ == std::scientific);
    formatGroup->actions().at(2)->setChecked(this->model_->fmt_ == std::defaultfloat);
    errorAct->setChecked(this->model_->withErrors_);
    errorAct->setEnabled(slice_ && !slice_->empty() && slice_->hasErrors());
}

void QTabularDataView::setPrecision(int v)
{
    model_->precision_ = v;
    updateData();
}

/************ QPlotDataView  *****************/

QPlotDataView::QPlotDataView(QWidget *parent)
    : QAbstractDataView(parent)
{
    linePlot = new QMatPlotWidget;
    linePlot->setStyleSheet("background: white");

    legendView_ = new LegendView;
    legendView_->setColorOrder(linePlot->colorOrder());
    legendView_->hide();

    /* create layout: splitter lets the user resize the legend panel */
    splitter_ = new QSplitter(Qt::Horizontal);
    splitter_->setContentsMargins(0, 0, 0, 0);
    splitter_->addWidget(linePlot);
    splitter_->addWidget(legendView_);
    splitter_->setStretchFactor(0, 1);
    splitter_->setStretchFactor(1, 0);
    splitter_->setCollapsible(0, false);
    splitter_->setCollapsible(1, false);
    QSizePolicy plcy = splitter_->sizePolicy();
    plcy.setVerticalPolicy(QSizePolicy::Expanding);
    splitter_->setSizePolicy(plcy);
    handleWidth_ = splitter_->handleWidth();
    //splitter_->setHandleWidth(0);

    auto *hbox = new QHBoxLayout;
    hbox->addWidget(splitter_);
    setLayout(hbox);

    connect(legendView_, &LegendView::selectionChanged,
            this, [this](const QVector<uint> &)
            { renderPlot_(); });

    connect(legendView_, &LegendView::panelToggled, this, [this](bool open)
            {
        const QList<int> sizes = splitter_->sizes();
        const int total = sizes[0] + sizes[1];
        if (open) {
            splitter_->setSizes({total - legendOpenWidth_, legendOpenWidth_});
            //splitter_->setHandleWidth(handleWidth_);
        } else {
            legendOpenWidth_ = sizes[1];
            const int closedW = legendView_->collapsedWidth();
            splitter_->setSizes({total - closedW, closedW});
            //splitter_->setHandleWidth(0);
        } });

    createOptionsMenu();
    connect(optionsMenu_, &QMenu::aboutToShow, this, &QPlotDataView::updateOptionsMenu);
}

QIcon QPlotDataView::icon() const
{
    return QIcon(":/qdatabrowser/icons/lucide/chart-spline.svg");
}

void QPlotDataView::exportImage() const
{
    // Export the plot to 160x120mm page
    linePlot->exportToFile("export.pdf", QSize(160, 120));
}

void QPlotDataView::setPlotType(QDataBrowser::PlotType t)
{
    type_ = t;
    updateView_();
}

void QPlotDataView::updateView()
{
    QAbstractDataView::updateView();
}

void QPlotDataView::updateData()
{
    renderPlot_();
}

void QPlotDataView::updateView_()
{
    updateLegend_();
    renderPlot_();
}

QPlotDataView::State QPlotDataView::state() const
{
    State s;
    s.plotType = type_;
    s.autoScaleX = linePlot->autoScaleX();
    s.autoScaleY = linePlot->autoScaleY();
    s.logScaleX = linePlot->logScaleX();
    s.logScaleY = linePlot->logScaleY();
    s.grid = linePlot->grid();
    s.legendOpenWidth = legendOpenWidth_;
    s.legendCollapsed = legendView_->isCollapsed();
    s.legendCheckedValues = legendView_->checkedValues();
    return s;
}

void QPlotDataView::setState(const State &s)
{
    type_ = s.plotType;
    legendOpenWidth_ = s.legendOpenWidth;
    linePlot->setAutoScaleX(s.autoScaleX);
    linePlot->setAutoScaleY(s.autoScaleY);
    if (s.logScaleX)
        linePlot->setLogScaleX();
    else
        linePlot->setLinearScaleX();
    if (s.logScaleY)
        linePlot->setLogScaleY();
    else
        linePlot->setLinearScaleY();
    linePlot->setGrid(s.grid);
    legendView_->setCollapsed(s.legendCollapsed);
    legendView_->setCheckedValues(s.legendCheckedValues);
}

void QPlotDataView::updateLegend_()
{
    const bool showLegend = slice_ && !slice_->empty() && slice_->ndim() == 2 && slice_->dim()[1] > 1;
    if (!showLegend)
    {
        if (legendView_->isVisible() && !legendView_->isCollapsed())
        {
            const QList<int> sizes = splitter_->sizes();
            legendOpenWidth_ = sizes[1];
        }
        legendView_->hide();
        return;
    }

    QStringList valueLabels;
    sliceSelector_->getAxisValueLabels(slice_->dy(), valueLabels);

    // Preserve existing per-series selection when only values changed (same Y count).
    // Pass an empty checked vector when the Y count changes so all start checked.
    QVector<uint> prevSel;
    //if (legendView_->isVisible() && legendView_->count() == valueLabels.size())
    if (legendView_->count() == valueLabels.size())
        prevSel = legendView_->checkedValues();

    // legendView_->setColorOrder(linePlot->colorOrder());
    legendView_->setValues(valueLabels, prevSel);

    if (!legendView_->isVisible())
    {
        int w;
        if (legendView_->isCollapsed())
            w = legendView_->collapsedWidth();
        else
        {
            // On the first ever show legendOpenWidth_ is 0; read the (halved) sizeHint
            // so the splitter gets a compact initial allocation without any deferred call.
            if (legendOpenWidth_ <= 0)
                legendOpenWidth_ = legendView_->sizeHint().width();
            w = legendOpenWidth_;
        }
        legendView_->show();
        const QList<int> sizes = splitter_->sizes();
        splitter_->setSizes({sizes[0] + sizes[1] - w, w});
    }
}

void QPlotDataView::renderPlot_()
{
    linePlot->clear();
    linePlot->setXlabel("");
    linePlot->setYlabel("");
    linePlot->setTitle("");

    if (!slice_ || slice_->empty() || !slice_->is_numeric())
        return;

    bool haserr = slice_->hasErrors();
    if (!haserr && type_ == QDataBrowser::ErrorBar)
        type_ = QDataBrowser::Line;

    if (slice_->ndim() == 2)
    {
        const double *p = slice_->data().data();
        const double *dp = p;
        if (haserr)
            dp = slice_->errors().data();

        int nx = slice_->dim()[0];
        auto clr = linePlot->colorOrder();
        int nc = clr.size();

        // Legend drives the per-series selection; empty = nothing shown
        const QVector<uint> ySel = legendView_->checkedValues();
        const QSet<uint> checkedSet(ySel.begin(), ySel.end());
        for (uint j = 0; j < slice_->dim()[1]; j++, p += nx, dp += nx)
        {
            if (!checkedSet.contains(j))
                continue;
            AbstractDataSet::vec_t v(p, p + nx);
            switch (type_)
            {
            case QDataBrowser::Line:
                linePlot->plot(slice_->x(), v, QString(), clr[j % nc]);
                break;
            case QDataBrowser::Points:
                linePlot->plot(slice_->x(), v, "o", clr[j % nc]);
                break;
            case QDataBrowser::LineAndPoints:
                linePlot->plot(slice_->x(), v, "o-", clr[j % nc]);
                break;
            case QDataBrowser::Stairs:
                linePlot->stairs(slice_->x(), v, QString(), clr[j % nc]);
                break;
            case QDataBrowser::ErrorBar:
            {
                AbstractDataSet::vec_t dv(dp, dp + nx);
                linePlot->errorbar(slice_->x(), v, dv, "o-", clr[j % nc]);
            }
            break;
            }
        }
    }
    else
    {
        switch (type_)
        {
        case QDataBrowser::Line:
            linePlot->plot(slice_->x(), slice_->data());
            break;
        case QDataBrowser::Points:
            linePlot->plot(slice_->x(), slice_->data(), "o");
            break;
        case QDataBrowser::LineAndPoints:
            linePlot->plot(slice_->x(), slice_->data(), "o-");
            break;
        case QDataBrowser::Stairs:
            linePlot->stairs(slice_->x(), slice_->data());
            break;
        case QDataBrowser::ErrorBar:
            linePlot->errorbar(slice_->x(), slice_->data(), slice_->errors(), "o-");
            break;
        }
    }
    linePlot->setXlabel(slice_->dim_desc(0).c_str());
    linePlot->setTitle(slice_->description().c_str());
}

void QPlotDataView::createOptionsMenu()
{
    optionsMenu_ = new QMenu((QWidget *)this);

    QMenu *m;
    QAction *a;

    // X Axis submenu
    int iax = 0;
    m = optionsMenu_->addMenu("X Axis");
    autoScaleAct[iax] = m->addAction("Auto Scale", linePlot, SLOT(setAutoScaleX(bool)));
    autoScaleAct[iax]->setCheckable(true);
    autoScaleAct[iax]->setChecked(linePlot->autoScaleX());
    m->addSeparator();
    linLogGroup[iax] = new QActionGroup(this);
    a = m->addAction(QString("Linear Scale"), linePlot, SLOT(setLinearScaleX()));
    a->setCheckable(true);
    a->setChecked(linePlot->linearScaleX());
    linLogGroup[iax]->addAction(a);
    a = m->addAction(QString("Log Scale"), linePlot, SLOT(setLogScaleX()));
    a->setCheckable(true);
    a->setChecked(linePlot->logScaleX());
    linLogGroup[iax]->addAction(a);

    // Y Axis submenu
    iax = 1;
    m = optionsMenu_->addMenu("Y Axis");
    autoScaleAct[iax] = m->addAction("Auto Scale", linePlot, SLOT(setAutoScaleY(bool)));
    autoScaleAct[iax]->setCheckable(true);
    autoScaleAct[iax]->setChecked(linePlot->autoScaleY());
    m->addSeparator();
    linLogGroup[iax] = new QActionGroup(this);
    a = m->addAction(QString("Linear Scale"), linePlot, SLOT(setLinearScaleY()));
    a->setCheckable(true);
    a->setChecked(linePlot->linearScaleY());
    linLogGroup[iax]->addAction(a);
    a = m->addAction(QString("Log Scale"), linePlot, SLOT(setLogScaleY()));
    a->setCheckable(true);
    a->setChecked(linePlot->logScaleY());
    linLogGroup[iax]->addAction(a);

    optionsMenu_->addSeparator();

    m = optionsMenu_->addMenu("Plot type");
    plotTypeGroup = new QActionGroup(this);
    a = m->addAction("Line", this, [this]()
                     { this->setPlotType(QDataBrowser::Line); });
    a->setCheckable(true);
    a->setChecked(type_ == QDataBrowser::Line);
    plotTypeGroup->addAction(a);
    a = m->addAction("Points", this, [this]()
                     { this->setPlotType(QDataBrowser::Points); });
    a->setCheckable(true);
    a->setChecked(type_ == QDataBrowser::Points);
    plotTypeGroup->addAction(a);
    a = m->addAction("Line+Points", this, [this]()
                     { this->setPlotType(QDataBrowser::LineAndPoints); });
    a->setCheckable(true);
    a->setChecked(type_ == QDataBrowser::LineAndPoints);
    plotTypeGroup->addAction(a);
    a = m->addAction("Stairs", this, [this]()
                     { this->setPlotType(QDataBrowser::Stairs); });
    a->setCheckable(true);
    a->setChecked(type_ == QDataBrowser::Stairs);
    plotTypeGroup->addAction(a);
    a = m->addAction("Error Bars", this, [this]()
                     { this->setPlotType(QDataBrowser::ErrorBar); });
    a->setCheckable(true);
    a->setChecked(type_ == QDataBrowser::ErrorBar);
    plotTypeGroup->addAction(a);
    a->setEnabled(slice_ && !slice_->empty() && slice_->hasErrors());

    optionsMenu_->addSeparator();

    gridAct = optionsMenu_->addAction("Grid", linePlot, SLOT(setGrid(bool)));
    gridAct->setCheckable(true);
    gridAct->setChecked(linePlot->grid());
}

void QPlotDataView::updateOptionsMenu()
{
    autoScaleAct[0]->setChecked(linePlot->autoScaleX());
    autoScaleAct[1]->setChecked(linePlot->autoScaleY());

    linLogGroup[0]->actions().at(0)->setChecked(linePlot->linearScaleX());
    linLogGroup[0]->actions().at(1)->setChecked(linePlot->logScaleX());
    linLogGroup[1]->actions().at(0)->setChecked(linePlot->linearScaleY());
    linLogGroup[1]->actions().at(1)->setChecked(linePlot->logScaleY());

    int k = 0;
    for (QAction *a : plotTypeGroup->actions())
        a->setChecked(type_ == k++);

    bool haserr = slice_ && !slice_->empty() && slice_->hasErrors();
    plotTypeGroup->actions().last()->setEnabled(haserr);
}

/************ QHeatMapDataView  *****************/

QHeatMapDataView::QHeatMapDataView(QWidget *parent)
    : QAbstractDataView(parent)
{
    heatMap = new QMatPlotWidget;
    heatMap->setStyleSheet("background: white");

    /* create layout */
    QVBoxLayout *vbox = new QVBoxLayout;
    setLayout(vbox);
    vbox->addWidget(heatMap);

    cmap_ = QMatPlotWidget::Viridis;
    createOptionsMenu();
    connect(optionsMenu_, &QMenu::aboutToShow, this, &QHeatMapDataView::updateOptionsMenu);
}

QIcon QHeatMapDataView::icon() const
{
    return QIcon(":/qdatabrowser/icons/lucide/map.svg");
}

void QHeatMapDataView::exportImage() const
{
    // Export the plot to 160x120mm page
    heatMap->exportToFile("export.pdf", QSize(160, 120));
}

void QHeatMapDataView::updateData()
{
    updateView_();
}

void QHeatMapDataView::updateView_()
{
    heatMap->clear();
    heatMap->setXlabel("");
    heatMap->setYlabel("");
    heatMap->setTitle("");

    if (!slice_ || slice_->empty() || !slice_->is_numeric()) {
        return;
    }

    int ndim = slice_->ndim();
    auto dim = slice_->dim();

    heatMap->imagesc(slice_->data(), dim[0]);
    heatMap->setXlabel(slice_->dim_name(0).c_str());
    if (ndim > 1)
        heatMap->setYlabel(slice_->dim_name(1).c_str());
    heatMap->setTitle(slice_->description().c_str());
}

QHeatMapDataView::State QHeatMapDataView::state() const
{
    State s;
    s.colormap = cmap_;
    s.grid = heatMap->grid();
    return s;
}

void QHeatMapDataView::setState(const State &s)
{
    cmap_ = s.colormap;
    heatMap->setColorMap(static_cast<QMatPlotWidget::ColorMapType>(cmap_));
    heatMap->setGrid(s.grid);
}

void QHeatMapDataView::createOptionsMenu()
{
    optionsMenu_ = new QMenu((QWidget *)this);

    QMenu *m;
    QAction *a;

    m = optionsMenu_->addMenu("Colormap");
    colormapGroup = new QActionGroup(this);
    a = m->addAction("Viridis", heatMap, [this]()
                     {
        this->heatMap->setColorMap(QMatPlotWidget::Viridis);
        cmap_ = QMatPlotWidget::Viridis;
        updateView_(); });
    a->setCheckable(true);
    a->setChecked(cmap_ == QMatPlotWidget::Viridis);
    colormapGroup->addAction(a);
    a = m->addAction("Turbo", heatMap, [this]()
                     {
        this->heatMap->setColorMap(QMatPlotWidget::Turbo);
        cmap_ = QMatPlotWidget::Turbo;
        updateView_(); });
    a->setCheckable(true);
    a->setChecked(cmap_ == QMatPlotWidget::Turbo);
    colormapGroup->addAction(a);
    a = m->addAction("Jet", heatMap, [this]()
                     {
        this->heatMap->setColorMap(QMatPlotWidget::Jet);
        cmap_ = QMatPlotWidget::Jet;
        updateView_(); });
    a->setCheckable(true);
    a->setChecked(cmap_ == QMatPlotWidget::Jet);
    colormapGroup->addAction(a);
    a = m->addAction("Gray", heatMap, [this]()
                     {
        this->heatMap->setColorMap(QMatPlotWidget::Gray);
        cmap_ = QMatPlotWidget::Gray;
        updateView_(); });
    a->setCheckable(true);
    a->setChecked(cmap_ == QMatPlotWidget::Gray);
    colormapGroup->addAction(a);

    // optionsMenu_->addSeparator();

    m = optionsMenu_->addMenu("Color scale");
    linLogGroup = new QActionGroup(this);
    a = m->addAction(QString("Linear Scale"));
    a->setCheckable(true);
    a->setChecked(true);
    linLogGroup->addAction(a);
    a = m->addAction(QString("Log Scale"));
    a->setCheckable(true);
    a->setChecked(false);
    linLogGroup->addAction(a);

    // optionsMenu_->addSeparator();

    gridAct = optionsMenu_->addAction("Grid", heatMap, SLOT(setGrid(bool)));
    gridAct->setCheckable(true);
    gridAct->setChecked(heatMap->grid());
}

void QHeatMapDataView::updateOptionsMenu()
{
    int k = 0;
    for (QAction *a : colormapGroup->actions())
        a->setChecked(cmap_ == k++);
    gridAct->setChecked(heatMap->grid());
}
