#pragma once
#include <vector>
#include "pin.H"
#include "event.h"

namespace inst
{
    class EventKeeper
    {
        static int EventBlockSize;

        std::vector<Event> events0;
        std::vector<Event> events1;
        std::vector<Event>* pEventsMain;
        std::vector<Event>* pEventsSave;
        std::vector<Event> preloadEvents;
        volatile bool finished = false;
        PIN_SEMAPHORE saved;
        PIN_SEMAPHORE filled;
        int iterIndex = 0;

    public:
        int totalEvents = 0;

        EventKeeper();
        static void threadFunc(void* arg);
        void addEvent(const Event& event);
        void finish();
        void reset();
        bool hasNext();
        Event next();
    };
} //namespace isnt
