/**
 * @file Urgent.cpp
 * @brief Non-template members of the Urgent state machine front-end.
 *
 * States, actions and guards are templates and live in Urgent.hpp; put
 * out-of-line helpers that they call here.
 */

#include "Urgent.hpp"

Urgent_::Urgent_(Log& logger) : m_logger(&logger) {}
