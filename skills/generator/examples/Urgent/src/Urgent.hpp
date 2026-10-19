/**
 * @file Urgent.hpp
 * @brief Boost.MSM front-end (Urgent_) and back-end (Urgent) of the
 *        Urgent state machine.
 *
 * Implements the topology table in Urgent.md (diagram urgent-state-machine.png).
 */

#ifndef URGENT_HPP
#define URGENT_HPP

#include "Event.hpp"
#include "Log.hpp"
#include <boost/core/demangle.hpp>
#include <boost/mpl/vector/aux_/vector0.hpp>
#include <boost/msm/back/history_policies.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/states.hpp>
#include <string>
#include <typeinfo>

namespace msm = boost::msm;
namespace mpl = boost::mpl;

/**
 * @brief Front-end of the Urgent state machine.
 *
 * States reach the logger through the FSM argument (fsm.m_logger). Inside the
 * composite Operation, fsm is Operation itself, which copies the parent's
 * logger in its on_entry().
 *
 * Operation has a shallow history that only Resume activates: its back-end is
 * msm::back::ShallowHistory<mpl::vector<Event::Resume>>. Resume, which targets
 * the H pseudo-state, restores Operation's last active substate; Reset, which
 * targets Operation itself, restarts it at its initial state Idle.
 */
struct Urgent_ : public msm::front::state_machine_def<Urgent_>
{
    explicit Urgent_(Log& logger = getDefaultLogger());

    Log* m_logger;

    // Shorthands for the transition tables, visible to nested composites too
    typedef msm::front::none none;
    template <class Source, class Event, class Target, class Action = none, class Guard = none>
    using Row = msm::front::Row<Source, Event, Target, Action, Guard>;

    // --- Flags: one per stable state ---
    struct PausedFlag
    {
    };
    struct OperationFlag
    {
    };
    struct IdleFlag
    {
    };
    struct ProcessingFlag
    {
    };

    // --- States ---
    struct Paused : public msm::front::state<>
    {
        typedef mpl::vector1<PausedFlag> flag_list;

        template <class Event, class FSM> void on_entry(const Event&, FSM& fsm)
        {
            fsm.m_logger->info("[Urgent]: suspend()");
        }
    };

    // Composite state Operation
    struct Operation_ : public msm::front::state_machine_def<Operation_>
    {
        Log* m_logger = nullptr;

        typedef mpl::vector1<OperationFlag> flag_list;

        template <class Event, class FSM> void on_entry(const Event&, FSM& fsm)
        {
            m_logger = fsm.m_logger;
            fsm.m_logger->info("[Urgent]: entering: Operation");
        }

        struct Idle : public msm::front::state<>
        {
            typedef mpl::vector1<IdleFlag> flag_list;

            template <class Event, class FSM> void on_entry(const Event&, FSM& fsm)
            {
                fsm.m_logger->info("[Urgent]: entering: Idle");
            }
        };

        struct Processing : public msm::front::state<>
        {
            typedef mpl::vector1<ProcessingFlag> flag_list;

            template <class Event, class FSM> void on_entry(const Event&, FSM& fsm)
            {
                fsm.m_logger->info("[Urgent]: entering: Processing");
            }
        };

        typedef Idle initial_state;

        // clang-format off
        struct transition_table : mpl::vector<
            //    Start       Event           Next         Action  Guard
            //  +-----------+---------------+------------+-------+-------+
            Row < Idle      , Event::Start  , Processing , none  , none  >,
            Row < Processing, Event::Stop   , Idle       , none  , none  >
        > {};
        // clang-format on
    };

    typedef msm::back::
        state_machine<Operation_, msm::back::ShallowHistory<mpl::vector<Event::Resume>>>
            Operation;

    typedef Operation initial_state;

    // --- Transition actions (functors) ---

    // --- Transition table ---
    // clang-format off
    struct transition_table : mpl::vector<
        //    Start       Event              Next        Action    Guard
        //  +-----------+------------------+-----------+---------+---------+
        Row < Operation , Event::Signal    , Paused    , none    , none    >,
        Row < Paused    , Event::Resume    , Operation , none    , none    >,
        Row < Paused    , Event::Reset     , Operation , none    , none    >
    > {};
    // clang-format on

    /**
     * @brief Logs events that no transition of the active state handles.
     */
    template <class FSM, class Event> void no_transition(const Event& e, FSM&, int state)
    {
        m_logger->info(
            "[Urgent]: no transition from state " + std::to_string(state) + " on event " +
            boost::core::demangle(typeid(e).name()));
    }
};

// Back-end: the type to instantiate
typedef msm::back::state_machine<Urgent_> Urgent;

#endif
