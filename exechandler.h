#pragma once
#include <fstream>
#include <vector>
#include <stack>
#include <map>
#include <set>
#include <cstdint>
#include "pin.H"
#include "dwarfparser.h"

struct RoutineInfo
{
    int id = -1;
    std::string name;
    
    RoutineInfo() = default;
    RoutineInfo(int id, const std::string& name);
};

struct FuncCall
{
    const FuncInfo* funcInfo;
    void* frameBase;
    
    FuncCall(const FuncInfo* funcInfo, void* frameBase);
};

struct MemoryObject
{
    void* addr;
    size_t size;
    const VarInfo* varInfo;
    
    MemoryObject(void* addr = nullptr, size_t size = 0, const VarInfo* varInfo = nullptr);
    string name() const;
    void* lo() const;
    void* hi() const;
    bool isEmpty() const;
    bool operator<(const MemoryObject& o) const;
};

struct CallStack
{
    std::vector<FuncCall> calls;
    
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
    void handleAlloc(void* addr, size_t size);
    void handleFree(void* addr);
    MemoryObject findObject(void* addr, size_t size) const;
};

enum class AccessType
{
    Read,
    Write
};

struct Access
{
    void* addr;
    size_t size;
    uint64_t cycles;
    THREADID threadId;
    AccessType type;
    MemoryObject memoryObject;
    
    Access(void* addr, size_t size, uint64_t cycles, THREADID threadId,
           AccessType type, const MemoryObject& memoryObject);
    bool operator<(const Access& a) const;
};

class ExecHandler
{
    static const int MaxThreads = 64;

    std::map<int, RoutineInfo> routines;
    PIN_LOCK lock;
    std::vector<CallStack> callStacks;
    const DebugContext& dbgCtxt;
    MemoryObject currentHeapAlloc;
    HeapInfo heapInfo;
    std::set<Access> accesses;
    
    MemoryObject findNonStackObject(void* addr, size_t size);
    MemoryObject findStackObject(void* addr, size_t size);

public:
    ExecHandler(const DebugContext& dbgCtxt);
    int getRoutineId(RTN rtn);
    void handleHeapAllocBefore(THREADID threadId, size_t size);
    void handleHeapAllocAfter(THREADID threadId, void* addr);
    void handleHeapAlloc(THREADID threadId, void* addr, size_t size);
    void handleHeapFree(THREADID threadId, void* addr);
    void handleRoutineEnter(CONTEXT* ctxt, THREADID threadId, int routineId);
    void handleRoutineExit(CONTEXT* ctxt, THREADID threadId, int routineId);
    void handleMemoryRead(THREADID threadId, void* addr, size_t size);    
    void handleMemoryWrite(THREADID threadId, void* addr, size_t size);
    MemoryObject findObject(void* addr, int size);
    void analyseRoutine(RTN rtn);
    void analyseInstruction(INS ins);
    void saveMemoryAccesses() const;
};