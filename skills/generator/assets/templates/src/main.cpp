/**
 * @file main.cpp
 * @brief Interactive console application that stimulates the {{NAME}}
 *        active object with one key per event.
 */

#include "{{NAME}}ActObj.hpp"

#include "Event.hpp"
#include <cstdio>
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <utility>

namespace
{
/**
 * @brief Reads one key press without waiting for Enter and without echo.
 */
int
readKey()
{
    struct termios original;
    struct termios raw;

    if (tcgetattr(STDIN_FILENO, &original) != 0)
    {
        // Not a terminal (e.g. piped input): fall back to buffered reads
        return std::getchar();
    }
    raw = original;
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    int key = std::getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &original);
    return key;
}

void
printHelp()
{
    std::cout << "\nKeys:\n"
{{KEY_HELP}}
              << "  h - show this help\n"
              << "  q - quit\n"
              << std::endl;
}

/**
 * @brief Posts an event, constructed from args, to the active object.
 */
template <typename EventType, typename... Args>
void
send({{NAME}}ActObj& actObj, const char* name, Args&&... args)
{
    std::cout << "[main]: send '" << name << "'" << std::endl;
    if (!actObj.post<EventType>(std::forward<Args>(args)...))
    {
        std::cerr << "[main]: event queue full, '" << name << "' dropped" << std::endl;
    }
}
} // namespace

int
main()
{
    {{NAME}}ActObj actObj;
    bool running = true;

    actObj.activate();
    printHelp();
    while (running)
    {
        switch (readKey())
        {
{{KEY_CASES}}
            case 'h':
                printHelp();
                break;
            case 'q':
            case EOF:
                std::cout << "[main]: terminating" << std::endl;
                running = false;
                break;
            default:
                break;
        }
    }
    actObj.deactivate();
    return 0;
}
