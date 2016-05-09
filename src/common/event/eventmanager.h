#pragma once
#include <algorithm>
#include <cassert>
#include <fstream>
#include <vector>
#include "callstack.h"
#include "config.h"
#include "debuginfo/debugcontext.h"
#include "event.h"

class EventManager
{
    static const int EVENT_CHUNK_SIZE;
    CallStackGlobal callStackGlobal;
    const dbginfo::DebugContext& dbgContext;
    std::string eventPath;
    int totalEvents;
    int iterIndex = 0;
    int totalThreads = 0;
    std::vector<Event> preloadEvents;

public:
    EventManager(const dbginfo::DebugContext& dbgContext, const std::string& eventPath = std::string(), int totalEvents = 0):
        callStackGlobal(MAX_THREADS),
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
        callStackGlobal.clear();
    }

    bool hasNext()
    {
        return iterIndex < totalEvents;
    }

    size_t size() const
    {
        return totalEvents;
    }

    Event& next()
    {
        assert(hasNext());
        int mod = iterIndex % EVENT_CHUNK_SIZE;
        if (mod == 0)
        {
            dump();
            size_t eventCount = std::min(EVENT_CHUNK_SIZE, totalEvents - iterIndex);
            std::ifstream in(eventPath, std::ios::binary);
            in.seekg(iterIndex * sizeof(Event), in.beg);
            in.read((char*)&preloadEvents[0], eventCount * sizeof(Event));
        }
        iterIndex++;
        Event& e = preloadEvents[mod];
        switch (e.type)
        {
            case EventType::Call:
            case EventType::Ret:
            {
                totalThreads = std::max(totalThreads - 1, (int)e.routineEvent.threadId) + 1;
                auto* funcInfo = dbgContext.findFuncById(e.routineEvent.routineId);
                if (!funcInfo)
                {
                    break;
                }

                void* frameBase = (char*)e.routineEvent.stackPointerRegister + funcInfo->stackOffset;
                FuncCall funcCall(funcInfo, frameBase);
                if (e.type == EventType::Call)
                {
                    callStackGlobal.push(e.routineEvent.threadId, funcCall);
                }
                else
                {
                    callStackGlobal.pop(e.routineEvent.threadId, funcCall);
                }
                break;
            }
            //MemoryEvent
            default:
                totalThreads = std::max(totalThreads - 1, (int)e.memoryEvent.threadId) + 1;
                break;
        }
        return e;
    }

    void dump()
    {
        int curIndex = iterIndex - 1;
        if (curIndex <= 0 || curIndex >= totalEvents)
        {
            return;
        }
        int begIndex = (curIndex / EVENT_CHUNK_SIZE ) * EVENT_CHUNK_SIZE;
        int eventCount = std::min(EVENT_CHUNK_SIZE, totalEvents - begIndex);
        std::fstream out(eventPath, std::ios::binary | std::ios::in | std::ios::out);
        out.seekp(begIndex * sizeof(Event), out.beg);
        out.write((char*)&preloadEvents[0], eventCount * sizeof(Event));
    }

    const dbginfo::DebugContext& getDebugContext() const
    {
        return dbgContext;
    }

    int getTotalThreads() const
    {
        return totalThreads;
    }

    const dbginfo::FuncInfo* topFuncInfo(int threadId) const
    {
        if (callStackGlobal.empty(threadId))
        {
            return nullptr;
        }
        return callStackGlobal.top(threadId).funcInfo;
    }

    void save(std::ofstream& out) const
    {
        utils::save(eventPath, out);
        utils::save(totalEvents, out);
        utils::save(totalThreads, out);
    }

    void load(std::ifstream& in)
    {
        eventPath = utils::load<std::string>(in);
        totalEvents = utils::load<int>(in);
        totalThreads = utils::load<int>(in);
        std::cout << "[INFO] EventManager loaded: " << totalEvents << " events" << std::endl;
        reset();
    }
};
