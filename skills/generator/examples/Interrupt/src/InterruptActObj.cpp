/**
 * @file InterruptActObj.cpp
 * @brief Implements InterruptActObj, the active object that runs the
 *        Interrupt state machine.
 */

#include "InterruptActObj.hpp"

namespace
{
/**
 * @brief Unwraps a InterruptEvents variant and feeds the concrete event to the
 *        state machine.
 */
struct DispatchVisitor : public boost::static_visitor<void>
{
    explicit DispatchVisitor(Interrupt& sm) : m_sm(sm) {}

    template <typename EventType> void operator()(const EventType& event) const
    {
        m_sm.process_event(event);
    }

    Interrupt& m_sm;
};
} // namespace

InterruptActObj::InterruptActObj(std::size_t eventQueueCapacity, Log& logger)
    : m_logger(logger), m_sm(logger), m_queue(eventQueueCapacity)
{
    m_sm.start();
}

InterruptActObj::~InterruptActObj()
{
    deactivate();
}

void
InterruptActObj::dispatch(const InterruptEvents& event)
{
    {
        std::lock_guard<std::mutex> lock(m_smMutex);
        boost::apply_visitor(DispatchVisitor(m_sm), event);
    }
    m_smChanged.notify_all();
}

bool
InterruptActObj::process()
{
    auto event = m_queue.pop();
    if (!event)
    {
        return false;
    }
    dispatch(*event);
    return true;
}

void
InterruptActObj::activate()
{
    if (!m_thread.joinable())
    {
        m_thread = std::thread(&InterruptActObj::dispatchFromQueue, this);
    }
}

void
InterruptActObj::deactivate()
{
    m_queue.close();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

Log&
InterruptActObj::getLogger() const
{
    return m_logger;
}

void
InterruptActObj::dispatchFromQueue()
{
    while (process())
    {
    }
}
