#include "qdatabrowsertab.h"

#include "qdatabrowserpage.h"

#include <QClipboard>
#include <QCursor>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHelpEvent>
#include <QIcon>
#include <QLabel>
#include <QTabBar>
#include <QTimer>
#include <QToolButton>

namespace {

// reserves enough width for a tab's bold (selected) label, so the tab
// doesn't need to resize - and doesn't clip its text - when it becomes
// selected and its label switches to bold
class BoldAwareTabBar : public QTabBar
{
public:
    using QTabBar::QTabBar;

protected:
    QSize tabSizeHint(int index) const override
    {
        QSize hint = QTabBar::tabSizeHint(index);
        QFont boldFont = font();
        boldFont.setBold(true);
        const int extra = QFontMetrics(boldFont).horizontalAdvance(tabText(index))
            - QFontMetrics(font()).horizontalAdvance(tabText(index));
        if (extra > 0)
            hint.rwidth() += extra;
        return hint;
    }
};

} // namespace

QDataBrowserTab::QDataBrowserTab(QWidget *parent)
    : QTabWidget{parent}
{
    setTabBar(new BoldAwareTabBar(this));

    setDocumentMode(true);
    // active tab's label in bold
    setStyleSheet("QTabBar::tab:selected { font-weight: bold; }");

    setTabsClosable(false);
    connect(this, &QTabWidget::tabCloseRequested, this, &QDataBrowserTab::removePage);

    // keep the trailing part of long labels (the data name) visible
    tabBar()->setElideMode(Qt::ElideLeft);
    tabBar()->installEventFilter(this);
    tabBar()->setMovable(true);

    /* create top left toolbox */
    {
        QWidget *tlbox = new QWidget;
        QHBoxLayout *hbox = new QHBoxLayout;
        tlbox->setLayout(hbox);
        hbox->setContentsMargins(0, 0, 4, 4);

        leftPanelBt = new QToolButton;
        leftPanelBt->setIcon(QIcon(":/qdatabrowser/icons/lucide/panel-left.svg"));
        leftPanelBt->setCheckable(true);
        leftPanelBt->setChecked(true);
        leftPanelBt->setToolTip("Hide left panel");
        leftPanelBt->setAutoRaise(true);
        hbox->addWidget(leftPanelBt);

        setCornerWidget(tlbox, Qt::TopLeftCorner);
    }

    /* create top right toolbox */
    {
        QWidget *tlbox = new QWidget;
        QHBoxLayout *hbox = new QHBoxLayout;
        tlbox->setLayout(hbox);
        hbox->setContentsMargins(4, 0, 0, 4);

        btAddPage = new QToolButton;
        btAddPage->setIcon(QIcon(":/qdatabrowser/icons/lucide/plus.svg"));
        btAddPage->setToolTip("Add tab");
        btAddPage->setAutoRaise(true);
        connect(btAddPage, &QToolButton::clicked, this, &QDataBrowserTab::addPage);
        hbox->addWidget(btAddPage);

        setCornerWidget(tlbox, Qt::TopRightCorner);
    }

    addTab(new QDataBrowserPage, "");
}

QDataBrowserPage *QDataBrowserTab::currentPage() const
{
    return qobject_cast<QDataBrowserPage *>(currentWidget());
}

QDataBrowserPage *QDataBrowserTab::page(int i) const
{
    return qobject_cast<QDataBrowserPage *>(widget(i));
}

void QDataBrowserTab::setCurrentData(const QString &path, QStandardItem *i)
{
    int k = currentIndex();
    setTabText(k, path);
    setTabToolTip(k, path);
    currentPage()->setData(path, i);
}

void QDataBrowserTab::clear()
{
    for (int i = 0; i < count(); ++i) {
        QDataBrowserPage *w = qobject_cast<QDataBrowserPage *>(widget(i));
        w->clear();
    }
}

int QDataBrowserTab::addPage()
{
    QDataBrowserPage *cur = currentPage();
    QDataBrowserPage *page = cur ? cur->clone(this) : new QDataBrowserPage(this);
    int idx = addTab(page, cur ? tabText(currentIndex()) : QString());
    setTabToolTip(idx, cur ? tabToolTip(currentIndex()) : QString());
    setCurrentIndex(idx);
    setTabsClosable(true);
    refreshTabBarLayout();
    return idx;
}

int QDataBrowserTab::insertPage(int index)
{
    QDataBrowserPage *cur = currentPage();
    QDataBrowserPage *page = cur ? cur->clone(this) : new QDataBrowserPage(this);
    int idx = insertTab(index, page, cur ? tabText(currentIndex()) : QString());
    setTabToolTip(idx, cur ? tabToolTip(currentIndex()) : QString());
    setCurrentIndex(idx);
    setTabsClosable(true);
    refreshTabBarLayout();
    return idx;
}

void QDataBrowserTab::removePage(int index)
{
    if (index < 0 || index >= count() || count() == 1)
        return;

    hidePathPopup();
    QWidget *w = widget(index);
    removeTab(index);
    delete w;
    if (count() <= 1)
        setTabsClosable(false);
    refreshTabBarLayout();
}

void QDataBrowserTab::refreshTabBarLayout()
{
    tabBar()->updateGeometry();
    tabBar()->update();
}

bool QDataBrowserTab::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == tabBar() && event->type() == QEvent::ToolTip)
    {
        QHelpEvent *he = static_cast<QHelpEvent *>(event);
        int index = tabBar()->tabAt(he->pos());
        if (index >= 0 && !tabToolTip(index).isEmpty())
            showPathPopup(index);
        else
            hidePathPopup();
        return true;
    }
    return QTabWidget::eventFilter(obj, event);
}

void QDataBrowserTab::showPathPopup(int index)
{
    const QString path = tabToolTip(index);

    if (!pathPopup_)
    {
        pathPopup_ = new QWidget(this, Qt::ToolTip | Qt::FramelessWindowHint);
        pathPopup_->setAttribute(Qt::WA_ShowWithoutActivating);
        pathPopup_->setObjectName("pathPopup");
        pathPopup_->setStyleSheet(
            "QWidget#pathPopup { background-color: palette(base); border: 1px solid black; }");

        QHBoxLayout *hbox = new QHBoxLayout(pathPopup_);
        hbox->setContentsMargins(6, 4, 6, 4);
        hbox->setSpacing(6);

        pathPopupLabel_ = new QLabel(pathPopup_);
        hbox->addWidget(pathPopupLabel_);

        QToolButton *btCopy = new QToolButton(pathPopup_);
        btCopy->setIcon(QIcon(":/qdatabrowser/icons/lucide/copy.svg"));
        btCopy->setToolTip("Copy path");
        btCopy->setAutoRaise(true);
        connect(btCopy, &QToolButton::clicked, this, &QDataBrowserTab::onCopyPathClicked);
        hbox->addWidget(btCopy);

        pathPopupPollTimer_ = new QTimer(this);
        pathPopupPollTimer_->setInterval(200);
        connect(pathPopupPollTimer_,
                &QTimer::timeout,
                this,
                &QDataBrowserTab::checkPathPopupHover);
    }

    pathPopupTabIndex_ = index;
    pathPopupLabel_->setText(path);
    pathPopup_->adjustSize();
    pathPopup_->move(QCursor::pos() + QPoint(0, 4));
    pathPopup_->show();
    pathPopupPollTimer_->start();
}

void QDataBrowserTab::hidePathPopup()
{
    if (pathPopupPollTimer_)
        pathPopupPollTimer_->stop();
    if (pathPopup_)
        pathPopup_->hide();
    pathPopupTabIndex_ = -1;
}

void QDataBrowserTab::checkPathPopupHover()
{
    if (!pathPopup_ || !pathPopup_->isVisible())
        return;

    const QPoint cursor = QCursor::pos();

    // don't let our popup linger on top of the tab's own close-button tooltip
    if (pathPopupTabIndex_ >= 0)
    {
        QWidget *closeBt = tabBar()->tabButton(pathPopupTabIndex_, QTabBar::RightSide);
        if (!closeBt)
            closeBt = tabBar()->tabButton(pathPopupTabIndex_, QTabBar::LeftSide);
        if (closeBt)
        {
            const QRect closeRectGlobal(closeBt->mapToGlobal(QPoint(0, 0)), closeBt->size());
            if (closeRectGlobal.contains(cursor))
            {
                hidePathPopup();
                return;
            }
        }
    }

    // widen the hit area upward a bit to cover the small gap between the
    // cursor and the popup's top edge (it's spawned just below the cursor)
    const QRect hoverRect = pathPopup_->geometry().adjusted(0, -8, 0, 0);
    if (!hoverRect.contains(cursor))
        hidePathPopup();
}

void QDataBrowserTab::onCopyPathClicked()
{
    if (pathPopupTabIndex_ >= 0)
        QGuiApplication::clipboard()->setText(tabToolTip(pathPopupTabIndex_));
    hidePathPopup();
}
