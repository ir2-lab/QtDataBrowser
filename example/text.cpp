#include "text.h"

text2d::text2d()
    : AbstractDataSet("text2d", {N, M}), y(N * M)
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
    : AbstractDataSet("text1d", {1}), y(1)
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