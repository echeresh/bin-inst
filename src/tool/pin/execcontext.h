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
        void handleAlloc(dbginfo::DebugContext& dbgCtxt, MemoryEvent& memoryEvent, const std::string& varName);
        void handleFree(dbginfo::DebugContext& dbgCtxt, MemoryEvent& memoryEvent);
        MemoryObject findObject(void* addr, size_t size) const;
    };

    class ExecContext
    {
        const std::string binPath;
        dbginfo::DebugContext& dbgCtxt;

        HeapInfo heapInfo;
        CallStackGlobal callStackGlobal;

        PinEventDumper eventDumper;

        MemoryObject findNonStackObject(void* addr, size_t size);
        MemoryObject findStackObject(void* addr, size_t size);
        std::string getVarNameFromFile(const SourceLocation& sourceLoc) const;
        SourceLocation getSourceLocation(ADDRINT inst) const;

    public:
        ExecContext(const std::string& binPath, dbginfo::DebugContext& dbgCtxt);
        int getRoutineId(RTN rtn);
        void addEvent(const Event& event);
        MemoryObject findObject(void* addr, int size);
        EventManager dumpEvents();
        void saveMemoryAccesses() const;
    };
} //namespace pin
