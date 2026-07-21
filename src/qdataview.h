#ifndef QABSTRACTDATAVIEW_H
#define QABSTRACTDATAVIEW_H

//#include <QWidget>

#include "qdatabrowser.h"

#include <QVector>

#include <ios>

class QTableView;
class QLabel;
class QMenu;
class QActionGroup;
class QStackedWidget;
class QPlainTextEdit;

class DataSlice;
class QDataTableModel;
class QMatPlotWidget;
class QDataSliceSelector;
class FilterView;
class LegendView;
class QSplitter;

class QAbstractDataView : public QWidget
{
    Q_OBJECT
public:
    explicit QAbstractDataView(QWidget *parent = nullptr);

    const DataSlice *slice() const { return slice_; }
    QDataSliceSelector *sliceSelector() { return sliceSelector_; }
    void setSliceSelector(QDataSliceSelector *s);
    virtual QWidget *view() = 0;
    virtual QIcon icon() const = 0;
    virtual bool canExportImage() const { return false; }
    virtual void exportImage() const {}
    virtual QMenu *optionsMenu() { return nullptr; }

signals:
    void viewUpdated();

public slots:
    virtual void updateView();
    virtual void updateData();

protected:
    // data slice
    DataSlice *slice_{nullptr};
    QDataSliceSelector *sliceSelector_{nullptr};

    virtual void updateView_() = 0;
};

class QTabularDataView : public QAbstractDataView
{
    Q_OBJECT
public:
    explicit QTabularDataView(QWidget *parent = nullptr);

    QWidget *view() override { return (QWidget *)view_; }
    QMenu *optionsMenu() override { return optionsMenu_; }
    QIcon icon() const override;

    struct State
    {
        using NumberFormat = std::ios_base &(*)(std::ios_base &);
        NumberFormat format = std::defaultfloat;
        bool withErrors = false;
        int precision = 6;
    };
    State state() const;
    void setState(const State &s);

public slots:
    void updateData() override;

protected:
    QDataTableModel *model_;

    // view widgets
    QTableView *view_;
    QLabel *title_;
    QStackedWidget *stack_;
    QPlainTextEdit *scalarView_;

    // Options menu & actions
    QMenu *optionsMenu_;
    QAction *errorAct;
    QActionGroup *formatGroup;

    virtual void updateView_() override;
    void createOptionsMenu();

protected slots:
    void updateOptionsMenu();
    void setPrecision(int v);
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

    struct State
    {
        QDataBrowser::PlotType plotType = QDataBrowser::Line;
        bool autoScaleX = true;
        bool autoScaleY = true;
        bool logScaleX = false;
        bool logScaleY = false;
        bool grid = false;
        int legendOpenWidth = 0;
        bool legendCollapsed = false;
        QVector<uint> legendCheckedValues;
    };
    State state() const;
    void setState(const State &s);

public slots:
    void setPlotType(QDataBrowser::PlotType t);
    void updateView() override;
    void updateData() override;

protected:
    // view widgets
    QMatPlotWidget *linePlot;
    LegendView     *legendView_;
    QSplitter      *splitter_;
    int             legendOpenWidth_{0};
    int handleWidth_{0};
    QDataBrowser::PlotType type_{QDataBrowser::Line};

    // Options menu & actions
    QMenu *optionsMenu_;
    QAction *autoScaleAct[2];
    QAction *gridAct;
    QActionGroup *linLogGroup[2];
    QActionGroup *plotTypeGroup;

    virtual void updateView_() override;
    void createOptionsMenu();

private:
    void updateLegend_();
    void renderPlot_();

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

    struct State
    {
        int colormap = 0;
        bool grid = false;
    };
    State state() const;
    void setState(const State &s);

public slots:
    void updateData() override;

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
