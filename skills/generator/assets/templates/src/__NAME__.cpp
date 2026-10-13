/**
 * @file {{NAME}}.cpp
 * @brief Non-template members of the {{NAME}} state machine front-end.
 *
 * States, actions and guards are templates and live in {{NAME}}.hpp; put
 * out-of-line helpers that they call here.
 */

#include "{{NAME}}.hpp"

{{NAME}}_::{{NAME}}_(Log& logger) : m_logger(&logger) {}
