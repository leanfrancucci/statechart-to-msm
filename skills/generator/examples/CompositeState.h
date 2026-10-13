/**
 * @file CompositeState.h
 * @brief Simple State Machine Example using Boost.MSM.
 */

#ifndef __COMPOSITESTATE_HPP__
#define __COMPOSITESTATE_HPP__

#include <iostream>
#include <typeinfo>
#include <boost/core/demangle.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/common.hpp>
#include <boost/msm/front/states.hpp>
#include <ostream>

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;

/**
 * @brief Hierarchical State Machine representing the S1 composite state and
 * substate S11.
 */
struct CompositeState_ : public msm::front::state_machine_def<CompositeState_>
{
    // Flags, one per state, to query the active state configuration with
    // is_flag_active<>() instead of relying on state ids
    struct S1Flag {};
    struct S11Flag {};

    // --- COMPOSITE STATE (S1) DEFINITION ---
    struct S1_ : public msm::front::state_machine_def<S1_>
    {
        // Tracking/Spy variables for behavioral checking
        int onEntryCount = 0;

        typedef mpl::vector1<S1Flag> flag_list;

        template <class Event, class FSM>
        void on_entry(const Event&, FSM&)
        {
            ++onEntryCount;
            std::cout << "entering: S1" << std::endl;
        }

        template <class Event, class FSM>
        void on_exit(const Event&, FSM&)
        {
            std::cout << "leaving: S1" << std::endl;
        }

        // --- SUBSTATE (S11) DEFINITION ---
        struct S11 : public msm::front::state<>
        {
            typedef mpl::vector1<S11Flag> flag_list;

            template <class Event, class FSM>
            void on_entry(const Event&, FSM&)
            {
                std::cout << "entering: S11" << std::endl;
            }
            template <class Event, class FSM>
            void on_exit(const Event&, FSM&)
            {
                std::cout << "leaving: S11" << std::endl;
            }
        };

        // Define S1's initial nested state
        typedef S11 initial_state;

        // Transition table for submachine S1 (currently empty)
        // clang-format off
        struct transition_table : mpl::vector<
            //    Start     Event         Next      Action               Guard
            //  +---------+--------------+---------+---------------------+----------------------+
        > {};
        // clang-format on
    };

    // Instantiate S1 as a back-end submachine inside the outer front-end
    typedef msm::back::state_machine<S1_> S1;

    // Define the outer state machine's initial state
    typedef S1 initial_state;

    // Transition table for the CompositeState (currently empty)
    // clang-format off
    struct transition_table : mpl::vector<
        //    Start     Event         Next      Action               Guard
        //  +---------+--------------+---------+---------------------+----------------------+
    > {};
    // clang-format on

    template <class FSM, class Event>
    void no_transition(const Event& e, FSM&, int state)
    {
        std::cout << "no transition from state " << state << " on event "
                  << boost::core::demangle(typeid(e).name()) << std::endl;
    }
};

// Define the back-end state machine that will be instantiated in the test
typedef msm::back::state_machine<CompositeState_> CompositeState;

#endif
