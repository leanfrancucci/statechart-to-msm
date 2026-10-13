/**
 *  @file   Event.hpp
 *  @brief	...
 */

#ifndef __EVENT_HPP__
#define __EVENT_HPP__

#include <string>

namespace Event
{
struct EventA
{
};

struct EventB
{
    static int aliveCount;

    EventB(std::string name, int diskType)
        : m_name(name), m_discType(diskType)
    {
        aliveCount++;
    }

    EventB(const EventB& other)
        : m_name(other.m_name), m_discType(other.m_discType)
    {
        aliveCount++;
    }

    EventB(EventB&& other) noexcept
        : m_name(std::move(other.m_name)), m_discType(other.m_discType)
    {
        aliveCount++;
    }

    EventB& operator=(const EventB& other)
    {
        if (this != &other)
        {
            m_name = other.m_name;
            m_discType = other.m_discType;
        }
        return *this;
    }

    EventB& operator=(EventB&& other) noexcept
    {
        if (this != &other)
        {
            m_name = std::move(other.m_name);
            m_discType = other.m_discType;
        }
        return *this;
    }

    ~EventB()
    {
        aliveCount--;
    }

    std::string m_name;
    int m_discType;
};

struct EventC
{
    EventC(){}
    template <class Event>
    EventC(Event const&){}
};

struct EventD
{
};

struct EventE
{
};

struct Start
{
};

struct Stop
{
};

}

#endif
