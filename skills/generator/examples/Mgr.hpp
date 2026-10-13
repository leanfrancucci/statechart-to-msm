/**
 * @file Mgr.hpp
 * @brief Declares Mgr, an active object whose behavior is a Boost.MSM
 *        hierarchical state machine fed by a thread-safe event queue.
 *
 * Mgr can be driven in two ways:
 * - As an active object: activate() starts a dispatcher thread that pops
 *   events posted with post() or start() and dispatches them to the state
 *   machine; deactivate() stops it and discards pending events.
 * - From an external event loop: process() pops and dispatches one queued
 *   event, and dispatch() bypasses the queue entirely.
 *
 * The Initial state is a workaround to execute an action before entering the
 * Active composite state. The model intends an
 * initial transition straight into Active with the effect initAction(), but
 * Boost.MSM's initial_state cannot carry an action. Instead, the state
 * machine's initial transition targets the simple Initial state, and
 * Initial's anonymous (eventless) transition to Active runs initAction().
 * In UML terms that is a regular completion transition, not the initial
 * transition of the state machine. It fires as soon as start() enters
 * Initial, so Initial is only occupied transiently.
 *
 * The state machine definition is private to Mgr.cpp, so clients see only
 * the events (Events), the posting API and the state queries. Posting,
 * dispatching and querying states are thread-safe.
 */

#ifndef __MGR_HPP__
#define __MGR_HPP__

#include <boost/shared_ptr.hpp>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include "Event.hpp"
#include "EventQueue.hpp"
#include "Log.hpp"

// Define all the events that are supported by the event processor
using Events = boost::variant<Event::Start, Event::Stop>;

/**
 * @class Mgr
 * @brief Manages operational states and dispatches using an internal
 *        hierarchical state machine.
 */
class Mgr
{
  public:
    // Default queue capacity limit coupled to EventQueue max capacity
    static constexpr size_t DefaultEventCapacity = EventQueue<Events>::DefaultMaxCapacity;
    // Default id
    static constexpr int DefaultId = 8;

    /**
      * @brief Constructs a new Mgr instance.
      *
      * @param id Identifier for the manager instance.
      * @param eventQueueCapacity Initial capacity of the event queue.
      * @param logger Reference to the Log dependency.
      */
    Mgr(int id = DefaultId, size_t eventQueueCapacity = DefaultEventCapacity,
        Log& logger = getDefaultLogger());

    /**
      * @brief Destructor for Mgr. Stops the dispatcher thread, if running.
      */
    virtual ~Mgr();

    /**
      * @brief Enqueues the Start event onto the event queue.
      *
      * @return true if the event was successfully enqueued, false if the 
      *         event queue is full.
      */
    bool start();

    /**
      * @brief Checks if the manager is currently in the Active composite state.
      *
      * @return true if Active is the currently active state, false otherwise.
      */
    bool isActive() const;

    /**
      * @brief Checks if the manager is currently in the Idle substate.
      *
      * @return true if Idle is the currently active state, false otherwise.
      */
    bool isIdle() const;

    /**
      * @brief Checks if the manager is currently in the Processing substate.
      *
      * @return true if Processing is the currently active state, false otherwise.
      */
    bool isProcessing() const;

    /**
     * @brief Blocks until the manager reaches the Processing substate.
     *
     * Safe to call from any thread while the dispatcher thread is running.
     *
     * @param timeout Maximum time to wait.
     * @return true if Processing was reached, false if the timeout expired.
     */
    bool waitForProcessing(std::chrono::milliseconds timeout);

    /**
     * @brief Pops and processes the next pending event from the queue.
     *
     * @note It must be called from one single thread.
     */
    void process();

    /**
     * @brief Dispatches an event directly to the state machine.
     *
     * Bypasses the internal queue to trigger state transitions immediately.
     * Ideal when Mgr runs within an external event loop (such as a Qt thread)
     * or unit tests.
     *
     * @param event The variant event containing the concrete event to dispatch.
     */
    void dispatch(const Events& event);

    /**
     * @brief Starts the dispatcher thread, which pops events from the queue
     *        and dispatches them to the state machine.
     *
     * Calling it while the thread is already running has no effect.
     */
    void activate();

    /**
     * @brief Closes the event queue and waits for the dispatcher thread.
     *
     * Events still pending in the queue are discarded; an event being
     * dispatched when this is called completes first. Further calls to start()
     * or post() return false.
     */
    void deactivate();

    /**
      * @brief Gets a reference to the injected logger.
      * @return Reference to the Log instance.
      */
    Log& getLogger() const;

    /**
      * @class DebugInfo
      * @brief Helper class to debug and test the state machine.
      */
    class DebugInfo
    {
      public:
        /**
          * @brief Gets the number of times initAction was executed.
          *
          * @return The initial transition action count.
          */
        int getInitActionCount() const;

        /**
          * @brief Resets the initial transition action counter to zero.
          */
        void resetInitActionCount();

        /**
          * @brief Increments the initial transition action counter.
          */
        void incInitActionCount();

      private:
        int executedInitActionCount = 0;
    };

    /**
     * @brief Enqueues a new event, constructed in place, for the dispatcher
     *        thread to process.
     *
     * Thread-safe: it may be called from any thread. The event is dispatched
     * asynchronously once activate() has been called; use waitForProcessing()
     * to wait for its effect on the state machine.
     *
     * @tparam EventType Concrete event type; must be one of the Events
     *         alternatives.
     * @tparam Args Variadic argument types for the EventType constructor.
     * @param args Arguments forwarded to the EventType constructor.
     * @return true if the event was enqueued, false if the event queue is full
     *         or has been closed by deactivate().
     */
    template <typename EventType, typename... Args>
    bool post(Args&&... args)
    {
        return m_queue.push<EventType>(std::forward<Args>(args)...);
    }

    DebugInfo m_info;

  private:
    int m_id;
    std::shared_ptr<void> m_sm;
    EventQueue<Events> m_queue;
    Log& m_logger;
    // Serializes state machine access between the dispatcher thread and
    // callers of the state queries
    mutable std::mutex m_smMutex;
    // Notified after every dispatched event
    std::condition_variable m_smChanged;
    std::thread m_thread;

    /**
     * @brief Body of the dispatcher thread started by activate().
     *
     * Blocks on the event queue and dispatches each popped event to the state
     * machine. Returns when deactivate() closes the queue; events still
     * pending at that point are discarded.
     */
    void dispatchFromQueue();
};

#endif
