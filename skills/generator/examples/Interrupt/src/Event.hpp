/**
 * @file Event.hpp
 * @brief Events (triggers) accepted by the Interrupt state machine.
 *
 * One struct per transition trigger of the state diagram. Add data members
 * when an event carries parameters that guards or actions need, and keep a
 * default constructor if the event is posted from main.cpp.
 */

#ifndef SIMPLE_STATE_EVENT_HPP
#define SIMPLE_STATE_EVENT_HPP

#include <string>
#include <utility>

namespace Event
{
struct Start
{
};

struct Signal
{
};

struct Resume
{
};

struct Input
{
};
} // namespace Event

#endif
