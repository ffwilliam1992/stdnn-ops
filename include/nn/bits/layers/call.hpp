#pragma once
#include <nn/common.hpp>

namespace nn::ops
{
template <typename T, typename Op, typename... Ts>
T *new_result(const Op &op, const Ts &... args)
{
    auto y = new T(op(args.shape()...));
    op(ref(*y), view(args)...);
    return y;
}

template <typename T, typename Init>
T *new_parameter(const shape<T::rank> &shp, const Init &init)
{
    auto w = new T(shp);
    init(ref(*w));
    return w;
}

}  // namespace nn::ops
