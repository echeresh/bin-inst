#pragma once
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <cassert>
#include <sstream>
#include <thread>
#include "common/debuginfo/debugcontext.h"
#include "common/event/eventmanager.h"
#include "common/utils.h"
#include "pin.H"
#include "pineventdumper.h"

namespace pin
{
    struct FuncCall
    {
        const dbginfo::FuncInfo* funcInfo;
        void* frameBase;

        FuncCall(const dbginfo::FuncInfo* funcInfo, void* frameBase);
    };

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

    struct CallStack
    {
        std::vector<FuncCall> calls;
        ADDRINT lastCallInstruction = 0;

        void addCall(const FuncCall& call);
        MemoryObject findObject(void* addr, size_t size);
        std::string str() const;
        const FuncCall& top() const;
        void pop();

    };

    class HeapInfo
    {
        std::set<MemoryObject> objects;

    public:
        void handleAlloc(void* addr, size_t size, const std::string& varName);
        void handleFree(void* addr);
        MemoryObject findObject(void* addr, size_t size) const;
    };

    class ExecContext
    {
        static const int MaxThreads = 64;

        const std::string binPath;
        dbginfo::DebugContext& dbgCtxt;

        HeapInfo heapInfo;
        std::vector<CallStack> callStacks;

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
