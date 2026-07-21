#pragma once

class wave1d : public AbstractDataSet
{
    constexpr static const size_t N = 1000;

public:
    wave1d()
        : AbstractDataSet("wave1d", {N}), y(N)
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