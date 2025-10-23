#ifndef QDATASLICESELECTOR_H
#define QDATASLICESELECTOR_H

#include "dataslice.h"

#include <QWidget>

class QButtonGroup;
class QComboBox;
class QGridLayout;
class QLabel;
class QSlider;
class QLineEdit;
class QToolButton;

class QDataSliceSelector : public QWidget
{
    Q_OBJECT

public:
    explicit QDataSliceSelector(QWidget *parent = nullptr);

    void clear();
    void assign(DataStorePtr D, int dim = 1);

    DataSlice *slice() { return &slice_; }
    void updateData();

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
