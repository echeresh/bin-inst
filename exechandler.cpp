#include "exechandler.h"
using namespace std;

inline uint64_t rdtsc() __attribute__((always_inline));
inline uint64_t rdtsc()
{
    uint64_t x;
    __asm__ volatile ("rdtsc\n\tshl $32, %%rdx\n\tor %%rdx, %%rax" : "=a" (x) : : "rdx");
    return x;
}

RoutineInfo::RoutineInfo(int id, const std::string& name) :
    id(id),
    name(name)
{
}

FuncCall::FuncCall(const FuncInfo* funcInfo, void* frameBase) :
    funcInfo(funcInfo),
    frameBase(frameBase)
{
}

MemoryObject::MemoryObject(void* addr, size_t size, const VarInfo* varInfo) :
    addr(addr),
    size(size),
    varInfo(varInfo)
{
}

string MemoryObject::name() const
{
    return varInfo ? varInfo->name : "";
}

void* MemoryObject::lo() const
{
    return addr;
}

void* MemoryObject::hi() const
{
    return (char*)addr + size;
}

bool MemoryObject::isEmpty() const
{
    return !addr;
}

bool MemoryObject::operator<(const MemoryObject& o) const
{
    return addr < o.addr;
}

//------------------------------------------------------------------------------
//CallStack
//------------------------------------------------------------------------------

void CallStack::addCall(const FuncCall& call)
{
    //cleanup call stack
    while (!calls.empty() && calls.back().frameBase <= call.frameBase)
        calls.pop_back();
    calls.push_back(call);
}

MemoryObject CallStack::findObject(void* addr, size_t size)
{
    //cout << "stack find object " << calls.size() << endl;
    for (int i = calls.size() - 1; i >= 0; i--)
    {
        auto& call = calls[i];
        if (addr <= call.frameBase)
        {
            for (auto& var : call.funcInfo->vars)
            {
               
                if (var->location.entries.empty())
                    continue;
                assert(var->location.entries.front().reg == LocSource::FrameBase);
                char* varAddr = (char*)call.frameBase + var->location.entries.front().off;
                if (varAddr <= addr && addr <= varAddr + var->size - 1)
                {
                    //cout << "found object: " << var->name << endl;
                    assert((char*)addr + size <= varAddr + var->size);
                    return MemoryObject(varAddr, var->size, var);
                }
            }
        }
    }
    return MemoryObject();
}

string CallStack::str() const
{
    ostringstream oss;
    for (int i = 0; i < (int)calls.size(); i++)
        oss << calls[i].funcInfo->name << " frame base: " << calls[i].frameBase << endl;
    return oss.str();
}

const FuncCall& CallStack::top() const
{
    return calls.back();
}

void CallStack::pop()
{
    calls.pop_back();
}

//------------------------------------------------------------------------------
//Locker
//------------------------------------------------------------------------------

class Locker
{
    PIN_LOCK* lock;
public:
    Locker(PIN_LOCK* lock, int id) :
        lock(lock)
    {
        PIN_GetLock(lock, id);
    }
    ~Locker()
    {
        PIN_ReleaseLock(lock);
    }
};

//------------------------------------------------------------------------------
//Analysis functions for PIN
//------------------------------------------------------------------------------
static void routineEnter(ExecHandler* execHandler, CONTEXT* ctxt, THREADID threadId, UINT32 rtnId)
{
    execHandler->handleRoutineEnter(ctxt, threadId, rtnId);
}

static void routineExit(ExecHandler* execHandler, CONTEXT* ctxt, THREADID threadId, UINT32 rtnId)
{
    execHandler->handleRoutineExit(ctxt, threadId, rtnId);
}

static void heapAllocBefore(ExecHandler* execHandler, THREADID threadId, size_t size)
{
    execHandler->handleHeapAllocBefore(threadId, size);
}

static void heapAllocAfter(ExecHandler* execHandler, THREADID threadId, void* addr)
{
    execHandler->handleHeapAllocAfter(threadId, addr);
}

static void heapFree(ExecHandler* execHandler, THREADID threadId, void* addr)
{
    execHandler->handleHeapFree(threadId, addr);
}

static void memoryRead(ExecHandler* execHandler, VOID* ip, THREADID threadId, VOID* addr, UINT32 size)
{
    execHandler->handleMemoryRead(threadId, addr, size);
}

static void memoryWrite(ExecHandler* execHandler, VOID* ip, THREADID threadId, VOID* addr, UINT32 size)
{
    execHandler->handleMemoryWrite(threadId, addr, size);
}

template <class Cont, class TKey>
typename Cont::iterator lessFirst(Cont& c, const TKey& key)
{
    if (c.empty())
        return c.end();

    auto it = c.upper_bound(key);
    if (it == c.begin())
        return c.end();
    return --it;
}

//------------------------------------------------------------------------------
//HeapInfo
//------------------------------------------------------------------------------

void HeapInfo::handleAlloc(void* addr, size_t size)
{
    auto ret = objects.insert(MemoryObject(addr, size));
    if (!ret.second)
    {
        //cout << "heap-error: " << addr << " " << size << endl;
    }
}

void HeapInfo::handleFree(void* addr)
{
    auto nErased = objects.erase(MemoryObject(addr));
    assert(nErased >= 0);
}

MemoryObject HeapInfo::findObject(void* addr, size_t size) const
{
    auto it = lessFirst(objects, addr);
    if (it == objects.end())
        return MemoryObject();
    
    if (it->lo() <= addr && addr < it->hi())
    {
        assert((char*)addr + size <= it->hi());
        return *it;
    }
    return MemoryObject(); 
}

//------------------------------------------------------------------------------
//Access
//------------------------------------------------------------------------------

Access::Access(void* addr, size_t size, uint64_t cycles, THREADID threadId,
       AccessType type, const MemoryObject& memoryObject) :
    addr(addr),
    size(size),
    cycles(cycles),
    threadId(threadId),
    type(type),
    memoryObject(memoryObject)
{
}

bool Access::operator<(const Access& a) const
{
    return cycles < a.cycles;
}

//------------------------------------------------------------------------------
//ExecHandler
//------------------------------------------------------------------------------

MemoryObject ExecHandler::findNonStackObject(void* addr, size_t size)
{
    return MemoryObject();
}

MemoryObject ExecHandler::findStackObject(void* addr, size_t size)
{
    return callStacks[0].findObject(addr, size);
}

ExecHandler::ExecHandler(const DebugContext& dbgCtxt) :
    callStacks(MaxThreads),
    dbgCtxt(dbgCtxt)
{
}

int ExecHandler::getRoutineId(RTN rtn)
{
    int id = RTN_Id(rtn);
    string name = RTN_Name(rtn);
    routines[id] = RoutineInfo(id, name);
    return id;
}

void ExecHandler::handleHeapAllocBefore(THREADID threadId, size_t size)
{
    Locker locker(&lock, threadId);
    currentHeapAlloc.size = size;
}

void ExecHandler::handleHeapAllocAfter(THREADID threadId, void* addr)
{
    Locker locker(&lock, threadId);
    handleHeapAlloc(threadId, addr, currentHeapAlloc.size);
}

void ExecHandler::handleHeapAlloc(THREADID threadId, void* addr, size_t size)
{
    heapInfo.handleAlloc(addr, size);
}

void ExecHandler::handleHeapFree(THREADID threadId, void* addr)
{
    Locker locker(&lock, threadId);
    heapInfo.handleFree(addr);
}

void ExecHandler::handleRoutineEnter(CONTEXT* ctxt, THREADID threadId, int routineId)
{
    Locker locker(&lock, threadId);
    auto& rtnInfo = routines[routineId];
    auto* funcInfo = dbgCtxt.findFuncByName(rtnInfo.name);
    //cout << "call " << rtnInfo.name << " " << rdtsc() << endl;
    if (!funcInfo || funcInfo->location.entries.empty())
        return;

    // cout << endl << "call stack: " << endl;
    // cout << callStacks[threadId].str() << endl;
    // cout << endl;
    // cout << "frame base is known: " << rtnInfo.name << endl;
    auto& loc = funcInfo->location.entries.front();
    assert(loc.reg == LocSource::RSP);
    void* frameBase = (char*)PIN_GetContextReg(ctxt, REG_STACK_PTR) + loc.off;
    callStacks[threadId].addCall(FuncCall(funcInfo, frameBase));
}

void ExecHandler::handleRoutineExit(CONTEXT* ctxt, THREADID threadId, int routineId)
{
    Locker locker(&lock, threadId);
    auto& rtnInfo = routines[routineId];
    auto* funcInfo = dbgCtxt.findFuncByName(rtnInfo.name);
    //cout << "exit " << rtnInfo.name << endl;
    if (!funcInfo)
    {
        //cout << "not found: " << rtnInfo.name << endl;
        return;
    }
    if (funcInfo->location.entries.empty())
    {
        //cout << "frame base is unknown: " << rtnInfo.name << endl;
        return;
    }
    // cout << endl << "call stack: " << endl;
    // cout << callStacks[threadId].str() << endl;
    // cout << endl;
    assert(callStacks[threadId].top().funcInfo == funcInfo);
    callStacks[threadId].pop();
}

void ExecHandler::handleMemoryRead(THREADID threadId, void* addr, size_t size)
{
    Locker locker(&lock, threadId);
    auto mo = findObject(addr, size);
    if (!mo.isEmpty())
        accesses.insert(Access(addr, size, rdtsc(), threadId, AccessType::Read, mo));
}


void ExecHandler::handleMemoryWrite(THREADID threadId, void* addr, size_t size)
{
    Locker locker(&lock, threadId);
    auto mo = findObject(addr, size);
    if (!mo.isEmpty())
        accesses.insert(Access(addr, size, rdtsc(), threadId, AccessType::Write, mo));
}

MemoryObject ExecHandler::findObject(void* addr, int size)
{
    auto mo = findStackObject(addr, size);
    if (mo.isEmpty())
        return findNonStackObject(addr, size);
    return mo;
}

void ExecHandler::analyseRoutine(RTN rtn)
{   
    RTN_Open(rtn);
    int id = getRoutineId(rtn);
    RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)routineEnter,
                   IARG_PTR, this, IARG_CONTEXT,
                   IARG_THREAD_ID, IARG_UINT32, id, IARG_END);
    RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)routineExit,
                   IARG_PTR, this, IARG_CONTEXT,
                   IARG_THREAD_ID, IARG_UINT32, id, IARG_END);
    string name = RTN_Name(rtn);
    if (name == "malloc" || name == "__libc_malloc")
    {
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)heapAllocBefore,
                   IARG_PTR, this, IARG_THREAD_ID,
                   IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_END);
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)heapAllocAfter,
                   IARG_PTR, this, IARG_THREAD_ID,
                   IARG_FUNCRET_EXITPOINT_VALUE, IARG_END);
    }
    else if (name == "free" || name == "__libc_free")
    {
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)heapFree,
                   IARG_PTR, this, IARG_THREAD_ID,
                   IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_END);
    }
    RTN_Close(rtn);
}

void ExecHandler::analyseInstruction(INS ins)
{
    UINT32 memOperands = INS_MemoryOperandCount(ins);
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        bool f = false;
        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            f = true;
            auto size = INS_MemoryOperandSize(ins, memOp);
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)memoryRead,
                IARG_PTR, this, IARG_INST_PTR,
                IARG_THREAD_ID,
                IARG_MEMORYOP_EA, memOp,
                IARG_UINT32, size,
                IARG_END);
        }
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            f = true;
            auto size = INS_MemoryOperandSize(ins, memOp);
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)memoryWrite,
                IARG_PTR, this, IARG_INST_PTR,
                IARG_THREAD_ID,
                IARG_MEMORYOP_EA, memOp,
                IARG_UINT32, size,
                IARG_END);
        }
        assert(f);
    }
}

void ExecHandler::saveMemoryAccesses() const
{
    cout << "save..." << endl;
    uint64_t minCycles = (uint64_t)-1;
    for (auto& e : accesses)
        minCycles = min(minCycles, e.cycles);
    for (auto& e : accesses)
    {
        ostringstream oss;
        oss << "acc-" << (e.type == AccessType::Read ? "R-" : "W-")
            << e.memoryObject.name() << "-sz-"
            << e.memoryObject.size << "-thr-" << e.threadId;
        ofstream out(oss.str(), ios::app);
        out << (uint64_t)e.addr << " " << e.cycles - minCycles << endl;
    }
}