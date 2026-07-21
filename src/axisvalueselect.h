// AxisValueSelect.h
//
// A single-row axis-slider control:
//
//   [axis dropdown] [«] [<] [──── slider ────] [>] [»] [value display]
//
// The user picks which axis to navigate via the dropdown. Each axis tracks
// its own slider position independently. The widget hides itself when empty.
//
// Call insertAxis / removeAxis to manage axes. clearAxes() resets everything.

#pragma once

#include <QWidget>
#include <QStringList>
#include <QSlider>

class QToolButton;
class QLabel;
class QLineEdit;
class QComboBox;

class HSliderWithButtons;

class AxisValueSelect : public QWidget
{
    Q_OBJECT

public:
    explicit AxisValueSelect(QWidget *parent = nullptr);

    // Add or update an axis. id is caller-assigned (e.g. a dimension index).
    // Preserves the current slider position when updating an existing axis.
    void insertAxis(int id, const QString &name, const QStringList &values, int page);

    // Remove the axis with the given id. Hides the widget when no axes remain.
    void removeAxis(int id);

    // Remove all axes and hide the widget.
    void clearAxes();

    int axisCount() const;

    // Current slider position for the axis with the given id. Returns -1 if not found.
    int currentIndex(int id) const;

    // set the index for axis id
    void setValue(int id, int index);

signals:
    void valueChanged(int id, int index);

private:
    void loadAxis(int comboIndex);
    void updateDisplay();
    int axisIndexById(int id) const;

    struct AxisData {
        int id = 0;
        QString name;
        QStringList values;
        int pageStep = 1;
        int currentIndex = 0;
    };

    QList<AxisData> m_axes;
    QComboBox *m_combo = nullptr;
    HSliderWithButtons *m_sliderBar = nullptr;
    QLineEdit *m_valueLabel = nullptr;
    bool m_loading = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// HSliderWithButtons
//
// A horizontal QSlider flanked by four QToolButtons:
//
//   [chevrons-left] [chevron-left] [────── slider ──────] [chevron-right] [chevrons-right]
//        pageStep−        step−                                  step+           pageStep+
//
// All QSlider signals are forwarded. All QSlider slots are exposed so callers
// can treat this widget as a drop-in wherever a QSlider would be used.

class HSliderWithButtons : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(int singleStep READ singleStep WRITE setSingleStep)
    Q_PROPERTY(int pageStep READ pageStep WRITE setPageStep)

public:
    explicit HSliderWithButtons(QWidget *parent = nullptr);

    int minimum() const;
    int maximum() const;
    int value() const;
    int singleStep() const;
    int pageStep() const;

    void setMinimum(int min);
    void setMaximum(int max);
    void setRange(int min, int max);
    void setSingleStep(int step);
    void setPageStep(int step);

    void setTickPosition(QSlider::TickPosition pos);
    void setTickInterval(int ti);

    QSlider *slider() const;

signals:
    void valueChanged(int value);
    void sliderPressed();
    void sliderMoved(int value);
    void sliderReleased();
    void rangeChanged(int min, int max);
    void actionTriggered(int action);

public slots:
    void setValue(int value);

private:
    QSlider *m_slider = nullptr;
    QToolButton *m_prevPage = nullptr;
    QToolButton *m_prevStep = nullptr;
    QToolButton *m_nextStep = nullptr;
    QToolButton *m_nextPage = nullptr;

    void page(bool up);
};
