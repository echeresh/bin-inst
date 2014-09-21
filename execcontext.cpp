#include "execcontext.h"
using namespace std;

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
//ExecContext
//------------------------------------------------------------------------------

inline std::string to_string(EventType type)
{
    switch (type)
    {
    case EventType::Call:
        return "Call";
    case EventType::Ret:
        return "Ret";
    case EventType::Read:
        return "Read";
    case EventType::Write:
        return "Write";
    case EventType::Alloc:
        return "Alloc";
    case EventType::Free:
        return "Free";
    }
   assert(false);
   return "";
}

//------------------------------------------------------------------------------
//Events classes
//------------------------------------------------------------------------------

std::string MemoryEvent::str(const ExecContext& execCtxt) const
{
    ostringstream oss;
    oss << "thread: " << threadId << "; addr: " << addr
        << "; size: " << size << "; t: " << t;
    return oss.str();
}


std::string RoutineEvent::str(const ExecContext& execCtxt) const
{
    ostringstream oss;
    auto* rtnInfo = execCtxt.findRoutine(routineId);
    if (rtnInfo)
        oss << rtnInfo->name << "; ";
    oss << "thread: " << threadId << "; t: " << t;
    return oss.str();
}

Event::Event(EventType type, THREADID threadId, void* addr, size_t size, ADDRINT instAddr) :
    type(type)
{
    switch (type)
    {
    case EventType::Read:
    case EventType::Write:
    case EventType::Alloc:
    case EventType::Free:
        memoryEvent.t = rdtsc();
        memoryEvent.threadId = threadId;
        memoryEvent.addr = addr;
        memoryEvent.size = size;
        memoryEvent.instAddr = instAddr;
        break;
    default:
        assert(false);
    }
}

Event::Event(EventType type, THREADID threadId, int routineId, void* spValue, ADDRINT instAddr) :
    type(type)
{
    switch (type)
    {
    case EventType::CallInst:
    case EventType::Call:
    case EventType::Ret:
        routineEvent.t = rdtsc();
        routineEvent.threadId = threadId;
        routineEvent.routineId = routineId;
        routineEvent.spValue = spValue;
        routineEvent.instAddr = instAddr;
        break;
    default:
        assert(false);
    }
}

std::string Event::str(const ExecContext& execCtxt) const
{
    ostringstream oss;
    oss << ::to_string(type) << "[";
    switch (type)
    {
    case EventType::Read:
    case EventType::Write:
    case EventType::Alloc:
    case EventType::Free:
        oss << memoryEvent.str(execCtxt);
        break;
    case EventType::Call:
    case EventType::Ret:
        oss << routineEvent.str(execCtxt);
        break;
    }
    oss << "]";
    return oss.str();
}

std::string EventKeeper::EventPath = "/sda3/events";
int EventKeeper::EventBlockLength = 10000000;

EventKeeper::EventKeeper() :
    pEvents0(&events0),
    pEvents1(&events1)
{
    remove(EventPath.c_str());
    PIN_SemaphoreInit(&saved);
    PIN_SemaphoreInit(&filled);
    PIN_SemaphoreSet(&saved);
    PIN_SpawnInternalThread(threadFunc, this, 0, nullptr);
}
    
void EventKeeper::threadFunc(void* arg)
{
    EventKeeper* es = (EventKeeper*)arg;
    for(;;)
    {
        //cout << "thread: wait filled" << endl;
        PIN_SemaphoreWait(&es->filled);
        //cout << "thread: clear filled" << endl;
        PIN_SemaphoreClear(&es->filled);
        
        ofstream out(EventPath, ios::app | ios::binary);
        out.write((char*)&(*es->pEvents1)[0], es->pEvents1->size()*sizeof(Event));
        cout << "EVENTS WERE DROPPED: " << es->pEvents1->size() << " : " << es->pEvents1->size()*sizeof(Event) << endl;
        es->totalEvents += es->pEvents1->size();
        es->pEvents1->clear();
        es->pEvents1->shrink_to_fit();
        //cout << "thread: set saved" << endl;
        PIN_SemaphoreSet(&es->saved);
        if (es->finished)
            return;
    }
}
    
void EventKeeper::addEvent(const Event& event)
{
    pEvents0->push_back(event);
    
    //cout << "\r" << pEvents0->size();

    if (pEvents0->size() > EventBlockLength) 
    {
        auto t = rdtsc();
        PIN_SemaphoreWait(&saved);
        PIN_SemaphoreClear(&saved);
        
        // auto* tmp = pEvents1;
        // pEvents1 = pEvents0;
        // pEvents0 = tmp;
        swap(pEvents0, pEvents1);
        
        PIN_SemaphoreSet(&filled);
        cout << "QUEUED: " << rdtsc() - t << endl;
    }
}

void EventKeeper::finish()
{
    PIN_SemaphoreWait(&saved);
    PIN_SemaphoreClear(&saved);
    swap(pEvents0, pEvents1);
    finished = true;
    PIN_SemaphoreSet(&filled);
    PIN_SemaphoreWait(&saved);
}

void EventKeeper::reset()
{
    iterIndex = 0;
    preloadEvents.resize(EventBlockLength);
}

bool EventKeeper::hasNext()
{
    return iterIndex < totalEvents;
}

Event EventKeeper::next()
{
    assert(hasNext());
    int mod = iterIndex % EventBlockLength;
    if (mod == 0)
    {
        ifstream in(EventPath, ios::binary);
        in.seekg(iterIndex*sizeof(Event), in.beg);
        in.read((char*)&preloadEvents[0], EventBlockLength*sizeof(Event));
    }
    iterIndex++;
    return preloadEvents[mod];
}

//------------------------------------------------------------------------------
//ExecContext
//------------------------------------------------------------------------------

 ExecContext::ExecContext(const std::string& binPath, const dbg::DebugContext& dbgCtxt) :
    binPath(binPath),
    dbgCtxt(dbgCtxt),
    callStacks(MaxThreads)
    
{
}
MemoryObject ExecContext::findNonStackObject(void* addr, size_t size)
{
    auto mo = heapInfo.findObject(addr, size);
    if (!mo.isEmpty())
        return mo;
    auto* varInfo = dbgCtxt.findVarByAddress(addr);
    if (varInfo)
        return MemoryObject((void*)varInfo->loc, varInfo->size, varInfo);
    return MemoryObject();
}

MemoryObject ExecContext::findStackObject(void* addr, size_t size)
{
    int i = 0;
    //for (int i = 0; i < callStacks.size(); i++)
    {
        auto mo = callStacks[i].findObject(addr, size);
        if (!mo.isEmpty())
            return mo;
    }
    return MemoryObject();
}

std::string ExecContext::getVarNameFromFile(const std::string& filePath, int lineNumber) const
{
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

int ExecContext::getRoutineId(RTN rtn)
{
    int id = RTN_Id(rtn);
    string name = RTN_Name(rtn);
    routines[id] = RoutineInfo(id, name);
    return id;
}

void ExecContext::addEvent(const Event& event)
{
    eventKeeper.addEvent(event);
}

MemoryObject ExecContext::findObject(void* addr, int size)
{
    if (addr > (void*)0x70000000000)
        return findStackObject(addr, size);
    //auto mo = findStackObject(addr, size);
    //if (mo.isEmpty())
        return findNonStackObject(addr, size);
    //return mo;
}

void ExecContext::processEvents()
{
    ofstream callOut("calls");
    map<long long, ofstream*> accFiles;
    map<const dbg::FuncInfo*, int> routineLabels;
    map<const dbg::FuncInfo*, ostringstream*> routineCalls;
    eventKeeper.reset();
    int processed = 0;
    while (eventKeeper.hasNext())
    {
        processed++;
        cout << "completed: " << processed*100./eventKeeper.totalEvents << "%" << endl;
        Event e = eventKeeper.next();
        switch (e.type)
        {
        case EventType::Alloc:
        {
            int column;
            int line;
            string fileName;
            PIN_LockClient();
            PIN_GetSourceLocation(callStacks[e.memoryEvent.threadId].lastCallInstruction,
                                  &column, &line, &fileName);
            PIN_UnlockClient();
            //cout << "malloc before: " << column << " " << line << " " << fileName << endl;
            string name = getVarNameFromFile(fileName, line);
            heapInfo.handleAlloc(e.memoryEvent.addr, e.memoryEvent.size, name);
            break;
        }
        case EventType::Free:
        {
            heapInfo.handleFree(e.memoryEvent.addr);
            break;
        }
        case EventType::CallInst:
        {
            callStacks[e.routineEvent.threadId].lastCallInstruction =
                e.routineEvent.instAddr;
            break;
        }
        case EventType::Call:
        case EventType::Ret:
        {
            auto& rtnInfo = routines[e.routineEvent.routineId];
            auto* funcInfo = dbgCtxt.findFuncByName(rtnInfo.name);
            if (!funcInfo)
                break;
    
            if (e.type == EventType::Call)
            {
                //cout << "call " << rtnInfo.name << endl;
                void* frameBase = (char*)e.routineEvent.spValue + funcInfo->loc;
                callStacks[e.routineEvent.threadId].addCall(FuncCall(funcInfo, frameBase));
            }
            else
            {
                //cout << "exit " << rtnInfo.name << endl;
                assert(callStacks[e.routineEvent.threadId].top().funcInfo == funcInfo);
                void* frameBase = (char*)e.routineEvent.spValue + funcInfo->loc;
                assert(callStacks[e.routineEvent.threadId].top().frameBase == frameBase);
                callStacks[e.routineEvent.threadId].pop();
            }
            
            if (routineLabels.find(funcInfo) == routineLabels.end())
            {
                routineLabels[funcInfo] = -routineLabels.size()*128;
                routineCalls[funcInfo] = new ostringstream;
            }
            int ylabel = routineLabels[funcInfo];
            auto& oss = *routineCalls[funcInfo];
            oss << e.routineEvent.t << " " << ylabel << " " << rtnInfo.name << endl;
            if (e.type == EventType::Ret)
                oss << endl;
            
            break;
        }
        case EventType::Read:
        case EventType::Write:
        {
            auto mo = findObject(e.memoryEvent.addr, e.memoryEvent.size);
            if (!mo.isEmpty())
            {
                assert(mo.varInfo);
                long long hc = (long long)mo.varInfo << 8;
                hc += e.memoryEvent.threadId*2 + (e.type == EventType::Read ? 0 : 1);
                if (accFiles.find(hc) == accFiles.end())
                {
                    ostringstream oss;
                    oss << "acc-" << (e.type == EventType::Read ? "R-" : "W-")
                        << mo.name() << "-sz-"
                        << mo.size << "-thr-" << (e.memoryEvent.threadId == 0 ? 0 : e.memoryEvent.threadId - 1);
                    //skipped internal thread with id = 1
                    accFiles[hc] = new ofstream(oss.str());
                }
                *accFiles[hc] << (uint64_t)e.memoryEvent.addr << " " << e.memoryEvent.t << endl;
            }
            break;
        }
        }
    }
    for (auto& e : routineCalls)
    {
        callOut << e.second->str();
        delete e.second;
    }
    
    for (auto& e : accFiles)
        delete e.second;
}

const RoutineInfo* ExecContext::findRoutine(int routineId) const
{
    auto it = routines.find(routineId);
    return it == routines.end() ? nullptr : &it->second;
}

ExecContext::~ExecContext()
{
    eventKeeper.finish();
    processEvents();
}