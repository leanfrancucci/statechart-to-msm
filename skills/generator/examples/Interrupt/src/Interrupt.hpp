/**
 * @file Interrupt.hpp
 * @brief Boost.MSM front-end (Interrupt_) and back-end (Interrupt) of the
 *        Interrupt state machine.
 *
 * Implements the topology table in Interrupt.md (diagram interrupt-state-machine.png).
 */

#ifndef INTERRUPT_HPP
#define INTERRUPT_HPP

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
 * @brief Front-end of the Interrupt state machine.
 *
 * States reach the logger through the FSM argument (fsm.m_logger). Inside the
 * composites Operation and Process, fsm is the composite itself, which copies
 * its parent's logger in its on_entry().
 *
 * Operation's default entry is its H pseudo-state, so every entry restores its
 * last active substate: its back-end is msm::back::AlwaysHistory, with Idle as
 * the initial state for the first entry. The history is shallow: Process has
 * no history of its own, so a restored Process restarts at Stage1.
 */
struct Interrupt_ : public msm::front::state_machine_def<Interrupt_>
{
    explicit Interrupt_(Log& logger = getDefaultLogger());

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
    struct ProcessFlag
    {
    };
    struct Stage1Flag
    {
    };
    struct Stage2Flag
    {
    };

    // --- States ---
    struct Paused : public msm::front::state<>
    {
        typedef mpl::vector1<PausedFlag> flag_list;

        template <class Event, class FSM> void on_entry(const Event&, FSM& fsm)
        {
            fsm.m_logger->info("[Interrupt]: suspend()");
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
            fsm.m_logger->info("[Interrupt]: entering: Operation");
        }

        struct Idle : public msm::front::state<>
        {
            typedef mpl::vector1<IdleFlag> flag_list;

            template <class Event, class FSM> void on_entry(const Event&, FSM& fsm)
            {
                fsm.m_logger->info("[Interrupt]: entering: Idle");
            }
        };

        // Composite state Process
        struct Process_ : public msm::front::state_machine_def<Process_>
        {
            Log* m_logger = nullptr;

            typedef mpl::vector1<ProcessFlag> flag_list;

            template <class Event, class FSM> void on_entry(const Event&, FSM& fsm)
            {
                m_logger = fsm.m_logger;
                fsm.m_logger->info("[Interrupt]: entering: Process");
            }

            struct Stage1 : public msm::front::state<>
            {
                typedef mpl::vector1<Stage1Flag> flag_list;

                template <class Event, class FSM> void on_entry(const Event&, FSM& fsm)
                {
                    fsm.m_logger->info("[Interrupt]: entering: Stage1");
                }
            };

            struct Stage2 : public msm::front::state<>
            {
                typedef mpl::vector1<Stage2Flag> flag_list;

                template <class Event, class FSM> void on_entry(const Event&, FSM& fsm)
                {
                    fsm.m_logger->info("[Interrupt]: entering: Stage2");
                }
            };

            typedef Stage1 initial_state;

            // clang-format off
            struct transition_table : mpl::vector<
                //    Start     Event          Next      Action   Guard
                //  +---------+---------------+---------+-------+------+
                Row < Stage1  , Event::Input, Stage2    , none  , none >,
                Row < Stage2  , Event::Input, Stage1    , none  , none >
            > {};
            // clang-format on
        };

        typedef msm::back::state_machine<Process_> Process;

        typedef Idle initial_state;

        // clang-format off
        struct transition_table : mpl::vector<
            //    Start     Event          Next      Action   Guard
            //  +---------+---------------+---------+-------+------+
            Row < Idle    , Event::Start  , Process , none  , none >
        > {};
        // clang-format on
    };

    typedef msm::back::state_machine<Operation_, msm::back::AlwaysHistory> Operation;

    typedef Operation initial_state;

    // --- Transition actions (functors) ---

    // --- Transition table ---
    // clang-format off
    struct transition_table : mpl::vector<
        //    Start       Event              Next        Action    Guard
        //  +-----------+------------------+-----------+---------+---------+
        Row < Operation , Event::Signal    , Paused    , none    , none    >,
        Row < Paused    , Event::Resume    , Operation , none    , none    >
    > {};
    // clang-format on

    /**
     * @brief Logs events that no transition of the active state handles.
     */
    template <class FSM, class Event> void no_transition(const Event& e, FSM&, int state)
    {
        m_logger->info(
            "[Interrupt]: no transition from state " + std::to_string(state) + " on event " +
            boost::core::demangle(typeid(e).name()));
    }
};

// Back-end: the type to instantiate
typedef msm::back::state_machine<Interrupt_> Interrupt;

#endif
