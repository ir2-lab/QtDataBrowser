#ifndef DATASLICE_H
#define DATASLICE_H

#include "abstractdatastore.h"

#include <QSharedPointer>

typedef QSharedPointer<AbstractDataStore> DataStorePtr;

class DataSlice : public AbstractDataStore
{
public:
    DataSlice() = default;

    const DataStorePtr dataStore() const { return D_; }

    const dim_t &i0() const { return i0_; }
    const dim_t &dim_order() const { return dim_order_; }
    const int dx() const { return dim_idx_[0]; }
    const int dy() const { return dim_idx_[1]; }

    const vec_t &x() const { return x_; }
    const vec_t &y() const { return y_; }
    const vec_t &data() const { return data_; }
    double x(int i) const { return x_[i]; }
    double y(int i) const { return y_[i]; }
    double operator()(size_t i) const { return data_[i]; }
    double operator()(size_t i, size_t j) const { return data_[i + j * dim_[0]]; }

    void clear();
    void assign(const DataStorePtr d, size_t dx, const dim_t &i0);
    void assign(const DataStorePtr d, size_t dx, size_t dy, const dim_t &i0);
    void assign(const DataStorePtr d, size_t dims = 2);
    void assign(const dim_t &new_i0);
    void update();

    void export_csv(std::ostream &os);

protected:
    dim_t dim_idx_;
    dim_t dim_order_;
    dim_t i0_;
    vec_t data_, x_, y_;
    QWeakPointer<AbstractDataStore> D_;

    void assign_(const dim_t &new_i0);

    size_t get_y(size_t d, const dim_t &i0, size_t n, double *v) const override { return 0; }
    size_t get_x(size_t d, size_t n, double *v) const override;
};

Q_DECLARE_METATYPE(DataStorePtr)

#endif // DATASLICE_H
