/**
 * @file test_EventQueue.cpp
 * @brief Self-documenting unit tests for the generic EventQueue.
 */

#include <chrono>
#include <gtest/gtest.h>
#include <boost/variant.hpp>
#include <boost/core/demangle.hpp>
#include <boost/optional.hpp>
#include <typeinfo>
#include <string>
#include <thread>
#include "EventQueue.hpp"

class QueueTest : public testing::Test
{
protected:
    static void SetUpTestSuite() {}
};

struct EventA {};

struct EventB
{
    static int aliveCount;

    EventB(std::string name, int diskType)
        : m_name(name), m_discType(diskType)
    {
        aliveCount++;
    }

    EventB(const EventB& other)
        : m_name(other.m_name), m_discType(other.m_discType)
    {
        aliveCount++;
    }

    EventB(EventB&& other) noexcept
        : m_name(std::move(other.m_name)), m_discType(other.m_discType)
    {
        aliveCount++;
    }

    EventB& operator=(const EventB& other)
    {
        if (this != &other)
        {
            m_name = other.m_name;
            m_discType = other.m_discType;
        }
        return *this;
    }

    EventB& operator=(EventB&& other) noexcept
    {
        if (this != &other)
        {
            m_name = std::move(other.m_name);
            m_discType = other.m_discType;
        }
        return *this;
    }

    ~EventB()
    {
        aliveCount--;
    }

    std::string m_name;
    int m_discType;
};

int EventB::aliveCount = 0;

struct EventC {};

// Define all the events that are supported by the event processor
using Events = boost::variant<EventA, EventB, EventC>;

struct MySM;

/**
 * @struct DispatchVisitor
 * @brief C++11-safe static visitor to dispatch events to MySM.
 */
struct DispatchVisitor : public boost::static_visitor<void>
{
    MySM& sm;
    explicit DispatchVisitor(MySM& s) : sm(s) {}

    template <typename T>
    void operator()(const T& concrete_event) const;
};

// Simulate an event processor
struct MySM
{
    // Tracking/Spy variables for behavioral assertions
    mutable int eventACount = 0;
    mutable int eventBCount = 0;
    mutable int eventCCount = 0;
    mutable std::string lastEventBName = "";
    mutable int lastEventBType = 0;

    MySM(EventQueue<Events>& queue) : m_queue(queue) {}

    void process_event(const EventA& event)
    {
        EXPECT_EQ("EventA", boost::core::demangle(typeid(event).name()));
        ++eventACount;
    }

    void process_event(const EventB& event)
    {
        EXPECT_EQ("EventB", boost::core::demangle(typeid(event).name()));
        ++eventBCount;
        lastEventBName = event.m_name;
        lastEventBType = event.m_discType;
    }

    void process_event(const EventC& event)
    {
        EXPECT_EQ("EventC", boost::core::demangle(typeid(event).name()));
        m_queue.push<EventA>();
        ++eventCCount;
    }

    /**
     * @brief Dispatches a popped optional event to the state machine.
     *
     * Extracts the active event from the boost::optional wrapper and uses
     * a boost::variant visitor to invoke the matching process_event()
     * overload.
     *
     * @tparam EventVariant Bounded variant type of all possible events.
     * @param event Optional wrapped variant event to dispatch.
     * @return true if successfully processed, false if optional is empty.
     */
    template <typename EventVariant>
    bool dispatch(const EventVariant& event)
    {
        boost::apply_visitor(DispatchVisitor(*this), event);
        return true;
    }

    EventQueue<Events>& m_queue;
};

// Implement DispatchVisitor operator now that MySM is fully defined
template <typename T>
inline void DispatchVisitor::operator()(const T& concrete_event) const
{
    sm.process_event(concrete_event);
}

TEST_F(QueueTest, IsEmpty_OnInitialization_ReturnsTrue)
{
    EventQueue<Events> queue;

    EXPECT_TRUE(queue.empty() == true);
}

TEST_F(QueueTest, Push_SingleEventWithoutArgs_IncreasesSize)
{
    EventQueue<Events> queue;

    queue.push<EventA>();
    EXPECT_TRUE(queue.empty() == false);
}

TEST_F(QueueTest, Push_SingleEventWithArgs_IncreasesSize)
{
    EventQueue<Events> queue;

    queue.push<EventB>("diskA", 2);
    EXPECT_TRUE(queue.empty() == false);
}

TEST_F(QueueTest, Push_HeterogeneousEvents_AllAreStored)
{
    EventQueue<Events> queue;

    queue.push<EventA>();
    queue.push<EventB>("diskA", 2);
    EXPECT_TRUE(queue.empty() == false);
}

TEST_F(QueueTest, Size_OnEmptyQueue_ReturnsZero)
{
    EventQueue<Events> queue;

    EXPECT_EQ(0, queue.size());
}

TEST_F(QueueTest, Size_OnPopulatedQueue_ReturnsCorrectCount)
{
    EventQueue<Events> queue;

    queue.push<EventA>();
    queue.push<EventB>("diskA", 2);

    EXPECT_EQ(2, queue.size());
}

TEST_F(QueueTest, Dispatch_SingleEvent_InvokesSMHandlerAndDecreasesSize)
{
    EventQueue<Events> queue;
    MySM sm(queue);

    queue.push<EventA>();
    auto event = queue.pop();

    EXPECT_EQ(queue.size(), 0);

    bool result = sm.dispatch(*event);
    EXPECT_TRUE(result == true);
    EXPECT_EQ(sm.eventACount, 1);
}

TEST_F(QueueTest, Dispatch_MultipleEvents_InvokesSMHandlerInOrder)
{
    EventQueue<Events> queue;
    MySM sm(queue);
    bool result;
    int nIter = 3;

    for (int i = 0; i < nIter; ++i)
    {
        queue.push<EventA>();
    }
    for (int i = 0; i < nIter; ++i)
    {
        auto event = queue.pop();
        result = sm.dispatch(*event);
        EXPECT_TRUE(result == true);
        EXPECT_EQ(queue.size(), nIter - i - 1);
        EXPECT_EQ(sm.eventACount, i + 1);
    }
}

TEST_F(QueueTest, BlocksOnEmptyQueue)
{
    EventQueue<Events> queue;
    std::thread t([&queue]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        queue.push<EventA>();
    });

    auto event = queue.pop();
    t.join();
}

/**
 * @brief Verifies that EventQueue owns and cleans up its elements.
 * 
 * Confirms that destroying the queue automatically triggers the destructors
 * of any remaining (undispatched) events, preventing silent memory leaks in
 * long-running loops.
 */
TEST_F(QueueTest, QueueDestruction_WithPendingEvents_DestructsAllEvents)
{
    EXPECT_EQ(EventB::aliveCount, 0);
    {
        EventQueue<Events> queue;

        queue.push<EventB>("diskA", 2);
        queue.push<EventB>("diskB", 8);

        EXPECT_EQ(EventB::aliveCount, 2);
    }

    EXPECT_EQ(EventB::aliveCount, 0);
}

/**
 * @brief Stress tests the queue to verify SBO limits and heap stability.
 * 
 * In real-time embedded targets, we must ensure high-throughput processing
 * does not trigger memory fragmentation or overflow the static storage
 * blocks. Pushing and popping are streamed to respect the capacity limit of 32.
 */
TEST_F(QueueTest, Dispatch_HighVolumeStress_SucceedsAndReclaimsAllMemory)
{
    EventQueue<Events> queue;
    MySM sm(queue);

    EXPECT_EQ(EventB::aliveCount, 0);

    const int stressCount = 5000;
    int processedCount = 0;

    for (int i = 0; i < stressCount; ++i)
    {
        bool pushed = false;
        if (i % 2 == 0)
        {
            pushed = queue.push<EventA>();
        }
        else
        {
            pushed = queue.push<EventB>("stressDisk", i);
        }
        EXPECT_TRUE(pushed);

        // Stream elements out when the queue reaches capacity (32)
        if (queue.full() || i == stressCount - 1)
        {
            while (!queue.empty())
            {
                auto event = queue.pop();
                sm.dispatch(*event);
                processedCount++;
            }
        }
    }

    EXPECT_EQ(queue.size(), 0);
    EXPECT_EQ(EventB::aliveCount, 0);
    EXPECT_EQ(processedCount, stressCount);
    EXPECT_EQ(sm.eventACount, stressCount / 2);
    EXPECT_EQ(sm.eventBCount, stressCount / 2);
}

/**
 * @brief Verifies reentrant enqueuing during a state machine's
 *        process_event execution.
 * 
 * When a state machine processes an event (like EventC) and internally
 * pushes another event (like EventA) into the shared queue, the queue
 * must safely append the new event to the tail without corruption or
 * re-entrancy issues.
 */
TEST_F(QueueTest, Dispatch_WithReentrantPush_CorrectlyAppendsAndProcesses)
{
    EventQueue<Events> queue;
    MySM sm(queue);
    bool result;

    queue.push<EventC>();
    
    auto event1 = queue.pop();
    result = sm.dispatch(*event1);

    EXPECT_TRUE(result == true);
    EXPECT_EQ(sm.eventCCount, 1);
    EXPECT_EQ(queue.size(), 1);

    auto event2 = queue.pop();
    result = sm.dispatch(*event2);
    EXPECT_TRUE(result == true);
    EXPECT_EQ(sm.eventACount, 1);
}

/**
 * @brief Verifies that calling pop() on a populated queue returns the front
 *        event safely wrapped in boost::optional and decreases the size.
 */
TEST_F(QueueTest, Pop_OnPopulatedQueue_ReturnsMovedEventAndDecreasesSize)
{
    EventQueue<Events> queue;
    MySM sm(queue);

    queue.push<EventA>();
    auto result = queue.pop();

    EXPECT_EQ(queue.size(), 0);
    boost::apply_visitor(DispatchVisitor(sm), *result);
    EXPECT_EQ(sm.eventACount, 1);
}

/**
 * @brief Verifies that pop() transfers ownership of the event's lifecycle
 *        cleanly and that the event is destructed when the optional goes
 *        out of scope.
 */
TEST_F(QueueTest, Pop_LifetimeTransfer_DestructsCorrectly)
{
    EXPECT_EQ(EventB::aliveCount, 0);
    {
        EventQueue<Events> queue;
        queue.push<EventB>("temporary", 42);
        EXPECT_EQ(EventB::aliveCount, 1);

        {
            auto result = queue.pop();
            EXPECT_EQ(queue.size(), 0);
            
            // The event is no longer in the queue, but lives in the optional
            EXPECT_EQ(EventB::aliveCount, 1);
        } // 'result' goes out of scope here

        // The event must be completely destructed now
        EXPECT_EQ(EventB::aliveCount, 0);
    }
}

/**
 * @brief Verifies that full() returns true when the queue reaches its
 *        configured maximum capacity limit.
 */
TEST_F(QueueTest, Full_AtMaximumCapacity_ReturnsTrue)
{
    // Construct queue with a strict maximum capacity of 2
    EventQueue<Events> queue(2);

    EXPECT_FALSE(queue.full());

    queue.push<EventA>();
    EXPECT_FALSE(queue.full());

    queue.push<EventA>();
    EXPECT_TRUE(queue.full());
}

/**
 * @brief Verifies that calling push() on a full queue safely rejects the
 *        new element, returns false, and does not increase size.
 */
TEST_F(QueueTest, Push_WhenQueueIsFull_ReturnsFalseAndRejectsEvent)
{
    // Construct queue with a strict maximum capacity of 1
    EventQueue<Events> queue(1);

    // First push succeeds
    bool push1 = queue.push<EventA>();
    EXPECT_TRUE(push1);
    EXPECT_EQ(queue.size(), 1);
    EXPECT_TRUE(queue.full());

    // Second push fails due to capacity boundary
    bool push2 = queue.push<EventB>("overflow", 9);
    EXPECT_FALSE(push2);
    EXPECT_EQ(queue.size(), 1); // Size remains 1
}

/**
 * @brief Verifies that constructing EventQueue with a capacity exceeding
 *        DefaultMaxCapacity throws std::invalid_argument.
 */
TEST_F(QueueTest, Constructor_CapacityExceedsMaxLimit_ThrowsInvalidArgument)
{
    EXPECT_THROW(
        { EventQueue<Events> queue(EventQueue<Events>::DefaultMaxCapacity + 1); },
        std::invalid_argument
    );
}
