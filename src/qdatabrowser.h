#ifndef QDATABROWSER_H
#define QDATABROWSER_H

#include "abstractdatastore.h"

#include <QModelIndex>
#include <QSplitter>

class QStandardItemModel;
class QStandardItem;
class QTreeView;
class QToolButton;
class QLabel;
class QStackedWidget;
class QTabWidget;
class QTableWidget;

class QAbstractDataView;
class QDataSliceSelector;

class QDataBrowser : public QSplitter
{
    Q_OBJECT

    Q_PROPERTY(QString treeTitle READ treeTitle WRITE setTreeTitle)
    Q_PROPERTY(bool ignoreSingletonDims READ ignoreSingletonDims CONSTANT)

public:
    enum PlotType { Line, Points, LineAndPoints, Stairs, ErrorBar };
    enum ViewType { Table, Plot, HeatMap };

    Q_ENUM(PlotType)
    Q_ENUM(ViewType)

public:
    explicit QDataBrowser(QWidget *parent = nullptr, bool ignoreSingletonDims = true);

    // init static lib resources (icons, etc.)
    static void initResources();

    // properties
    const QString &treeTitle() const { return treeTitle_; }
    void setTreeTitle(const QString &t);
    bool ignoreSingletonDims() const { return ignoreSingletonDims_; }
    void setIgnoreSingletonDims(bool on = true) { ignoreSingletonDims_ = on; }

    // Insert a group node in the data model
    // below the specified location
    bool addGroup(const QString &name,
                  const QString &location = "/",
                  const QString &desc = QString());

    // Insert a data node in the data model at location
    // The name is given in the data
    // QDataBrowser takes ownership of the pointer
    // It is stored as a shared pointer
    // will be deleted when not needed
    bool addData(AbstractDataStore *data, const QString &location = "/");

    // select an item in the data tree
    bool selectItem(const QString &path);

    // call to signify that data at & below the give path have changed
    // views are updated
    void dataUpdated(const QString &path = "/");

    // remove the data node at path and its child nodes
    void clear(const QString &path = "/");

    QDataBrowser::PlotType plotType() const;
    QDataBrowser::ViewType activeView() const;

public slots:
    void setPlotType(QDataBrowser::PlotType t);
    void setActiveView(QDataBrowser::ViewType t);

signals:

private:
    // singleton dims (with size=1) will not
    // be presented in the slicer selection
    bool ignoreSingletonDims_{true};
    QVariant dataProxy;

    // data tree model
    QStandardItemModel *dataModel;
    QString treeTitle_;

    // view widgets
    static const int nViews = 3;
    QDataSliceSelector *sliceSelector[nViews];
    QAbstractDataView *dataView[nViews];
    QTreeView *dataTree;
    QTableWidget *infoTable;

    // layout widgets
    QTabWidget *viewTab;
    QSplitter *bottomSplitter;
    QStackedWidget *bottomPanel;

    // controls
    QLabel *dataName;
    QToolButton *copyPathBt;
    QToolButton *optionsBt;
    QToolButton *btExport;
    QToolButton *leftPanelBt;
    int lastLeftPanelPos;
    QToolButton *bottomPanelBt;
    int lastBottomPanelPos;

    QAction *actExportCSV;
    QAction *actExportImg;

    QStandardItem *fromPath(const QString &path) const;
    QStandardItem *findChild(const QString &name, QStandardItem *parent) const;
    bool isGroup(QStandardItem *i);
    QString itemPath(QStandardItem *i);
    bool dataUpdated(QStandardItem *i);
    bool isBelow(const QModelIndex &i, const QModelIndex &g);
    void updateInfoTable(QStandardItem *i);

private slots:
    void onLeftSplitterMoved(int pos, int index);
    void onBottomSplitterMoved(int pos, int index);
    void onLeftPanelBtClicked(bool c);
    void onBottomPanelBtClicked(bool c);
    void onDataItemSelect(const QModelIndex &selected, const QModelIndex &deselected);
    void onCopyPath();
    void onSliceReset();
    void onSliceChanged();
    void onExportCSV();
    void onExportPlot();
    void onCurrentViewChanged(int i);
    void onViewUpdated();
};

#endif // QDATABROWSER_H
