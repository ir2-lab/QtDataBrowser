#include "dataslice.h"

#include <iostream>

void DataSlice::clear()
{
    dim_idx_.clear();
    dim_order_.clear();
    i0_.clear();
    data_.clear();
    x_.clear();
    y_.clear();
    dim_.clear();
    name_.clear();
    desc_.clear();
    dim_name_.clear();
    dim_desc_.clear();
    D_.clear();
}

void DataSlice::assign(const DataStorePtr d, size_t dx, const dim_t &i0)
{
    clear();
    D_ = d;
    dim_idx_ = {dx, 0UL - 1};
    dim_ = {d->dim()[dx]};
    size_t sz = dim_[0];
    data_.resize(sz);
    x_.resize(sz);
    y_.resize(1);
    d->get_x(dx, sz, x_.data());
    y_[0] = 0;

    assign_(i0);

    // names etc
    name_ = d->name();
    desc_ = d->description();
    dim_name_.push_back(d->dim_name(dx));
    dim_desc_.push_back(d->dim_desc(dx));

    // build dim order
    dim_order_.clear();
    dim_order_.push_back(dx);
    for (int i = 0; i < d->dim_.size(); ++i) {
        if (i != dx)
            dim_order_.push_back(i);
    }
}

void DataSlice::assign(const DataStorePtr d, size_t dx, size_t dy, const dim_t &i0)
{
    clear();
    D_ = d;
    dim_idx_ = {dx, dy};

    dim_ = {d->dim()[dx], d->dim()[dy]};
    size_t sz = dim_[0] * dim_[1];
    data_.resize(sz);
    x_.resize(dim_[0]);
    y_.resize(dim_[1]);
    d->get_x(dx, dim_[0], x_.data());
    d->get_x(dy, dim_[1], y_.data());

    assign_(i0);

    // names etc
    name_ = d->name();
    desc_ = d->description();
    dim_name_.push_back(d->dim_name(dx));
    dim_desc_.push_back(d->dim_desc(dx));
    dim_name_.push_back(d->dim_name(dy));
    dim_desc_.push_back(d->dim_desc(dy));

    // build dim order
    dim_order_.clear();
    dim_order_.push_back(dx);
    dim_order_.push_back(dy);
    for (int i = 0; i < d->dim_.size(); ++i) {
        if (i != dx && i != dy)
            dim_order_.push_back(i);
    }
}

void DataSlice::assign(const DataStorePtr d, size_t dims)
{
    clear();
    D_ = d;
    if (!d || d->empty())
        return;
    if (dims < 1 || dims > 2)
        dims = 2;
    dims = std::min(dims, std::min(d->ndim(), 2UL));
    dims = std::max(dims, 1UL);

    // get dimensions
    dim_t dim = d->dim();
    dim_t i0(dim.size(), 0);

    // find max dim
    auto it = find_max(dim);
    size_t dx = it - dim.begin();

    if (dims == 1) {
        assign(d, dx, i0);
        return;
    }

    // find next max
    dim[dx] = 0;
    it = find_max(dim);
    size_t dy = it - dim.begin();
    assign(d, dx, dy, i0);
}

void DataSlice::assign(const dim_t &new_i0)
{
    assert(i0_.size() == new_i0.size());
    assign_(new_i0);
}

void DataSlice::update()
{
    assign_(i0_);
}

void DataSlice::export_csv(std::ostream &os)
{
    if (empty())
        return;

    if (ndim() == 1) {
        os << "x,y" << std::endl;
        for (size_t i = 0; i < dim_[0]; ++i)
            os << x_[i] << ", " << data_[i] << std::endl;
    } else {
        for (size_t i = 0; i < dim_[0]; ++i) {
            os << data_[i];
            for (size_t j = 1; j < dim_[1]; ++j) {
                os << ", " << data_[i + j * dim_[0]];
            }
            os << std::endl;
        }
    }
}

void DataSlice::assign_(const dim_t &new_i0)
{
    DataStorePtr d = D_.lock();
    if (!d) { // data pointer has been deleted
        clear();
        return;
    }
    i0_ = new_i0;
    if (ndim() == 1) {
        i0_[dx()] = 0;
        d->get_y(dx(), i0_, dim_[0], data_.data());
    } else {
        i0_[dx()] = 0;
        i0_[dy()] = 0;
        double *p = data_.data();
        dim_t j1(i0_);
        // copy row-by-row [column-major storage]
        for (size_t i = 0; i < dim_[1]; ++i) {
            j1[dy()] = i;
            d->get_y(dx(), j1, dim_[0], p);
            p += dim_[0];
        }
    }
}

size_t DataSlice::get_x(size_t d, size_t n, double *v) const
{
    const vec_t &z = (d) ? y_ : x_;
    for (size_t i = 0; i < n; ++i) {
        v[i] = z[i];
    }
    return n;
}
