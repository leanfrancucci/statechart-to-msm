/**
 * @file {{NAME}}ActObj.cpp
 * @brief Implements {{NAME}}ActObj, the active object that runs the
 *        {{NAME}} state machine.
 */

#include "{{NAME}}ActObj.hpp"

namespace
{
/**
 * @brief Unwraps a {{NAME}}Events variant and feeds the concrete event to the
 *        state machine.
 */
struct DispatchVisitor : public boost::static_visitor<void>
{
    explicit DispatchVisitor({{NAME}}& sm) : m_sm(sm) {}

    template <typename EventType> void operator()(const EventType& event) const
    {
        m_sm.process_event(event);
    }

    {{NAME}}& m_sm;
};
} // namespace

{{NAME}}ActObj::{{NAME}}ActObj(std::size_t eventQueueCapacity, Log& logger)
    : m_logger(logger), m_sm(logger), m_queue(eventQueueCapacity)
{
    m_sm.start();
}

{{NAME}}ActObj::~{{NAME}}ActObj()
{
    deactivate();
}

void
{{NAME}}ActObj::dispatch(const {{NAME}}Events& event)
{
    {
        std::lock_guard<std::mutex> lock(m_smMutex);
        boost::apply_visitor(DispatchVisitor(m_sm), event);
    }
    m_smChanged.notify_all();
}

bool
{{NAME}}ActObj::process()
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
{{NAME}}ActObj::activate()
{
    if (!m_thread.joinable())
    {
        m_thread = std::thread(&{{NAME}}ActObj::dispatchFromQueue, this);
    }
}

void
{{NAME}}ActObj::deactivate()
{
    m_queue.close();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

Log&
{{NAME}}ActObj::getLogger() const
{
    return m_logger;
}

void
{{NAME}}ActObj::dispatchFromQueue()
{
    while (process())
    {
    }
}
