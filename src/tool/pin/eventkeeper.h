#pragma once
#include <vector>
#include "pin.H"
#include "event.h"

namespace pin
{
    class PinEventDumper
    {
        static int EventBlockSize;

        std::vector<Event> events0;
        std::vector<Event> events1;
        std::vector<Event>* pEventsMain;
        std::vector<Event>* pEventsSave;

        volatile bool finished = false;
        PIN_SEMAPHORE saved;
        PIN_SEMAPHORE filled;

    public:
        int totalEvents = 0;

        EventKeeper();
        static void threadFunc(void* arg);
        void addEvent(const Event& event);
        void finish();
    };
} //namespace pin
