#include "qdatabrowserpage.h"

#include "dataslice.h"
#include "qdatasliceselector.h"
#include "qdataview.h"

#include <QAction>
#include <QFileDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include <fstream>

QDataBrowserPage::QDataBrowserPage(QWidget *parent)
    : QWidget{parent}
{
    QVBoxLayout *vbox = new QVBoxLayout;
    setLayout(vbox);
    int spc = vbox->spacing();
    vbox->setSpacing(2 * spc);
    vbox->setContentsMargins(0, 6, 0, 0);

    sliceSelector = new QDataSliceSelector;
    vbox->addWidget(sliceSelector);
    connect(sliceSelector,
            &QDataSliceSelector::sliceChanged,
            this,
            &QDataBrowserPage::onSliceChanged);

    /* create Tab widget with 3 dataView panels */
    viewTab = new QTabWidget;

    QStringList TabLabels{QStringLiteral("Table"),
                          QStringLiteral("LinePlot"),
                          QStringLiteral("HeatMap")};

    // Create data view widgets
    dataView[0] = new QTabularDataView;
    dataView[1] = new QPlotDataView;
    dataView[2] = new QHeatMapDataView;

    // Create Tabs
    for (int i = 0; i < nViews; ++i) {
        dataView[i]->setSliceSelector(sliceSelector);
        viewTab->addTab(dataView[i], dataView[i]->icon(), TabLabels.at(i));
        connect(dataView[i],
                &QAbstractDataView::viewUpdated,
                this,
                &QDataBrowserPage::onViewUpdated);
    }

    /* top right tab-widget toolbox */
    {
        QWidget *tlbox = new QWidget;
        QHBoxLayout *hbox = new QHBoxLayout;
        tlbox->setLayout(hbox);
        hbox->setContentsMargins(0, 1, 0, 1);

        optionsBt = new QToolButton;
        optionsBt->setIcon(QIcon(":/qdatabrowser/icons/lucide/settings-2.svg"));
        optionsBt->setPopupMode(QToolButton::InstantPopup);
        optionsBt->setToolTip("View options menu");
        hbox->addWidget(optionsBt);

        btExport = new QToolButton;
        btExport->setIcon(QIcon(":/qdatabrowser/icons/lucide/download.svg"));
        btExport->setText("Export");
        btExport->setToolTip("Export data/view");
        btExport->setPopupMode(QToolButton::InstantPopup);
        btExport->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        {
            QMenu *toolMenu = new QMenu(this);
            actExportCSV = toolMenu->addAction("Export data to CSV ...");
            actExportCSV->setEnabled(false);
            connect(actExportCSV, &QAction::triggered, this, &QDataBrowserPage::onExportCSV);
            actExportImg = toolMenu->addAction("Export plot to file ...");
            actExportImg->setEnabled(false);
            connect(actExportImg, &QAction::triggered, this, &QDataBrowserPage::onExportPlot);
            btExport->setMenu(toolMenu);
        }
        hbox->addWidget(btExport);
        viewTab->setCornerWidget(tlbox);
    }

    vbox->addWidget(viewTab);

    connect(viewTab, &QTabWidget::currentChanged, this, &QDataBrowserPage::onCurrentViewChanged);

    clear();
}

QDataBrowser::PlotType QDataBrowserPage::plotType() const
{
    return ((QPlotDataView *) dataView[1])->plotType();
}

QDataBrowser::ViewType QDataBrowserPage::activeView() const
{
    return QDataBrowser::ViewType(viewTab->currentIndex());
}

void QDataBrowserPage::clear()
{
    // store state only on valid data
    if (!dataPath_.isEmpty() && sliceSelector->slice()->dataStore())
        savedState_ = state();

    if (item_)
        disconnect(item_->model(),
                   &QStandardItemModel::itemChanged,
                   this,
                   &QDataBrowserPage::onItemChanged);

    dataPath_.clear();
    item_ = nullptr;
    sliceSelector->clear();
    for (int i = 0; i < nViews; ++i) {
        dataView[i]->updateView();
        viewTab->setTabVisible(i, false);
    }
}

void QDataBrowserPage::updateData()
{
    sliceSelector->updateData();
}

void QDataBrowserPage::setData(const QString &path, QStandardItem *i)
{
    clear();
    if (path.isEmpty())
        return;

    dataPath_ = path;
    item_ = i;
    connect(i->model(), &QStandardItemModel::itemChanged, this, &QDataBrowserPage::onItemChanged);
    DataSetPtr D = i->data().value<DataSetPtr>();

    bool hasSaved = !savedState_.path.isEmpty();

    if (hasSaved) {
        sliceSelector->assign(D, savedState_.sliceState);
        static_cast<QTabularDataView *>(dataView[0])->setState(savedState_.tableState);
        static_cast<QPlotDataView *>(dataView[1])->setState(savedState_.plotState);
        static_cast<QHeatMapDataView *>(dataView[2])->setState(savedState_.heatMapState);
    } else {
        sliceSelector->assign(D, 2);
    }

    if (D) {
        if (D->is_numeric()) {
            for (int i = 0; i < nViews; ++i) {
                viewTab->setTabVisible(i, true);
                dataView[i]->updateView();
            }
            if (hasSaved)
                setActiveView(savedState_.activeView);
        } else {
            viewTab->setTabVisible(0, true);
            dataView[0]->updateView();
            setActiveView(QDataBrowser::Table);
        }
    }
}

QDataBrowserPage::State QDataBrowserPage::state() const
{
    State s;
    s.path = dataPath_;
    s.sliceState = sliceSelector->state();
    s.activeView = activeView();
    s.tableState = static_cast<QTabularDataView *>(dataView[0])->state();
    s.plotState = static_cast<QPlotDataView *>(dataView[1])->state();
    s.heatMapState = static_cast<QHeatMapDataView *>(dataView[2])->state();
    return s;
}

QDataBrowserPage *QDataBrowserPage::clone(QWidget *parent) const
{
    QDataBrowserPage *p = new QDataBrowserPage(parent);
    p->setData(dataPath(), item_);
    p->setPlotType(plotType());
    p->setActiveView(activeView());
    return p;
}

void QDataBrowserPage::setPlotType(QDataBrowser::PlotType t)
{
    ((QPlotDataView *) dataView[1])->setPlotType(t);
}

void QDataBrowserPage::setActiveView(QDataBrowser::ViewType t)
{
    viewTab->setCurrentIndex(t);
}

void QDataBrowserPage::onItemChanged(QStandardItem *i)
{
    if (i == item_)
        sliceSelector->updateData();
}

void QDataBrowserPage::onSliceReset() {}

void QDataBrowserPage::onSliceChanged()
{
    int i = viewTab->currentIndex();
    bool ret = !dataView[i]->sliceSelector()->slice()->empty();
    actExportCSV->setEnabled(ret);
    actExportImg->setEnabled(ret && dataView[i]->canExportImage());
}

void QDataBrowserPage::onExportCSV()
{
    int i = viewTab->currentIndex();
    if (dataView[i]->sliceSelector()->slice()->empty())
        return;

    QString fname = QFileDialog::getSaveFileName(this,
                                                 tr("Export data to CSV ..."),
                                                 "export.csv",
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

    dataView[i]->sliceSelector()->slice()->export_csv(of);
}

void QDataBrowserPage::onExportPlot()
{
    int i = viewTab->currentIndex();
    if (!dataView[i]->canExportImage())
        return;
    dataView[i]->exportImage();
}

void QDataBrowserPage::onCurrentViewChanged(int i)
{
    bool ret = !dataView[i]->sliceSelector()->slice()->empty();
    actExportCSV->setEnabled(ret);
    actExportImg->setEnabled(ret && dataView[i]->canExportImage());
    optionsBt->setMenu(dataView[i]->optionsMenu());
}

void QDataBrowserPage::onViewUpdated()
{
    int i = viewTab->currentIndex();
    bool ret = !dataView[i]->sliceSelector()->slice()->empty();
    actExportCSV->setEnabled(ret);
    actExportImg->setEnabled(ret && dataView[i]->canExportImage());
}
