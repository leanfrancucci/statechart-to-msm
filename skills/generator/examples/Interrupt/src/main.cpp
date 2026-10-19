/**
 * @file main.cpp
 * @brief Interactive console application that stimulates the Interrupt
 *        active object with one key per event.
 */

#include "InterruptActObj.hpp"

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
              << "  1 - send 'Start'\n"
              << "  2 - send 'Signal'\n"
              << "  3 - send 'Resume'\n"
              << "  4 - send 'Input'\n"
              << "  h - show this help\n"
              << "  q - quit\n"
              << std::endl;
}

/**
 * @brief Posts an event, constructed from args, to the active object.
 */
template <typename EventType, typename... Args>
void
send(InterruptActObj& actObj, const char* name, Args&&... args)
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
    InterruptActObj actObj;
    bool running = true;

    actObj.activate();
    printHelp();
    while (running)
    {
        switch (readKey())
        {
            case '1':
                send<Event::Start>(actObj, "Start");
                break;
            case '2':
                send<Event::Signal>(actObj, "Signal");
                break;
            case '3':
                send<Event::Resume>(actObj, "Resume");
                break;
            case '4':
                send<Event::Input>(actObj, "Input");
                break;
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
