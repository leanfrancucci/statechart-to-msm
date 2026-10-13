/**
 * @file SimpleState.h
 * @brief Simple State Machine Example using Boost.MSM
 */

#ifndef __SIMPLESTATE_H__
#define __SIMPLESTATE_H__

#include <iostream>
#include <typeinfo>
#include <boost/core/demangle.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/common.hpp>
#include <boost/msm/front/states.hpp>
#include <ostream>
#include "Event.hpp"

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;

/**
 * @brief Define the state machine structure.
 *
 * @notes States are defined using the basic front-end, transitions use functors.
 */
struct SimpleState_ : public msm::front::state_machine_def<SimpleState_>
{
    typedef int activate_deferred_events;

    // State Machine Attributes
    int m_x = 0;

    // Tracking/Spy variables for behavioral checking and test assertions
    int actionACalled = 0;
    int actionBCalled = 0;
    int actionCCalled = 0;
    int actionECalled = 0;
    int actionIntS4Called = 0;
    int calculateXCalled = 0;
    int xIsZeroCalled = 0;
    int xIsNotZeroCalled = 0;

    // Flags, one per stable state, to query the active state configuration
    // with is_flag_active<>() instead of relying on state ids
    struct S1Flag {};
    struct S2Flag {};
    struct S3Flag {};
    struct S4Flag {};
    struct S41Flag {};

    // List states
    struct S1 : public msm::front::state<>
    {
        // Tracking/Spy variables for behavioral checking
        int onEntryCount = 0;

        typedef mpl::vector1<S1Flag> flag_list;

        // If the EventE event arrives in this state, defer it until a state
        // handles it or rejects it
        typedef mpl::vector<Event::EventE> deferred_events;

        template <class Event, class FSM>
        void on_entry(const Event& event, FSM& fsm)
        {
            ++onEntryCount;
        }

        template <class Event, class FSM>
        void on_exit(const Event& event, FSM& fsm)
        {
        }
    };

    struct S2 : public msm::front::state<>
    {
        typedef mpl::vector1<S2Flag> flag_list;

        // If the EventE event arrives in this state, defer it until a state
        // handles it or rejects it
        typedef mpl::vector<Event::EventE> deferred_events;
    };

    struct S3 : public msm::front::state<>
    {
        typedef mpl::vector1<S3Flag> flag_list;
    };

    // Represents a dynamic branching state based on guards.
    struct Choice1 : public msm::front::state<>
    {
    };

    // Composite state (or submachine in terms of Boost)
    struct S4_ : public msm::front::state_machine_def<S4_>
    {
        // Tracking/Spy variables for behavioral checking and test assertions
        int actionACalled = 0;

        typedef mpl::vector1<S4Flag> flag_list;

        template <class Event, class FSM>
        void on_entry(const Event&, FSM&)
        {
        }
        template <class Event, class FSM>
        void on_exit(const Event&, FSM&)
        {
        }

        struct S41 : public msm::front::state<>,
                     public msm::front::explicit_entry<0>
        {
            typedef mpl::vector1<S41Flag> flag_list;
        };

        struct Exit1 : public msm::front::exit_pseudo_state<Event::EventC> 
        {
            template <class Event, class FSM>
            void on_entry(const Event&, FSM&)
            {
                std::cout << "entering: S4::Exit1" << std::endl;
            }
            template <class Event, class FSM>
            void on_exit(const Event&, FSM&)
            {
                std::cout << "leaving: S4::Exit1" << std::endl;
            }
        };

        struct Exit2 : public msm::front::exit_pseudo_state<Event::EventA> 
        {
            template <class Event, class FSM>
            void on_entry(const Event&, FSM&)
            {
                std::cout << "entering: S4::Exit2" << std::endl;
            }
            template <class Event, class FSM>
            void on_exit(const Event&, FSM&)
            {
                std::cout << "leaving: S4::Exit2" << std::endl;
            }
        };

        typedef S41 initial_state;

        // Transition table for S4
        struct transition_table : mpl::vector<
        //    Start     Event         Next      Action               Guard
        //  +---------+--------------+---------+---------------------+----------------------+
        Row < S41     , Event::EventC, Exit1   , none                , none                 >,
        Row < S41     , Event::EventA, Exit2   , none                , none                 >
        //  +---------+--------------+---------+---------------------+----------------------+
        > {};
    };

    // back-end for composite state
    typedef msm::back::state_machine<S4_> S4;

    // Define the initial state
    typedef S1 initial_state;

    // Define transition actions
    struct s1_EventAEff
    {
        template <class Event, class FSM, class SourceState, class TargetState>
        void operator()(
            Event const& event, FSM& fsm, SourceState& st, TargetState& ts)
        {
            fsm.actionACalled++;
        }
    };

    struct s1_EventBEff
    {
        template <class Event, class FSM, class SourceState, class TargetState>
        void operator()(
            Event const& event, FSM& fsm, SourceState& st, TargetState& ts)
        {
            fsm.actionBCalled++;
        }
    };

    struct s12_eff
    {
        template <class Event, class FSM, class SourceState, class TargetState>
        void operator()(
            Event const& event, FSM& fsm, SourceState& st, TargetState& ts)
        {
            fsm.actionCCalled++;
        }
    };

    struct calculateX
    {
        template <class Event, class FSM, class SourceState, class TargetState>
        void operator()(
            Event const& event, FSM& fsm, SourceState& st, TargetState& ts)
        {
            fsm.calculateXCalled++;
        }
    };

    struct xIsZero
    {
        template <class Event, class FSM, class SourceState, class TargetState>
        void operator()(
            Event const& event, FSM& fsm, SourceState& st, TargetState& ts)
        {
            fsm.xIsZeroCalled++;
        }
    };

    struct xIsNotZero
    {
        template <class Event, class FSM, class SourceState, class TargetState>
        void operator()(
            Event const& event, FSM& fsm, SourceState& st, TargetState& ts)
        {
            fsm.xIsNotZeroCalled++;
        }
    };

    struct s4_EventEEff
    {
        template <class Event, class FSM, class SourceState, class TargetState>
        void operator()(
            Event const& event, FSM& fsm, SourceState& st, TargetState& ts)
        {
            fsm.actionIntS4Called++;
        }
    };

    struct s32_eff
    {
        template <class Event, class FSM, class SourceState, class TargetState>
        void operator()(
            Event const& event, FSM& fsm, SourceState& st, TargetState& ts)
        {
            fsm.actionECalled++;
        }
    };

    struct s4FinalAction
    {
        template <class Event, class FSM, class SourceState, class TargetState>
        void operator()(
            Event const& event, FSM& fsm, SourceState& st, TargetState& ts)
        {
        }
    };

    // Define guard conditions
    struct isValidEvent
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(EVT const& evt, FSM& fsm, SourceState& src, TargetState& tgt)
        {
            return ((evt.m_discType == 8) and (evt.m_name == "diskA"));
        }
    };

    struct isXZero
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(EVT const& evt, FSM& fsm, SourceState& src, TargetState& tgt)
        {
            return (fsm.m_x == 0);
        }
    };

    struct elseBranch
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(EVT const& evt, FSM& fsm, SourceState& src, TargetState& tgt)
        {
            return (fsm.m_x != 0);
        }
    };

    // Transition table for SimpleState
    struct transition_table : mpl::vector<
    //    Start     Event         Next      Action               Guard
    //  +---------+--------------+---------+---------------------+----------------------+
    Row < S1      , Event::EventA, none    , s1_EventAEff        , none                 >,
    Row < S1      , Event::EventB, none    , s1_EventBEff        , isValidEvent         >,
    Row < S1      , Event::EventC, S2      , s12_eff             , none                 >,
    Row < S1      , Event::EventD, S4      , none                , none                 >,
    Row < S1      , Event::EventE, none    , Defer               , none                 >,
    Row < S2      , Event::EventA, S3      , none                , none                 >,
    Row < S2      , Event::EventE, none    , Defer               , none                 >,
    Row < S3      , Event::EventA, Choice1 , calculateX          , none                 >,
    Row < S3      , Event::EventE, S2      , s32_eff             , none                 >,
    Row < S3      , Event::EventC, S4::direct< S4_::S41>
                                           , none                , none                 >,
    Row < Choice1 , none         , S3      , xIsZero             , isXZero              >,
    Row < Choice1 , none         , S3      , xIsNotZero          , elseBranch           >,
    Row < S4      , Event::EventE, none    , s4_EventEEff        , none                 >,
    Row < S4      , Event::EventD, S1      , none                , none                 >,
    Row < S4::exit_pt <S4_::Exit1>      
                  , Event::EventC, S1      , none                , none                 >,
    Row < S4::exit_pt <S4_::Exit2>      
                  , Event::EventA, S2      , s4FinalAction       , none                 >
    //  +---------+--------------+---------+---------------------+----------------------+
    > {};

    /**
    * @brief Replace the default no-transition response.
    */
    template <class FSM, class Event>
    void no_transition(const Event& e, FSM& fsm, int state)
    {
        std::cout << "no transition from state " << state << " on event "
            << boost::core::demangle(typeid(e).name()) << std::endl;
    }
};

// Pick a back-end
typedef msm::back::state_machine<SimpleState_> SimpleState;

#endif
