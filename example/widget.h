#pragma once

#include <QDataBrowser>
#include <QWidget>

class QMenu;
class QTimer;

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);

private slots:
    void loadRandom1();
    void loadRandom2();
    void loadWave();
    void loadText();
    void clear();

protected:
    void timerEvent(QTimerEvent *e) override;

private:
    void createMenu();

    QDataBrowser *dataBrowser_;
    QMenu *dataMenu_;
    int timer_id1, timer_id2;
    std::shared_ptr<AbstractDataSet::vec_t> y1, y2;
};
