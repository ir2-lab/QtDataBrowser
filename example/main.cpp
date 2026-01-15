#include <QDataBrowser>

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
        size_t m;
        if (d == 0)
        {
            m = std::min(n, size_t(M));
            const double *p = y.data() + k0;
            for (int i = 0; i < m; ++i, p += N)
                v[i] = *p;
        }
        else
        {
            m = std::min(n, size_t(N));
            const double *p = y.data() + k0;
            std::copy(p, p + m, v);
        }
        return m;
    }

protected:
    vec_t y;
};

class random3d : public AbstractDataStore
{

public:
    random3d(const std::string &name, size_t n, size_t m, size_t l)
        : AbstractDataStore(name, {n, m, l}), y(new vec_t(n * m * l)), dy(new vec_t(n * m * l))
    {
        randomize(y.get());
        randomize(dy.get(), 0.1);
        desc_ = "Random data array";
        dim_name_[0] = "X";
        dim_name_[1] = "Type";
        dim_name_[2] = "A";
        dim_desc_[2] = "Few random numbers";
        dim_desc_[1] = "Categories";
        dim_desc_[0] = "Many random numbers";
    }

    static void randomize(vec_t *y, double s = 1.0)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> u(0.0, s);
        for (int i = 0; i < y->size(); ++i)
        {
            (*y)[i] = u(gen);
        }
    }

    bool hasErrors() const override { return true; }
    bool is_x_categorical(size_t d) const override { return d == 1; }
    size_t get_x_categorical(size_t d, strvec_t &categories) const override
    {
        const char *cat[] = {"First", "Second", "Third"};
        for (int i = 0; i < dim_[1]; ++i)
            categories[i] = cat[i % 3];
        return dim_[1];
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
        size_t m;
        if (d == 0)
        {
            m = std::min(n, dim_[0]);
            const double *p = y->data() + k0;
            for (int i = 0; i < m; ++i, p += dim_[1] * dim_[2])
                v[i] = *p;
        }
        else if (d == 1)
        {
            m = std::min(n, dim_[1]);
            const double *p = y->data() + k0;
            for (int i = 0; i < m; ++i, p += dim_[2])
                v[i] = *p;
        }
        else
        {
            m = std::min(n, dim_[2]);
            const double *p = y->data() + k0;
            std::copy(p, p + m, v);
        }
        return m;
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

class text2d : public AbstractDataStore
{
    constexpr static const size_t N = 2;
    constexpr static const size_t M = 3;

public:
    text2d();
    bool is_numeric() const override { return false; }
    size_t get_y_text(size_t d, const dim_t &i0, strvec_t &y) const override;

protected:
    strvec_t y;
};

class text1d : public AbstractDataStore
{
public:
    text1d();
    bool is_numeric() const override { return false; }
    size_t get_y_text(size_t d, const dim_t &i0, strvec_t &y) const override;

protected:
    strvec_t y;
};

class MyTimer : public QTimer
{
public:
    MyTimer(QDataBrowser *w, const std::string &path, std::shared_ptr<AbstractDataStore::vec_t> y)
        : w_(w), path_(path), y_(y)
    {
    }

protected:
    void timerEvent(QTimerEvent *) override
    {
        if (y_)
            random3d::randomize(y_.get());
        if (w_)
            w_->dataUpdated(path_.c_str());
    }

    QPointer<QDataBrowser> w_;
    std::string path_;
    std::shared_ptr<AbstractDataStore::vec_t> y_;
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
        // QApplication::setStyle("Adwaita");
        // QApplication::setStyle("Adwaita-Dark");
    }

    QWidget win;
    win.setLocale(QLocale::c());
    win.setWindowTitle("QDataBrowser Example");
    QVBoxLayout *vbox = new QVBoxLayout;
    win.setLayout(vbox);
    QFrame *frm = new QFrame;
    QVBoxLayout *vbox1 = new QVBoxLayout;
    frm->setLayout(vbox1);
    vbox1->setContentsMargins(0, 0, 0, 0);
    // frm->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    QDataBrowser *dataBrowser = new QDataBrowser;
    dataBrowser->setIgnoreSingletonDims();
    vbox1->addWidget(dataBrowser);
    vbox->addWidget(frm);
    win.show();

    random3d *R1 = new random3d("R1", 1, 2, 10);
    std::shared_ptr<AbstractDataStore::vec_t> y1 = R1->data();
    random3d *R2 = new random3d("R2", 5, 3, 25);
    std::shared_ptr<AbstractDataStore::vec_t> y2 = R2->data();

    dataBrowser->addGroup("RandomData", "/", "Various random data arrays");

    dataBrowser->addGroup("3D", "/RandomData");
    dataBrowser->addData(R1, "/RandomData/3D");
    dataBrowser->addData(R2, "/RandomData/3D");

    MyTimer timer1(dataBrowser, "/RandomData/3D/R1", y1);
    timer1.start(1000);
    MyTimer timer2(dataBrowser, "/RandomData/3D/R2", y2);
    timer2.start(1500);

    dataBrowser->addGroup("2D", "/RandomData");
    dataBrowser->addData(new random2d(), "/RandomData/2D");

    dataBrowser->addData(new wave1d(), "RandomData");

    dataBrowser->addGroup("TextData", "/", "Various text data arrays");
    dataBrowser->addData(new text2d, "/TextData");
    dataBrowser->addData(new text1d, "/TextData");

    dataBrowser->selectItem("/RandomData/3D/R1");
    dataBrowser->setActiveView(QDataBrowser::Plot);
    dataBrowser->setPlotType(QDataBrowser::ErrorBar);

    // auto f1 = [dataBrowser] { dataBrowser->clear("/RandomData/2D"); };
    // QTimer::singleShot(10000, dataBrowser, f1);

    return app.exec();
}

text2d::text2d()
    : AbstractDataStore("text2d", {N, M}), y(N * M)
{
    for (int i = 0; i < N * M; ++i)
        y[i] = std::to_string(i + 1);
}

size_t text2d::get_y_text(size_t d, const dim_t &i0, strvec_t &t) const
{
    if (d == 0)
    {
        size_t k = dim_[0] * i0[1];
        size_t m = std::min(y.size(), dim_[0]);
        for (size_t i = 0; i < dim_[0]; ++i, ++k)
            t[i] = y[k];
        return m;
    }
    size_t k = i0[0];
    size_t s = dim_[0];
    size_t m = std::min(y.size(), dim_[1]);
    for (size_t i = 0; i < dim_[1]; ++i, k += s)
        t[i] = y[k];
    return m;
}

text1d::text1d()
    : AbstractDataStore("text1d", {1}), y(1)
{
    y[0] =
        R"(
{
    "Simulation": {
        "simulation_type": "FullCascade",
        "screening_type": "ZBL",
        "electronic_stopping": "SRIM13",
        "electronic_straggling": "Off",
        "nrt_calculation": "NRT_element",
        "intra_cascade_recombination": false,
        "time_ordered_cascades": true,
        "correlated_recombination": true,
        "move_recoil": false,
        "recoil_sub_ed": false
    },
    "Transport": {
        "flight_path_type": "Constant",
        "flight_path_const": 1.0,
        "min_energy": 1.0,
        "min_recoil_energy": 1.0,
        "min_scattering_angle": 2.0,
        "max_rel_eloss": 0.05,
        "mfp_range": [
            1.0,
            1e+30
        ]
    },
    "IonBeam": {
        "ion": {
            "symbol": "Fe",
            "atomic_number": 26,
            "atomic_mass": 55.935
        },
        "energy_distribution": {
            "type": "SingleValue",
            "center": 500000.0,
            "fwhm": 1.0
        },
        "spatial_distribution": {
            "geometry": "Surface",
            "type": "SingleValue",
            "center": [
                0.0,
                600.0,
                600.0
            ],
            "fwhm": 1.0
        },
        "angular_distribution": {
            "type": "SingleValue",
            "center": [
                1.0,
                0.0,
                0.0
            ],
            "fwhm": 1.0
        }
    },
    "Target": {
        "origin": [
            0.0,
            0.0,
            0.0
        ],
        "size": [
            600.0,
            1200.0,
            1200.0
        ],
        "cell_count": [
            100,
            1,
            1
        ],
        "periodic_bc": [
            0,
            1,
            1
        ],
        "materials": [
            {
                "id": "Fe",
                "density": 7.8658,
                "composition": [
                    {
                        "element": {
                            "symbol": "Fe",
                            "atomic_number": 26,
                            "atomic_mass": 55.8452
                        },
                        "X": 1.0,
                        "Ed": 40.0,
                        "El": 3.0,
                        "Es": 3.0,
                        "Er": 40.0,
                        "Rc": 0.946
                    }
                ],
                "color": "#55aaff"
            }
        ],
        "regions": [
            {
                "id": "R1",
                "material_id": "Fe",
                "origin": [
                    0.0,
                    0.0,
                    0.0
                ],
                "size": [
                    1200.0,
                    1200.0,
                    1200.0
                ]
            }
        ]
    },
    "Output": {
        "title": "500 keV Fe on Fe",
        "outfilename": "usertally_test",
        "storage_interval": 1000,
        "store_exit_events": false,
        "store_pka_events": false,
        "store_damage_events": false,
        "store_dedx": true
    },
    "Run": {
        "max_no_ions": 100000,
        "max_cpu_time": 0,
        "threads": 4,
        "seed": 123456789
    },
    "UserTally": [
        {
            "id": "Implanted Fe",
            "event": "IonStop",
            "coordinates": "cyl",
            "x": [],
            "y": [],
            "z": [
                0.0,
                20.0,
                40.0,
                60.0,
                80.0,
                100.0,
                120.0,
                140.0,
                160.0,
                180.0,
                200.0,
                220.0,
                240.0,
                260.0,
                280.0,
                300.0,
                320.0,
                340.0,
                360.0,
                380.0,
                400.0
            ],
            "rho": [
                0.0,
                20.0,
                40.0,
                60.0,
                80.0,
                100.0,
                120.0,
                140.0,
                160.0,
                180.0,
                200.0
            ],
            "phi": [],
            "r": [],
            "theta": [],
            "zaxis": [
                1.0,
                0.0,
                0.0
            ],
            "xzvec": [
                1.0,
                0.0,
                1.0
            ],
            "org": [
                0.0,
                600.0,
                600.0
            ],
            "vx": [],
            "vy": [],
            "vz": [],
            "vrho": [],
            "vphi": [],
            "vr": [],
            "vtheta": [],
            "atom_id": [
                0.0,
                1.0
            ]
        },
        {
            "id": "Fe Interstitials",
            "event": "IonStop",
            "coordinates": "cyl",
            "x": [],
            "y": [],
            "z": [
                0.0,
                20.0,
                40.0,
                60.0,
                80.0,
                100.0,
                120.0,
                140.0,
                160.0,
                180.0,
                200.0,
                220.0,
                240.0,
                260.0,
                280.0,
                300.0,
                320.0,
                340.0,
                360.0,
                380.0,
                400.0
            ],
            "rho": [
                0.0,
                20.0,
                40.0,
                60.0,
                80.0,
                100.0,
                120.0,
                140.0,
                160.0,
                180.0,
                200.0
            ],
            "phi": [],
            "r": [],
            "theta": [],
            "zaxis": [
                1.0,
                0.0,
                0.0
            ],
            "xzvec": [
                1.0,
                0.0,
                1.0
            ],
            "org": [
                0.0,
                600.0,
                600.0
            ],
            "vx": [],
            "vy": [],
            "vz": [],
            "vrho": [],
            "vphi": [],
            "vr": [],
            "vtheta": [],
            "atom_id": [
                1.0,
                2.0
            ]
        }
    ]
}
)";
}

size_t text1d::get_y_text(size_t d, const dim_t &i0, strvec_t &t) const
{
    if (t.size() == 1)
        t[0] = y[0];
    return 1;
}
