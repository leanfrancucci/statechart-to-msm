/**
 * @file UrgentActObj.cpp
 * @brief Implements UrgentActObj, the active object that runs the
 *        Urgent state machine.
 */

#include "UrgentActObj.hpp"

namespace
{
/**
 * @brief Unwraps a UrgentEvents variant and feeds the concrete event to the
 *        state machine.
 */
struct DispatchVisitor : public boost::static_visitor<void>
{
    explicit DispatchVisitor(Urgent& sm) : m_sm(sm) {}

    template <typename EventType> void operator()(const EventType& event) const
    {
        m_sm.process_event(event);
    }

    Urgent& m_sm;
};
} // namespace

UrgentActObj::UrgentActObj(std::size_t eventQueueCapacity, Log& logger)
    : m_logger(logger), m_sm(logger), m_queue(eventQueueCapacity)
{
    m_sm.start();
}

UrgentActObj::~UrgentActObj()
{
    deactivate();
}

void
UrgentActObj::dispatch(const UrgentEvents& event)
{
    {
        std::lock_guard<std::mutex> lock(m_smMutex);
        boost::apply_visitor(DispatchVisitor(m_sm), event);
    }
    m_smChanged.notify_all();
}

bool
UrgentActObj::process()
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
UrgentActObj::activate()
{
    if (!m_thread.joinable())
    {
        m_thread = std::thread(&UrgentActObj::dispatchFromQueue, this);
    }
}

void
UrgentActObj::deactivate()
{
    m_queue.close();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

Log&
UrgentActObj::getLogger() const
{
    return m_logger;
}

void
UrgentActObj::dispatchFromQueue()
{
    while (process())
    {
    }
}
