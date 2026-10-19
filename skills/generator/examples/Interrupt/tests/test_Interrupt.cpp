/**
 * @file test_Interrupt.cpp
 * @brief Unit tests of the Interrupt state machine and its active object,
 *        one or more per row of the topology table in Interrupt.md.
 *
 * Operation's default entry is its history pseudo-state, so every entry into
 * Operation restores its last active substate (msm::back::AlwaysHistory). The
 * history is shallow: the nested composite Process has no history of its own,
 * so a restored Process always restarts at Stage1.
 */

#include "Interrupt.hpp"
#include "InterruptActObj.hpp"

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

class InterruptTest : public ::testing::Test
{
  protected:
    RecordingLog log;
    Interrupt sm {log};
};
} // namespace

// Rows: [*] -> Operation, Operation's [*] -> H, and H's default -> Idle
TEST_F(InterruptTest, StartEntersOperationAtIdle)
{
    sm.start();

    EXPECT_TRUE(sm.is_flag_active<Interrupt_::OperationFlag>());
    EXPECT_TRUE(sm.is_flag_active<Interrupt_::IdleFlag>());
    const std::vector<std::string> expected = {
        "[Interrupt]: entering: Operation",
        "[Interrupt]: entering: Idle"};
    EXPECT_EQ(log.messages(), expected);
}

// Rows: Idle --Start--> Process, and Process's [*] -> Stage1
TEST_F(InterruptTest, StartInIdleEntersProcessAtStage1)
{
    sm.start();
    log.clear();

    sm.process_event(Event::Start());

    EXPECT_TRUE(sm.is_flag_active<Interrupt_::ProcessFlag>());
    EXPECT_TRUE(sm.is_flag_active<Interrupt_::Stage1Flag>());
    EXPECT_FALSE(sm.is_flag_active<Interrupt_::IdleFlag>());
    const std::vector<std::string> expected = {
        "[Interrupt]: entering: Process",
        "[Interrupt]: entering: Stage1"};
    EXPECT_EQ(log.messages(), expected);
}

// Rows: Stage1 --Input--> Stage2 and Stage2 --Input--> Stage1
TEST_F(InterruptTest, InputTogglesBetweenStages)
{
    sm.start();
    sm.process_event(Event::Start());
    log.clear();

    sm.process_event(Event::Input());
    EXPECT_TRUE(sm.is_flag_active<Interrupt_::Stage2Flag>());

    sm.process_event(Event::Input());
    EXPECT_TRUE(sm.is_flag_active<Interrupt_::Stage1Flag>());

    const std::vector<std::string> expected = {
        "[Interrupt]: entering: Stage2",
        "[Interrupt]: entering: Stage1"};
    EXPECT_EQ(log.messages(), expected);
}

// Row: Operation --Signal--> Paused
TEST_F(InterruptTest, SignalInOperationEntersPausedAndSuspends)
{
    sm.start();
    log.clear();

    sm.process_event(Event::Signal());

    EXPECT_TRUE(sm.is_flag_active<Interrupt_::PausedFlag>());
    EXPECT_FALSE(sm.is_flag_active<Interrupt_::OperationFlag>());
    const std::vector<std::string> expected = {"[Interrupt]: suspend()"};
    EXPECT_EQ(log.messages(), expected);
}

// Row: Paused --Resume--> Operation, entered through its history: Idle
TEST_F(InterruptTest, ResumeRestoresIdle)
{
    sm.start();
    sm.process_event(Event::Signal());

    sm.process_event(Event::Resume());

    EXPECT_TRUE(sm.is_flag_active<Interrupt_::OperationFlag>());
    EXPECT_TRUE(sm.is_flag_active<Interrupt_::IdleFlag>());
}

// Row: Paused --Resume--> Operation, entered through its history: Process.
// The history is shallow, so Process restarts at Stage1, not at Stage2.
TEST_F(InterruptTest, ResumeRestoresProcessButRestartsItAtStage1)
{
    sm.start();
    sm.process_event(Event::Start());
    sm.process_event(Event::Input());
    ASSERT_TRUE(sm.is_flag_active<Interrupt_::Stage2Flag>());
    sm.process_event(Event::Signal());
    log.clear();

    sm.process_event(Event::Resume());

    EXPECT_TRUE(sm.is_flag_active<Interrupt_::ProcessFlag>());
    EXPECT_TRUE(sm.is_flag_active<Interrupt_::Stage1Flag>());
    EXPECT_FALSE(sm.is_flag_active<Interrupt_::Stage2Flag>());
    EXPECT_FALSE(sm.is_flag_active<Interrupt_::IdleFlag>());
    const std::vector<std::string> expected = {
        "[Interrupt]: entering: Operation",
        "[Interrupt]: entering: Process",
        "[Interrupt]: entering: Stage1"};
    EXPECT_EQ(log.messages(), expected);
}

TEST_F(InterruptTest, IdleIgnoresInputAndResume)
{
    sm.start();

    sm.process_event(Event::Input());
    sm.process_event(Event::Resume());

    EXPECT_TRUE(sm.is_flag_active<Interrupt_::IdleFlag>());
}

TEST_F(InterruptTest, PausedIgnoresStartAndInput)
{
    sm.start();
    sm.process_event(Event::Signal());

    sm.process_event(Event::Start());
    sm.process_event(Event::Input());

    EXPECT_TRUE(sm.is_flag_active<Interrupt_::PausedFlag>());
}

TEST(InterruptActObjTest, PostedEventsDriveHistoryEndToEnd)
{
    RecordingLog log;
    InterruptActObj actObj(InterruptActObj::DefaultEventCapacity, log);
    const auto timeout = std::chrono::milliseconds(1000);

    actObj.activate();
    ASSERT_TRUE(actObj.waitForFlag<Interrupt_::IdleFlag>(timeout));

    EXPECT_TRUE(actObj.post<Event::Start>());
    ASSERT_TRUE(actObj.waitForFlag<Interrupt_::Stage1Flag>(timeout));
    EXPECT_TRUE(actObj.post<Event::Signal>());
    ASSERT_TRUE(actObj.waitForFlag<Interrupt_::PausedFlag>(timeout));
    EXPECT_TRUE(actObj.post<Event::Resume>());
    EXPECT_TRUE(actObj.waitForFlag<Interrupt_::ProcessFlag>(timeout));

    actObj.deactivate();
}

TEST(InterruptActObjTest, PostIsRejectedAfterDeactivate)
{
    RecordingLog log;
    InterruptActObj actObj(InterruptActObj::DefaultEventCapacity, log);

    actObj.activate();
    EXPECT_TRUE(actObj.post<Event::Start>());
    actObj.deactivate();

    EXPECT_FALSE(actObj.post<Event::Start>());
}
