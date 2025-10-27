#include "qdataview.h"

#include <QLabel>
#include <QMenu>
#include <QTableView>
#include <QVBoxLayout>

#include "qdatasliceselector.h"

#include "QMatPlotWidget.h"

QAbstractDataView::QAbstractDataView(QWidget *parent)
    : QWidget{parent}
{
}

void QAbstractDataView::setData(DataSlice *s)
{
    slice_ = s;
    updateView_();
}

void QAbstractDataView::updateView()
{
    updateView_();
    emit viewUpdated();
}

/************* QTabularDataView *******************/

class QDataTableModel : public QAbstractTableModel
{
public:
    QDataTableModel(QObject *parent = 0)
        : QAbstractTableModel(parent), slice_(nullptr)
    {
    }

    void setData(DataSlice *s)
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
        return (*slice_)(index.row(), index.column());
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

            return (slice_->is_x_categorical(0)) ? QVariant(slice_->x_category()[i].c_str())
                                                 : QVariant(slice_->x(i));
        }
        else if (orientation == Qt::Horizontal)
        {
            if (i < 0 || i >= columnCount())
                return QVariant();

            return (slice_->is_x_categorical(1)) ? QVariant(slice_->y_category()[i].c_str())
                                                 : QVariant(slice_->x(i));
        }
        return QVariant();
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

    /* create layout */
    QVBoxLayout *vbox = new QVBoxLayout;
    setLayout(vbox);
    vbox->addWidget(title_);
    vbox->addWidget(view_);
}

QIcon QTabularDataView::icon() const
{
    return QIcon(":/qdatabrowser/lucide/sheet.svg");
}

void QTabularDataView::updateView_()
{
    model_->setData(slice_);
    if (!slice_->description().empty())
        title_->setText(slice_->description().c_str());
    else
        title_->setText(slice_->name().c_str());
}

/************ QPlotDataView  *****************/

QPlotDataView::QPlotDataView(QWidget *parent)
    : QAbstractDataView(parent)
{
    linePlot = new QMatPlotWidget;
    linePlot->setStyleSheet("background: white");

    /* create layout */
    QVBoxLayout *vbox = new QVBoxLayout;
    setLayout(vbox);
    vbox->addWidget(linePlot);

    createOptionsMenu();
    connect(optionsMenu_, &QMenu::aboutToShow, this, &QPlotDataView::updateOptionsMenu);
}

QIcon QPlotDataView::icon() const
{
    return QIcon(":/qdatabrowser/lucide/chart-spline.svg");
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

void QPlotDataView::setData(DataSlice *s)
{
    QAbstractDataView::setData(s);
}

void QPlotDataView::updateView_()
{
    linePlot->clear();
    linePlot->setXlabel("");
    linePlot->setYlabel("");
    linePlot->setTitle("");

    bool haserr = slice_ && !slice_->empty() && slice_->hasErrors();
    if (!haserr && type_ == QDataBrowser::ErrorBar)
        type_ = QDataBrowser::Line;

    if (!slice_ || slice_->empty()) {
        return;
    }

    switch (type_) {
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
    linePlot->setXlabel(slice_->dim_desc(0).c_str());
    //if (slice_->ndim() > 1)
    //    linePlot->setYlabel(slice_->dim_desc(1).c_str());
    linePlot->setTitle(slice_->description().c_str());
}

void QPlotDataView::createOptionsMenu()
{
    optionsMenu_ = new QMenu((QWidget *) this);

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
    a = m->addAction("Line", this, [this]() { this->setPlotType(QDataBrowser::Line); });
    a->setCheckable(true);
    a->setChecked(type_ == QDataBrowser::Line);
    plotTypeGroup->addAction(a);
    a = m->addAction("Points", this, [this]() { this->setPlotType(QDataBrowser::Points); });
    a->setCheckable(true);
    a->setChecked(type_ == QDataBrowser::Points);
    plotTypeGroup->addAction(a);
    a = m->addAction("Line+Points", this, [this]() {
        this->setPlotType(QDataBrowser::LineAndPoints);
    });
    a->setCheckable(true);
    a->setChecked(type_ == QDataBrowser::LineAndPoints);
    plotTypeGroup->addAction(a);
    a = m->addAction("Stairs", this, [this]() { this->setPlotType(QDataBrowser::Stairs); });
    a->setCheckable(true);
    a->setChecked(type_ == QDataBrowser::Stairs);
    plotTypeGroup->addAction(a);
    a = m->addAction("Error Bars", this, [this]() { this->setPlotType(QDataBrowser::ErrorBar); });
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
    return QIcon(":/qdatabrowser/lucide/map.svg");
}

void QHeatMapDataView::exportImage() const
{
    // Export the plot to 160x120mm page
    heatMap->exportToFile("export.pdf", QSize(160, 120));
}

void QHeatMapDataView::updateView_()
{
    heatMap->clear();
    heatMap->setXlabel("");
    heatMap->setYlabel("");
    heatMap->setTitle("");

    if (!slice_ || slice_->empty()) {
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

void QHeatMapDataView::createOptionsMenu()
{
    optionsMenu_ = new QMenu((QWidget *) this);

    QMenu *m;
    QAction *a;

    m = optionsMenu_->addMenu("Colormap");
    colormapGroup = new QActionGroup(this);
    a = m->addAction("Viridis", heatMap, [this]() {
        this->heatMap->setColorMap(QMatPlotWidget::Viridis);
        cmap_ = QMatPlotWidget::Viridis;
        updateView_();
    });
    a->setCheckable(true);
    a->setChecked(cmap_ == QMatPlotWidget::Viridis);
    colormapGroup->addAction(a);
    a = m->addAction("Turbo", heatMap, [this]() {
        this->heatMap->setColorMap(QMatPlotWidget::Turbo);
        cmap_ = QMatPlotWidget::Turbo;
        updateView_();
    });
    a->setCheckable(true);
    a->setChecked(cmap_ == QMatPlotWidget::Turbo);
    colormapGroup->addAction(a);
    a = m->addAction("Jet", heatMap, [this]() {
        this->heatMap->setColorMap(QMatPlotWidget::Jet);
        cmap_ = QMatPlotWidget::Jet;
        updateView_();
    });
    a->setCheckable(true);
    a->setChecked(cmap_ == QMatPlotWidget::Jet);
    colormapGroup->addAction(a);
    a = m->addAction("Gray", heatMap, [this]() {
        this->heatMap->setColorMap(QMatPlotWidget::Gray);
        cmap_ = QMatPlotWidget::Gray;
        updateView_();
    });
    a->setCheckable(true);
    a->setChecked(cmap_ == QMatPlotWidget::Gray);
    colormapGroup->addAction(a);

    //optionsMenu_->addSeparator();

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

    //optionsMenu_->addSeparator();

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
