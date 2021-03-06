#include "testing.hpp"

#include <nn/layers>

template <typename dense> void test_dense_layer()
{
    static_assert(std::is_class<dense>::value, "");

    auto x = ttl::tensor<float, 2>(2, 8);
    dense l1(10);
    l1(ref(x));
}

template <typename conv> void test_conv_layer()
{
    static_assert(std::is_class<conv>::value, "");
    // static_assert(std::is_constructible<L>::value, "");

    auto x = ttl::tensor<float, 4>(2, 32, 32, 32);
    conv l1(conv::ksize(3, 3), 32);
    l1(ref(x));
    conv l2(conv::ksize(3, 3), 32, conv::padding(1, 1));
    l2(ref(x));
}

template <typename pool> void test_pool_layer()
{
    static_assert(std::is_class<pool>::value, "");
    // static_assert(std::is_constructible<L>::value, "");

    auto x = ttl::tensor<float, 4>(2, 32, 32, 32);
    pool l1(pool::ksize(2, 2));
    l1(ref(x));
}

TEST(layers_test, test_1)
{
    test_dense_layer<nn::layers::dense<>>();
    test_dense_layer<nn::layers::dense<nn::ops::pointwise<nn::ops::relu>>>();

    test_conv_layer<nn::layers::conv<>>();
    test_conv_layer<nn::layers::conv<nn::ops::nhwc>>();
    // FIXME: support conv<nchw, rscd>
    // test_conv_layer<nn::layers::conv<nn::ops::nchw>>();
    test_conv_layer<nn::layers::conv<nn::ops::nchw, nn::ops::dcrs>>();

    test_pool_layer<nn::layers::pool<nn::ops::pool_max>>();
    test_pool_layer<nn::layers::pool<nn::ops::pool_max, nn::ops::nhwc>>();
    test_pool_layer<nn::layers::pool<nn::ops::pool_max, nn::ops::nchw>>();
}
