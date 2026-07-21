#ifndef QDATABROWSERTAB_H
#define QDATABROWSERTAB_H

#include <QTabWidget>

#include "qtdatabrowser_export.h"

class QToolButton;
class QDataBrowserPage;
class QStandardItem;
class QLabel;
class QTimer;
class QEvent;

class QDataBrowserTab : public QTabWidget
{
    Q_OBJECT

public:
    explicit QDataBrowserTab(QWidget *parent = nullptr);

    QDataBrowserPage *currentPage() const;
    QDataBrowserPage *page(int i) const;
    void setCurrentData(const QString &path, QStandardItem *i);
    void clear();

    QToolButton *leftPanelButton() const { return leftPanelBt; }

public slots:
    int addPage();
    int insertPage(int index);
    void removePage(int index);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QToolButton *leftPanelBt;
    QToolButton *btAddPage;

    // hover popup showing a tab's full path plus a copy-path button,
    // shown in place of the plain tab tooltip
    QWidget *pathPopup_{nullptr};
    QLabel *pathPopupLabel_{nullptr};
    QTimer *pathPopupPollTimer_{nullptr};
    int pathPopupTabIndex_{-1};

    void showPathPopup(int index);
    void hidePathPopup();
    void refreshTabBarLayout();

private slots:
    void onCopyPathClicked();
    void checkPathPopupHover();
};

#endif // QDATABROWSERTAB_H
