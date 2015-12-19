#include "eventkeeper.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include "common/utils.h"
#include "config.h"

namespace inst
{
    int EventKeeper::EventBlockSize = 1 << 23;

    EventKeeper::EventKeeper() :
        pEventsMain(&events0),
        pEventsSave(&events1)
    {
        std::remove(INST_EVENT_KEEPER_PATH.c_str());
        PIN_SemaphoreInit(&saved);
        PIN_SemaphoreInit(&filled);
        PIN_SemaphoreSet(&saved);
        PIN_SpawnInternalThread(threadFunc, this, 0, nullptr);
    }

    void EventKeeper::threadFunc(void* arg)
    {
        EventKeeper* eventKeeper = (EventKeeper*)arg;
        for(;;)
        {
            //std::cout << "thread: wait filled" << std::endl;
            PIN_SemaphoreWait(&eventKeeper->filled);
            //std::cout << "thread: clear filled" << std::endl;
            PIN_SemaphoreClear(&eventKeeper->filled);

            std::ofstream out(INST_EVENT_KEEPER_PATH, ios::app | ios::binary);
            assert(out.good());
            out.write((char*)&(*eventKeeper->pEventsSave)[0], eventKeeper->pEventsSave->size() * sizeof(Event));
            std::cout << "EVENTS WERE DROPPED: " << eventKeeper->pEventsSave->size() << " : "
                      << eventKeeper->pEventsSave->size() * sizeof(Event) << std::endl;
            eventKeeper->totalEvents += eventKeeper->pEventsSave->size();
            eventKeeper->pEventsSave->clear();
            eventKeeper->pEventsSave->shrink_to_fit();
            //std::cout << "thread: set saved" << std::endl;
            PIN_SemaphoreSet(&eventKeeper->saved);
            if (eventKeeper->finished)
            {
                return;
            }
        }
    }

    void EventKeeper::addEvent(const Event& event)
    {
        pEventsMain->push_back(event);

        if (pEventsMain->size() > EventBlockSize)
        {
            auto t = rdtsc();
            PIN_SemaphoreWait(&saved);
            PIN_SemaphoreClear(&saved);

            std::swap(pEventsMain, pEventsSave);

            PIN_SemaphoreSet(&filled);
            std::cout << "QUEUED: " << rdtsc() - t << std::endl;
        }
    }

    void EventKeeper::finish()
    {
        PIN_SemaphoreWait(&saved);
        PIN_SemaphoreClear(&saved);
        std::swap(pEventsMain, pEventsSave);
        finished = true;
        PIN_SemaphoreSet(&filled);
        PIN_SemaphoreWait(&saved);
    }

    void EventKeeper::reset()
    {
        iterIndex = 0;
        preloadEvents.resize(EventBlockSize);
    }

    bool EventKeeper::hasNext()
    {
        return iterIndex < totalEvents;
    }

    Event EventKeeper::next()
    {
        assert(hasNext());
        int mod = iterIndex % EventBlockSize;
        if (mod == 0)
        {
            size_t eventCount = std::min(EventBlockSize, totalEvents - iterIndex);
            std::ifstream in(INST_EVENT_KEEPER_PATH, std::ios::binary);
            in.seekg(iterIndex * sizeof(Event), in.beg);
            in.read((char*)&preloadEvents[0], EventBlockSize * sizeof(Event));
        }
        iterIndex++;
        return preloadEvents[mod];
    }
} //namespace inst
