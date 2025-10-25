#include "qdataview.h"

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

    /* create layout */
    QVBoxLayout *vbox = new QVBoxLayout;
    setLayout(vbox);
    vbox->addWidget(view_);
}

QIcon QTabularDataView::icon() const
{
    return QIcon(":/qdatabrowser/lucide/sheet.svg");
}

void QTabularDataView::updateView_()
{
    model_->setData(slice_);
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

void QPlotDataView::updateView_()
{
    linePlot->clear();
    linePlot->setXlabel("");
    linePlot->setYlabel("");
    linePlot->setTitle("");

    if (!slice_ || slice_->empty())
    {
        return;
    }

    linePlot->plot(slice_->x(), slice_->data());
    linePlot->setXlabel(slice_->dim_name(0).c_str());
    if (slice_->ndim() > 1)
        linePlot->setYlabel(slice_->dim_name(1).c_str());
    linePlot->setTitle(slice_->description().c_str());
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
