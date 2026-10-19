/**
 * @file InterruptActObj.hpp
 * @brief Declares InterruptActObj, an active object whose behavior is the
 *        Interrupt state machine, fed by a thread-safe event queue.
 *
 * InterruptActObj can be driven in two ways:
 * - As an active object: activate() starts a dispatcher thread that pops the
 *   events enqueued with post() and dispatches them to the state machine;
 *   deactivate() stops it and discards pending events.
 * - From an external event loop or a unit test: process() pops and
 *   dispatches one queued event, and dispatch() bypasses the queue.
 *
 * Every dispatch holds an internal mutex, so actions and guards must not
 * call back into the state queries of this object or they deadlock.
 */

#ifndef INTERRUPT_ACTOBJ_HPP
#define INTERRUPT_ACTOBJ_HPP

#include "Interrupt.hpp"

#include "Event.hpp"
#include "EventQueue.hpp"
#include "Log.hpp"
#include <boost/blank.hpp>
#include <boost/variant.hpp>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <thread>
#include <utility>

// Every event the state machine accepts; keep in sync with Event.hpp.
// boost::blank stands in when the state machine has no events.
using InterruptEvents = boost::variant<Event::Start, Event::Signal, Event::Resume, Event::Input>;

/**
 * @class InterruptActObj
 * @brief Active object that owns and serializes access to a Interrupt state
 *        machine.
 */
class InterruptActObj
{
  public:
    typedef EventQueue<InterruptEvents> Queue;

    static constexpr std::size_t DefaultEventCapacity = Queue::DefaultMaxCapacity;

    /**
     * @brief Constructs the active object and starts the state machine, which
     *        runs its initial transition.
     *
     * @param eventQueueCapacity Maximum number of pending events.
     * @param logger Logger injected into the state machine.
     */
    explicit InterruptActObj(
        std::size_t eventQueueCapacity = DefaultEventCapacity, Log& logger = getDefaultLogger());

    /**
     * @brief Stops the dispatcher thread, if running.
     */
    ~InterruptActObj();

    InterruptActObj(const InterruptActObj&) = delete;
    InterruptActObj& operator=(const InterruptActObj&) = delete;

    /**
     * @brief Enqueues a new event, constructed in place.
     *
     * Thread-safe. The event is dispatched asynchronously once activate() has
     * been called; use waitForFlag() to wait for its effect.
     *
     * @tparam EventType One of the InterruptEvents alternatives.
     * @param args Arguments forwarded to the EventType constructor.
     * @return true if enqueued, false if the queue is full or closed.
     */
    template <typename EventType, typename... Args> bool post(Args&&... args)
    {
        return m_queue.push<EventType>(std::forward<Args>(args)...);
    }

    /**
     * @brief Dispatches an event directly to the state machine, bypassing the
     *        queue. Thread-safe.
     */
    void dispatch(const InterruptEvents& event);

    /**
     * @brief Pops the next queued event, blocking until one is available, and
     *        dispatches it.
     *
     * Do not mix with activate(): only one thread may consume the queue.
     *
     * @return false if the queue was closed by deactivate().
     */
    bool process();

    /**
     * @brief Starts the dispatcher thread. No effect if already running.
     */
    void activate();

    /**
     * @brief Closes the event queue and joins the dispatcher thread.
     *
     * Pending events are discarded; an event being dispatched completes
     * first. Further calls to post() return false.
     */
    void deactivate();

    /**
     * @brief Checks whether a state carrying Flag is active. Thread-safe.
     *
     * @tparam Flag A flag type declared in Interrupt_ (e.g. Interrupt_::IdleFlag).
     */
    template <class Flag> bool isFlagActive() const
    {
        std::lock_guard<std::mutex> lock(m_smMutex);
        return m_sm.is_flag_active<Flag>();
    }

    /**
     * @brief Blocks until a state carrying Flag is active.
     *
     * @tparam Flag A flag type declared in Interrupt_.
     * @param timeout Maximum time to wait.
     * @return true if the flag became active, false on timeout.
     */
    template <class Flag> bool waitForFlag(std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(m_smMutex);
        return m_smChanged.wait_for(lock, timeout, [this]() {
            return m_sm.is_flag_active<Flag>();
        });
    }

    /**
     * @brief Gets the injected logger.
     */
    Log& getLogger() const;

  private:
    Log& m_logger;
    Interrupt m_sm;
    Queue m_queue;
    // Serializes state machine access between the dispatcher thread and the
    // callers of dispatch() and the state queries
    mutable std::mutex m_smMutex;
    // Notified after every dispatched event
    std::condition_variable m_smChanged;
    std::thread m_thread;

    void dispatchFromQueue();
};

#endif
