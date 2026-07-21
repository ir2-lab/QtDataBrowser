// LegendView.h
//
// A persistent side-panel showing the Y-series legend for a line plot.
// Mirrors the FilterView selection controls (search, tri-state select-all,
// checkable list with colored icons) but lives inline instead of as a popup.
// Emits selectionChanged whenever the user toggles an item, so the host view
// can re-render immediately.

#pragma once

#include <QIcon>
#include <QVector>
#include <QWidget>

class QCheckBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QToolButton;

class LegendView : public QWidget
{
    Q_OBJECT

public:
    explicit LegendView(QWidget *parent = nullptr);

    // Replace the list contents; all items start checked.
    void setValues(const QStringList &values, const QVector<uint> &checked);

    // Update the color-coded line icons used in the list.
    void setColorOrder(const QVector<QRgb> &v);

    // Indices of currently checked items.
    QVector<uint> checkedValues() const;
    void setCheckedValues(const QVector<uint> &v);

    // Number of items in the list.
    int count() const;

    // Widget width when the panel content is collapsed (button only).
    int collapsedWidth() const;
    bool isCollapsed() const;
    void setCollapsed(bool collapsed);

signals:
    void selectionChanged(const QVector<uint> &values);
    void panelToggled(bool open);

private slots:
    void applySearch(const QString &text);
    void onSelectAllChanged(int state);
    void onItemChanged(QListWidgetItem *item);
    void onPanelToggled(bool open);

private:
    void updateSelectAllState();

    QToolButton *panelButton = nullptr;
    QLabel *titleLabel = nullptr;
    QWidget *contentWidget = nullptr;

    QLineEdit *searchEdit = nullptr;
    QCheckBox *selectAllBox = nullptr;
    QListWidget *listWidget = nullptr;

    QVector<QRgb> colorOrder;
    QVector<QIcon> iconOrder;
    bool updating = false;
    bool collapsed_ = false;
};
