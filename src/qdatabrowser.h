#ifndef QDATABROWSER_H
#define QDATABROWSER_H

#include <cassert>
#include <string>
#include <vector>

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

class AbstractDataStore;

class QDataBrowser : public QSplitter
{
    Q_OBJECT

    Q_PROPERTY(QString treeTitle READ treeTitle WRITE setTreeTitle)
    Q_PROPERTY(bool ignoreSingletonDims READ ignoreSingletonDims CONSTANT)

public:
    enum PlotType
    {
        Line,
        Points,
        LineAndPoints,
        Stairs,
        ErrorBar
    };
    enum ViewType
    {
        Table,
        Plot,
        HeatMap
    };

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

class AbstractDataStore
{
public:
    typedef std::vector<size_t> dim_t;
    typedef std::vector<double> vec_t;
    typedef std::vector<std::string> strvec_t;

    AbstractDataStore() = default;
    AbstractDataStore(const AbstractDataStore &other) = default;
    AbstractDataStore(const std::string &n, const dim_t &d)
        : dim_(d), name_(n), dim_name_(d.size()), dim_desc_(d.size())
    {
        assert(!empty());
        for (int i = 0; i < dim_name_.size(); ++i)
        {
            std::string s("D");
            s += std::to_string(i);
            dim_name_[i] = s;
        }
    }
    virtual ~AbstractDataStore() {}

    size_t ndim() const { return dim_.size(); }
    const dim_t &dim() const { return dim_; }
    size_t size() const
    {
        if (dim_.size() == 0)
            return 0;
        size_t n{1};
        for (const auto &m : dim_)
            n *= m;
        return n;
    }
    bool is_scalar() const { return size() == 1; }
    bool empty() const { return size() == 0; }
    const std::string &name() const { return name_; }
    const std::string &description() const { return desc_; }
    const std::string &dim_name(size_t d) const { return dim_name_[d]; }
    const std::string &dim_desc(size_t d) const { return dim_desc_[d]; }

    virtual bool is_numeric() const { return true; }
    virtual bool hasErrors() const { return false; }
    virtual bool is_x_categorical(size_t d) const { return false; }

    size_t get_y(size_t d, const dim_t &i0, vec_t &y) const
    {
        return get_y(d, i0, y.size(), y.data());
    }

    virtual size_t get_y_text(size_t d, const dim_t &i0, strvec_t &y) const { return 0; }

    size_t get_dy(size_t d, const dim_t &i0, vec_t &y) const
    {
        return get_dy(d, i0, y.size(), y.data());
    }
    size_t get_x(size_t d, vec_t &x) const { return get_x(d, x.size(), x.data()); }
    virtual size_t get_x_categorical(size_t d, strvec_t &x) const { return 0; }

protected:
    dim_t dim_;
    std::string name_;
    std::string desc_;
    std::vector<std::string> dim_name_;
    std::vector<std::string> dim_desc_;

    virtual size_t get_y(size_t d, const dim_t &i0, size_t n, double *v) const { return 0; }
    virtual size_t get_dy(size_t d, const dim_t &i0, size_t n, double *v) const { return 0; }
    virtual size_t get_x(size_t d, size_t n, double *v) const;

    friend class DataSlice;
};

inline size_t AbstractDataStore::get_x(size_t d, size_t n, double *v) const
{
    assert(d < dim_.size());
    const size_t m = std::min(dim_[d], n);
    for (size_t i = 0; i < m; ++i)
        v[i] = 1.0 * i;
    return m;
}

inline AbstractDataStore::dim_t::const_iterator find_max(const AbstractDataStore::dim_t &dim)
{
    auto jt = dim.begin();
    size_t nx = *jt;
    for (auto it = dim.begin(); it != dim.end(); ++it)
    {
        const size_t &ni = *it;
        if (ni > nx)
        {
            nx = ni;
            jt = it;
        }
    }
    return jt;
}

#endif // QDATABROWSER_H
