#include "qdatabrowser.h"

#include <QClipboard>
#include <QComboBox>
#include <QFileDialog>
#include <QFrame>
#include <QGuiApplication>
#include <QItemSelectionModel>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>

#include <fstream>

#include "qdataview.h"
#include "qdatasliceselector.h"

inline void __initResource__()
{
    Q_INIT_RESOURCE(qtdatabrowser_resources);
}

void QDataBrowser::initResources()
{
    __initResource__();
}

QDataBrowser::QDataBrowser(QWidget *parent)
    : QSplitter{parent}, lastLeftPanelPos(100)
{
    /* create data model */
    dataModel = new QStandardItemModel(0, 1, this);
    dataModel->setHeaderData(0, Qt::Horizontal, "Data Tables");

    /* create left-side tree widget */
    dataTree = new QTreeView;
    dataTree->setModel(dataModel);
    dataTree->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(dataTree->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            &QDataBrowser::onDataItemSelect);
    // dataTree->setStyleSheet("background: white"); // ivory, honeydew

    /* create 2nd splitter */
    bottomSplitter = new QSplitter(Qt::Vertical);

    /* create right hand widget */
    QWidget *rhw = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout;
    rhw->setLayout(vbox);
    vbox->setContentsMargins(6, 6, 6, 6);
    // vbox->setSpacing(0);
    bottomSplitter->addWidget(rhw);
    bottomSplitter->setCollapsible(0, false);

    /* create top toolbox */
    {
        QWidget *tlbox = new QWidget;
        // tlbox->setStyleSheet("background: LightGray"); // mintcream
        QHBoxLayout *hbox = new QHBoxLayout;
        tlbox->setLayout(hbox);
        hbox->setContentsMargins(0, 0, 0, 0);

        dataName = new QLabel("data > data");
        dataName->setStyleSheet("font-weight: bold");
        // dataName->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
        hbox->addWidget(dataName);

        copyPathBt = new QToolButton;
        copyPathBt->setIcon(QIcon(":/qdatabrowser/lucide/copy.svg"));
        copyPathBt->setToolTip("Copy path to clipboard");
        copyPathBt->setAutoRaise(true);
        connect(copyPathBt, &QToolButton::clicked, this, &QDataBrowser::onCopyPath);
        hbox->addWidget(copyPathBt);

        hbox->addStretch();

        btExport = new QToolButton;
        btExport->setIcon(QIcon(":/qdatabrowser/lucide/download.svg"));
        btExport->setText("Export");
        btExport->setPopupMode(QToolButton::InstantPopup);
        btExport->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        {
            QMenu *toolMenu = new QMenu;
            actExportCSV = toolMenu->addAction("Export data to CSV ...");
            actExportCSV->setEnabled(false);
            connect(actExportCSV, &QAction::triggered, this, &QDataBrowser::onExportCSV);
            actExportImg = toolMenu->addAction("Export plot to file ...");
            actExportImg->setEnabled(false);
            connect(actExportImg, &QAction::triggered, this, &QDataBrowser::onExportPlot);
            btExport->setMenu(toolMenu);
        }
        hbox->addWidget(btExport);

        QFrame *frm = new QFrame;
        frm->setFrameStyle(QFrame::VLine | QFrame::Sunken);
        hbox->addWidget(frm);

        leftPanelBt = new QToolButton;
        leftPanelBt->setIcon(QIcon(":/qdatabrowser/lucide/panel-left.svg"));
        leftPanelBt->setCheckable(true);
        leftPanelBt->setChecked(true);
        leftPanelBt->setToolTip("Hide left panel");
        connect(this, &QSplitter::splitterMoved, this, &QDataBrowser::onLeftSplitterMoved);
        connect(leftPanelBt, &QToolButton::clicked, this, &QDataBrowser::onLeftPanelBtClicked);
        hbox->addWidget(leftPanelBt);

        bottomPanelBt = new QToolButton;
        bottomPanelBt->setIcon(QIcon(":/qdatabrowser/lucide/panel-bottom.svg"));
        bottomPanelBt->setCheckable(true);
        bottomPanelBt->setChecked(true);
        bottomPanelBt->setToolTip("Hide bottom panel");
        connect(bottomSplitter,
                &QSplitter::splitterMoved,
                this,
                &QDataBrowser::onBottomSplitterMoved);
        connect(bottomPanelBt, &QToolButton::clicked, this, &QDataBrowser::onBottomPanelBtClicked);
        hbox->addWidget(bottomPanelBt);

        vbox->addWidget(tlbox);
    }

    /* create dataView panel */
    viewTab = new QTabWidget;
    viewTab->setStyleSheet("background: white");
    dataView[0] = new QTabularDataView;
    viewTab->addTab(dataView[0], dataView[0]->icon(), "Table");
    dataView[1] = new QPlotDataView;
    viewTab->addTab(dataView[1], dataView[1]->icon(), "Line");
    vbox->addWidget(viewTab);

    /* create bottom toolbox */
    {
        bottomPanel = new QStackedWidget;

        for (int i = 0; i < nViews; ++i)
        {
            sliceSelector[i] = new QDataSliceSelector;
            bottomPanel->addWidget(sliceSelector[i]);
            dataView[i]->setData(sliceSelector[i]->slice());
            connect(sliceSelector[i],
                    &QDataSliceSelector::sliceChanged,
                    dataView[i],
                    &QAbstractDataView::updateView);
            connect(sliceSelector[i],
                    &QDataSliceSelector::sliceChanged,
                    this,
                    &QDataBrowser::onSliceChanged);
            connect(dataView[i],
                    &QAbstractDataView::viewUpdated,
                    this,
                    &QDataBrowser::onViewUpdated);
        }

        connect(viewTab, &QTabWidget::currentChanged, bottomPanel, &QStackedWidget::setCurrentIndex);
        connect(viewTab, &QTabWidget::currentChanged, this, &QDataBrowser::onCurrentViewChanged);

        bottomSplitter->addWidget(bottomPanel);
    }

    addWidget(dataTree);
    addWidget(bottomSplitter);
    setCollapsible(1, false);
}

bool QDataBrowser::addGroup(const QString &name, const QString &location, const QString &desc)
{
    QStandardItem *parent = fromPath(location);
    if (!parent)
        return false;
    if (!isGroup(parent))
        return false;
    QStandardItem *g = new QStandardItem(QIcon(":/qdatabrowser/lucide/folder.svg"), name);
    AbstractDataStore *d{nullptr};
    g->setData(QVariant::fromValue(d));
    g->setSelectable(false);
    g->setEditable(false);
    g->setToolTip(desc.isEmpty() ? "Group" : desc);
    parent->appendRow(g);
    return true;
}

bool QDataBrowser::addData(AbstractDataStore *data, const QString &location)
{
    QStandardItem *parent = fromPath(location);
    if (!parent)
        return false;
    if (!isGroup(parent))
        return false;
    QStandardItem *g = new QStandardItem(QIcon(":/qdatabrowser/lucide/layers.svg"),
                                         data->name().c_str());
    g->setData(QVariant::fromValue(data));
    g->setEditable(false);
    g->setToolTip(data->description().empty() ? "Data array" : data->description().c_str());
    parent->appendRow(g);
    return true;
}

bool QDataBrowser::selectItem(const QString &path)
{
    QStandardItem *item = fromPath(path);
    if (!item)
        return false;
    QModelIndex i = item->index();
    bool ret = i.isValid();
    if (ret)
        dataTree->selectionModel()->setCurrentIndex(i, QItemSelectionModel::NoUpdate);
    return ret;
}

QStandardItem *QDataBrowser::fromPath(const QString &path) const
{
    if (path == "/" || path == "")
        return dataModel->invisibleRootItem();

    QStringList lst = path.split('/');
    if (path.startsWith('/'))
        lst.takeFirst();
    QStandardItem *i = dataModel->invisibleRootItem();
    for (const QString &s : lst)
    {
        QStandardItem *j = findChild(s, i);
        if (j == nullptr)
            return nullptr;
        else
            i = j;
    }
    return i;
}

QStandardItem *QDataBrowser::findChild(const QString &name, QStandardItem *parent) const
{
    for (int row = 0; row < parent->rowCount(); ++row)
    {
        QStandardItem *ch = parent->child(row);
        QString s = ch->text();
        if (s == name)
            return ch;
    }
    return nullptr;
}

bool QDataBrowser::isGroup(QStandardItem *i)
{
    if (i == nullptr)
        return false;
    return i == dataModel->invisibleRootItem() || i->data().value<AbstractDataStore *>() == nullptr;
}

QString QDataBrowser::itemPath(QStandardItem *i)
{
    if (i == nullptr || i == dataModel->invisibleRootItem())
        return QString();
    return QString("%1/%2").arg(itemPath(i->parent())).arg(i->text());
}

void QDataBrowser::onLeftSplitterMoved(int pos, int index)
{
    int leftSize = sizes().front();

    if (leftSize > 0)
    {
        leftPanelBt->setChecked(true);
        leftPanelBt->setToolTip("Hide left panel");
    }
    else
    {
        leftPanelBt->setChecked(false);
        leftPanelBt->setToolTip("Show left panel");
    }
    lastLeftPanelPos = qMax(leftSize, 50);
}

void QDataBrowser::onBottomSplitterMoved(int pos, int index)
{
    int bottomSize = bottomSplitter->sizes().back();

    if (bottomSize > 0)
    {
        bottomPanelBt->setChecked(true);
        bottomPanelBt->setToolTip("Hide bottom panel");
    }
    else
    {
        bottomPanelBt->setChecked(false);
        bottomPanelBt->setToolTip("Show bottom panel");
    }
    lastBottomPanelPos = qMax(bottomSize, 50);
}

void QDataBrowser::onLeftPanelBtClicked(bool c)
{
    QList<int> sz = sizes();
    if (c)
    {
        sz.front() = lastLeftPanelPos;
        sz.back() -= lastLeftPanelPos;
        setSizes(sz);
        leftPanelBt->setToolTip("Hide left panel");
    }
    else
    {
        lastLeftPanelPos = sz.front();
        sz.front() = 0;
        sz.back() += lastLeftPanelPos;
        setSizes(sz);
        leftPanelBt->setToolTip("Show left panel");
    }
}

void QDataBrowser::onBottomPanelBtClicked(bool c)
{
    QList<int> sz = bottomSplitter->sizes();
    if (c)
    {
        sz.back() = lastBottomPanelPos;
        sz.front() -= lastBottomPanelPos;
        bottomSplitter->setSizes(sz);
        bottomPanelBt->setToolTip("Hide bottom panel");
    }
    else
    {
        lastBottomPanelPos = sz.back();
        sz.back() = 0;
        sz.front() += lastBottomPanelPos;
        bottomSplitter->setSizes(sz);
        bottomPanelBt->setToolTip("Show bottom panel");
    }
}

void QDataBrowser::onDataItemSelect(const QModelIndex &selected, const QModelIndex &deselected)
{
    const int dim0[nViews] = {2, 1};
    QStandardItem *i = selected.isValid() ? dataModel->itemFromIndex(selected) : nullptr;
    if (i)
    {
        AbstractDataStore *D = i->data().value<AbstractDataStore *>();
        for (int i = 0; i < nViews; ++i)
        {
            sliceSelector[i]->assign(D, dim0[i]);
            dataView[i]->updateView();
        }
        dataName->setText(itemPath(i));
    }
    else
    {
        for (int i = 0; i < nViews; ++i)
        {
            sliceSelector[i]->clear();
            dataView[i]->updateView();
        }
        dataName->setText(QString());
    }
}

void QDataBrowser::onCopyPath()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    QString text = dataName->text();
    clipboard->setText(text);
}

void QDataBrowser::onSliceReset() {}

void QDataBrowser::onSliceChanged()
{
    int i = viewTab->currentIndex();
    bool ret = !sliceSelector[i]->slice()->empty();
    actExportCSV->setEnabled(ret);
    actExportImg->setEnabled(ret && dataView[i]->canExportImage());
}

void QDataBrowser::onExportCSV()
{
    int i = viewTab->currentIndex();
    if (sliceSelector[i]->slice()->empty())
        return;

    QString fname = QFileDialog::getSaveFileName(this,
                                                 tr("Export data to CSV ..."),
                                                 "opentrim_export.csv",
                                                 tr("CSV files [*.csv](*.csv);; All files (*.*)"));
    if (fname.isNull())
        return;

    // csv export
    std::ofstream of(fname.toStdString());

    if (!of.is_open()) {
        QMessageBox::critical(window(),
                              "Export data to CSV ...",
                              QString("Error opening file:\n%1").arg(fname));
        return;
    }

    sliceSelector[i]->slice()->export_csv(of);
}

void QDataBrowser::onExportPlot()
{
    int i = viewTab->currentIndex();
    if (!dataView[i]->canExportImage())
        return;
    dataView[i]->exportImage();
}

void QDataBrowser::onCurrentViewChanged(int i)
{
    bool ret = !sliceSelector[i]->slice()->empty();
    actExportCSV->setEnabled(ret);
    actExportImg->setEnabled(ret && dataView[i]->canExportImage());
}

void QDataBrowser::onViewUpdated()
{
    int i = viewTab->currentIndex();
    bool ret = !sliceSelector[i]->slice()->empty();
    actExportCSV->setEnabled(ret);
    actExportImg->setEnabled(ret && dataView[i]->canExportImage());
}
