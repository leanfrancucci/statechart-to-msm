/**
 * @file test_CompositeState.cpp
 *
 * Test cases are based on Boost.MSM v1.68.0. They use the functor front-end.
 * However, the states use the basic front-end.
 */

#include <gtest/gtest.h>
#include "Event.hpp"
#include "CompositeState.h"

using namespace Event;
class CompositeStateTest: public testing::Test
{
    protected:
        // Active states are checked through their flags (is_flag_active<>()),
        // never through state ids: ids follow the order of the transition
        // table and shift silently when it changes.
        static void SetUpTestSuite()
        {
        }
};

TEST_F(CompositeStateTest, CompositeStateAsInitialState)
{
    CompositeState sm;
    sm.start();

    EXPECT_TRUE(sm.is_flag_active<CompositeState::S1Flag>());
    EXPECT_TRUE(sm.is_flag_active<CompositeState::S11Flag>());
}
