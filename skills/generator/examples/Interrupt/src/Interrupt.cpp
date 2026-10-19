/**
 * @file Interrupt.cpp
 * @brief Non-template members of the Interrupt state machine front-end.
 *
 * States, actions and guards are templates and live in Interrupt.hpp; put
 * out-of-line helpers that they call here.
 */

#include "Interrupt.hpp"

Interrupt_::Interrupt_(Log& logger) : m_logger(&logger) {}
