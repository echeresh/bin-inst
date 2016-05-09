#include <cassert>
#include <fstream>
#include <iostream>
#include "common/utils.h"
#include "config.h"
#include "pineventdumper.h"

namespace pin
{
    int PinEventDumper::EventBlockSize = 1 << 23;

    PinEventDumper::PinEventDumper(const std::string& eventPath) :
        eventPath(eventPath),
        pEventsMain(&events0),
        pEventsSave(&events1)
    {
        std::remove(eventPath.c_str());
        PIN_SemaphoreInit(&saved);
        PIN_SemaphoreInit(&filled);
        PIN_SemaphoreSet(&saved);
        PIN_SpawnInternalThread(threadFunc, this, 0, nullptr);
    }

    void PinEventDumper::threadFunc(void* arg)
    {
        PinEventDumper* eventDumper = (PinEventDumper*)arg;
        for(;;)
        {
            //std::cout << "thread: wait filled" << std::endl;
            PIN_SemaphoreWait(&eventDumper->filled);
            //std::cout << "thread: clear filled" << std::endl;
            PIN_SemaphoreClear(&eventDumper->filled);
            std::ofstream out(eventDumper->eventPath, ios::app | ios::binary);
            assert(out.good());
            out.write((char*)&(*eventDumper->pEventsSave)[0], eventDumper->pEventsSave->size() * sizeof(Event));
            std::cout << "EVENTS SAVED: " << eventDumper->pEventsSave->size() << " : "
                      << eventDumper->pEventsSave->size() * sizeof(Event) << std::endl;
            eventDumper->totalEvents += eventDumper->pEventsSave->size();
            eventDumper->pEventsSave->clear();
            eventDumper->pEventsSave->shrink_to_fit();
            //std::cout << "thread: set saved" << std::endl;
            PIN_SemaphoreSet(&eventDumper->saved);
            if (eventDumper->finished)
            {
                return;
            }
        }
    }

    void PinEventDumper::addEvent(const Event& event)
    {
        pEventsMain->push_back(event);
        if (pEventsMain->size() > EventBlockSize)
        {
            auto t = utils::rdtsc();
            PIN_SemaphoreWait(&saved);
            PIN_SemaphoreClear(&saved);

            std::swap(pEventsMain, pEventsSave);

            PIN_SemaphoreSet(&filled);
            std::cout << "SCHEDULED TO SAVE: " << utils::rdtsc() - t << std::endl;
        }
    }

    EventManager PinEventDumper::finalize(const dbginfo::DebugContext& dbgCtxt)
    {
        PIN_SemaphoreWait(&saved);
        PIN_SemaphoreClear(&saved);
        std::swap(pEventsMain, pEventsSave);
        finished = true;
        PIN_SemaphoreSet(&filled);
        PIN_SemaphoreWait(&saved);
        return EventManager(dbgCtxt, eventPath, totalEvents);
    }

} //namespace pin
