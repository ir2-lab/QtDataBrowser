#ifndef QDATASLICESELECTOR_H
#define QDATASLICESELECTOR_H

#include "abstractdatastore.h"

#include <iostream>

#include <QWidget>

class QButtonGroup;
class QComboBox;
class QGridLayout;
class QLabel;
class QSlider;
class QLineEdit;
class QToolButton;

class DataSlice : public AbstractDataStore
{
public:
    DataSlice()
        : D_(nullptr)
    {
        dim_ = {0, 0};
    }

    const AbstractDataStore *dataStore() const { return D_; }
    size_t ndim() const
    {
        if (size())
            return dim_[1] == 1 ? 1 : 2;

        return 0;
    }
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
    void assign(const AbstractDataStore *d, size_t dx, const dim_t &i0);
    void assign(const AbstractDataStore *d, size_t dx, size_t dy, const dim_t &i0);
    void assign(const AbstractDataStore *d, size_t dims = 2);
    void assign(const dim_t &new_i0);

    void export_csv(std::ostream &os);

protected:
    dim_t dim_idx_;
    dim_t dim_order_;
    dim_t i0_;
    vec_t data_, x_, y_;
    const AbstractDataStore *D_;

    void assign_(const dim_t &new_i0);

    size_t get_y(size_t d, const dim_t &i0, size_t n, double *v) const override;
    size_t get_x(size_t d, size_t n, double *v) const override;
};

class QDataSliceSelector : public QWidget
{
    Q_OBJECT

public:
    explicit QDataSliceSelector(QWidget *parent = nullptr);

    void clear();
    void assign(AbstractDataStore *D, int dim = 1);

    DataSlice *slice() { return &slice_; }

signals:
    void sliceReset();
    void sliceChanged();

protected:
    // data
    DataSlice slice_;

    // controls
    QComboBox *cbX;
    QComboBox *cbY;
    QToolButton *btExchangeXY;

    // grid of dims
    static const int maxTicks = 15;
    QWidget *gridPanel;
    QGridLayout *grid;
    struct gridElement
    {
        int d;
        QLabel *label;
        QSlider *slider;
        QLineEdit *value;
    };
    QVector<gridElement> gridElements;
    void clearCtrls();
    void initCtrls();

    enum updFlag { All, XYex, SldrOnly };

    void updateCtrls(updFlag f);

    void connectCtrls();
    void disconnectCtrls();
    void blockCtrls(bool b);
    QString dimLabel(int d);

protected slots:
    void onX(int new_dx);
    void onY(int d);
    void onExchangeXY(bool);
    void onI0(int v);
};

#endif // QDATASLICESELECTOR_H
