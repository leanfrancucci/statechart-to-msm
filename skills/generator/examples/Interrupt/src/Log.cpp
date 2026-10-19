/**
 * @file Log.cpp
 */

#include "Log.hpp"
#include <iostream>

void
ConsoleLogger::info(const std::string& msg)
{
    std::cout << msg << std::endl;
}

Log&
getDefaultLogger()
{
    static ConsoleLogger defaultLogger;
    return defaultLogger;
}
