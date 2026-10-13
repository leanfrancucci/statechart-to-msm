/**
 * @file Log.hpp
 * @brief Simple logging interface and a console logger.
 */

#ifndef __LOG_HPP__
#define __LOG_HPP__

#include <string>

/**
 * @class Log
 * @brief Abstract interface class for logging.
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
 * @brief Concrete logger implementation printing to std::cout.
 */
class ConsoleLogger : public Log
{
  public:
    /**
      * @brief Overrides the base log function to print to std::cout.
      * @param msg Message to print.
      */
    void info(const std::string& msg) override;
};

/**
 * @brief Gets a global static reference to the default ConsoleLogger.
 * @return Reference to the default Log instance.
 */
Log& getDefaultLogger();

#endif // __LOG_HPP__
