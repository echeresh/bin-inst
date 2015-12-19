#include "execcontext.h"
using namespace std;

namespace inst
{
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
                                         varName + "#" + std::to_string(id++),
                                         size, (ssize_t)addr);
        auto ret = objects.insert(MemoryObject(addr, size, varInfo));
        if (!ret.second)
        {
            //cout << "heap-error: " << addr << " " << size << endl;
        }
        cout << "alloc: " << size << "#" + std::to_string(id) << endl;
    }

    void HeapInfo::handleFree(void* addr)
    {
        auto it = objects.find(MemoryObject(addr));
        if (it == objects.end())
            return;
        //delete it->varInfo;
        objects.erase(it);
    }

    MemoryObject HeapInfo::findObject(void* addr, size_t size) const
    {
        //cout << "heap " << objects.size() << endl;
        auto it = lessFirst(objects, addr);
        if (it == objects.end())
        {
            std::cout << "not found1" << std::endl;
            return MemoryObject();
        }

        if (it->lo() <= addr && addr < it->hi())
        {
            assert((char*)addr + size <= it->hi());
            return *it;
        }
        std::cout << "not found2" << std::endl;
        return MemoryObject();
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
        static double tt;
        double t = dsecnd();
        MemoryObject mo;
        if (addr > (void*)0x70000000000)
        {
            mo = findStackObject(addr, size);
        }
        else
        {
            mo = findNonStackObject(addr, size);
        }

        t = dsecnd() - t;
        tt += t;
        //cout << t << " " << tt << endl;
        return mo;
    }

    void ExecContext::processEvents()
    {
        eventKeeper.finish();

        std::ofstream callOut("calls");
        std::map<long long, std::ofstream*> accFiles;
        std::map<const dbg::FuncInfo*, int> routineLabels;
        std::map<const dbg::FuncInfo*, std::ostringstream*> routineCalls;
        eventKeeper.reset();
        int processed = 0;
        double t_all = dsecnd();
        double tt = 0;
        while (eventKeeper.hasNext())
        {
            double t = dsecnd();
            processed++;
            if (processed % 10 == 0)
            {
                std::cout << "completed: " << processed * 100. / eventKeeper.totalEvents << "%" << std::endl;
            }

            Event e = eventKeeper.next();

            //cout << e.str(*this) << endl;
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
                    cout << "malloc before: " << column << " " << line << " " << fileName << endl;
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
                    //cout << "call or ret" << endl;
                    if (!funcInfo)
                    {
                        break;
                    }

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
                    auto& memoryEvent = e.memoryEvent;
                    auto mo = findObject(memoryEvent.addr, memoryEvent.size);
                    //cout << "rw object" << endl;
                    if (!mo.isEmpty())
                    {
                        patternAnalyzer.pushAccess(pattern::Access((byte*)memoryEvent.addr, memoryEvent.size,
                            e.type == EventType::Read ? pattern::AccessType::Read : pattern::AccessType::Write,
                            mo.name()));
                        cout << "rw found " << mo.name() << endl;
                        assert(mo.varInfo);
                        long long hc = (long long)mo.varInfo >> 3;
                        hc += memoryEvent.threadId * 2 + (e.type == EventType::Read ? 0 : 1);
                        //cout << "hc = " << hc << endl;
                        if (accFiles.find(hc) == accFiles.end())
                        {
                            cout << "hc2 = " << hc << endl;
                            ostringstream oss;
                            oss << "_trash/acc-" << (e.type == EventType::Read ? "R-" : "W-")
                                << mo.name() << "-sz-"
                                << mo.size << "-thr-" << (memoryEvent.threadId == 0 ? 0 : memoryEvent.threadId - 1);
                            //skipped internal thread with id = 1
                            accFiles[hc] = new ofstream(oss.str());
                        }
                        *accFiles[hc] << (uint64_t)memoryEvent.addr << " " << memoryEvent.t << endl;
                    }
                    break;
                }
            }
            t = dsecnd() - t;
            tt += t;
            //cout << "(1) " << t << endl;
        }
        cout << dsecnd() - t_all << endl;
        cout << tt << endl;
        for (auto& e : routineCalls)
        {
            callOut << e.second->str();
            delete e.second;
        }

        for (auto& e : accFiles)
        {
            delete e.second;
        }
    }

    const RoutineInfo* ExecContext::findRoutine(int routineId) const
    {
        auto it = routines.find(routineId);
        return it == routines.end() ? nullptr : &it->second;
    }
} //namespace inst
