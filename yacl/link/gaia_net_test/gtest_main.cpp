//
// Created by parallels on 3/25/21.
//
// #include "// gaialog.h"
#include <gtest/gtest.h>
#include <iostream>

//TEST(SquareRootTest, NegativeNos) {
    //    ASSERT_EQ(-1.0, squareRoot(-15.0));
    //    ASSERT_EQ(-1.0, squareRoot(-0.2));
//}

uint64_t test_func() {
	//// gaialog_assert(false);
	//// gaialog_assert_msg(false, "this file {}", 1);
	return (uint64_t)&test_func;
}


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    // gaialog::init("test", ".", true, 2, 104857600, false);
    // gaialog_debug("{}", test_func());	//not eval test_func() if not real log
    return RUN_ALL_TESTS();
}
