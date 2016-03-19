#pragma once
#include <cassert>
#include <fstream>
#include <vector>
#include "config.h"
#include "debuginfo/debugcontext.h"
#include "event.h"

class EventManager
{
    static const int EVENT_CHUNK_SIZE = 1 << 23;
    const dbginfo::DebugContext& dbgContext;
    std::string eventPath;
    int totalEvents;
    int iterIndex = 0;
    std::vector<Event> preloadEvents;

public:
    EventManager(const dbginfo::DebugContext& dbgContext, const std::string& eventPath = std::string(), int totalEvents = 0):
        dbgContext(dbgContext),
        eventPath(eventPath),
        totalEvents(totalEvents)
    {
        reset();
    }

    void reset()
    {
        iterIndex = 0;
        preloadEvents.resize(EVENT_CHUNK_SIZE);
    }

    bool hasNext()
    {
        return iterIndex < totalEvents;
    }

    size_t size() const
    {
        return totalEvents;
    }

    Event next()
    {
        assert(hasNext());
        int mod = iterIndex % EVENT_CHUNK_SIZE;
        if (mod == 0)
        {
            size_t eventCount = std::min(EVENT_CHUNK_SIZE, totalEvents - iterIndex);
            std::ifstream in(eventPath, std::ios::binary);
            in.seekg(iterIndex * sizeof(Event), in.beg);
            in.read((char*)&preloadEvents[0], eventCount * sizeof(Event));
        }
        iterIndex++;
        return preloadEvents[mod];
    }

    const dbginfo::DebugContext& getDebugContext() const
    {
        return dbgContext;
    }

    void save(std::ofstream& out) const
    {
        utils::save(eventPath, out);
        utils::save(totalEvents, out);
    }

    void load(std::ifstream& in)
    {
        eventPath = utils::load<std::string>(in);
        totalEvents = utils::load<int>(in);
        reset();
    }
};
