#pragma once

#include <random>

class random2d : public AbstractDataSet
{
    constexpr static const size_t N = 100;
    constexpr static const size_t M = 3;

public:
    random2d()
        : AbstractDataSet("random2d", {M, N}), y(M * N)
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

class random3d : public AbstractDataSet
{

public:
    random3d(const std::string &name, size_t n, size_t m, size_t l)
        : AbstractDataSet(name, {n, m, l}), y(new vec_t(n * m * l)), dy(new vec_t(n * m * l))
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