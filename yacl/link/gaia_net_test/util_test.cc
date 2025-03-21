#include "gaia_utils/fast_resize.h"

#include <gtest/gtest.h>

GAIA_FAST_RESIZE_VECTOR_TYPE(int)

TEST(gaia_utils, resize) {
    std::vector<int> a;
    gaia_utils::fast_resize(a, 100);
    GTEST_ASSERT_EQ(a.size(), 100);

    std::string b;
    gaia_utils::fast_resize(b, 100);
    GTEST_ASSERT_EQ(b.size(), 100);
}
