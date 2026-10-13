/**
 * @file main.cpp
 */

#include <chrono>
#include <iostream>
#include <unistd.h>
#include <termios.h>
#include "Event.hpp"
#include "Mgr.hpp"

using namespace std;

static struct termios orgt;

static int
mygetch(void)
{
    struct termios newt;
    int ch;

    tcgetattr(STDIN_FILENO, &orgt);
    newt = orgt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &orgt);
    return ch;
}

static void
printHelp()
{
    cout << "\nEntering an option:" << endl;
    cout << "a - send 'Start' event to the Mgr active object" << endl;
    cout << "o - send 'Stop' event to the Mgr active object" << endl;
    cout << "w - wait for 'Mgr' reaches 'Processing' state before killing it"
         << endl;
    cout << "q - quit\n" << endl;
}

int
main()
{
    char option;
    bool reached;
    bool running = true;
    Mgr mgr;

    mgr.activate();

    printHelp();
    while (running)
    {
        option = mygetch();
        switch (option) 
        {
            case 'a':
                cout << "[main]: send 'Start' event" << endl;
                mgr.post<Event::Start>();
                break;
            case 'o':
                cout << "[main]: send 'Stop' event" << endl;
                mgr.post<Event::Stop>();
                break;
            case 'q':
                cout << "[main]: terminating this program..." << endl;
                mgr.deactivate();
                running = false;
                break;
            case 'w':
                cout << "[main]: wait for 'Mgr' reaches 'Processing' "
                    "state before killing it" << endl;
                reached = mgr.waitForProcessing(std::chrono::seconds(1));
                mgr.deactivate();
                running = false;

                if (!reached)
                {
                    cerr << "[main]: timeout waiting for the Processing state" << endl;
                }
                break;
            default:
                break;
        } 
    }

    return 0;
}
