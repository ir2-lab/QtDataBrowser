#pragma once

class volume3d : public AbstractDataSet
{
    constexpr static const size_t N = 50;
    constexpr static const size_t M = N * N * N;

public:
    volume3d(const vec_t &sigma)
        : AbstractDataSet("Volume3D",
                          {N, N, N},
                          {"x", "y", "z"},
                          {"Length [m]", "Width [m]", "Height [m]"}),
          Y(M)
    {
        desc_ = "3D Volume data";
        int l = 0;
        vec_t q(sigma);
        for (int i = 0; i < 3; ++i)
            q[i] = 0.5 / sigma[i] / sigma[i];
        for (int i = 0; i < N; ++i)
        {
            double x = 2.0 * i / (N - 1) - 1.0;
            for (int j = 0; j < N; ++j)
            {
                double y = 2.0 * j / (N - 1);
                for (int k = 0; k < N; ++k)
                {
                    double z = 2.0 * k / (N - 1) - 1.0;
                    double r = q[0] * x * x + q[1] * y * y + q[2] * z * z;
                    Y[l++] = exp(-r);
                }
            }
        }
    }

protected:
    vec_t Y;
    size_t _get_(size_t d, const dim_t &i0, const double *y, size_t n, double *v) const
    {
        dim_t i(i0);
        size_t k0 = (i[0] * N + i[1]) * N + i[2];
        size_t m;
        if (d == 0)
        {
            m = std::min(n, dim_[0]);
            const double *p = y + k0;
            for (int i = 0; i < m; ++i, p += dim_[1] * dim_[2])
                v[i] = *p;
        }
        else if (d == 1)
        {
            m = std::min(n, dim_[1]);
            const double *p = y + k0;
            for (int i = 0; i < m; ++i, p += dim_[2])
                v[i] = *p;
        }
        else
        {
            m = std::min(n, dim_[2]);
            const double *p = y + k0;
            std::copy(p, p + m, v);
        }
        return m;
    }
    size_t get_y(size_t d, const dim_t &i0, size_t n, double *v) const override
    {
        return _get_(d, i0, Y.data(), n, v);
    }
    size_t get_x(size_t d, size_t n, double *v) const override
    {
        size_t m = std::min(n, N);
        for (size_t i = 0; i < m; ++i)
        {
            v[i] = d == 1 ? 2.0 * i / (N - 1) : 2.0 * i / (N - 1) - 1.0;
        }
        return m;
    }
};