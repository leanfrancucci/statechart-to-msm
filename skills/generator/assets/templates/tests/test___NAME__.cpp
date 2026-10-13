/**
 * @file test_{{NAME}}.cpp
 * @brief Unit tests of the {{NAME}} state machine and its active object.
 *
 * Scaffolded tests cover only the placeholder skeleton. Rewrite them from the
 * UML topology table: at least one test per row (state entered, entry/exit
 * and effect traces in order, guards taking each branch), plus the events a
 * state must ignore.
 */

#include "{{NAME}}.hpp"
#include "{{NAME}}ActObj.hpp"

#include "Event.hpp"
#include "Log.hpp"
#include <algorithm>
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

class {{NAME}}Test : public ::testing::Test
{
  protected:
    RecordingLog log;
    {{NAME}} sm {log};
};
} // namespace

TEST_F({{NAME}}Test, StartEntersInitialState)
{
    sm.start();

    EXPECT_TRUE(sm.is_flag_active<{{NAME}}_::IdleFlag>());
    EXPECT_TRUE(log.contains("[{{NAME}}]: entering: Idle"));
}

TEST({{NAME}}ActObjTest, PostIsRejectedAfterDeactivate)
{
    RecordingLog log;
    {{NAME}}ActObj actObj({{NAME}}ActObj::DefaultEventCapacity, log);

    actObj.activate();
    EXPECT_TRUE(actObj.post<{{FIRST_EVENT_TYPE}}>());
    actObj.deactivate();

    EXPECT_FALSE(actObj.post<{{FIRST_EVENT_TYPE}}>());
}
