#ifndef QDATASLICESELECTOR_H
#define QDATASLICESELECTOR_H

#include "dataslice.h"

#include <QWidget>

class QComboBox;
class QLabel;
class QToolButton;
class QHBoxLayout;

class AxisValueSelect;
class FilterView;

class QDataSliceSelector : public QWidget
{
    Q_OBJECT

public:
    struct State
    {
        int dx = -1;
        int dy = -1;
        AbstractDataSet::dim_t i0;
    };

    explicit QDataSliceSelector(QWidget *parent = nullptr);

    void clear();
    void assign(DataSetPtr D, int dim = 1);
    void assign(DataSetPtr D, const State &prev);

    DataSlice *slice() { return &slice_; }
    void updateData();

    void setXLabel(const QString &text);
    void setYLabel(const QString &text);

    void getAxisValueLabels(int axisId, QStringList &labels);

    State state() const;

signals:
    void sliceReset();
    void sliceChanged();
    void sliceDataChanged();

protected:
    DataSlice slice_;
    static const int maxTicks = 15;

private:
    QLabel *lblX_{nullptr};
    QLabel *lblY_{nullptr};
    QComboBox *cbX_{nullptr};
    QComboBox *cbY_{nullptr};
    QToolButton *btExchangeXY_{nullptr};
    AxisValueSelect *dimSelect_{nullptr};
    QWidget *xy_{nullptr};

    void clearCtrls();
    void initCtrls();
    void finishAssign(DataSetPtr D);

    enum updFlag
    {
        All,
        XYex,
        SldrOnly
    };
    void updateCtrls(updFlag f);
    void connectCtrls();
    void disconnectCtrls();
    void blockCtrls(bool b);
    QString dimLabel(int d);

private slots:
    void onX(int new_dx);
    void onY(int new_dy);
    void onExchangeXY(bool);
    void onI0(int axisId, int v);
};

#endif // QDATASLICESELECTOR_H
