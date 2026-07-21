#ifndef QDATABROWSERPAGE_H
#define QDATABROWSERPAGE_H

#include <QWidget>

#include "qdatasliceselector.h"
#include "qdataview.h"

class QLineEdit;
class QTabWidget;

class QDataBrowserPage : public QWidget
{
    Q_OBJECT

public:
    explicit QDataBrowserPage(QWidget *parent = nullptr);

    QDataBrowser::PlotType plotType() const;
    QDataBrowser::ViewType activeView() const;
    const QString &dataPath() const { return dataPath_; }
    const QStandardItem *dataItem() const { return item_; }

    void clear();
    void updateData();
    void setData(const QString &path, QStandardItem *i);

    QDataBrowserPage *clone(QWidget *parent = nullptr) const;

    struct State
    {
        QString path;
        QDataSliceSelector::State sliceState;
        QDataBrowser::ViewType activeView{QDataBrowser::Table};
        QTabularDataView::State tableState;
        QPlotDataView::State plotState;
        QHeatMapDataView::State heatMapState;
    };
    State state() const;
    const State &savedState() const { return savedState_; }

public slots:
    void setPlotType(QDataBrowser::PlotType t);
    void setActiveView(QDataBrowser::ViewType t);
    void onItemChanged(QStandardItem *i);

signals:

protected:
    // path to the data
    QString dataPath_;

    // data model item
    QStandardItem *item_{nullptr};

    // state captured at the last setData() call
    State savedState_;

    // view widgets
    static const int nViews = 3;
    QAbstractDataView *dataView[nViews];
    QTabWidget *viewTab;

    // web browser like path ctrl
    // QToolButton *btBack;
    // QToolButton *btForward;
    // QLineEdit *edtPath;

    // controls
    QToolButton *optionsBt;
    QToolButton *btExport;
    QDataSliceSelector *sliceSelector;

    // actions
    QAction *actExportCSV;
    QAction *actExportImg;

private slots:
    void onSliceReset();
    void onSliceChanged();
    void onExportCSV();
    void onExportPlot();
    void onCurrentViewChanged(int i);
    void onViewUpdated();
};

#endif // QDATABROWSERPAGE_H
