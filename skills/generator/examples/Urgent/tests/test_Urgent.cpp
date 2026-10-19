/**
 * @file test_Urgent.cpp
 * @brief Unit tests of the Urgent state machine and its active object,
 *        one or more per row of the topology table in Urgent.md.
 *
 * Operation has a shallow history that only Resume activates
 * (msm::back::ShallowHistory<mpl::vector<Event::Resume>>): Resume restores the
 * last active substate, while Reset re-enters Operation at its initial state.
 */

#include "Urgent.hpp"
#include "UrgentActObj.hpp"

#include "Event.hpp"
#include "Log.hpp"
#include <algorithm>
#include <chrono>
#include <gtest/gtest.h>
#include <mutex>
#include <string>
#include <vector>

namespace
{
/**
 * @brief Logger that records every trace so tests can assert on the order
 *        of entry, exit and transition actions.
 *
 * Thread-safe: the active object logs from its dispatcher thread.
 */
class RecordingLog : public Log
{
  public:
    void info(const std::string& msg) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_messages.push_back(msg);
    }

    std::vector<std::string> messages() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_messages;
    }

    bool contains(const std::string& msg) const
    {
        const auto all = messages();
        return std::find(all.begin(), all.end(), msg) != all.end();
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_messages.clear();
    }

  private:
    mutable std::mutex m_mutex;
    std::vector<std::string> m_messages;
};

class UrgentTest : public ::testing::Test
{
  protected:
    RecordingLog log;
    Urgent sm {log};
};
} // namespace

// Row: [*] -> Operation, and Operation's [*] -> Idle
TEST_F(UrgentTest, StartEntersOperationAtIdle)
{
    sm.start();

    EXPECT_TRUE(sm.is_flag_active<Urgent_::OperationFlag>());
    EXPECT_TRUE(sm.is_flag_active<Urgent_::IdleFlag>());
    const std::vector<std::string> expected = {
        "[Urgent]: entering: Operation",
        "[Urgent]: entering: Idle"};
    EXPECT_EQ(log.messages(), expected);
}

// Row: Idle --Start--> Processing
TEST_F(UrgentTest, StartInIdleEntersProcessing)
{
    sm.start();
    log.clear();

    sm.process_event(Event::Start());

    EXPECT_TRUE(sm.is_flag_active<Urgent_::ProcessingFlag>());
    EXPECT_FALSE(sm.is_flag_active<Urgent_::IdleFlag>());
    const std::vector<std::string> expected = {"[Urgent]: entering: Processing"};
    EXPECT_EQ(log.messages(), expected);
}

// Row: Processing --Stop--> Idle
TEST_F(UrgentTest, StopInProcessingReturnsToIdle)
{
    sm.start();
    sm.process_event(Event::Start());
    log.clear();

    sm.process_event(Event::Stop());

    EXPECT_TRUE(sm.is_flag_active<Urgent_::IdleFlag>());
    const std::vector<std::string> expected = {"[Urgent]: entering: Idle"};
    EXPECT_EQ(log.messages(), expected);
}

// Row: Operation --Signal--> Paused
TEST_F(UrgentTest, SignalInOperationEntersPausedAndSuspends)
{
    sm.start();
    sm.process_event(Event::Start());
    log.clear();

    sm.process_event(Event::Signal());

    EXPECT_TRUE(sm.is_flag_active<Urgent_::PausedFlag>());
    EXPECT_FALSE(sm.is_flag_active<Urgent_::OperationFlag>());
    const std::vector<std::string> expected = {"[Urgent]: suspend()"};
    EXPECT_EQ(log.messages(), expected);
}

// Row: Paused --Resume--> H: the shallow history restores Processing
TEST_F(UrgentTest, ResumeRestoresLastActiveSubstate)
{
    sm.start();
    sm.process_event(Event::Start());
    sm.process_event(Event::Signal());
    log.clear();

    sm.process_event(Event::Resume());

    EXPECT_TRUE(sm.is_flag_active<Urgent_::OperationFlag>());
    EXPECT_TRUE(sm.is_flag_active<Urgent_::ProcessingFlag>());
    EXPECT_FALSE(sm.is_flag_active<Urgent_::IdleFlag>());
    const std::vector<std::string> expected = {
        "[Urgent]: entering: Operation",
        "[Urgent]: entering: Processing"};
    EXPECT_EQ(log.messages(), expected);
}

// Row: Paused --Resume--> H, when the last active substate was Idle
TEST_F(UrgentTest, ResumeRestoresIdleWhenIdleWasLastActive)
{
    sm.start();
    sm.process_event(Event::Signal());

    sm.process_event(Event::Resume());

    EXPECT_TRUE(sm.is_flag_active<Urgent_::IdleFlag>());
}

// Row: Paused --Reset--> Operation: no history, Operation restarts at Idle
TEST_F(UrgentTest, ResetReentersOperationAtInitialState)
{
    sm.start();
    sm.process_event(Event::Start());
    sm.process_event(Event::Signal());
    log.clear();

    sm.process_event(Event::Reset());

    EXPECT_TRUE(sm.is_flag_active<Urgent_::OperationFlag>());
    EXPECT_TRUE(sm.is_flag_active<Urgent_::IdleFlag>());
    EXPECT_FALSE(sm.is_flag_active<Urgent_::ProcessingFlag>());
    const std::vector<std::string> expected = {
        "[Urgent]: entering: Operation",
        "[Urgent]: entering: Idle"};
    EXPECT_EQ(log.messages(), expected);
}

// The history survives a Reset: after Reset, Start and a new pause, Resume
// restores the substate that was active at that pause
TEST_F(UrgentTest, ResumeAfterResetRestoresSubstateOfLatestPause)
{
    sm.start();
    sm.process_event(Event::Start());
    sm.process_event(Event::Signal());
    sm.process_event(Event::Reset());
    sm.process_event(Event::Signal());

    sm.process_event(Event::Resume());

    EXPECT_TRUE(sm.is_flag_active<Urgent_::IdleFlag>());
}

TEST_F(UrgentTest, IdleIgnoresStop)
{
    sm.start();

    sm.process_event(Event::Stop());

    EXPECT_TRUE(sm.is_flag_active<Urgent_::IdleFlag>());
}

TEST_F(UrgentTest, PausedIgnoresStartAndStop)
{
    sm.start();
    sm.process_event(Event::Signal());

    sm.process_event(Event::Start());
    sm.process_event(Event::Stop());

    EXPECT_TRUE(sm.is_flag_active<Urgent_::PausedFlag>());
}

TEST(UrgentActObjTest, PostedEventsDriveHistoryEndToEnd)
{
    RecordingLog log;
    UrgentActObj actObj(UrgentActObj::DefaultEventCapacity, log);
    const auto timeout = std::chrono::milliseconds(1000);

    actObj.activate();
    ASSERT_TRUE(actObj.waitForFlag<Urgent_::IdleFlag>(timeout));

    EXPECT_TRUE(actObj.post<Event::Start>());
    ASSERT_TRUE(actObj.waitForFlag<Urgent_::ProcessingFlag>(timeout));
    EXPECT_TRUE(actObj.post<Event::Signal>());
    ASSERT_TRUE(actObj.waitForFlag<Urgent_::PausedFlag>(timeout));
    EXPECT_TRUE(actObj.post<Event::Resume>());
    EXPECT_TRUE(actObj.waitForFlag<Urgent_::ProcessingFlag>(timeout));

    actObj.deactivate();
}

TEST(UrgentActObjTest, PostIsRejectedAfterDeactivate)
{
    RecordingLog log;
    UrgentActObj actObj(UrgentActObj::DefaultEventCapacity, log);

    actObj.activate();
    EXPECT_TRUE(actObj.post<Event::Start>());
    actObj.deactivate();

    EXPECT_FALSE(actObj.post<Event::Start>());
}
