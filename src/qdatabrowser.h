#ifndef QDATABROWSER_H
#define QDATABROWSER_H

#include <cassert>
#include <memory>
#include <string>
#include <vector>

#include <QModelIndex>
#include <QSplitter>
#include <QStandardItemModel>

#include "qtdatabrowser_export.h"

class QStandardItemModel;
class QStandardItem;
class QTreeView;
class QToolButton;
class QLabel;
class QStackedWidget;
class QTableWidget;

class QAbstractDataView;
class QDataSliceSelector;
class QDataBrowserPage;
class QDataBrowserTab;

/**
 * @brief The AbstractDataSet class provides an interface to a multidimensional dataset
 *
 * A multi-dimensional DataSet has the following minimum components:
 * - A name (string)
 * - A dimension vector [N1, N2, ..., Nm], m>=1, Ni>=1
 * - An m-dimensional data array Y[N1, N2, N3, ..., Nm]
 *
 * The data can be either numeric of text.
 * Numeric data are represented by double numbers and text data by strings (each element of Y is a string).
 *
 * Optionally, the DataSet can also have
 * - A description (string)
 * - A set of vectors, X1[N1], X2[N2], ..., Xm[Nm], with the independent (x-) values
 * for all the dimensions/axes
 * - Some of the axes may be categorical. In this case Xi[Ni] is a string array
 * - A set of axis names
 * - A set of axis descriptions
 * - An array of errors, dY, the same shape/size as Y
 *
 */
class QTDATABROWSER_EXPORT AbstractDataSet
{
public:
    /// dimension array type
    typedef std::vector<size_t> dim_t;
    /// numeric data array type
    typedef std::vector<double> vec_t;
    /// text data array type
    typedef std::vector<std::string> strvec_t;

    AbstractDataSet() = default;
    AbstractDataSet(const AbstractDataSet &other) = default;

    /**
     * @brief Construct a new AbstractDataSet object
     *
     * If d_name is empty, default dimension names D0, D1, ... are assigned.
     *
     * If d_desc is empty, no dimension descriptions are assigned.
     *
     * @param n DataSet name
     * @param d dimension size vector, [N_0, N_1, ...]
     * @param d_name dimension names, optional
     * @param d_desc dimension descriptions, optional
     */
    AbstractDataSet(const std::string &n,
                    const dim_t &d,
                    const strvec_t &d_name = {},
                    const strvec_t &d_desc = {});

    virtual ~AbstractDataSet() {}

    /// Return the # of dimensions
    size_t ndim() const { return dim_.size(); }
    /// Return the dimensions array
    const dim_t &dim() const { return dim_; }
    /// Return total # of elements
    size_t size() const
    {
        if (dim_.size() == 0)
            return 0;
        size_t n{1};
        for (const auto &m : dim_)
            n *= m;
        return n;
    }
    /// Return true if size()==1
    bool is_scalar() const { return ndim() == 1 && dim_[0] == 1; }
    /// Return true if size()==0
    bool empty() const { return size() == 0; }
    /// Return the DataSet name
    const std::string &name() const { return name_; }
    /// Return the DataSet description
    const std::string &description() const { return desc_; }
    /// Return the name of dimension @a d
    const std::string &dim_name(size_t d) const { return dim_name_[d]; }
    /// Return the description of dimension @a d
    const std::string &dim_desc(size_t d) const { return dim_desc_[d]; }
    /// Return true if the data is numeric
    virtual bool is_numeric() const { return true; }
    /// Return true if errors to the data are also available
    virtual bool hasErrors() const { return false; }
    /// Return true if dimension @a d has a categorical axis
    virtual bool is_x_categorical(size_t d) const { return false; }
    /// Return numeric data along dimension @a d starting at index @a i0
    size_t get_y(size_t d, const dim_t &i0, vec_t &y) const
    {
        return get_y(d, i0, y.size(), y.data());
    }
    /// Return text data along dimension @a d starting at index @a i0
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

inline size_t AbstractDataSet::get_x(size_t d, size_t n, double *v) const
{
    assert(d < dim_.size());
    const size_t m = std::min(dim_[d], n);
    for (size_t i = 0; i < m; ++i)
        v[i] = 1.0 * i;
    return m;
}

inline AbstractDataSet::dim_t::const_iterator find_max(const AbstractDataSet::dim_t &dim)
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

class QTDATABROWSER_EXPORT QDataModel : public QStandardItemModel
{
    Q_OBJECT

    Q_PROPERTY(QString title READ title WRITE setTitle)
    Q_PROPERTY(bool squeezeSingletonDims READ squeezeSingletonDims WRITE setSqueezeSingletonDims)

public:
    explicit QDataModel(const QString &title, QObject *parent = nullptr);

    bool squeezeSingletonDims() const { return squeezeSingletonDims_; }
    void setSqueezeSingletonDims(bool on = true) { squeezeSingletonDims_ = on; }

    QString title() const { return headerData(0, Qt::Horizontal).toString(); }
    void setTitle(const QString &t) { setHeaderData(0, Qt::Horizontal, t); }

    QStandardItem *itemFromPath(const QString &path) const;
    QString pathFromItem(QStandardItem *item) const;
    QString pathFromIndex(const QModelIndex &index) const;

    // adds a group at loc; creates missing intermediate groups as needed
    // returns false if any segment of loc is a data item
    bool addGroup(const QString &name, const QString &loc = "/");

    // adds a data item at loc; name is taken from d->name(); takes ownership
    bool addData(std::unique_ptr<AbstractDataSet> d, const QString &loc = "/");

    // removes the node at path and all its descendants; "/" clears everything
    void clear(const QString &path = "/");

    // call to signify that data at & below the given path have changed
    // views are updated
    void setDatasetChanged(const QString &path = "/");

    bool isGroup(const QModelIndex &i) const;
    bool isEmpty() const;

    enum FindFlags { FindDataSet = 0x01, FindGroup = 0x02, FindAll = 0x03 };

    int countItems(const QString &path = "/", FindFlags f = FindAll, bool recursive = true);

    // find all items that match the filter
    QModelIndexList match(const QString &nameFilter = "*",
                          FindFlags f = FindAll,
                          const QString &from = "/",
                          bool recursive = true,
                          int hits = -1) const;

signals:

private:
    bool squeezeSingletonDims_{true};
    QStandardItem *ensurePath_(const QString &path, bool createMissing) const;
    QStandardItem *findChild_(const QString &name, QStandardItem *parent) const;
    bool isGroupItem_(QStandardItem *item) const;
    void setDatasetChanged_(QStandardItem *i);
    void countHelper_(QStandardItem *parent, FindFlags f, bool recursive, uint &cnt) const;
    void matchHelper_(QStandardItem *parent,
                      const QString &nameFilter,
                      FindFlags f,
                      bool recursive,
                      int hits,
                      QModelIndexList &lst) const;
};

class QTDATABROWSER_EXPORT QDataBrowser : public QSplitter
{
    Q_OBJECT

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
    explicit QDataBrowser(QWidget *parent = nullptr);

    // access the underlying tree model
    QDataModel *model() const { return dataModel; }
    void setModel(QDataModel *m);

    // get/set data path of current page
    QString currentDataPath() const;
    bool setCurrentDataPath(const QString &path);
    // get/set current page view type
    ViewType currentViewType() const;
    void setCurrentViewType(ViewType v);
    // set/get current plot type
    PlotType currentPlotType() const;
    void setCurrentPlotType(PlotType p);

    // page API
    int pageCount() const;
    void addPage(const QString &path = "/");
    void insertPage(int index, const QString &path = "/");
    int currentPage() const;
    void setCurrentPage(int index);
    bool hasSavedState() const;

signals:

private:
    // data model
    QDataModel *dataModel;

    // left panel widgets
    QTreeView *dataTree;
    QTableWidget *infoTable;

    // right panel widgets
    QDataBrowserTab *pagesTab;
    int lastLeftPanelPos;

    // set while dataTree's selection is being synced to match the active
    // page, so onDataItemSelect doesn't treat it as a user-driven pick
    // and reload the page
    bool syncingTreeSelection_{false};

    void updateInfoTable(const QStandardItem *i = nullptr);

private slots:
    void onLeftSplitterMoved(int pos, int index);
    void onLeftPanelBtClicked(bool c);
    void onDataItemSelect(const QModelIndex &selected, const QModelIndex &deselected);
    void onPageChanged(int index);
};

#endif // QDATABROWSER_H
