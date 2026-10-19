/**
 * @file EventQueue.hpp
 * @brief Bounded, thread-safe queue of variant events for state machines.
 */

#ifndef SIMPLE_STATE_EVENTQUEUE_HPP
#define SIMPLE_STATE_EVENTQUEUE_HPP

#include <boost/optional.hpp>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <utility>

template <typename EventVariant> class EventQueue
{
  public:
    // Default capacity limit when none is specified
    static constexpr std::size_t DefaultMaxCapacity = 32;

    /**
     * @brief Constructs an EventQueue with a specified maximum capacity.
     *
     * @param maxSize Maximum allowed number of elements in the queue.
     * @throws std::invalid_argument if maxSize is zero or exceeds
     *         DefaultMaxCapacity.
     */
    explicit EventQueue(std::size_t maxSize = DefaultMaxCapacity) : m_maxSize(maxSize)
    {
        if ((m_maxSize == 0) || (m_maxSize > DefaultMaxCapacity))
        {
            throw std::invalid_argument("EventQueue: capacity out of range");
        }
    }

    EventQueue(const EventQueue&) = delete;
    EventQueue& operator=(const EventQueue&) = delete;

    /**
     * @brief Enqueues a new event constructed in place if capacity permits.
     *
     * @tparam EventType Concrete type of event to create and push.
     * @tparam Args Variadic argument types for the EventType constructor.
     * @param args Arguments forwarded to the EventType constructor.
     * @return true if pushed, false if the queue is full or closed.
     */
    template <typename EventType, typename... Args> bool push(Args&&... args)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_closed || (m_queue.size() >= m_maxSize))
            {
                return false;
            }
            m_queue.push(EventType(std::forward<Args>(args)...));
        }
        m_condvar.notify_one();
        return true;
    }

    /**
     * @brief Checks if the queue is empty.
     */
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    /**
     * @brief Gets the number of events currently in the queue.
     */
    std::size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
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
     * @brief Pops the front event, blocking until one is available or the
     *        queue is closed.
     *
     * @return The front event, or boost::none if the queue is closed.
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

        EventVariant event = std::move(m_queue.front());
        m_queue.pop();
        return event;
    }

  private:
    std::size_t m_maxSize;
    bool m_closed = false;
    std::queue<EventVariant> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_condvar;
};

#endif
