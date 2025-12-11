#include "qdatabrowser.h"

#include "dataslice.h"
#include "qdatasliceselector.h"
#include "qdataview.h"

#include <QClipboard>
#include <QComboBox>
#include <QFileDialog>
#include <QFrame>
#include <QGuiApplication>
#include <QItemSelectionModel>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>

#include <fstream>

bool hasSingletonDim(const DataStorePtr d)
{
    if (d->empty())
        return false;
    for (size_t d : d->dim()) {
        if (d == 1)
            return true;
    }
    return false;
}

class SqueezedDataStore : public AbstractDataStore
{
public:
    SqueezedDataStore(const DataStorePtr d)
        : D_(d)
    {
        name_ = d->name();
        desc_ = d->description();
        if (!d->empty()) {
            if (d->size() == 1) { // scalar
                dim_ = {1};
                dim_name_ = {d->dim_name(0)};
                dim_desc_ = {d->dim_desc(0)};
                dim_idx_ = {0};
            } else {
                for (int i = 0; i < d->dim().size(); ++i) {
                    size_t n = d->dim()[i];
                    if (n > 1) {
                        dim_.push_back(n);
                        dim_idx_.push_back(i);
                        dim_name_.push_back(d->dim_name(i));
                        dim_desc_.push_back(d->dim_desc(i));
                    }
                }
            }
        }
    }
    virtual ~SqueezedDataStore() {}

    bool is_numeric() const override { return D_.isNull() ? true : D_.lock()->is_numeric(); }
    bool hasErrors() const override { return D_.isNull() ? false : D_.lock()->hasErrors(); }
    bool is_x_categorical(size_t d) const override
    {
        return D_.isNull() ? false : D_.lock()->is_x_categorical(dim_idx_[d]);
    }
    size_t get_y_text(size_t d, const dim_t &i0, strvec_t &y) const override
    {
        return D_.isNull() ? 0 : D_.lock()->get_y_text(dim_idx_[d], i1(i0), y);
    }
    size_t get_x_categorical(size_t d, strvec_t &x) const override
    {
        return D_.isNull() ? 0 : D_.lock()->get_x_categorical(dim_idx_[d], x);
    }

protected:
    QWeakPointer<AbstractDataStore> D_;
    dim_t dim_idx_;

    // expand a pointer to squeezed data (i0) to a pointer to original data (i1)
    dim_t i1(const dim_t &i0) const
    {
        dim_t i1_(D_.lock()->ndim(), 0);
        for (int i = 0; i < ndim(); ++i)
            i1_[dim_idx_[i]] = i0[i];
        return i1_;
    }

    size_t get_y(size_t d, const dim_t &i0, size_t n, double *v) const override
    {
        if (D_.isNull())
            return 0;
        DataStorePtr p = D_.lock();
        std::vector<double> buff(n);
        int m = p->get_y(dim_idx_[d], i1(i0), buff);
        std::copy(buff.begin(), buff.begin() + m, v);
        return m;
    }
    size_t get_dy(size_t d, const dim_t &i0, size_t n, double *v) const override
    {
        if (D_.isNull())
            return 0;
        DataStorePtr p = D_.lock();
        std::vector<double> buff(n);
        int m = p->get_dy(dim_idx_[d], i1(i0), buff);
        std::copy(buff.begin(), buff.begin() + m, v);
        return m;
    }
    size_t get_x(size_t d, size_t n, double *v) const override
    {
        if (D_.isNull())
            return 0;
        DataStorePtr p = D_.lock();
        std::vector<double> buff(n);
        int m = p->get_x(dim_idx_[d], buff);
        std::copy(buff.begin(), buff.begin() + m, v);
        return m;
    }

private:
    SqueezedDataStore();
};

// this must be outside any namespace
inline void __initResource__()
{
    Q_INIT_RESOURCE(qtdatabrowser_resources);
}

void QDataBrowser::initResources()
{
    __initResource__();
}

QDataBrowser::QDataBrowser(QWidget *parent, bool ignoreSingletonDims)
    : QSplitter{parent}
    , ignoreSingletonDims_(ignoreSingletonDims)
    , lastLeftPanelPos(100)
{
    /* create data model */
    dataModel = new QStandardItemModel(0, 1, this);
    setTreeTitle("Data Tables");

    /* create left-side tree widget */
    dataTree = new QTreeView;
    dataTree->setModel(dataModel);
    dataTree->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(dataTree->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            &QDataBrowser::onDataItemSelect);
    // dataTree->setStyleSheet("background: white"); // ivory, honeydew

    /* create 2nd splitter */
    bottomSplitter = new QSplitter(Qt::Vertical);

    /* create right hand widget */
    QWidget *rhw = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout;
    rhw->setLayout(vbox);
    vbox->setContentsMargins(0, 0, 0, 0);
    // vbox->setSpacing(0);
    bottomSplitter->addWidget(rhw);
    bottomSplitter->setCollapsible(0, false);

    /* create top toolbox */
    {
        QWidget *tlbox = new QWidget;
        // tlbox->setStyleSheet("background: LightGray"); // mintcream
        QHBoxLayout *hbox = new QHBoxLayout;
        tlbox->setLayout(hbox);
        hbox->setContentsMargins(0, 0, 0, 0);

        QLabel *lbl = new QLabel("Path: ");
        hbox->addWidget(lbl);

        dataName = new QLabel("");
        dataName->setStyleSheet("font-weight: bold");
        // dataName->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
        hbox->addWidget(dataName);

        copyPathBt = new QToolButton;
        copyPathBt->setIcon(QIcon(":/qdatabrowser/lucide/copy.svg"));
        copyPathBt->setToolTip("Copy path to clipboard");
        copyPathBt->setAutoRaise(true);
        connect(copyPathBt, &QToolButton::clicked, this, &QDataBrowser::onCopyPath);
        hbox->addWidget(copyPathBt);
        copyPathBt->hide();

        hbox->addStretch();

        optionsBt = new QToolButton;
        optionsBt->setIcon(QIcon(":/qdatabrowser/lucide/settings-2.svg"));
        optionsBt->setPopupMode(QToolButton::InstantPopup);
        optionsBt->setToolTip("View options menu");
        hbox->addWidget(optionsBt);

        btExport = new QToolButton;
        btExport->setIcon(QIcon(":/qdatabrowser/lucide/download.svg"));
        btExport->setText("Export");
        btExport->setToolTip("Export data/view");
        btExport->setPopupMode(QToolButton::InstantPopup);
        btExport->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        {
            QMenu *toolMenu = new QMenu(this);
            actExportCSV = toolMenu->addAction("Export data to CSV ...");
            actExportCSV->setEnabled(false);
            connect(actExportCSV, &QAction::triggered, this, &QDataBrowser::onExportCSV);
            actExportImg = toolMenu->addAction("Export plot to file ...");
            actExportImg->setEnabled(false);
            connect(actExportImg, &QAction::triggered, this, &QDataBrowser::onExportPlot);
            btExport->setMenu(toolMenu);
        }
        hbox->addWidget(btExport);

        QFrame *frm = new QFrame;
        frm->setFrameStyle(QFrame::VLine | QFrame::Sunken);
        hbox->addWidget(frm);

        leftPanelBt = new QToolButton;
        leftPanelBt->setIcon(QIcon(":/qdatabrowser/lucide/panel-left.svg"));
        leftPanelBt->setCheckable(true);
        leftPanelBt->setChecked(true);
        leftPanelBt->setToolTip("Hide left panel");
        connect(this, &QSplitter::splitterMoved, this, &QDataBrowser::onLeftSplitterMoved);
        connect(leftPanelBt, &QToolButton::clicked, this, &QDataBrowser::onLeftPanelBtClicked);
        hbox->addWidget(leftPanelBt);

        bottomPanelBt = new QToolButton;
        bottomPanelBt->setIcon(QIcon(":/qdatabrowser/lucide/panel-bottom.svg"));
        bottomPanelBt->setCheckable(true);
        bottomPanelBt->setChecked(true);
        bottomPanelBt->setToolTip("Hide bottom panel");
        connect(bottomSplitter,
                &QSplitter::splitterMoved,
                this,
                &QDataBrowser::onBottomSplitterMoved);
        connect(bottomPanelBt, &QToolButton::clicked, this, &QDataBrowser::onBottomPanelBtClicked);
        hbox->addWidget(bottomPanelBt);

        vbox->addWidget(tlbox);
    }

    /* create dataView panel */
    viewTab = new QTabWidget;
    //viewTab->setStyleSheet("background: white");
    dataView[0] = new QTabularDataView;
    viewTab->addTab(dataView[0], dataView[0]->icon(), "Table");
    dataView[1] = new QPlotDataView;
    viewTab->addTab(dataView[1], dataView[1]->icon(), "Line");
    dataView[2] = new QHeatMapDataView;
    viewTab->addTab(dataView[2], dataView[2]->icon(), "HeatMap");
    vbox->addWidget(viewTab);

    /* create bottom toolbox */
    {
        bottomPanel = new QStackedWidget;

        for (int i = 0; i < nViews; ++i)
        {
            sliceSelector[i] = new QDataSliceSelector;
            bottomPanel->addWidget(sliceSelector[i]);
            dataView[i]->setData(sliceSelector[i]->slice());
            connect(sliceSelector[i],
                    &QDataSliceSelector::sliceChanged,
                    dataView[i],
                    &QAbstractDataView::updateView);
            connect(sliceSelector[i],
                    &QDataSliceSelector::sliceChanged,
                    this,
                    &QDataBrowser::onSliceChanged);
            connect(dataView[i],
                    &QAbstractDataView::viewUpdated,
                    this,
                    &QDataBrowser::onViewUpdated);
        }

        connect(viewTab, &QTabWidget::currentChanged, bottomPanel, &QStackedWidget::setCurrentIndex);
        connect(viewTab, &QTabWidget::currentChanged, this, &QDataBrowser::onCurrentViewChanged);

        bottomSplitter->addWidget(bottomPanel);
    }

    addWidget(dataTree);
    addWidget(bottomSplitter);
    setCollapsible(1, false);
}

void QDataBrowser::setTreeTitle(const QString &t)
{
    treeTitle_ = t;
    dataModel->setHeaderData(0, Qt::Horizontal, treeTitle_);
}

bool QDataBrowser::addGroup(const QString &name, const QString &location, const QString &desc)
{
    QStandardItem *parent = fromPath(location);
    if (!parent)
        return false;
    if (!isGroup(parent))
        return false;
    QStandardItem *g = new QStandardItem(QIcon(":/qdatabrowser/lucide/folder.svg"), name);
    g->setData(QVariant::fromValue(DataStorePtr{}));
    g->setSelectable(false);
    g->setEditable(false);
    g->setToolTip(desc.isEmpty() ? "Group" : desc);
    parent->appendRow(g);
    return true;
}

bool QDataBrowser::addData(AbstractDataStore *data, const QString &location)
{
    QStandardItem *parent = fromPath(location);
    if (!parent)
        return false;
    if (!isGroup(parent))
        return false;

    QString name(data->name().c_str());
    QStandardItem *node = findChild(name, parent);
    if (!node)
    {
        node = new QStandardItem(QIcon(":/qdatabrowser/lucide/layers.svg"), name);
        parent->appendRow(node);
    }

    node->setData(QVariant::fromValue(DataStorePtr(data)));
    node->setEditable(false);
    node->setToolTip(data->description().empty() ? "Data array" : data->description().c_str());

    return true;
}

bool QDataBrowser::selectItem(const QString &path)
{
    QStandardItem *item = fromPath(path);
    if (!item)
        return false;

    QModelIndex i = item->index();
    if (!i.isValid())
        return false;

    dataTree->setCurrentIndex(i);
    return true;
}

void QDataBrowser::dataUpdated(const QString &path)
{
    QStandardItem *item = fromPath(path);
    if (!item)
        return;
    dataUpdated(item);
}

void QDataBrowser::clear(const QString &path)
{
    QStandardItem *item = fromPath(path);

    if (!item)
        return;

    if (item == dataModel->invisibleRootItem())
    {
        dataModel->removeRows(0, dataModel->rowCount());
        // setTreeTitle(treeTitle_);
        onDataItemSelect(QModelIndex(), QModelIndex());
        return;
    }

    QModelIndex I = item->index();
    if (!I.isValid())
        return;

    QModelIndex C = dataTree->currentIndex();
    bool currentDeleted = isGroup(item) ? isBelow(C, I) : I == C;

    dataModel->removeRow(I.row(), I.parent());

    if (currentDeleted)
    {
        for (int i = 0; i < nViews; ++i)
        {
            sliceSelector[i]->clear();
            dataView[i]->updateView();
        }
    }
}

QDataBrowser::PlotType QDataBrowser::plotType() const
{
    return ((QPlotDataView *) dataView[1])->plotType();
}

QDataBrowser::ViewType QDataBrowser::activeView() const
{
    return QDataBrowser::ViewType(viewTab->currentIndex());
}

void QDataBrowser::setPlotType(PlotType t)
{
    ((QPlotDataView *) dataView[1])->setPlotType(t);
}

void QDataBrowser::setActiveView(ViewType t)
{
    viewTab->setCurrentIndex(t);
}

QStandardItem *QDataBrowser::fromPath(const QString &path) const
{
    if (path == "/" || path == "")
        return dataModel->invisibleRootItem();

    QStringList lst = path.split('/');
    if (path.startsWith('/'))
        lst.takeFirst();
    QStandardItem *i = dataModel->invisibleRootItem();
    for (const QString &s : lst)
    {
        QStandardItem *j = findChild(s, i);
        if (j == nullptr)
            return nullptr;
        else
            i = j;
    }
    return i;
}

QStandardItem *QDataBrowser::findChild(const QString &name, QStandardItem *parent) const
{
    for (int row = 0; row < parent->rowCount(); ++row)
    {
        QStandardItem *ch = parent->child(row);
        QString s = ch->text();
        if (s == name)
            return ch;
    }
    return nullptr;
}

bool QDataBrowser::isGroup(QStandardItem *i)
{
    if (i == nullptr)
        return false;
    return i == dataModel->invisibleRootItem() || i->data().value<DataStorePtr>().isNull();
}

QString QDataBrowser::itemPath(QStandardItem *i)
{
    if (i == nullptr || i == dataModel->invisibleRootItem())
        return QString();
    return QString("%1/%2").arg(itemPath(i->parent())).arg(i->text());
}

bool QDataBrowser::dataUpdated(QStandardItem *i)
{
    if (isGroup(i))
    {
        for (int r = 0; r < i->rowCount(); ++r)
        {
            if (dataUpdated(i->child(r)))
                return true;
        }
        return false;
    }
    else if (i->index() == dataTree->currentIndex())
    {
        for (int i = 0; i < nViews; ++i)
            sliceSelector[i]->updateData();
        return true;
    }
    return false;
}

bool QDataBrowser::isBelow(const QModelIndex &i, const QModelIndex &g)
{
    QModelIndex p = i;
    while (p.isValid())
    {
        p = p.parent();
        if (p == g)
            return true;
    }
    return false;
}

void QDataBrowser::onLeftSplitterMoved(int pos, int index)
{
    int leftSize = sizes().front();

    if (leftSize > 0)
    {
        leftPanelBt->setChecked(true);
        leftPanelBt->setToolTip("Hide left panel");
    }
    else
    {
        leftPanelBt->setChecked(false);
        leftPanelBt->setToolTip("Show left panel");
    }
    lastLeftPanelPos = qMax(leftSize, 50);
}

void QDataBrowser::onBottomSplitterMoved(int pos, int index)
{
    int bottomSize = bottomSplitter->sizes().back();

    if (bottomSize > 0)
    {
        bottomPanelBt->setChecked(true);
        bottomPanelBt->setToolTip("Hide bottom panel");
    }
    else
    {
        bottomPanelBt->setChecked(false);
        bottomPanelBt->setToolTip("Show bottom panel");
    }
    lastBottomPanelPos = qMax(bottomSize, 50);
}

void QDataBrowser::onLeftPanelBtClicked(bool c)
{
    QList<int> sz = sizes();
    if (c)
    {
        sz.front() = lastLeftPanelPos;
        sz.back() -= lastLeftPanelPos;
        setSizes(sz);
        leftPanelBt->setToolTip("Hide left panel");
    }
    else
    {
        lastLeftPanelPos = sz.front();
        sz.front() = 0;
        sz.back() += lastLeftPanelPos;
        setSizes(sz);
        leftPanelBt->setToolTip("Show left panel");
    }
}

void QDataBrowser::onBottomPanelBtClicked(bool c)
{
    QList<int> sz = bottomSplitter->sizes();
    if (c)
    {
        sz.back() = lastBottomPanelPos;
        sz.front() -= lastBottomPanelPos;
        bottomSplitter->setSizes(sz);
        bottomPanelBt->setToolTip("Hide bottom panel");
    }
    else
    {
        lastBottomPanelPos = sz.back();
        sz.back() = 0;
        sz.front() += lastBottomPanelPos;
        bottomSplitter->setSizes(sz);
        bottomPanelBt->setToolTip("Show bottom panel");
    }
}

void QDataBrowser::onDataItemSelect(const QModelIndex &selected, const QModelIndex &deselected)
{
    const int dim0[nViews] = {2, 1, 2};
    QStandardItem *i = selected.isValid() ? dataModel->itemFromIndex(selected) : nullptr;
    for (int v = 0; v < nViews; ++v) {
        sliceSelector[v]->clear();
        dataView[v]->updateView();
    }
    if (i) {
        // get the data
        DataStorePtr D = i->data().value<DataStorePtr>();
        // handle singleton dims option
        if (D && ignoreSingletonDims_ && hasSingletonDim(D)) {
            // create a squeezed proxy data wrapper
            D = DataStorePtr(new SqueezedDataStore(D));
            // store it in the proxy container
            dataProxy.setValue(D);
        }
        if (D) {
            if (D->is_numeric()) {
                for (int i = 0; i < nViews; ++i) {
                    sliceSelector[i]->assign(D, dim0[i]);
                    dataView[i]->updateView();
                }
            } else {
                sliceSelector[0]->assign(D, dim0[0]);
                dataView[0]->updateView();
                setActiveView(QDataBrowser::Table);
            }
        }
        dataName->setText(itemPath(i));
        copyPathBt->show();
    } else {
        dataName->setText(QString());
        copyPathBt->hide();
    }
}

void QDataBrowser::onCopyPath()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    QString text = dataName->text();
    clipboard->setText(text);
}

void QDataBrowser::onSliceReset() {}

void QDataBrowser::onSliceChanged()
{
    int i = viewTab->currentIndex();
    bool ret = !sliceSelector[i]->slice()->empty();
    actExportCSV->setEnabled(ret);
    actExportImg->setEnabled(ret && dataView[i]->canExportImage());
}

void QDataBrowser::onExportCSV()
{
    int i = viewTab->currentIndex();
    if (sliceSelector[i]->slice()->empty())
        return;

    QString fname = QFileDialog::getSaveFileName(this,
                                                 tr("Export data to CSV ..."),
                                                 "export.csv",
                                                 tr("CSV files [*.csv](*.csv);; All files (*.*)"));
    if (fname.isNull())
        return;

    // csv export
    std::ofstream of(fname.toStdString());

    if (!of.is_open())
    {
        QMessageBox::critical(window(),
                              "Export data to CSV ...",
                              QString("Error opening file:\n%1").arg(fname));
        return;
    }

    sliceSelector[i]->slice()->export_csv(of);
}

void QDataBrowser::onExportPlot()
{
    int i = viewTab->currentIndex();
    if (!dataView[i]->canExportImage())
        return;
    dataView[i]->exportImage();
}

void QDataBrowser::onCurrentViewChanged(int i)
{
    bool ret = !sliceSelector[i]->slice()->empty();
    actExportCSV->setEnabled(ret);
    actExportImg->setEnabled(ret && dataView[i]->canExportImage());
    optionsBt->setMenu(dataView[i]->optionsMenu());
}

void QDataBrowser::onViewUpdated()
{
    int i = viewTab->currentIndex();
    bool ret = !sliceSelector[i]->slice()->empty();
    actExportCSV->setEnabled(ret);
    actExportImg->setEnabled(ret && dataView[i]->canExportImage());
}
