#ifndef ABSTRACTDATASTORE_H
#define ABSTRACTDATASTORE_H

#include <cassert>
#include <string>
#include <vector>

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

#endif // ABSTRACTDATASTORE_H
