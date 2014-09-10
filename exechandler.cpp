#include "exechandler.h"
using namespace std;

inline uint64_t rdtsc() __attribute__((always_inline));
inline uint64_t rdtsc()
{
    uint64_t x;
    __asm__ volatile ("rdtsc\n\tshl $32, %%rdx\n\tor %%rdx, %%rax" : "=a" (x) : : "rdx");
    return x;
}

//------------------------------------------------------------------------------
//RoutineInfo
//------------------------------------------------------------------------------

RoutineInfo::RoutineInfo(int id, const std::string& name) :
    id(id),
    name(name)
{
}

//------------------------------------------------------------------------------
//FuncCall
//------------------------------------------------------------------------------

FuncCall::FuncCall(const dbg::FuncInfo* funcInfo, void* frameBase) :
    funcInfo(funcInfo),
    frameBase(frameBase)
{
}

//------------------------------------------------------------------------------
//MemoryObject
//------------------------------------------------------------------------------

MemoryObject::MemoryObject(void* addr, size_t size, const dbg::VarInfo* varInfo) :
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
                char* varAddr = (char*)call.frameBase + var->loc;
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

static void routineCall2Before(ExecHandler* execHandler, THREADID threadId, ADDRINT instAddr, UINT32 rtnId)
{
    //cout << "routineCall2Before " << execHandler->routines[rtnId].name << endl;
    execHandler->setLastCallInstruction(instAddr, threadId);
}

static void routineCall2After(ExecHandler* execHandler, THREADID threadId, ADDRINT instAddr, UINT32 rtnId)
{
    //cout << "routineCall2After: " << execHandler->routines[rtnId].name << endl;
}

namespace mem
{
    PIN_LOCK lock;
    
    //not thread safe
    size_t mallocSizeRequested;
    size_t callocSizeRequested;
    
    static void mallocBefore(ExecHandler* execHandler, THREADID threadId, size_t size)
    {
        Locker locker(&lock, threadId);
        mallocSizeRequested = size;
    }

    static void mallocAfter(ExecHandler* execHandler, THREADID threadId, void* addr)
    {
        Locker locker(&lock, threadId);
        if (!addr || mallocSizeRequested == 0)
            return;
        execHandler->handleHeapAlloc(threadId, addr, mallocSizeRequested);
        mallocSizeRequested = 0;
    }
    
    static void callocBefore(ExecHandler* execHandler, THREADID threadId, size_t nmemb, size_t size)
    {
        Locker locker(&lock, threadId);
        callocSizeRequested = nmemb*size;
    }

    static void callocAfter(ExecHandler* execHandler, THREADID threadId, void* addr)
    {
        Locker locker(&lock, threadId);
        if (!addr || callocSizeRequested == 0)
            return;
        execHandler->handleHeapAlloc(threadId, addr, callocSizeRequested);
        callocSizeRequested = 0;
    }

    static void freeBefore(ExecHandler* execHandler, THREADID threadId, void* addr)
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
};


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

void HeapInfo::handleAlloc(void* addr, size_t size, const std::string& varName)
{
    static int id = 0;
    auto* varInfo = new dbg::VarInfo(dbg::StorageType::Dynamic,
                                     varName + "#" + to_string(id++), 
                                     size, (ssize_t)addr);
    auto ret = objects.insert(MemoryObject(addr, size, varInfo));
    if (!ret.second)
    {
        //cout << "heap-error: " << addr << " " << size << endl;
    }
    cout << "alloc: " << size << endl;
}

void HeapInfo::handleFree(void* addr)
{
    auto it = objects.find(MemoryObject(addr));
    if (it == objects.end())
        return;
    delete it->varInfo;
    objects.erase(it);
}

MemoryObject HeapInfo::findObject(void* addr, size_t size) const
{
    //cout << "heap " << objects.size() << endl;
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
    return heapInfo.findObject(addr, size);
}

MemoryObject ExecHandler::findStackObject(void* addr, size_t size)
{
    for (int i = 0; i < callStacks.size(); i++)
    {
        auto mo = callStacks[0].findObject(addr, size);
        if (!mo.isEmpty())
            return mo;
    }
    return MemoryObject();
}

string ExecHandler::getVarNameFromFile(const string& filePath, int lineNumber) const
{
    //optimize: preload all files before analysis
    if (lineNumber == 0 || filePath.empty())
        return "";
    ifstream in(filePath);
    string line;
    for (int i = 0; i < lineNumber; i++)
    {
        getline(in, line);
        if (in.eof())
            return "";
    }
    if (line.find("alloc") == string::npos)
        return "";
    auto pos = line.find("=");
    assert(pos != string::npos);
    
    pos--;
    while (isspace(line[pos]))
        pos--;
        
    string var;
    for (int i = pos; i >= 0; i--)
    {
        char c = line[i];
        if (!isalnum(c) && c != '_')
            break;
        var = line[i] + var ;
    }
    assert(!var.empty());
    cout << "var: " << var << endl;
    return var;
}

ExecHandler::ExecHandler(const string& binPath, const dbg::DebugContext& dbgCtxt) :
    binPath(binPath),
    callStacks(MaxThreads),
    dbgCtxt(dbgCtxt)
{
    PIN_InitLock(&lock);
}

int ExecHandler::getRoutineId(RTN rtn)
{
    int id = RTN_Id(rtn);
    string name = RTN_Name(rtn);
    routines[id] = RoutineInfo(id, name);
    return id;
}

void ExecHandler::handleHeapAlloc(THREADID threadId, void* addr, size_t size)
{
    ADDRINT instAddr = callStacks[threadId].lastCallInstruction;
    
    int column;
    int line;
    string fileName;
    PIN_LockClient();
    PIN_GetSourceLocation(instAddr, &column, &line, &fileName);
    PIN_UnlockClient();
    //cout << "malloc before: " << column << " " << line << " " << fileName << endl;
    string name = getVarNameFromFile(fileName, line);
    heapInfo.handleAlloc(addr, size, name);
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
    if (!funcInfo)
        return;
    
    cout << "call " << rtnInfo.name << endl;
    //cout << endl << "call stack: " << endl;
    // cout << callStacks[threadId].str() << endl;
    // cout << endl;
    // cout << "frame base is known: " << rtnInfo.name << endl;
    void* frameBase = (char*)PIN_GetContextReg(ctxt, REG_STACK_PTR) + funcInfo->loc;
    callStacks[threadId].addCall(FuncCall(funcInfo, frameBase));
}

void ExecHandler::handleRoutineExit(CONTEXT* ctxt, THREADID threadId, int routineId)
{
    Locker locker(&lock, threadId);
    auto& rtnInfo = routines[routineId];
    auto* funcInfo = dbgCtxt.findFuncByName(rtnInfo.name);
    if (!funcInfo)
        return;
    
    cout << "exit " << rtnInfo.name << endl;
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

void ExecHandler::instrumentRoutine(RTN rtn)
{
    int id = getRoutineId(rtn);
    //string name = RTN_Name(rtn);
    RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)routineEnter,
                   IARG_PTR, this, IARG_CONTEXT,
                   IARG_THREAD_ID, IARG_UINT32, id, IARG_END);
    RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)routineExit,
                   IARG_PTR, this, IARG_CONTEXT,
                   IARG_THREAD_ID, IARG_UINT32, id, IARG_END);
}

void ExecHandler::instrumentInstruction(INS ins)
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
                ins, IPOINT_BEFORE, (AFUNPTR)mem::memoryRead,
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
                ins, IPOINT_BEFORE, (AFUNPTR)mem::memoryWrite,
                IARG_PTR, this, IARG_INST_PTR,
                IARG_THREAD_ID,
                IARG_MEMORYOP_EA, memOp,
                IARG_UINT32, size,
                IARG_END);
        }
        assert(f);
    }
    //benchmark for separate function calls
    if (INS_IsCall(ins) && INS_IsDirectBranchOrCall(ins))
    {
        INS next = INS_Next(ins);
        if (!INS_Valid(next))
            return;
        ADDRINT target = INS_DirectBranchOrCallTargetAddress(ins);
        RTN rtn = RTN_FindByAddress(target);
        if (!RTN_Valid(rtn))
            return;
        //skip .plt
        if (RTN_Name(rtn) == ".plt")
            return;
        int id = getRoutineId(rtn);
        
        INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)routineCall2Before,
                IARG_PTR, this,
                IARG_THREAD_ID, IARG_INST_PTR,
                IARG_UINT32, id,
                IARG_END);
        INS_InsertPredicatedCall(
                next, IPOINT_BEFORE, (AFUNPTR)routineCall2After,
                IARG_PTR, this,
                IARG_THREAD_ID, IARG_INST_PTR,
                IARG_UINT32, id,
                IARG_END);
    }
    //cout << INS_Disassemble(ins) << endl;
}

void ExecHandler::instrumentImageLoad(IMG img)
{
    bool external = IMG_Name(img) != binPath;
    for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
        for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn))
        {
            RTN_Open(rtn);
            if (external)
                instrumentRoutineExternal(rtn);
            else
            {
                instrumentRoutine(rtn);
                for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins))
                    instrumentInstruction(ins);
            }
            RTN_Close(rtn);
        }
}

void ExecHandler::instrumentRoutineExternal(RTN rtn)
{
    string name = RTN_Name(rtn);
    if (name == "malloc" || name == "__libc_malloc")
    {
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)mem::mallocBefore,
                   IARG_PTR, this, IARG_THREAD_ID,
                   IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_END);
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)mem::mallocAfter,
                   IARG_PTR, this, IARG_THREAD_ID,
                   IARG_FUNCRET_EXITPOINT_VALUE, IARG_END);
    }
    if (name == "calloc" || name == "__libc_calloc")
    {
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)mem::callocBefore,
                   IARG_PTR, this, IARG_THREAD_ID,
                   IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                   IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                   IARG_END);
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)mem::callocAfter,
                   IARG_PTR, this, IARG_THREAD_ID,
                   IARG_FUNCRET_EXITPOINT_VALUE, IARG_END);
    }
    else if (name == "free" || name == "__libc_free")
    {
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)mem::freeBefore,
                   IARG_PTR, this, IARG_THREAD_ID,
                   IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_END);
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

void ExecHandler::setLastCallInstruction(ADDRINT instAddr, THREADID threadId)
{
    Locker locker(&lock, threadId);
    callStacks[threadId].lastCallInstruction = instAddr;
}