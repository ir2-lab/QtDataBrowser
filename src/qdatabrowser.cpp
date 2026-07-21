#include "qdatabrowser.h"

#include "dataslice.h"
#include "qdatabrowserpage.h"
#include "qdatabrowsertab.h"
#include "qdatasliceselector.h"
#include "qdataview.h"

#include <QClipboard>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFrame>
#include <QGuiApplication>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QTableWidget>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>

AbstractDataSet::AbstractDataSet(const std::string &n,
                                 const dim_t &d,
                                 const strvec_t &d_name,
                                 const strvec_t &d_desc)
    : dim_(d)
    , name_(n)
    , dim_name_(d_name)
    , dim_desc_(d_desc)
{
    assert(!empty());
    if (dim_name_.empty()) {
        dim_name_.resize(d.size());
        for (int i = 0; i < (int) dim_name_.size(); ++i) {
            std::string s("D");
            s += std::to_string(i);
            dim_name_[i] = s;
        }
    }
    if (dim_desc_.empty()) {
        dim_desc_.resize(d.size());
    }
}

class SqueezedDataSet : public AbstractDataSet
{
public:
    SqueezedDataSet(const DataSetPtr d)
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
                for (int i = 0; i < (int) d->dim().size(); ++i) {
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
    virtual ~SqueezedDataSet() {}

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
    QWeakPointer<AbstractDataSet> D_;
    dim_t dim_idx_;

    // expand a pointer to squeezed data (i0) to a pointer to original data (i1)
    dim_t i1(const dim_t &i0) const
    {
        dim_t i1_(D_.lock()->ndim(), 0);
        for (int i = 0; i < (int) ndim(); ++i)
            i1_[dim_idx_[i]] = i0[i];
        return i1_;
    }

    size_t get_y(size_t d, const dim_t &i0, size_t n, double *v) const override
    {
        if (D_.isNull())
            return 0;
        DataSetPtr p = D_.lock();
        std::vector<double> buff(n);
        int m = p->get_y(dim_idx_[d], i1(i0), buff);
        std::copy(buff.begin(), buff.begin() + m, v);
        return m;
    }
    size_t get_dy(size_t d, const dim_t &i0, size_t n, double *v) const override
    {
        if (D_.isNull())
            return 0;
        DataSetPtr p = D_.lock();
        std::vector<double> buff(n);
        int m = p->get_dy(dim_idx_[d], i1(i0), buff);
        std::copy(buff.begin(), buff.begin() + m, v);
        return m;
    }
    size_t get_x(size_t d, size_t n, double *v) const override
    {
        if (D_.isNull())
            return 0;
        DataSetPtr p = D_.lock();
        std::vector<double> buff(n);
        int m = p->get_x(dim_idx_[d], buff);
        std::copy(buff.begin(), buff.begin() + m, v);
        return m;
    }

private:
    SqueezedDataSet();
};

// ── QDataModel ────────────────────────────────────────────────────────────

QDataModel::QDataModel(const QString &title, QObject *parent)
    : QStandardItemModel(0, 1, parent)
{
    setTitle(title);
}

bool QDataModel::isGroupItem_(QStandardItem *item) const
{
    if (!item)
        return false;
    if (item == invisibleRootItem())
        return true;
    return item->data().value<DataSetPtr>().isNull();
}

bool QDataModel::isGroup(const QModelIndex &i) const
{
    if (!i.isValid())
        return true;
    return isGroupItem_(itemFromIndex(i));
}

bool QDataModel::isEmpty() const
{
    return invisibleRootItem()->rowCount();
}

int QDataModel::countItems(const QString &path, FindFlags f, bool recursive)
{
    uint c = 0;
    QStandardItem *item = itemFromPath(path);
    if (item && isGroupItem_(item)) {
        countHelper_(item, f, recursive, c);
    }
    return c;
}

QModelIndexList QDataModel::match(
    const QString &nameFilter, FindFlags f, const QString &from, bool recursive, int hits) const
{
    QModelIndexList lst;
    QStandardItem *item = itemFromPath(from);
    if (item && isGroupItem_(item)) {
        matchHelper_(item, nameFilter, f, recursive, hits, lst);
    }
    return lst;
}

QStandardItem *QDataModel::findChild_(const QString &name, QStandardItem *parent) const
{
    for (int row = 0; row < parent->rowCount(); ++row)
    {
        QStandardItem *ch = parent->child(row);
        if (ch->text() == name)
            return ch;
    }
    return nullptr;
}

QStandardItem *QDataModel::ensurePath_(const QString &path, bool createMissing) const
{
    if (path.isEmpty() || path == "/")
        return invisibleRootItem();

    QStringList parts = path.split('/');
    if (parts.first().isEmpty())
        parts.takeFirst();

    QStandardItem *cur = invisibleRootItem();
    for (const QString &part : parts)
    {
        QStandardItem *child = findChild_(part, cur);
        if (!child)
        {
            if (!createMissing)
                return nullptr;
            child = new QStandardItem(QIcon(":/qdatabrowser/icons/lucide/folder.svg"), part);
            child->setData(QVariant::fromValue(DataSetPtr{}));
            child->setSelectable(false);
            child->setEditable(false);
            cur->appendRow(child);
        }
        else if (!isGroupItem_(child))
        {
            return nullptr;
        }
        cur = child;
    }
    return cur;
}

QStandardItem *QDataModel::itemFromPath(const QString &path) const
{
    if (path.isEmpty() || path == "/")
        return invisibleRootItem();

    QStringList parts = path.split('/');
    if (parts.first().isEmpty())
        parts.takeFirst();

    QStandardItem *item = invisibleRootItem();
    for (const QString &part : parts) {
        QStandardItem *child = findChild_(part, item);
        if (!child)
            return nullptr;
        item = child;
    }
    return item;
}

QString QDataModel::pathFromItem(QStandardItem *item) const
{
    if (!item)
        return QString();
    if (item == invisibleRootItem())
        return "/";
    QStringList parts;
    QStandardItem *cur = item;
    while (cur && cur != invisibleRootItem()) {
        parts.prepend(cur->text());
        cur = cur->parent() ? cur->parent() : invisibleRootItem();
        if (cur == invisibleRootItem())
            break;
    }
    return "/" + parts.join('/');
}

QString QDataModel::pathFromIndex(const QModelIndex &index) const
{
    return pathFromItem(itemFromIndex(index));
}

bool QDataModel::addGroup(const QString &name, const QString &loc)
{
    QStandardItem *parent = ensurePath_(loc, true);
    if (!parent)
        return false;
    QStandardItem *g = new QStandardItem(QIcon(":/qdatabrowser/icons/lucide/folder.svg"), name);
    g->setData(QVariant::fromValue(DataSetPtr{}));
    g->setSelectable(false);
    g->setEditable(false);
    parent->appendRow(g);
    return true;
}

bool hasSingletonDim(const AbstractDataSet *d)
{
    if (d->empty())
        return false;
    for (size_t d : d->dim()) {
        if (d == 1)
            return true;
    }
    return false;
}

bool QDataModel::addData(std::unique_ptr<AbstractDataSet> d, const QString &loc)
{
    QStandardItem *parent = ensurePath_(loc, true);
    if (!parent)
        return false;

    // create or reuse a node
    QString name(d->name().c_str());
    QStandardItem *node = findChild_(name, parent);
    if (!node)
    {
        node = new QStandardItem(QIcon(":/qdatabrowser/icons/lucide/layers.svg"), name);
        parent->appendRow(node);
    }
    node->setToolTip(d->description().empty() ? "Data array" : d->description().c_str());

    // clear previous data
    node->setData(QVariant(), Qt::UserRole + 1);
    node->setData(QVariant(), Qt::UserRole + 2);

    // set current data
    if (squeezeSingletonDims_ && hasSingletonDim(d.get())) {
        // handle singleton dims option
        DataSetPtr D0(d.release());
        // create a squeezed proxy data wrapper
        DataSetPtr D(new SqueezedDataSet(D0));
        // store both in the model node
        node->setData(QVariant::fromValue(D), Qt::UserRole + 1);
        node->setData(QVariant::fromValue(D0), Qt::UserRole + 2);
    } else {
        node->setData(QVariant::fromValue(DataSetPtr(d.release())), Qt::UserRole + 1);
    }
    node->setEditable(false);
    return true;
}

void QDataModel::clear(const QString &path)
{
    if (path.isEmpty() || path == "/")
    {
        removeRows(0, rowCount());
        return;
    }
    QStandardItem *item = ensurePath_(path, false);
    if (!item || item == invisibleRootItem())
        return;
    QModelIndex idx = item->index();
    removeRow(idx.row(), idx.parent());
}

void QDataModel::setDatasetChanged(const QString &path)
{
    QStandardItem *item = itemFromPath(path);
    if (item)
        setDatasetChanged_(item);
}

void QDataModel::setDatasetChanged_(QStandardItem *i)
{
    if (isGroupItem_(i)) {
        for (int r = 0; r < i->rowCount(); ++r) {
            setDatasetChanged_(i->child(r));
        }
    } else
        emit itemChanged(i);
}

void QDataModel::countHelper_(QStandardItem *parent, FindFlags f, bool recursive, uint &cnt) const
{
    for (int r = 0; r < parent->rowCount(); ++r) {
        QStandardItem *item = parent->child(r);
        bool g = isGroupItem_(item);
        int fi = g ? FindGroup : FindDataSet;
        if (fi & f)
            cnt++;
        if (g && recursive)
            countHelper_(item, f, recursive, cnt);
    }
}

void QDataModel::matchHelper_(QStandardItem *parent,
                                  const QString &nameFilter,
                                  FindFlags f,
                                  bool recursive,
                                  int hits,
                                  QModelIndexList &lst) const
{
    for (int r = 0; r < parent->rowCount(); ++r) {
        QStandardItem *item = parent->child(r);
        bool g = isGroupItem_(item);
        int fi = g ? FindGroup : FindDataSet;
        if ((fi & f) && QDir::match(nameFilter, item->text())) {
            lst.append(indexFromItem(item));
            if (hits > 0 && lst.count() >= hits)
                return;
        }
        if (g && recursive) {
            matchHelper_(item, nameFilter, f, recursive, hits, lst);
            if (hits > 0 && lst.count() >= hits)
                return;
        }
    }
}

// ── end QDataModel ────────────────────────────────────────────────────────

QDataBrowser::QDataBrowser(QWidget *parent)
    : QSplitter{parent}
    , lastLeftPanelPos(100)
{
    /* create data model */
    dataModel = new QDataModel("");
    //dataModel->setTitle("");

    /* create left-side tree widget */
    dataTree = new QTreeView;
    dataTree->setModel(dataModel);
    dataTree->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(dataTree->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            &QDataBrowser::onDataItemSelect);
    // dataTree->setStyleSheet("background: white"); // ivory, honeydew

    /* create left-side info table */
    infoTable = new QTableWidget;
    infoTable->setAlternatingRowColors(true);
    infoTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // infoTable->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

    QSplitter *leftSplitter = new QSplitter(Qt::Vertical);
    leftSplitter->setChildrenCollapsible(false);
    leftSplitter->addWidget(dataTree);
    {
        QWidget *w = new QWidget;
        QVBoxLayout *vbox = new QVBoxLayout;
        vbox->setContentsMargins(0, 0, 0, 0);
        vbox->setSpacing(0);
        w->setLayout(vbox);

        QLabel *lbl = new QLabel("Properties");
        lbl->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
        vbox->addWidget(lbl);

        vbox->addWidget(infoTable);
        leftSplitter->addWidget(w);
    }

    /* create right hand widget */
    pagesTab = new QDataBrowserTab;
    connect(this, &QSplitter::splitterMoved, this, &QDataBrowser::onLeftSplitterMoved);
    connect(pagesTab->leftPanelButton(),
            &QToolButton::clicked,
            this,
            &QDataBrowser::onLeftPanelBtClicked);
    connect(pagesTab, &QTabWidget::currentChanged, this, &QDataBrowser::onPageChanged);

    addWidget(leftSplitter);
    addWidget(pagesTab);
    setCollapsible(1, false);
}

void QDataBrowser::setModel(QDataModel *m)
{
    if (dataModel == m)
        return;

    if (dataModel)
        disconnect(dataTree->selectionModel(), nullptr, this, nullptr);

    dataModel = m;
    m->setParent(this);
    dataTree->setModel(dataModel);
    connect(dataTree->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            &QDataBrowser::onDataItemSelect);

    pagesTab->clear();
    int npages = pagesTab->count();
    int ndatasets = dataModel->countItems("/", QDataModel::FindDataSet);

    // remove pages that will not be filled, exept the current one
    while (npages > 1 && npages > ndatasets) {
        int i = npages - 1;
        while (i == pagesTab->currentIndex() && i > 0)
            i--;
        pagesTab->removeTab(i);
        npages--;
    }

    if (ndatasets == 0)
        return;

    if (ndatasets == 1) {
        QModelIndexList lst = dataModel->match("*", QDataModel::FindDataSet);
        assert(!lst.isEmpty());
        dataTree->setCurrentIndex(lst.front());
        return;
    }

    // get datasets for all pages
    QModelIndexList lst = dataModel->match("*", QDataModel::FindDataSet, "/", true, npages);
    assert(lst.count() == npages);
    for (int i = 0; i < pagesTab->count(); ++i) {
        auto p = pagesTab->page(i);
        const QString &path = p->savedState().path;
        bool found = false;
        if (!path.isEmpty()) {
            QStandardItem *item = dataModel->itemFromPath(path);
            if (item) {
                pagesTab->setTabText(i, path);
                pagesTab->setTabToolTip(i, path);
                p->setData(path, item);
                found = true;
            }
        }
        if (!found) {
            QStandardItem *item = dataModel->itemFromIndex(lst.at(i));
            if (item) {
                QString dpath = dataModel->pathFromItem(item);
                pagesTab->setTabText(i, dpath);
                pagesTab->setTabToolTip(i, dpath);
                p->setData(dpath, item);
            }
        }
    }

    const QStandardItem *item = pagesTab->currentPage()->dataItem();
    syncingTreeSelection_ = true;
    dataTree->setCurrentIndex(dataModel->indexFromItem(item));
    syncingTreeSelection_ = false;
    updateInfoTable(item);
}

QString QDataBrowser::currentDataPath() const
{
    return pagesTab->currentPage()->dataPath();
}

bool QDataBrowser::setCurrentDataPath(const QString &path)
{
    QStandardItem *item = dataModel->itemFromPath(path);
    if (!item)
        return false;
    dataTree->setCurrentIndex(dataModel->indexFromItem(item));
    return true;
}

void QDataBrowser::setCurrentViewType(ViewType v)
{
    pagesTab->currentPage()->setActiveView(v);
}

void QDataBrowser::setCurrentPlotType(PlotType p)
{
    pagesTab->currentPage()->setPlotType(p);
}

QDataBrowser::PlotType QDataBrowser::currentPlotType() const
{
    return pagesTab->currentPage()->plotType();
}

QDataBrowser::ViewType QDataBrowser::currentViewType() const
{
    return pagesTab->currentPage()->activeView();
}

int QDataBrowser::pageCount() const
{
    return pagesTab->count();
}

void QDataBrowser::addPage(const QString &path)
{
    pagesTab->addPage();
    setCurrentDataPath(path);
}

void QDataBrowser::insertPage(int index, const QString &path)
{
    pagesTab->insertPage(index);
    setCurrentDataPath(path);
}

int QDataBrowser::currentPage() const
{
    return pagesTab->currentIndex();
}

void QDataBrowser::setCurrentPage(int index)
{
    pagesTab->setCurrentIndex(index);
}

bool QDataBrowser::hasSavedState() const
{
    for (int i = 0; i < pagesTab->count(); ++i) {
        if (!(pagesTab->page(i)->savedState().path.isEmpty()))
            return true;
    }
    return false;
}

void QDataBrowser::updateInfoTable(const QStandardItem *it)
{
    infoTable->clear();

    if (!it)
        return;

    DataSetPtr D = it->data().value<DataSetPtr>();
    if (!D)
        return;

    if (dataModel->squeezeSingletonDims()) {
        QVariant V = it->data(Qt::UserRole + 2);
        if (!V.isNull())
            D = V.value<DataSetPtr>();
    }

    infoTable->setColumnCount(2);
    infoTable->setRowCount(4 + D->ndim());

    // infoTable->horizontalHeader()->hide();
    infoTable->verticalHeader()->hide();

    QStringList lbls;
    lbls << "Properties" << "";
    // infoTable->setHorizontalHeaderLabels(lbls);

    QTableWidgetItem *item;
    int r = 0;
    item = new QTableWidgetItem("Name");
    infoTable->setItem(r, 0, item);
    item = new QTableWidgetItem(D->name().c_str());
    infoTable->setItem(r, 1, item);

    r++;
    item = new QTableWidgetItem("Type");
    infoTable->setItem(r, 0, item);
    item = new QTableWidgetItem(D->is_numeric() ? "Numeric" : "Text");
    infoTable->setItem(r, 1, item);

    r++;
    item = new QTableWidgetItem("Description");
    infoTable->setItem(r, 0, item);
    item = new QTableWidgetItem(D->description().c_str());
    infoTable->setItem(r, 1, item);

    r++;
    item = new QTableWidgetItem("Shape");
    infoTable->setItem(r, 0, item);
    QString shapeStr;
    if (D->is_scalar())
        shapeStr = "Scalar";
    else
    {
        shapeStr = QString::number(D->dim()[0]);
        for (int i = 1; i < (int) D->ndim(); ++i) {
            shapeStr += " × ";
            shapeStr += QString::number(D->dim()[i]);
        }
    }
    item = new QTableWidgetItem(shapeStr);
    infoTable->setItem(r, 1, item);

    for (int i = 0; i < (int) D->ndim(); ++i) {
        r++;
        item = new QTableWidgetItem(QString("D%1").arg(i));
        infoTable->setItem(r, 0, item);
        if (D->dim_desc(i).empty())
        {
            item = new QTableWidgetItem(
                D->dim_name(i).c_str());
        }
        else
        {
            item = new QTableWidgetItem(
                QString("%1, %2").arg(D->dim_name(i).c_str()).arg(D->dim_desc(i).c_str()));
        }
        infoTable->setItem(r, 1, item);
    }

    // infoTable->resizeColumnToContents(0);
    infoTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    infoTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    infoTable->horizontalHeader()->hide();
}

void QDataBrowser::onLeftSplitterMoved(int pos, int index)
{
    int leftSize = sizes().front();

    if (leftSize > 0)
    {
        pagesTab->leftPanelButton()->setChecked(true);
        pagesTab->leftPanelButton()->setToolTip("Hide left panel");
    }
    else
    {
        pagesTab->leftPanelButton()->setChecked(false);
        pagesTab->leftPanelButton()->setToolTip("Show left panel");
    }
    lastLeftPanelPos = qMax(leftSize, 50);
}

void QDataBrowser::onLeftPanelBtClicked(bool c)
{
    QList<int> sz = sizes();
    if (c)
    {
        sz.front() = lastLeftPanelPos;
        sz.back() -= lastLeftPanelPos;
        setSizes(sz);
        pagesTab->leftPanelButton()->setToolTip("Hide left panel");
    }
    else
    {
        lastLeftPanelPos = sz.front();
        sz.front() = 0;
        sz.back() += lastLeftPanelPos;
        setSizes(sz);
        pagesTab->leftPanelButton()->setToolTip("Show left panel");
    }
}

void QDataBrowser::onDataItemSelect(const QModelIndex &selected, const QModelIndex &deselected)
{
    if (syncingTreeSelection_)
        return;

    QStandardItem *i = selected.isValid() ? dataModel->itemFromIndex(selected) : nullptr;
    updateInfoTable(i);
    QString path = selected.isValid() ? dataModel->pathFromIndex(selected) : QString();
    pagesTab->setCurrentData(path, i);
}

void QDataBrowser::onPageChanged(int index)
{
    QDataBrowserPage *p = pagesTab->currentPage();
    if (!p)
        return;
    QStandardItem *item = dataModel->itemFromPath(p->dataPath());
    if (!item)
        return;
    syncingTreeSelection_ = true;
    dataTree->setCurrentIndex(dataModel->indexFromItem(item));
    syncingTreeSelection_ = false;
    updateInfoTable(item);
}
