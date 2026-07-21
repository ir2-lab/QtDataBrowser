#include "widget.h"

#include <QMenu>
#include <QMenuBar>
#include <QTimerEvent>
#include <QVBoxLayout>

#include "random_data.h"
#include "text.h"
#include "volume.h"
#include "wave.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("QDataBrowser Example");

    createMenu();
    QMenuBar *menuBar = new QMenuBar(this);
    menuBar->addMenu(dataMenu_);

    dataBrowser_ = new QDataBrowser(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMenuBar(menuBar);
    layout->addWidget(dataBrowser_);

    timer_id1 = startTimer(1000);
    timer_id2 = startTimer(2000);
}

void Widget::createMenu()
{
    dataMenu_ = new QMenu("Data", this);

    QAction *random1Action = dataMenu_->addAction("Random1");
    connect(random1Action, &QAction::triggered, this, &Widget::loadRandom1);

    QAction *random2Action = dataMenu_->addAction("Random2");
    connect(random2Action, &QAction::triggered, this, &Widget::loadRandom2);

    QAction *waveAction = dataMenu_->addAction("Wave");
    connect(waveAction, &QAction::triggered, this, &Widget::loadWave);

    QAction *textAction = dataMenu_->addAction("Text");
    connect(textAction, &QAction::triggered, this, &Widget::loadText);

    QAction *clearAction = dataMenu_->addAction("Clear");
    connect(clearAction, &QAction::triggered, this, &Widget::clear);
}

void Widget::loadRandom1()
{
    QDataModel *model = new QDataModel("Random1 Data");

    auto R1 = std::make_unique<random3d>("R1", 1, 2, 10);
    y1 = R1->data();
    auto R2 = std::make_unique<random3d>("R2", 5, 3, 25);
    y2 = R2->data();

    model->addGroup("RandomData");
    model->addGroup("3D", "/RandomData");
    model->addData(std::move(R1), "/RandomData/3D");
    model->addData(std::move(R2), "/RandomData/3D");

    model->addGroup("2D", "/RandomData");
    model->addData(std::make_unique<random2d>(), "/RandomData/2D");

    dataBrowser_->setModel(model);
}

void Widget::loadRandom2()
{
    QDataModel *model = new QDataModel("Random2 Data");

    auto R1 = std::make_unique<random3d>("R3", 1, 2, 10);
    y1 = R1->data();
    auto R2 = std::make_unique<random3d>("R2", 5, 3, 25);
    y2 = R2->data();

    model->addGroup("RandomData");
    model->addGroup("3D", "/RandomData");
    model->addData(std::move(R1), "/RandomData/3D");
    model->addData(std::move(R2), "/RandomData/3D");

    model->addGroup("2D", "/RandomData");
    model->addData(std::make_unique<random2d>(), "/RandomData/2D");

    dataBrowser_->setModel(model);
}

void Widget::loadWave()
{
    QDataModel *model = new QDataModel("Waves Data");
    model->addGroup("WaveData");
    model->addData(std::make_unique<wave1d>(), "/WaveData");
    auto R3 = std::make_unique<volume3d>(AbstractDataSet::vec_t{0.05, 0.1, 0.2});
    model->addGroup("VolumeData");
    model->addData(std::move(R3), "/VolumeData");
    dataBrowser_->setModel(model);
}

void Widget::loadText()
{
    QDataModel *model = new QDataModel("Text Data");
    model->addData(std::make_unique<text2d>(), "/TextData");
    model->addData(std::make_unique<text1d>(), "/TextData");
    dataBrowser_->setModel(model);
}

void Widget::clear()
{
    QDataModel *model = new QDataModel("");
    dataBrowser_->setModel(model);
}

void Widget::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == timer_id1) {
        if (y1) {
            random3d::randomize(y1.get());
            dataBrowser_->model()->setDatasetChanged();
        }
    } else if (e->timerId() == timer_id2) {
        if (y2) {
            random3d::randomize(y2.get());
            dataBrowser_->model()->setDatasetChanged();
        }
    }
}
