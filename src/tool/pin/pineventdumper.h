#pragma once
#include <vector>
#include "pin.H"
#include "debuginfo/debugcontext.h"
#include "common/event/eventmanager.h"
#include "config.h"

namespace pin
{
    class PinEventDumper
    {
        static int EventBlockSize;

        std::string eventPath;
        std::vector<Event> events0;
        std::vector<Event> events1;
        std::vector<Event>* pEventsMain;
        std::vector<Event>* pEventsSave;

        volatile bool finished = false;
        PIN_SEMAPHORE saved;
        PIN_SEMAPHORE filled;

    public:
        int totalEvents = 0;

        PinEventDumper(const std::string& eventPath = BIN_EVENT_PATH);
        static void threadFunc(void* arg);
        void addEvent(const Event& event);
        EventManager finalize(const dbginfo::DebugContext& dbgCtxt);
    };
} //namespace pin
