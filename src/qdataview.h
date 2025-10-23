#ifndef QABSTRACTDATAVIEW_H
#define QABSTRACTDATAVIEW_H

#include <QWidget>

class QTableView;

class DataSlice;
class QDataTableModel;
class QMatPlotWidget;

class QAbstractDataView : public QWidget
{
    Q_OBJECT
public:
    explicit QAbstractDataView(QWidget *parent = nullptr);

    const DataSlice *slice() const { return slice_; }
    virtual QWidget *view() = 0;
    virtual QIcon icon() const = 0;
    virtual bool canExportImage() const { return false; }
    virtual void exportImage() const {}

signals:
    void viewUpdated();

public slots:
    void setData(DataSlice *s);
    void updateView();

protected:
    // data slice
    DataSlice *slice_{nullptr};

    virtual void updateView_() = 0;
};

class QTabularDataView : public QAbstractDataView
{
public:
    explicit QTabularDataView(QWidget *parent = nullptr);

    QWidget *view() override { return (QWidget *) view_; }
    QIcon icon() const override;

protected:
    QDataTableModel *model_;

    // view widgets
    QTableView *view_;

    virtual void updateView_() override;
};

class QPlotDataView : public QAbstractDataView
{
public:
    explicit QPlotDataView(QWidget *parent = nullptr);

    QWidget *view() override { return (QWidget *) linePlot; }
    QIcon icon() const override;

    bool canExportImage() const override { return true; }
    void exportImage() const override;

protected:
    // view widgets
    QMatPlotWidget *linePlot;

    virtual void updateView_() override;
};

class QHeatMapDataView : public QAbstractDataView
{
public:
    explicit QHeatMapDataView(QWidget *parent = nullptr);

    QWidget *view() override { return (QWidget *) heatMap; }
    QIcon icon() const override;

    bool canExportImage() const override { return true; }
    void exportImage() const override;

protected:
    // view widgets
    QMatPlotWidget *heatMap;

    virtual void updateView_() override;
};

#endif // QABSTRACTDATAVIEW_H
