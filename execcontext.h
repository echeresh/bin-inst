#pragma once
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <cassert>
#include <sstream>
#include <thread>
#include "debuginfo.h"
#include "utils.h"
#include "pin.H"

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

enum class EventType
{
    CallInst,
    Call,
    Ret,
    Read,
    Write,
    Alloc,
    Free
};

struct ExecContext;

struct MemoryEvent
{
    uint64_t t;
    THREADID threadId;
    void* addr;
    size_t size;
    ADDRINT instAddr;
    
    std::string str(const ExecContext& execCtxt) const;
};

struct RoutineEvent
{
    uint64_t t;
    THREADID threadId;
    int routineId;
    void* spValue;
    ADDRINT instAddr;
    
    std::string str(const ExecContext& execCtxt) const;
};


struct Event
{
    EventType type;
    union
    {
        MemoryEvent memoryEvent;
        RoutineEvent routineEvent;
    };
    
    Event() = default;
    Event(EventType type, THREADID threadId, void* addr, size_t size = 0, ADDRINT instAddr = 0);
    Event(EventType type, THREADID threadId, int routineId, void* spValue, ADDRINT instAddr = 0);
    std::string str(const ExecContext& execCtxt) const;
};

class EventKeeper
{
    static std::string EventPath;
    static int EventBlockLength;

    std::vector<Event> events0;
    std::vector<Event> events1;
    std::vector<Event>* pEvents0;
    std::vector<Event>* pEvents1;
    std::vector<Event> preloadEvents;
    volatile bool finished = false;
    PIN_SEMAPHORE saved;
    PIN_SEMAPHORE filled;
    int iterIndex = 0;

public:
    int totalEvents = 0;

    EventKeeper();
    static void threadFunc(void* arg);
    void addEvent(const Event& event);
    void finish();
    void reset();
    bool hasNext();
    Event next();
};

class ExecContext
{
    static const int MaxThreads = 64;
    
    const std::string binPath;
    const dbg::DebugContext& dbgCtxt;

    HeapInfo heapInfo;
    std::vector<CallStack> callStacks;
    std::map<int, RoutineInfo> routines;

    std::set<Access> accesses;
    EventKeeper eventKeeper;
    
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
    ~ExecContext();
};