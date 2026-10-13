/**
 * @file test_SimpleState.cpp
 *
 * Test cases are based on Boost.MSM v1.68.0. They use the functor front-end.
 * However, the states use the basic front-end.
 */

#include <gtest/gtest.h>
#include "Event.hpp"
#include "SimpleState.h"

using namespace Event;

class SimpleStateTest: public testing::Test
{
  protected:
    // Active states are checked through their flags (is_flag_active<>()),
    // never through state ids: ids follow the order of the transition table
    // and shift silently when it changes.
    static void SetUpTestSuite() {}
};

TEST_F(SimpleStateTest, TriggerSimpleInternalTransitionWithoutGuard)
{
    SimpleState sm;

    // Start the highest-level SM. This will call on_entry and mark
    // the start of the SM in this case it will also immediately trigger all
    // anonymous transitions
    sm.start();

    auto& s1 = sm.get_state<SimpleState::S1&>();
    EXPECT_EQ(s1.onEntryCount, 1);
    EXPECT_EQ(sm.actionACalled, 0);

    sm.process_event(EventA());

    EXPECT_EQ(sm.actionACalled, 1);
    EXPECT_TRUE(sm.is_flag_active<SimpleState::S1Flag>());
}

TEST_F(SimpleStateTest, TriggerSimpleInternalTransitionWithGuard)
{
    SimpleState sm;

    sm.start();

    auto& s1 = sm.get_state<SimpleState::S1&>();
    EXPECT_EQ(s1.onEntryCount, 1);
    EXPECT_EQ(sm.actionBCalled, 0);

    sm.process_event(EventB("diskA", 8));

    EXPECT_EQ(sm.actionBCalled, 1);
    EXPECT_TRUE(sm.is_flag_active<SimpleState::S1Flag>());
}

TEST_F(SimpleStateTest, TriggerRegularTransition)
{
    SimpleState sm;

    sm.start();

    auto& s1 = sm.get_state<SimpleState::S1&>();
    EXPECT_EQ(s1.onEntryCount, 1);
    EXPECT_EQ(sm.actionBCalled, 0);

    sm.process_event(EventC());

    EXPECT_EQ(sm.actionCCalled, 1);
    EXPECT_TRUE(sm.is_flag_active<SimpleState::S2Flag>());
}

TEST_F(SimpleStateTest, TriggerSimpleInternalTransitionWithGuardFailed)
{
    SimpleState sm;
    sm.start();

    auto& s1 = sm.get_state<SimpleState::S1&>();
    EXPECT_EQ(s1.onEntryCount, 1);
    EXPECT_EQ(sm.actionBCalled, 0);

    sm.process_event(EventB("wrongDisk", 99));

    EXPECT_EQ(sm.actionBCalled, 0);
    EXPECT_TRUE(sm.is_flag_active<SimpleState::S1Flag>());
}

TEST_F(SimpleStateTest, TriggerNoTransitionFromS2)
{
    SimpleState sm;
    sm.start();

    sm.process_event(EventC());
    EXPECT_TRUE(sm.is_flag_active<SimpleState::S2Flag>());

    sm.process_event(EventB("no-disk", 4));

    EXPECT_TRUE(sm.is_flag_active<SimpleState::S2Flag>());
    EXPECT_EQ(sm.actionACalled, 0);
}

TEST_F(SimpleStateTest, TransitionToChoiceTakesTruePath)
{
    SimpleState sm;

    sm.start();
    sm.process_event(EventC());
    sm.process_event(EventA());

    EXPECT_TRUE(sm.is_flag_active<SimpleState::S3Flag>());
    EXPECT_EQ(sm.calculateXCalled, 0);
    EXPECT_EQ(sm.xIsZeroCalled, 0);
    sm.process_event(EventA());

    EXPECT_EQ(sm.calculateXCalled, 1);
    EXPECT_EQ(sm.xIsZeroCalled, 1);
    EXPECT_TRUE(sm.is_flag_active<SimpleState::S3Flag>());
}

TEST_F(SimpleStateTest, TransitionToCompositeState)
{
    SimpleState sm;

    sm.start();
    sm.process_event(EventD());

    EXPECT_TRUE(sm.is_flag_active<SimpleState::S4Flag>());
    EXPECT_TRUE(sm.is_flag_active<SimpleState::S41Flag>());
}

TEST_F(SimpleStateTest, TriggerCompositeInternalTransition)
{
    SimpleState sm;

    sm.start();
    sm.process_event(EventD());
    sm.process_event(EventE());

    EXPECT_TRUE(sm.is_flag_active<SimpleState::S4Flag>());
    EXPECT_TRUE(sm.is_flag_active<SimpleState::S41Flag>());
    EXPECT_EQ(sm.actionIntS4Called, 1);
}

TEST_F(SimpleStateTest, TriggerCompositeRegularTransition)
{
    SimpleState sm;

    sm.start();

    sm.process_event(EventD());
    EXPECT_TRUE(sm.is_flag_active<SimpleState::S4Flag>());

    sm.process_event(EventD());
    EXPECT_TRUE(sm.is_flag_active<SimpleState::S1Flag>());
}

TEST_F(SimpleStateTest, ExitFromInsideACompositeState)
{
    SimpleState sm;

    sm.start();

    sm.process_event(EventD());
    EXPECT_TRUE(sm.is_flag_active<SimpleState::S4Flag>());

    sm.process_event(EventC());
    EXPECT_TRUE(sm.is_flag_active<SimpleState::S1Flag>());
}

TEST_F(SimpleStateTest, ExitFromInsideACompositeStateThroughAFinalState)
{
    SimpleState sm;

    sm.start();

    sm.process_event(EventD());
    EXPECT_TRUE(sm.is_flag_active<SimpleState::S4Flag>());

    EXPECT_TRUE(sm.is_flag_active<SimpleState::S41Flag>());

    sm.process_event(EventA());
    EXPECT_TRUE(sm.is_flag_active<SimpleState::S2Flag>());
}

TEST_F(SimpleStateTest, EnterExplicitelyToCompositeState)
{
    SimpleState sm;

    sm.start();

    sm.process_event(EventC());
    sm.process_event(EventA());
    sm.process_event(EventC());

    EXPECT_TRUE(sm.is_flag_active<SimpleState::S4Flag>());
    EXPECT_TRUE(sm.is_flag_active<SimpleState::S41Flag>());
}

TEST_F(SimpleStateTest, DeferingMechanism)
{
    SimpleState sm;

    sm.start();

    EXPECT_EQ(sm.actionECalled, 0);
    sm.process_event(EventE());
    EXPECT_TRUE(sm.is_flag_active<SimpleState::S1Flag>());

    sm.process_event(EventC());
    EXPECT_TRUE(sm.is_flag_active<SimpleState::S2Flag>());

    sm.process_event(EventA());

    EXPECT_EQ(sm.actionECalled, 1);
    EXPECT_TRUE(sm.is_flag_active<SimpleState::S2Flag>());
}
