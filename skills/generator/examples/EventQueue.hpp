/**
 * @file EventQueue.hpp
 * @brief Generic variant-based Event Queue for state machines. 
 */

#ifndef __EVENTQUEUE_HPP__
#define __EVENTQUEUE_HPP__

#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <functional>
#include <queue>
#include <unistd.h>
#include <utility> // For std::move and std::forward
#include <stdexcept> // For std::invalid_argument
#include <mutex>
#include <condition_variable>

template<typename EventVariant>
class EventQueue
{
public:
    // Default capacity limit when none is specified
    static constexpr size_t DefaultMaxCapacity = 32;

    /**
      * @brief Constructs an EventQueue with a specified maximum capacity.
      *
      * @param maxSize Maximum allowed number of elements in the queue.
      * @throws std::invalid_argument if maxSize exceeds DefaultMaxCapacity.
      */
    explicit EventQueue(size_t maxSize = DefaultMaxCapacity)
        : m_maxSize(maxSize), m_condvar()
    {
        if (m_maxSize > DefaultMaxCapacity)
        {
            throw std::invalid_argument(
                "Requested queue capacity exceeds DefaultMaxCapacity"
            );
        }
    }

    virtual ~EventQueue() = default;

    /**
      * @brief Enqueues a new event constructed in-place if capacity permits.
      *
      * @tparam EventType Concrete type of event to create and push.
      * @tparam Args Variadic argument types for EventType constructor.
      * @param args Arguments forwarded to EventType constructor.
      * @return true if successfully pushed, false if queue is full or closed.
      */
    template <typename EventType, typename... Args>
    bool push(Args&&... args)
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (m_closed || (m_queue.size() >= m_maxSize))
        {
            return false;
        }

        m_queue.push(EventType(std::forward<Args>(args)...));
        m_condvar.notify_one();
        return true;
    }

    /**
      * @brief Checks if the event queue is empty.
      *
      * @return true if queue is empty, false otherwise.
      */
    bool empty()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    /**
      * @brief Checks if the event queue is at maximum capacity.
      *
      * @return true if queue is full, false otherwise.
      */
    bool full()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.size() >= m_maxSize;
    }

    /**
      * @brief Gets the number of events currently in the queue.
      *
      * @return The size of the queue.
      */
    size_t size()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

    /**
     * @brief Closes the queue and wakes up every thread blocked in pop().
     *
     * Pending events are discarded. After closing, push() rejects new events
     * and pop() returns boost::none.
     */
    void close()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_closed = true;
            std::queue<EventVariant>().swap(m_queue);
        }
        m_condvar.notify_all();
    }

    /**
     * @brief Pops and returns the front event from the queue.
     *
     * Blocks until an event is available or the queue is closed. Safely moves
     * the active event out of the queue using C++ move semantics to prevent
     * copying overhead (such as deep-copying std::string).
     *
     * @return boost::optional wrapping the front event, or boost::none if
     *         the queue is closed.
     */
    boost::optional<EventVariant> pop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_condvar.wait(lock, [this]() {
            return !m_queue.empty() || m_closed;
        });

        if (m_closed)
        {
            return boost::none;
        }

        // Moves the event safely to avoid copies of strings or variants
        EventVariant event = std::move(m_queue.front());
        m_queue.pop();
        return event;
    }

private:
    size_t m_maxSize;
    bool m_closed = false;
    std::queue<EventVariant> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_condvar;
};

#endif
