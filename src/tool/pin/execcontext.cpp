#include <iostream>
#include "execcontext.h"
using namespace std;

namespace pin
{
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

    void HeapInfo::handleAlloc(dbginfo::DebugContext& dbgCtxt, MemoryEvent& memoryEvent, const std::string& varName)
    {
        static int id = 0;
        auto* varInfo = dbgCtxt.addVar(dbginfo::VarInfo(dbginfo::StorageType::Dynamic,
                                                        varName + "#" + std::to_string(id++),
                                                        memoryEvent.size, (ssize_t)memoryEvent.addr));
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
        objects.erase(it);
    }

    MemoryObject HeapInfo::findObject(void* addr, size_t size) const
    {
        auto it = lessFirst(objects, addr);
        if (it == objects.end())
        {
            return MemoryObject();
        }

        if (it->lo() <= addr && addr < it->hi())
        {
            assert((char*)addr + size <= it->hi());
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

    MemoryObject ExecContext::findNonStackObject(void* addr, size_t size)
    {
        auto mo = heapInfo.findObject(addr, size);
        if (!mo.isEmpty())
        {
            return mo;
        }
        auto* varInfo = dbgCtxt.findVarByAddress(addr);
        if (varInfo)
        {
            return MemoryObject((void*)varInfo->stackOffset, varInfo->size, varInfo);
        }
        return MemoryObject();
    }

    MemoryObject ExecContext::findStackObject(void* addr, size_t size)
    {
        int i = 0;
        {
            {
                auto& calls = callStackGlobal.getCalls(i);
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
                                assert((char*)addr + size <= varAddr + var->size);
                                return MemoryObject(varAddr, var->size, var);
                            }
                        }
                    }
                }
            }
        }
        return MemoryObject();
    }

    std::string ExecContext::getVarNameFromFile(const SourceLocation& sourceLoc) const
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

    SourceLocation ExecContext::getSourceLocation(ADDRINT inst) const
    {
        int column;
        int line;
        string fileName;
        PIN_LockClient();
        PIN_GetSourceLocation(inst, &column, &line, &fileName);
        PIN_UnlockClient();
        return SourceLocation(fileName, line);
    }

    int ExecContext::getRoutineId(RTN rtn)
    {
        string name = RTN_Name(rtn);
        auto* func = dbgCtxt.findFuncByName(name);
        if (!func)
        {
            cout << "--------> " << name << endl;
            return -1;
        }
        assert(func);
        return func->id;
    }

    void ExecContext::addEvent(const Event& event)
    {
        eventDumper.addEvent(event);
    }

    MemoryObject ExecContext::findObject(void* addr, int size)
    {
        MemoryObject mo;
        if (addr > (void*)0x70000000000)
        {
            mo = findStackObject(addr, size);
        }
        else
        {
            mo = findNonStackObject(addr, size);
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
            if (processed % 10 == 0)
            {
                std::cout << "completed: " << processed * 100. / em.size() << "%" << std::endl;
            }

            Event& e = em.next();
            switch (e.type)
            {
                case EventType::Alloc:
                {
                    auto sourceLoc = getSourceLocation(callInstructions[e.memoryEvent.threadId]);
                    string varName = getVarNameFromFile(sourceLoc);
                    heapInfo.handleAlloc(dbgCtxt, e.memoryEvent, varName);
                    break;
                }
                case EventType::Free:
                {
                    heapInfo.handleFree(dbgCtxt, e.memoryEvent);
                    break;
                }
                //handle call instruction to save address of call instruction calling malloc
                case EventType::CallInst:
                {
                    callInstructions[e.routineEvent.threadId] = e.routineEvent.instAddr;
                    break;
                }
                /*case EventType::Call:
                case EventType::Ret:
                {
                    auto* funcInfo = dbgCtxt.findFuncById(e.routineEvent.routineId);
                    if (!funcInfo)
                    {
                        break;
                    }

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
                }*/
                case EventType::Read:
                case EventType::Write:
                {
                    auto& memoryEvent = e.memoryEvent;
                    auto mo = findObject(memoryEvent.addr, memoryEvent.size);
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
