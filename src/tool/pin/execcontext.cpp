#include <iostream>
#include "execcontext.h"
using namespace std;

VOID Fini(INT32 code, VOID *v);

namespace pin
{
    SourceLocation getSourceLocation(ADDRINT inst)
    {
        int column;
        int line;
        string fileName;
        PIN_LockClient();
        PIN_GetSourceLocation(inst, &column, &line, &fileName);
        PIN_UnlockClient();
        return SourceLocation(fileName, line);
    }

    std::string getVarNameFromFile(const SourceLocation& sourceLoc)
    {
        if (sourceLoc.line == 0 || sourceLoc.fileName.empty())
        {
            return "";
        }
        ifstream in(sourceLoc.fileName);
        string line;
        for (int i = 0; i < sourceLoc.line; i++)
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
    //------------------------------------------------------------------------------
    //MemoryObject
    //------------------------------------------------------------------------------

    MemoryObject::MemoryObject(void* addr, size_t size, const dbginfo::VarInfo* varInfo) :
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

    void HeapInfo::handleAlloc(dbginfo::DebugContext& dbgCtxt, MemoryEvent& memoryEvent, const SourceLocation& srcLoc)
    {
        static int id = 0;
        //std::string varName = getVarNameFromFile(srcLoc);
        std::cout << "alloc __dyn_#" + std::to_string(id) << " " << memoryEvent.addr << std::endl;
        auto* varInfo = dbgCtxt.addVar(dbginfo::VarInfo(dbginfo::StorageType::Dynamic,
                                                        "__dyn_" + std::to_string(id++),
                                                        memoryEvent.size, memoryEvent.size,
                                                        (ssize_t)memoryEvent.addr, srcLoc));
        memoryEvent.varId = varInfo->id;
        auto ret = objects.insert(MemoryObject(memoryEvent.addr, memoryEvent.size, varInfo));
        assert(ret.second);
    }

    void HeapInfo::handleFree(dbginfo::DebugContext& dbgCtxt, MemoryEvent& memoryEvent)
    {
        auto it = objects.find(MemoryObject(memoryEvent.addr));
        if (it == objects.end())
        {
            return;
        }
        std::cout << "free " << it->varInfo->name << " " << memoryEvent.addr << std::endl;
        objects.erase(it);
    }

    MemoryObject HeapInfo::findObject(const MemoryEvent& memoryEvent) const
    {
        size_t size = memoryEvent.size;
        void* addr = memoryEvent.addr;
        auto it = lessFirst(objects, addr);
        if (it == objects.end())
        {
            return MemoryObject();
        }

        if (it->lo() <= addr && addr < it->hi())
        {
            if (!((char*)addr + size <= it->hi()))
            {
                auto sourceLoc = getSourceLocation(memoryEvent.instAddr);
                std::cout << sourceLoc.str() << std::endl;
                std::cout << "assert 131: " << std::endl;
                std::cout << addr << std::endl;
                std::cout << size << std::endl;
                std::cout << it->lo() << std::endl;
                std::cout << it->hi() << std::endl;
            }
            return *it;
        }
        return MemoryObject();
    }

    //------------------------------------------------------------------------------
    //ExecContext
    //------------------------------------------------------------------------------

    ExecContext::ExecContext(const std::string& binPath, dbginfo::DebugContext& dbgCtxt) :
        binPath(binPath),
        dbgCtxt(dbgCtxt),
        callStackGlobal(MAX_THREADS)

    {
    }

    MemoryObject ExecContext::findNonStackObject(const MemoryEvent& memoryEvent)
    {
        size_t size = memoryEvent.size;
        void* addr = memoryEvent.addr;
        if (heapSupportEnabled)
        {
            auto mo = heapInfo.findObject(memoryEvent);
            if (!mo.isEmpty())
            {
                return mo;
            }
        }
        auto* varInfo = dbgCtxt.findVarByAddress(addr);
        if (varInfo)
        {
            return MemoryObject((void*)varInfo->stackOffset, varInfo->size, varInfo);
        }
        return MemoryObject();
    }

    MemoryObject ExecContext::findStackObject(const MemoryEvent& memoryEvent)
    {
        size_t size = memoryEvent.size;
        void* addr = memoryEvent.addr;
        int i = 0;
        {
            {
                auto& calls = callStackGlobal.getCalls(i);
                //cout << "findStackObject " << calls.size() << endl;
                for (int i = calls.size() - 1; i >= 0; i--)
                {
                    auto& call = calls[i];
                    if (addr <= call.frameBase)
                    {
                        for (auto& var : call.funcInfo->vars)
                        {
                            char* varAddr = (char*)call.frameBase + var->stackOffset;
                            if (varAddr <= addr && addr <= varAddr + var->size - 1)
                            {
                                //cout << "found object: " << var->name << endl;
                                /*if (!((char*)addr + size <= varAddr + var->size))
                                {
                                    std::cout << var->name << std::endl;
                                    std::cout << addr << std::endl;
                                    std::cout << size << std::endl;
                                    std::cout << varAddr << std::endl;
                                    std::cout << var->size << std::endl;
                                }*/
                                //assert((char*)addr + size <= varAddr + var->size);
                                return MemoryObject(varAddr, var->size, var);
                            }
                        }
                    }
                }
            }
        }
        return MemoryObject();
    }

    int ExecContext::getRoutineId(RTN rtn)
    {
        string name = RTN_Name(rtn);
        auto* func = dbgCtxt.findFuncByName(name);
        if (!func)
        {
            //cout << "--------> " << name << " -1" << endl;
            return -1;
        }
        assert(func);
        //cout << "--------> " << name << " " << func->id << endl;
        return func->id;
    }

    void ExecContext::addEvent(const Event& event)
    {
        if (event.type == EventType::Call || event.type == EventType::Ret)
        {
            int routineId = event.routineEvent.routineId;
            auto* funcInfo = dbgCtxt.findFuncById(routineId);
            if (funcInfo && funcInfo->name == "main")
            {
                if (event.type == EventType::Call)
                {
                    //std::cout << "PROFILING STARTED" << std::endl;
                    profilingEnabled = true;
                }
                else if (event.type == EventType::Ret)
                {
                    //dirty hack
                    eventDumper.addEvent(event);
                    Fini(0, nullptr);
                    exit(0);
                }
            }
        }
        if (profilingEnabled)
        {
            eventDumper.addEvent(event);
        }
    }

    MemoryObject ExecContext::findObject(const MemoryEvent& memoryEvent)
    {
        MemoryObject mo;
        if (memoryEvent.addr > (void*)0x70000000000)
        {
            mo = findStackObject(memoryEvent);
        }
        else
        {
            mo = findNonStackObject(memoryEvent);
        }
        return mo;
    }

    EventManager ExecContext::dumpEvents()
    {
        EventManager em = eventDumper.finalize(dbgCtxt);
        int processed = 0;
        double t_all = utils::dsecnd();
        double tt = 0;
        std::vector<ADDRINT> callInstructions(MAX_THREADS);
        while (em.hasNext())
        {
            processed++;
            if (processed % 10000 == 0)
            {
                std::cout << "Event postprocessing: " << processed * 100. / em.size() << "%" << " " << em.size() << std::endl;
            }

            Event& e = em.next();
            //cout << e.str(em) << endl;
            switch (e.type)
            {
                case EventType::Alloc:
                {
                    if (heapSupportEnabled)
                    {
                        auto sourceLoc = getSourceLocation(callInstructions[e.memoryEvent.threadId]);
                        heapInfo.handleAlloc(dbgCtxt, e.memoryEvent, sourceLoc);
                    }
                    break;
                }
                case EventType::Free:
                {
                    if (heapSupportEnabled)
                    {
                        heapInfo.handleFree(dbgCtxt, e.memoryEvent);
                    }
                    break;
                }
                //handle call instruction to save address of call instruction calling malloc
                case EventType::CallInst:
                {
                    auto& re = e.routineEvent;
                    callInstructions[e.routineEvent.threadId] = re.instAddr;
                    break;
                }
                case EventType::Call:
                case EventType::Ret:
                {
                    auto* funcInfo = dbgCtxt.findFuncById(e.routineEvent.routineId);
                    //cout << "ROUTINEID: " << e.routineEvent.routineId << endl;
                    if (!funcInfo)
                    {
                        break;
                    }
                    //cout << "NAME: " << funcInfo->name << endl;
                    void* frameBase = (char*)e.routineEvent.stackPointerRegister + funcInfo->stackOffset;
                    FuncCall funcCall(funcInfo, frameBase);
                    if (e.type == EventType::Call)
                    {
                        callStackGlobal.push(e.routineEvent.threadId, funcCall);
                    }
                    else
                    {
                        callStackGlobal.pop(e.routineEvent.threadId, funcCall);
                    }
                    break;
                }
                case EventType::Read:
                case EventType::Write:
                {
                    auto& memoryEvent = e.memoryEvent;
                    auto mo = findObject(memoryEvent);
                    if (!mo.isEmpty())
                    {
                        memoryEvent.varId = mo.varInfo->id;
                        if (auto sourceLoc = getSourceLocation(memoryEvent.instAddr))
                        {
                            dbgCtxt.setInstBinding(memoryEvent.instAddr, sourceLoc);
                        }
                        assert(mo.varInfo);
                    }
                    else
                    {
                        memoryEvent.varId = -1;
                    }
                    break;
                }
            }
        }
        em.dump();
        return em;
    }
} //namespace pin
