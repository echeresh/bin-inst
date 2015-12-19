#pragma once
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <cassert>
#include <sstream>
#include <thread>
#include "debuginfo/debuginfo.h"
#include "common/utils.h"
#include "pin.H"
#include "event.h"
#include "eventkeeper.h"
#include "pattern/pattern.h"

namespace inst
{
    struct RoutineInfo
    {
        int id = -1;
        std::string name;

        RoutineInfo() = default;
        RoutineInfo(int id, const std::string& name);
    };

    struct FuncCall
    {
        const dbg::FuncInfo* funcInfo;
        void* frameBase;

        FuncCall(const dbg::FuncInfo* funcInfo, void* frameBase);
    };

    struct MemoryObject
    {
        void* addr;
        size_t size;
        const dbg::VarInfo* varInfo;

        MemoryObject(void* addr = nullptr, size_t size = 0, const dbg::VarInfo* varInfo = nullptr);
        string name() const;
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
        string str() const;
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
        const dbg::DebugContext& dbgCtxt;

        HeapInfo heapInfo;
        std::vector<CallStack> callStacks;
        std::map<int, RoutineInfo> routines;

        EventKeeper eventKeeper;
        pattern::PatternAnalyzer patternAnalyzer;

        MemoryObject findNonStackObject(void* addr, size_t size);
        MemoryObject findStackObject(void* addr, size_t size);
        std::string getVarNameFromFile(const std::string& filePath, int lineNumber) const;

    public:
        ExecContext(const std::string& binPath, const dbg::DebugContext& dbgCtxt);
        int getRoutineId(RTN rtn);
        void addEvent(const Event& event);
        MemoryObject findObject(void* addr, int size);
        void processEvents();
        void saveMemoryAccesses() const;
        const RoutineInfo* findRoutine(int routineId) const;
    };
} //namespace inst
