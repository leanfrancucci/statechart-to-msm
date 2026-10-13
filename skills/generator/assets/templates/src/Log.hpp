/**
 * @file Log.hpp
 * @brief Logging interface and a console logger.
 */

#ifndef {{NAME_UPPER}}_LOG_HPP
#define {{NAME_UPPER}}_LOG_HPP

#include <string>

/**
 * @class Log
 * @brief Abstract logging interface, injected so tests can record traces.
 */
class Log
{
  public:
    virtual ~Log() = default;

    /**
     * @brief Logs an informational message.
     * @param msg Message to log.
     */
    virtual void info(const std::string& msg) = 0;
};

/**
 * @class ConsoleLogger
 * @brief Logger that prints every message to std::cout.
 */
class ConsoleLogger : public Log
{
  public:
    void info(const std::string& msg) override;
};

/**
 * @brief Gets the process-wide default ConsoleLogger.
 */
Log&
getDefaultLogger();

#endif
