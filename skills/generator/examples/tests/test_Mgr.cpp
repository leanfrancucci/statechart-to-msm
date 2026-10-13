/**
 * @file test_Mgr.cpp
 *
 * Test cases are based on Boost.MSM v1.68.0. They use the functor front-end.
 * However, the states use the basic front-end.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <string>
// #include "Event.hpp"
#include "Mgr.hpp"

class MgrTest : public testing::Test
{
  protected:
    static void SetUpTestSuite() {}
};

/**
 * @class MockLog
 * @brief Google Mock implementation of the Log interface.
 */
class MockLog : public Log
{
  public:
    MOCK_METHOD(void, info, (const std::string& msg), (override));
};

TEST_F(MgrTest, InitAssociatedStateMachineOnInstantiation)
{
    Mgr mgr;

    EXPECT_EQ(mgr.m_info.getInitActionCount(), 1);
    EXPECT_TRUE(mgr.isActive());
    EXPECT_TRUE(mgr.isIdle());
}

TEST_F(MgrTest, EnqueueEvent)
{
    Mgr mgr;

    bool result = mgr.start();
    mgr.process();

    EXPECT_TRUE(result);
    EXPECT_TRUE(mgr.isProcessing());
}

/**
 * @brief Verifies that calling start() on the manager fails and returns
 *        false if the underlying event queue has reached its capacity limit.
 */
TEST_F(MgrTest, Start_OnQueueAtCapacity_ReturnsFalseAndRejectsEvent)
{
    bool result;
    Mgr mgr(Mgr::DefaultId, 1);

    // First push succeeds
    result = mgr.start();
    EXPECT_TRUE(result);

    // Second push fails because the queue is at capacity
    result = mgr.start();
    EXPECT_FALSE(result);
}

/**
 * @brief Verifies that constructing a Mgr instance with a queue capacity
 *        exceeding DefaultEventCapacity throws std::invalid_argument.
 */
TEST_F(MgrTest, Constructor_CapacityExceedsMaxLimit_ThrowsInvalidArgument)
{
    EXPECT_THROW(
    {
        Mgr mgr(Mgr::DefaultId, Mgr::DefaultEventCapacity + 1);
    }, std::invalid_argument
    );
}

/**
 * @brief Verifies that the Logger reference can be successfully injected,
 *        and correctly captures state entry, exit, and transition actions
 *        in the exact expected order using Google Mock expectations.
 */
TEST_F(MgrTest, LoggerDependencyInjection_VerifiesExpectedLoggedSequence)
{
    MockLog mock;

    {
        // Enforce strict chronological logging sequence
        ::testing::InSequence seq;

        // Startup expectation sequence
        EXPECT_CALL(mock, info("[Mgr::StateMachine]: initAction()")).Times(1);
        EXPECT_CALL(mock, info("[Mgr::StateMachine]: entering: Active")).Times(1);
        EXPECT_CALL(mock, info("[Mgr::StateMachine]: entering: Idle")).Times(1);

        // Transition expectation sequence (Idle -> Processing)
        EXPECT_CALL(mock, info("[Mgr::StateMachine]: leaving: Idle")).Times(1);
        EXPECT_CALL(mock, info("[Mgr::StateMachine]: startProcess()")).Times(1);
        EXPECT_CALL(mock, info("[Mgr::StateMachine]: entering: Processing")).Times(1);
    }

    Mgr mgr(Mgr::DefaultId, Mgr::DefaultEventCapacity, mock);

    bool result = mgr.start();
    EXPECT_TRUE(result);

    mgr.process();
    EXPECT_TRUE(mgr.isProcessing());
}

/**
 * @brief Verifies that the manager constructor works safely and properly when
 *        all default arguments are ignored/overridden by the caller, using
 *        Google Mock to verify logging.
 */
TEST_F(MgrTest, Constructor_ExplicitArgumentsIgnoredDefaults_Succeeds)
{
    MockLog mock;

    {
        // Expect startup logging sequence
        ::testing::InSequence seq;
        EXPECT_CALL(mock, info("[Mgr::StateMachine]: initAction()")).Times(1);
        EXPECT_CALL(mock, info("[Mgr::StateMachine]: entering: Active")).Times(1);
        EXPECT_CALL(mock, info("[Mgr::StateMachine]: entering: Idle")).Times(1);
    }

    // Explicitly override all three default constructor arguments
    Mgr mgr(42, 16, mock);

    EXPECT_TRUE(mgr.isActive());
    EXPECT_TRUE(mgr.isIdle());
}

/**
 * @brief Verifies that direct dispatching of events (bypassing the queue)
 *        triggers immediate transitions inside the state machine.
 */
TEST_F(MgrTest, Dispatch_DirectEventDispatch_InvokesTransitionsImmediately)
{
    MockLog mock;

    {
        ::testing::InSequence seq;
        EXPECT_CALL(mock, info("[Mgr::StateMachine]: initAction()")).Times(1);
        EXPECT_CALL(mock, info("[Mgr::StateMachine]: entering: Active")).Times(1);
        EXPECT_CALL(mock, info("[Mgr::StateMachine]: entering: Idle")).Times(1);

        // Expect direct dispatch transitions
        EXPECT_CALL(mock, info("[Mgr::StateMachine]: leaving: Idle")).Times(1);
        EXPECT_CALL(mock, info("[Mgr::StateMachine]: startProcess()")).Times(1);
        EXPECT_CALL(mock, info("[Mgr::StateMachine]: entering: Processing")).Times(1);
    }

    // 1. Instantiate
    Mgr mgr(Mgr::DefaultId, Mgr::DefaultEventCapacity, mock);
    EXPECT_TRUE(mgr.isIdle());

    // 2. Dispatch event directly (bypassing the queue, transitions instantly!)
    mgr.dispatch(Event::Start{});

    // 3. Verify immediate state occupancy
    EXPECT_TRUE(mgr.isProcessing());
}

TEST_F(MgrTest, PostAnEvent)
{
    bool result;
    Mgr mgr;

    result = mgr.post<Event::Start>();
    EXPECT_TRUE(result);

    mgr.process();
    EXPECT_TRUE(mgr.isProcessing());
}

/**
 * @brief Verifies that an event posted to an activated manager is dispatched
 *        by the internal thread and drives the state machine to Processing.
 */
TEST_F(MgrTest, Activate_PostedEvent_ReachesProcessing)
{
    Mgr mgr;

    mgr.activate();
    EXPECT_TRUE(mgr.post<Event::Start>());

    EXPECT_TRUE(mgr.waitForProcessing(std::chrono::seconds(1)));
    mgr.deactivate();
}

/**
 * @brief Verifies that waitForProcessing() returns false once the timeout
 *        expires if no event drives the state machine to Processing.
 */
TEST_F(MgrTest, WaitForProcessing_NoEvent_TimesOut)
{
    Mgr mgr;

    mgr.activate();

    EXPECT_FALSE(mgr.waitForProcessing(std::chrono::milliseconds(50)));
    EXPECT_TRUE(mgr.isIdle());
}
