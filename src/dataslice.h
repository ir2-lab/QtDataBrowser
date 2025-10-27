#ifndef DATASLICE_H
#define DATASLICE_H

#include "abstractdatastore.h"

#include <QSharedPointer>
#include <cstring>

typedef QSharedPointer<AbstractDataStore> DataStorePtr;

class DataSlice : public AbstractDataStore
{
public:
    DataSlice() = default;

    const DataStorePtr dataStore() const { return D_; }

    bool is_numeric() const override { return !data_.empty(); }
    bool hasErrors() const override { return !err_.empty(); }
    bool is_x_categorical(size_t d) const override
    {
        if (d == 0)
            return !x_category_.empty();
        if (d == 1)
            return !y_category_.empty();
        return false;
    }

    size_t get_x_categorical(size_t d, strvec_t &categories) const override
    {
        categories = (d == 0) ? x_category_ : y_category_;
        return categories.size();
    }

    const dim_t &i0() const { return i0_; }
    const dim_t &dim_order() const { return dim_order_; }
    int dx() const { return dim_idx_[0]; }
    int dy() const { return dim_idx_[1]; }

    const vec_t &x() const { return x_; }
    const vec_t &y() const { return y_; }
    const strvec_t &x_category() const { return x_category_; }
    const strvec_t &y_category() const { return y_category_; }
    const vec_t &data() const { return data_; }
    const vec_t &errors() const { return err_; }
    double x(int i) const { return x_[i]; }
    double y(int i) const { return y_[i]; }
    double operator()(size_t i) const { return data_[i]; }
    double operator()(size_t i, size_t j) const { return data_[i + j * dim_[0]]; }
    const std::string &text_data(size_t i, size_t j) const { return txtdata_[i + j * dim_[0]]; }

    void clear();
    void assign(const DataStorePtr d, size_t dx, const dim_t &i0);
    void assign(const DataStorePtr d, size_t dx, size_t dy, const dim_t &i0);
    void assign(const DataStorePtr d, size_t dims = 2);
    void assign(const dim_t &new_i0);
    void update();

    void export_csv(std::ostream &os);

protected:
    dim_t dim_idx_;                    // slice x & y dimensions
    dim_t dim_order_;                  // order of D_ dimensions
    dim_t i0_;                         // offset into D_
    vec_t data_, err_, x_, y_;         // slice data
    strvec_t txtdata_;                 // text data
    strvec_t x_category_, y_category_; // category data for x & y
    QWeakPointer<AbstractDataStore> D_;

    size_t _get_(size_t d, const dim_t &i0, const vec_t &yy, size_t n, double *v) const;

    size_t get_y(size_t d, const dim_t &i0, size_t n, double *v) const override
    {
        return _get_(d, i0, data_, n, v);
    }
    size_t get_dy(size_t d, const dim_t &i0, size_t n, double *v) const override
    {
        return _get_(d, i0, err_, n, v);
    }
    virtual size_t get_x(size_t d, size_t n, double *v) const override;

    void assign_(const dim_t &new_i0);
};

Q_DECLARE_METATYPE(DataStorePtr)

#endif // DATASLICE_H
