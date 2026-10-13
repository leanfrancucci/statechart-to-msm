/**
 * @file Event.hpp
 * @brief Events (triggers) accepted by the {{NAME}} state machine.
 *
 * One struct per transition trigger of the state diagram. Add data members
 * when an event carries parameters that guards or actions need, and keep a
 * default constructor if the event is posted from main.cpp.
 */

#ifndef {{NAME_UPPER}}_EVENT_HPP
#define {{NAME_UPPER}}_EVENT_HPP

namespace Event
{
{{EVENT_STRUCTS}}
} // namespace Event

#endif
