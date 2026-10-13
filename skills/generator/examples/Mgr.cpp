/**
 * @file Mgr.cpp
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

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/common.hpp>
#include <boost/msm/front/states.hpp>
#include <boost/core/demangle.hpp>
#include <boost/ref.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <typeinfo>
#include "Event.hpp"
#include "EventQueue.hpp"
#include "Mgr.hpp"

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;

namespace
{
    /**
     * @brief Concrete state machine implementation
     */
    struct SM_ : public msm::front::state_machine_def<SM_>
    {
        // Pointer to the currently running Mgr instance
        Mgr* m_currentMgr;

        SM_(Mgr* mgr) : m_currentMgr(mgr) {}

        // Flags
        struct ActivatedFlag {};
        struct IdleFlag {};
        struct ProcessingFlag {};

        // List states
        struct Initial : public msm::front::state<>
        {
        };

        // Active composite state (or submachine in terms of Boost.MSM)
        struct Active_ : public msm::front::state_machine_def<Active_>
        {
            typedef mpl::vector1<ActivatedFlag> flag_list;

            // Pointer to the currently running Mgr instance, set by Mgr::Mgr()
            Mgr* m_currentMgr = nullptr;

            template <class Event, class FSM>
            void on_entry(const Event&, FSM& fsm)
            {
                if (fsm.m_currentMgr)
                {
                    fsm.m_currentMgr->getLogger().info("[Mgr::StateMachine]: entering: Active");
                }
            }

            template <class Event, class FSM>
            void on_exit(const Event&, FSM& fsm)
            {
                if (fsm.m_currentMgr)
                {
                    fsm.m_currentMgr->getLogger().info("[Mgr::StateMachine]: leaving: Active");
                }
            }

            // Substates of Active
            struct Idle : public msm::front::state<>
            {
                typedef mpl::vector1<IdleFlag> flag_list;

                template <class Event, class FSM>
                void on_entry(const Event&, FSM& fsm)
                {
                    if (fsm.m_currentMgr)
                    {
                        fsm.m_currentMgr->getLogger().info("[Mgr::StateMachine]: entering: Idle");
                    }
                }

                template <class Event, class FSM>
                void on_exit(const Event&, FSM& fsm)
                {
                    if (fsm.m_currentMgr)
                    {
                        fsm.m_currentMgr->getLogger().info("[Mgr::StateMachine]: leaving: Idle");
                    }
                }
            };

            struct Processing : public msm::front::state<>
            {
                typedef mpl::vector1<ProcessingFlag> flag_list;

                template <class Event, class FSM>
                void on_entry(const Event&, FSM& fsm)
                {
                    if (fsm.m_currentMgr)
                    {
                        fsm.m_currentMgr->getLogger().info("[Mgr::StateMachine]: entering: Processing");
                    }
                }

                template <class Event, class FSM>
                void on_exit(const Event&, FSM& fsm)
                {
                    if (fsm.m_currentMgr)
                    {
                        fsm.m_currentMgr->getLogger().info("[Mgr::StateMachine]: leaving: Processing");
                    }
                }
            };

            // Transition actions inside Active
            struct startProcess
            {
                template <class Event, class FSM, class SourceState, class TargetState>
                void operator()(Event const&, FSM& fsm, SourceState&, TargetState&)
                {
                    if (fsm.m_currentMgr)
                    {
                        fsm.m_currentMgr->getLogger().info("[Mgr::StateMachine]: startProcess()");
                    }
                }
            };

            struct stopProcess
            {
                template <class Event, class FSM, class SourceState, class TargetState>
                void operator()(Event const&, FSM& fsm, SourceState&, TargetState&)
                {
                    if (fsm.m_currentMgr)
                    {
                        fsm.m_currentMgr->getLogger().info("[Mgr::StateMachine]: stopProcess()");
                    }
                }
            };

            typedef Idle initial_state;

            // Transition table for Active_ composite state
            struct transition_table : mpl::vector<
                //    Start     Event         Next      Action               Guard
                //  +---------+--------------+---------+---------------------+----------------------+
                Row < Idle    , Event::Start , Processing, startProcess        , none                 >,
                Row < Processing, Event::Stop  , Idle    , stopProcess         , none                 >
            > {};
        };

        // back-end for Active
        typedef msm::back::state_machine<Active_> Active;

        // Define the initial state
        typedef Initial initial_state;

        // Define transition actions
        struct initialAction
        {
            template <class Event, class FSM, class SourceState, class TargetState>
            void operator()(Event const&, FSM& fsm, SourceState&, TargetState&)
            {
                if (fsm.m_currentMgr)
                {
                    fsm.m_currentMgr->getLogger().info("[Mgr::StateMachine]: initAction()");
                    fsm.m_currentMgr->m_info.incInitActionCount();
                }
            }
        };

        // Transition table for SM
        struct transition_table : mpl::vector<
            //    Start     Event         Next      Action               Guard
            //  +---------+--------------+---------+---------------------+----------------------+
            Row < Initial , none         , Active  , initialAction       , none                 >
        > {};

        /**
        * @brief Replace the default no-transition response.
        */
        template <class FSM, class Event>
        void no_transition(const Event& e, FSM& fsm, int state)
        {
            if (fsm.m_currentMgr)
            {
                fsm.m_currentMgr->getLogger().info(
                    "[Mgr::StateMachine]: no transition from state " + std::to_string(state) + 
                    " on event " + boost::core::demangle(typeid(e).name())
                );
            }
        }
    };

    // Define the back-end state machine that will be instantiated
    typedef msm::back::state_machine<SM_> SM;
}

Mgr::Mgr(int id, size_t eventQueueCapacity, Log& logger)
    : m_id(id), 
      m_sm(new SM(this)), 
      m_queue(eventQueueCapacity),
      m_logger(logger)
{
    if (eventQueueCapacity > DefaultEventCapacity)
    {
        throw std::invalid_argument(
            "[Mgr]: Requested event queue capacity exceeds DefaultEventCapacity"
        );
    }
    else
    {
        auto sm_backend = std::static_pointer_cast<SM>(m_sm);
        sm_backend->get_state<SM_::Active&>().m_currentMgr = this;
        sm_backend->start();
    }
}

Mgr::~Mgr()
{
    std::cout << "[Mgr]: destructor" << std::endl;
    deactivate();
}

bool
Mgr::start()
{
    return m_queue.push<Event::Start>();
}

bool
Mgr::isActive() const
{
    std::lock_guard<std::mutex> lock(m_smMutex);
    return std::static_pointer_cast<SM>(m_sm)->is_flag_active<SM::ActivatedFlag>();
}

bool
Mgr::isIdle() const
{
    std::lock_guard<std::mutex> lock(m_smMutex);
    return std::static_pointer_cast<SM>(m_sm)->is_flag_active<SM::IdleFlag>();
}

bool
Mgr::isProcessing() const
{
    std::lock_guard<std::mutex> lock(m_smMutex);
    return std::static_pointer_cast<SM>(m_sm)->is_flag_active<SM::ProcessingFlag>();
}

bool
Mgr::waitForProcessing(std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(m_smMutex);
    auto sm = std::static_pointer_cast<SM>(m_sm);

    return m_smChanged.wait_for(lock, timeout, [&sm]() {
        return sm->is_flag_active<SM::ProcessingFlag>();
    });
}

namespace
{
    struct DispatchVisitor : public boost::static_visitor<void>
    {
        std::shared_ptr<void> sm_ptr;

        explicit DispatchVisitor(std::shared_ptr<void> sm) : sm_ptr(sm) {}

        template <typename T> void operator()(const T& concrete_event) const
        {
            std::static_pointer_cast<SM>(sm_ptr)->process_event(concrete_event);
        }
    };
}

void
Mgr::process()
{
    auto event = m_queue.pop();
    if (event)
    {
        dispatch(*event);
    }
}

void
Mgr::dispatch(const Events& event)
{
    {
        std::lock_guard<std::mutex> lock(m_smMutex);
        boost::apply_visitor(DispatchVisitor(m_sm), event);
    }

    m_smChanged.notify_all();
}

Log&
Mgr::getLogger() const
{
    return m_logger;
}

int
Mgr::DebugInfo::getInitActionCount() const
{
    return executedInitActionCount; 
}

void
Mgr::DebugInfo::resetInitActionCount()
{
    executedInitActionCount = 0;
}

void
Mgr::DebugInfo::incInitActionCount()
{
    executedInitActionCount++;
}

void
Mgr::dispatchFromQueue()
{
    while (auto event = m_queue.pop())
    {
        std::cout << "[Mgr]: dispatch event" << std::endl;
        dispatch(*event);
    }
    std::cout << "[Mgr]: terminate" << std::endl;
}

void
Mgr::activate()
{
    if (!m_thread.joinable())
    {
        m_thread = std::thread(&Mgr::dispatchFromQueue, this);
    }
}

void
Mgr::deactivate()
{
    m_queue.close();
    if (m_thread.joinable())
    {
        std::cout << "[Mgr]: joinable" << std::endl;
        m_thread.join();
    }
    std::cout << "[Mgr]: deactivated" << std::endl;
}
