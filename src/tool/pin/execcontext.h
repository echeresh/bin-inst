#pragma once
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <cassert>
#include <sstream>
#include <thread>
#include "common/callstack.h"
#include "common/debuginfo/debugcontext.h"
#include "common/event/eventmanager.h"
#include "common/sourcelocation.h"
#include "common/utils.h"
#include "pin.H"
#include "pineventdumper.h"

namespace pin
{
    struct MemoryObject
    {
        void* addr;
        size_t size;
        const dbginfo::VarInfo* varInfo;

        MemoryObject(void* addr = nullptr, size_t size = 0, const dbginfo::VarInfo* varInfo = nullptr);
        std::string name() const;
        void* lo() const;
        void* hi() const;
        bool isEmpty() const;
        bool operator<(const MemoryObject& o) const;
    };

    class HeapInfo
    {
        std::set<MemoryObject> objects;

    public:
        void handleAlloc(dbginfo::DebugContext& dbgCtxt, MemoryEvent& memoryEvent, const SourceLocation& srcLoc);
        void handleFree(dbginfo::DebugContext& dbgCtxt, MemoryEvent& memoryEvent);
        MemoryObject findObject(const MemoryEvent& memoryEvent) const;
    };

    class ExecContext
    {
        const std::string binPath;
        dbginfo::DebugContext& dbgCtxt;

        HeapInfo heapInfo;
        CallStackGlobal callStackGlobal;

        PinEventDumper eventDumper;
        bool profilingEnabled = false;
        bool heapSupportEnabled = false;

        MemoryObject findNonStackObject(const MemoryEvent& memoryEvent);
        MemoryObject findStackObject(const MemoryEvent& memoryEvent);

    public:
        ExecContext(const std::string& binPath, dbginfo::DebugContext& dbgCtxt);
        int getRoutineId(RTN rtn);
        void addEvent(const Event& event);
        MemoryObject findObject(const MemoryEvent& memoryEvent);
        EventManager dumpEvents();
        void saveMemoryAccesses() const;
    };
} //namespace pin
