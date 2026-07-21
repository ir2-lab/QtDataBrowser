#pragma once

#include <QDataBrowser>

class text2d : public AbstractDataSet
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

class text1d : public AbstractDataSet
{
public:
    text1d();
    bool is_numeric() const override { return false; }
    size_t get_y_text(size_t d, const dim_t &i0, strvec_t &y) const override;

protected:
    strvec_t y;
};