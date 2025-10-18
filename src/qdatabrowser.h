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

class QAbstractDataView;
class QDataSliceSelector;

class QDataBrowser : public QSplitter
{
    Q_OBJECT
public:
    explicit QDataBrowser(QWidget *parent = nullptr);

    static void initResources();

    bool addGroup(const QString &name,
                  const QString &location = "/",
                  const QString &desc = QString());
    bool addData(AbstractDataStore *data, const QString &location = "/");
    bool selectItem(const QString &path);

signals:

private:
    static const int nViews = 2;
    QDataSliceSelector *sliceSelector[nViews];
    QAbstractDataView *dataView[nViews];
    QStandardItemModel *dataModel;

    // view widgets
    QTreeView *dataTree;
    QTabWidget *viewTab;
    QSplitter *bottomSplitter;
    QLabel *dataName;
    QStackedWidget *bottomPanel;

    // controls
    QToolButton *copyPathBt;
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

Q_DECLARE_METATYPE(AbstractDataStore *)

#endif // QDATABROWSER_H
