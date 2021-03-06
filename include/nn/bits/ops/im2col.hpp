#pragma once
#include <experimental/contract>
#include <experimental/new_type>
#include <nn/bits/ops/linear_sample.hpp>
#include <nn/bits/ops/reshape.hpp>
#include <nn/bits/ops/traits.hpp>
#include <stdtensor>

namespace nn::ops
{
template <typename image_order> class im2col_trait;

template <> class im2col_trait<hw>
{
  protected:
    struct ksize_trait;
    struct padding_trait;
    struct stride_trait;
    struct rate_trait;

    using ksize_t = std::experimental::new_type<shape<2>, ksize_trait>;
    using padding_t = std::experimental::new_type<shape<2>, padding_trait>;
    using stride_t = std::experimental::new_type<shape<2>, stride_trait>;
    using rate_t = std::experimental::new_type<shape<2>, rate_trait>;

    static constexpr auto default_padding = padding_t(0, 0);
    static constexpr auto default_stride = stride_t(1, 1);
    static constexpr auto default_rate = rate_t(1, 1);

    using sample_t = linear_sample_trait<size_t>;

    const sample_t h_sample_;
    const sample_t w_sample_;

    ksize_t get_ksize() const
    {
        return ksize_t(h_sample_.ksize_, w_sample_.ksize_);
    }

  public:
    static ksize_t ksize(int r, int s) { return ksize_t(r, s); };
    static padding_t padding(int r, int s) { return padding_t(r, s); };
    static stride_t stride(int r, int s) { return stride_t(r, s); };
    static rate_t rate(int r, int s) { return rate_t(r, s); };

    im2col_trait(const ksize_t &ksize)
        : im2col_trait(ksize, default_padding, default_stride)
    {
    }

    im2col_trait(const ksize_t &ksize, const padding_t &padding)
        : im2col_trait(ksize, padding, default_stride)
    {
    }

    im2col_trait(const ksize_t &ksize, const stride_t &stride)
        : im2col_trait(ksize, default_padding, stride)
    {
    }

    im2col_trait(const ksize_t &ksize, const padding_t &padding,
                 const stride_t &stride)
        : im2col_trait(ksize, padding, stride, default_rate)
    {
    }

    im2col_trait(const ksize_t &ksize, const padding_t &padding,
                 const stride_t &stride, const rate_t &rate)
        : h_sample_(ksize.dims[0], stride.dims[0], rate.dims[0],
                    padding.dims[0]),
          w_sample_(ksize.dims[1], stride.dims[1], rate.dims[1],
                    padding.dims[1])
    {
    }

    shape<2> operator()(const shape<2> &x) const
    {
        return shape<2>(h_sample_(x.dims[0]), w_sample_(x.dims[1]));
    }
};

template <typename image_order, typename ColOrder> class im2col;

template <> class im2col<hw, hwrs> : public im2col_trait<hw>
{
    using im2col_trait::im2col_trait;

  public:
    shape<4> operator()(const shape<2> &x) const
    {
        return internal::join_shape(im2col_trait::operator()(x), get_ksize());
    }

    template <typename R>
    void operator()(const ttl::tensor_ref<R, 4> &y,
                    const ttl::tensor_view<R, 2> &x) const
    {
        const auto [h, w] = x.shape().dims;
        const auto [h_, w_, r, s] = y.shape().dims;

        for (const auto i_ : range(h_)) {
            for (const auto j_ : range(w_)) {
                for (const auto u : range(r)) {
                    for (const auto v : range(s)) {
                        R value = 0;
                        const auto i = h_sample_(i_, u);
                        const auto j = w_sample_(j_, v);
                        if (h_sample_.inside(i, h) && w_sample_.inside(j, w)) {
                            value =
                                x.at(h_sample_.unpad(i), w_sample_.unpad(j));
                        }
                        y.at(i_, j_, u, v) = value;
                    }
                }
            }
        }
    }
};

template <> class im2col<hw, rshw> : public im2col_trait<hw>
{
    using im2col_trait::im2col_trait;

  public:
    shape<4> operator()(const shape<2> &x) const
    {
        return internal::join_shape(get_ksize(), im2col_trait::operator()(x));
    }

    template <typename R>
    void operator()(const ttl::tensor_ref<R, 4> &y,
                    const ttl::tensor_view<R, 2> &x) const
    {
        const auto [h, w] = x.shape().dims;
        const auto [r, s, h_, w_] = y.shape().dims;

        for (const auto u : range(r)) {
            for (const auto v : range(s)) {
                for (const auto i_ : range(h_)) {
                    for (const auto j_ : range(w_)) {
                        R value = 0;
                        const auto i = h_sample_(i_, u);
                        const auto j = w_sample_(j_, v);
                        if (h_sample_.inside(i, h) && w_sample_.inside(j, w)) {
                            value =
                                x.at(h_sample_.unpad(i), w_sample_.unpad(j));
                        }
                        y.at(u, v, i_, j_) = value;
                    }
                }
            }
        }
    }
};

// TODO: use vectorize
template <> class im2col<hwc, hwrsc> : public im2col_trait<hw>
{
    using im2col_trait::im2col_trait;

  public:
    shape<5> operator()(const shape<3> &x) const
    {
        const auto [r, s] = get_ksize().dims;
        return shape<5>(h_sample_(x.dims[0]), w_sample_(x.dims[1]), r, s,
                        x.dims[2]);
    }

    template <typename R>
    void operator()(const ttl::tensor_ref<R, 5> &y,
                    const ttl::tensor_view<R, 3> &x) const
    {
        const auto [h, w, c] = x.shape().dims;
        const auto [h_, w_, r, s, _c] = y.shape().dims;
        contract_assert(_c == c);

        for (const auto i_ : range(h_)) {
            for (const auto j_ : range(w_)) {
                for (const auto u : range(r)) {
                    for (const auto v : range(s)) {
                        for (const auto k : range(c)) {
                            R value = 0;
                            const auto i = h_sample_(i_, u);
                            const auto j = w_sample_(j_, v);
                            if (h_sample_.inside(i, h) &&
                                w_sample_.inside(j, w)) {
                                value = x.at(h_sample_.unpad(i),
                                             w_sample_.unpad(j), k);
                            }
                            y.at(i_, j_, u, v, k) = value;
                        }
                    }
                }
            }
        }
    }
};
}  // namespace nn::ops
