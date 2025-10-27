#ifndef QABSTRACTDATAVIEW_H
#define QABSTRACTDATAVIEW_H

//#include <QWidget>

#include "qdatabrowser.h"

class QTableView;
class QLabel;
class QMenu;
class QActionGroup;
class QStackedWidget;
class QPlainTextEdit;

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
    virtual QMenu *optionsMenu() { return nullptr; }

signals:
    void viewUpdated();

public slots:
    virtual void setData(DataSlice *s);
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

    QWidget *view() override { return (QWidget *)view_; }
    QIcon icon() const override;

protected:
    QDataTableModel *model_;

    // view widgets
    QTableView *view_;
    QLabel *title_;
    QStackedWidget *stack_;
    QPlainTextEdit *scalarView_;

    virtual void updateView_() override;
};

class QPlotDataView : public QAbstractDataView
{
    Q_OBJECT
public:
    explicit QPlotDataView(QWidget *parent = nullptr);

    QWidget *view() override { return (QWidget *)linePlot; }
    QIcon icon() const override;

    bool canExportImage() const override { return true; }
    void exportImage() const override;
    QMenu *optionsMenu() override { return optionsMenu_; }
    QDataBrowser::PlotType plotType() const { return type_; }

public slots:
    void setPlotType(QDataBrowser::PlotType t);
    void setData(DataSlice *s) override;

protected:
    // view widgets
    QMatPlotWidget *linePlot;
    QDataBrowser::PlotType type_{QDataBrowser::Line};

    // Options menu & actions
    QMenu *optionsMenu_;
    QAction *autoScaleAct[2];
    QAction *gridAct;
    QActionGroup *linLogGroup[2];
    QActionGroup *plotTypeGroup;

    virtual void updateView_() override;
    void createOptionsMenu();

protected slots:
    void updateOptionsMenu();
};

class QHeatMapDataView : public QAbstractDataView
{
    Q_OBJECT
public:
    explicit QHeatMapDataView(QWidget *parent = nullptr);

    QWidget *view() override { return (QWidget *)heatMap; }
    QIcon icon() const override;

    QMenu *optionsMenu() override { return optionsMenu_; }

    bool canExportImage() const override { return true; }
    void exportImage() const override;

protected:
    // view widgets
    QMatPlotWidget *heatMap;
    int cmap_;

    // Options menu & actions
    QMenu *optionsMenu_;
    QAction *gridAct;
    QActionGroup *linLogGroup;
    QActionGroup *colormapGroup;

    virtual void updateView_() override;
    void createOptionsMenu();

protected slots:
    void updateOptionsMenu();
};

#endif // QABSTRACTDATAVIEW_H
