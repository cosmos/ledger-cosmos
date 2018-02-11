#include "gtest/gtest.h"
#include "lib/BoolExample.h"

namespace
{
    TEST(ExampleTest, myTest1)
    {
        BoolExample tmp;

        EXPECT_TRUE(tmp.AlwaysTrue());
    }
}
