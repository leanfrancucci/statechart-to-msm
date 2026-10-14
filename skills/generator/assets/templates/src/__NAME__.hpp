/**
 * @file {{NAME}}.hpp
 * @brief Boost.MSM front-end ({{NAME}}_) and back-end ({{NAME}}) of the
 *        {{NAME}} state machine.
 *
 * Scaffolded skeleton: replace every TODO with the states, actions, guards
 * and transition rows of the UML topology table. It compiles as is, with a
 * single placeholder state (Idle) and an empty transition table.
 */

#ifndef {{NAME_UPPER}}_HPP
#define {{NAME_UPPER}}_HPP

#include "Event.hpp"
#include "Log.hpp"
#include <boost/core/demangle.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/states.hpp>
#include <string>
#include <typeinfo>

namespace msm = boost::msm;
namespace mpl = boost::mpl;

/**
 * @brief Front-end of the {{NAME}} state machine.
 *
 * States, actions and guards reach the logger through the FSM argument
 * (fsm.m_logger). Inside a composite state, fsm is the composite itself, so
 * give its front-end a Log* m_logger member and copy the parent's logger in
 * its on_entry(), which runs before any of its substates is entered:
 *
 *     template <class Event, class FSM> void on_entry(const Event&, FSM& fsm)
 *     {
 *         m_logger = fsm.m_logger;
 *     }
 */
struct {{NAME}}_ : public msm::front::state_machine_def<{{NAME}}_>
{
    explicit {{NAME}}_(Log& logger = getDefaultLogger());

    Log* m_logger;

    // Shorthands for the transition tables, visible to nested composites too
    typedef msm::front::none none;
    // Action of a Row that defers its event; the machine whose table uses it
    // also needs: typedef int activate_deferred_events;
    typedef msm::front::Defer Defer;
    template <class Source, class Event, class Target, class Action = none, class Guard = none>
    using Row = msm::front::Row<Source, Event, Target, Action, Guard>;

    // --- Flags: one per state the active object or the tests must query ---
    struct IdleFlag
    {
    };

    // --- States ---
    // TODO: replace the Idle placeholder with the states of the topology table
    struct Idle : public msm::front::state<>
    {
        typedef mpl::vector1<IdleFlag> flag_list;

        template <class Event, class FSM> void on_entry(const Event&, FSM& fsm)
        {
            fsm.m_logger->info("[{{NAME}}]: entering: Idle");
        }

        template <class Event, class FSM> void on_exit(const Event&, FSM& fsm)
        {
            fsm.m_logger->info("[{{NAME}}]: leaving: Idle");
        }
    };

    typedef Idle initial_state;

    // --- Transition actions (functors) ---
    // TODO: one functor per Action Effect of the topology table, e.g.:
    //
    // struct startProcess
    // {
    //     template <class Event, class FSM, class SourceState, class TargetState>
    //     void
    //     operator()(const Event&, FSM& fsm, SourceState&, TargetState&)
    //     {
    //         fsm.m_logger->info("[{{NAME}}]: startProcess()");
    //     }
    // };

    // --- Guards (functors returning bool) ---
    // TODO: one functor per Guard of the topology table

    // --- Transition table ---
    // TODO: one Row per transition of the topology table, e.g.:
    //   Row < Idle    , Event::Start , Running , startProcess        , none                 >,
    // clang-format off
    struct transition_table : mpl::vector<
        //    Start     Event         Next      Action               Guard
        //  +---------+--------------+---------+---------------------+----------------------+
    > {};
    // clang-format on

    /**
     * @brief Logs events that no transition of the active state handles.
     */
    template <class FSM, class Event> void no_transition(const Event& e, FSM&, int state)
    {
        m_logger->info(
            "[{{NAME}}]: no transition from state " + std::to_string(state) + " on event " +
            boost::core::demangle(typeid(e).name()));
    }
};

// Back-end: the type to instantiate
typedef msm::back::state_machine<{{NAME}}_> {{NAME}};

#endif
