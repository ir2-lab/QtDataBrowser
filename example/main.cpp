#include "qdatabrowser.h"

#include <QApplication>
#include <QPointer>
#include <QStyleFactory>
#include <QTimer>
#include <QVBoxLayout>

#include <memory>
#include <random>

class random2d : public AbstractDataStore
{
    constexpr static const size_t N = 100;
    constexpr static const size_t M = 3;

public:
    random2d()
        : AbstractDataStore("random2d", {M, N}), y(M * N)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> u(0.0, 1.0);
        for (int i = 0; i < M * N; ++i)
        {
            y[i] = u(gen);
        }
        desc_ = "Random data array [3x100]";
    }

    size_t idx(const dim_t &i) const { return i[0] * dim_[1] + i[1]; }

    size_t get_y(size_t d, const dim_t &i0, size_t n, double *v) const override
    {
        dim_t i(i0);
        size_t k0 = idx(i);
        if (d == 0)
        {
            size_t m = std::min(n, size_t(M));
            const double *p = y.data() + k0;
            for (int i = 0; i < m; ++i, p += N)
                v[i] = *p;
        }
        else
        {
            size_t m = std::min(n, size_t(N));
            const double *p = y.data() + k0;
            std::copy(p, p + m, v);
        }
        return 0;
    }

protected:
    vec_t y;
};

class random3d : public AbstractDataStore
{
    constexpr static const size_t N = 100;
    constexpr static const size_t M = 3;
    constexpr static const size_t L = 10;

public:
    random3d()
        : AbstractDataStore("random3d", {L, M, N})
        , y(new vec_t(L * M * N))
        , dy(new vec_t(L * M * N))
    {
        randomize(y.get());
        randomize(dy.get(), 0.1);
        desc_ = "Random data array [10x3x100]";
        dim_name_[0] = "A";
        dim_name_[1] = "Type";
        dim_name_[2] = "X";
        dim_desc_[0] = "Few random numbers";
        dim_desc_[1] = "Categories";
        dim_desc_[2] = "Many random numbers";
    }

    static void randomize(vec_t *y, double s = 1.0)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> u(0.0, s);
        for (int i = 0; i < L * M * N; ++i) {
            (*y)[i] = u(gen);
        }
    }

    bool hasErrors() const override { return true; }
    bool is_x_categorical(size_t d) const override { return d == 1; }
    size_t get_x_categorical(size_t d, strvec_t &categories) const override
    {
        categories[0] = "First";
        categories[1] = "Second";
        categories[2] = "Third";
        return 3;
    }

    size_t idx(const dim_t &i) const { return (i[0] * dim_[1] + i[1]) * dim_[2] + i[2]; }

    std::shared_ptr<vec_t> data() const { return y; }

protected:
    std::shared_ptr<vec_t> y;
    std::shared_ptr<vec_t> dy;

    size_t _get_(size_t d, const dim_t &i0, const vec_t *y, size_t n, double *v) const
    {
        dim_t i(i0);
        size_t k0 = idx(i);
        if (d == 0) {
            size_t m = std::min(n, size_t(L));
            const double *p = y->data() + k0;
            for (int i = 0; i < m; ++i, p += M * N)
                v[i] = *p;
        } else if (d == 1) {
            size_t m = std::min(n, size_t(M));
            const double *p = y->data() + k0;
            for (int i = 0; i < m; ++i, p += N)
                v[i] = *p;
        } else {
            size_t m = std::min(n, size_t(N));
            const double *p = y->data() + k0;
            std::copy(p, p + m, v);
        }
        return 0;
    }
    size_t get_y(size_t d, const dim_t &i0, size_t n, double *v) const override
    {
        return _get_(d, i0, y.get(), n, v);
    }
    size_t get_dy(size_t d, const dim_t &i0, size_t n, double *v) const override
    {
        return _get_(d, i0, dy.get(), n, v);
    }
};

class wave1d : public AbstractDataStore
{
    constexpr static const size_t N = 1000;

public:
    wave1d()
        : AbstractDataStore("wave1d", {N}), y(N)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> u(-0.1, 0.1);
        for (int i = 0; i < N; ++i)
        {
            y[i] = std::sin(1.0 * i / N * 7 * M_PI) + u(gen);
        }
        desc_ = "Sine wave plus noise";
    }

    size_t get_y(size_t d, const dim_t &i0, size_t n, double *v) const override
    {
        size_t m = std::min(n, size_t(N));
        const double *p = y.data();
        std::copy(p, p + m, v);
        return m;
    }

protected:
    vec_t y;
};

class MyTimer : public QTimer
{
public:
    MyTimer(QDataBrowser *w, std::shared_ptr<AbstractDataStore::vec_t> y)
        : w_(w)
        , y_(y)
    {}

protected:
    void timerEvent(QTimerEvent *) override
    {
        if (y_)
            random3d::randomize(y_.get());
        if (w_)
            w_->dataUpdated("/RandomData/3D/random3d");
    }

    std::shared_ptr<AbstractDataStore::vec_t> y_;
    QPointer<QDataBrowser> w_;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QDataBrowser::initResources();

    // select fusion style if available
    // for consistent look among different platforms
    QStringList style_keys = QStyleFactory::keys();
    if (style_keys.contains("Fusion", Qt::CaseInsensitive))
    {
        QStringList l = style_keys.filter("Fusion", Qt::CaseInsensitive);
        QApplication::setStyle(l.front());
        //QApplication::setStyle("Adwaita");
        //QApplication::setStyle("Adwaita-Dark");
    }

    QWidget win;
    win.setLocale(QLocale::c());
    QVBoxLayout *vbox = new QVBoxLayout;
    win.setLayout(vbox);
    QFrame *frm = new QFrame;
    QVBoxLayout *vbox1 = new QVBoxLayout;
    frm->setLayout(vbox1);
    vbox1->setContentsMargins(0, 0, 0, 0);
    frm->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    QDataBrowser *w = new QDataBrowser;
    vbox1->addWidget(w);
    vbox->addWidget(frm);
    win.show();

    random3d *r3d = new random3d();
    std::shared_ptr<AbstractDataStore::vec_t> y = r3d->data();

    w->addGroup("RandomData", "/", "Various random data arrays");
    w->addGroup("2D", "/RandomData");
    w->addData(new random2d(), "/RandomData/2D");
    w->addGroup("3D", "/RandomData");
    w->addData(r3d, "/RandomData/3D");
    w->addData(new wave1d(), "RandomData");

    w->selectItem("/RandomData/3D/random3d");
    w->setActiveView(QDataBrowser::Plot);
    w->setPlotType(QDataBrowser::ErrorBar);

    MyTimer timer(w, y);
    timer.start(1000);

    //auto f1 = [w] { w->clear("/RandomData/2D"); };
    //QTimer::singleShot(10000, w, f1);

    return app.exec();
}
