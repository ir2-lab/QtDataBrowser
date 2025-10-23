#ifndef ABSTRACTDATASTORE_H
#define ABSTRACTDATASTORE_H

#include <cassert>
#include <iterator>
#include <string>
#include <vector>

class AbstractDataStore
{
public:
    typedef std::vector<size_t> dim_t;
    typedef std::vector<double> vec_t;

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
    bool empty() const { return size() == 0; }
    const std::string &name() const { return name_; }
    const std::string &description() const { return desc_; }
    const std::string &dim_name(size_t d) const { return dim_name_[d]; }
    const std::string &dim_desc(size_t d) const { return dim_desc_[d]; }

    virtual vec_t x(const dim_t &i) const
    {
        vec_t v(i.size());
        for (int k = 0; k < i.size(); ++k)
            v[k] = i[k];
        return v;
    }

protected:
    dim_t dim_;
    std::string name_;
    std::string desc_;
    std::vector<std::string> dim_name_;
    std::vector<std::string> dim_desc_;

    virtual size_t get_y(size_t d, const dim_t &i0, size_t n, double *v) const = 0;
    virtual size_t get_x(size_t d, size_t n, double *v) const;

    friend class DataSlice;
};

enum class StorageOrder { RowMajor, ColumnMajor };

template<StorageOrder>
struct array_index;

/*
 * Row-major:
 * 
 * | 1 2 3 |
 * | 4 5 6 |
 * 
 */
template<>
struct array_index<StorageOrder::RowMajor>
{
    template<class T>
    static T k(const T *iFirst, const T *iLast, const T *dim)
    {
        return _k_(iLast, iFirst, dim + iLast - iFirst);
    }

private:
    template<class T>
    static T _k_(const T *i, const T *iend, const T *d)
    {
        if (i == iend)
            return *i;
        else
            return *i + *d * _k_(std::next(i, -1), iend, std::next(d, -1));
    }
};

/*
 * Col-major:
 * 
 * | 1 3 5 |
 * | 2 4 6 |
 * 
 */
template<>
struct array_index<StorageOrder::ColumnMajor>
{
    template<class T>
    static T k(const T *iFirst, const T *iLast, const T *dim)
    {
        return _k_(iFirst, iLast, dim);
    }

private:
    template<class T>
    static T _k_(const T *i, const T *iend, const T *d)
    {
        if (i == iend)
            return *i;
        else
            return *i + *d * _k_(std::next(i, 1), iend, std::next(d, 1));
    }
};

// struct array_index
// {
//     template<class T>
//     static T row_major(const T *i, const T *dim, T n)
//     {
//         return _idx_row_major_(i + n - 1, i, dim + n - 1);
//     }

//     template<class T>
//     static T row_major(const std::vector<T> i, const std::vector<T> dim)
//     {
//         return _idx_row_major_(&i.back(), &i.front(), &dim.back());
//     }

//     template<class T>
//     static T column_major(const T *i, const T *dim, T n)
//     {
//         return _idx_column_major_(i, i + n - 1, dim);
//     }

//     template<class T>
//     static T column_major(const std::vector<T> i, const std::vector<T> dim)
//     {
//         return _idx_column_major_(&i.front(), &i.back(), &dim.front());
//     }

// private:
//     template<class T>
//     static T _idx_row_major_(const T *i, const T *iend, const T *dim)
//     {
//         if (i == iend)
//             return *i;
//         T ii = *i--;
//         T di = *dim--;
//         return ii + di * _idx_row_major_(i, iend, dim);
//     }
//     template<class T>
//     static T _idx_column_major_(const T *i, const T *iend, const T *dim)
//     {
//         if (i == iend)
//             return *i;
//         T ii = *i++;
//         T di = *dim++;
//         return ii + di * _idx_column_major_(i, iend, dim);
//     }
// };

inline size_t AbstractDataStore::get_x(size_t d, size_t n, double *v) const
{
    assert(d < dim_.size());
    const size_t &m = std::min(dim_[d], n);
    for (size_t i = 0; i < m; ++i)
        v[i] = 1.0 * i;
    return n;
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

#endif // ABSTRACTDATASTORE_H
