#include "pinhandler.h"
#include "common/utils.h"
using namespace std;

namespace pin
{
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

    namespace mem
    {
        PIN_LOCK lock;

        //not thread safe
        size_t mallocSizeRequested;
        size_t callocSizeRequested;

        static void mallocBefore(PinHandler* execHandler, THREADID threadId, size_t size)
        {
            Locker locker(&lock, threadId);
            mallocSizeRequested = size;
        }

        static void mallocAfter(PinHandler* execHandler, THREADID threadId, void* addr)
        {
            Locker locker(&lock, threadId);
            if (!addr || mallocSizeRequested == 0)
            {
                return;
            }
            execHandler->handleHeapAlloc(threadId, addr, mallocSizeRequested);
            mallocSizeRequested = 0;
        }

        static void callocBefore(PinHandler* execHandler, THREADID threadId, size_t nmemb, size_t size)
        {
            Locker locker(&lock, threadId);
            callocSizeRequested = nmemb*size;
        }

        static void callocAfter(PinHandler* execHandler, THREADID threadId, void* addr)
        {
            Locker locker(&lock, threadId);
            if (!addr || callocSizeRequested == 0)
                return;
            execHandler->handleHeapAlloc(threadId, addr, callocSizeRequested);
            callocSizeRequested = 0;
        }

        static void freeBefore(PinHandler* execHandler, THREADID threadId, void* addr)
        {
            execHandler->handleHeapFree(threadId, addr);
        }

        static void memoryRead(PinHandler* execHandler, VOID* ip, THREADID threadId, VOID* addr, UINT32 size)
        {
            execHandler->handleMemoryRead(threadId, addr, size, ip);
        }

        static void memoryWrite(PinHandler* execHandler, VOID* ip, THREADID threadId, VOID* addr, UINT32 size)
        {
            execHandler->handleMemoryWrite(threadId, addr, size, ip);
        }
    };

    static void routineEnter(PinHandler* execHandler, CONTEXT* ctxt, THREADID threadId, UINT32 rtnId)
    {
        execHandler->handleRoutineEnter(ctxt, threadId, rtnId);
    }

    static void routineExit(PinHandler* execHandler, CONTEXT* ctxt, THREADID threadId, UINT32 rtnId)
    {
        execHandler->handleRoutineExit(ctxt, threadId, rtnId);
    }

    static void callInstBefore(PinHandler* execHandler, THREADID threadId, ADDRINT instAddr, UINT32 rtnId)
    {
        //cout << "routineCallAnyBefore " << execHandler->routines[rtnId].name << endl;
        execHandler->handleCallInst(instAddr, threadId);
    }

    static void callInstAfter(PinHandler* execHandler, THREADID threadId, ADDRINT instAddr, UINT32 rtnId)
    {
        //cout << "routineCall2After: " << execHandler->routines[rtnId].name << endl;
    }

    //------------------------------------------------------------------------------
    //PinHandler
    //------------------------------------------------------------------------------

    PinHandler::PinHandler(const string& binPath, dbginfo::DebugContext& dbgCtxt) :
        binPath(binPath),
        execCtxt(binPath, dbgCtxt)
    {
        PIN_InitLock(&lock);
    }

    void PinHandler::handleHeapAlloc(THREADID threadId, void* addr, size_t size)
    {
        Locker locker(&lock, threadId);
        Event e(EventType::Alloc, threadId, addr, size);
        execCtxt.addEvent(e);
    }

    void PinHandler::handleHeapFree(THREADID threadId, void* addr)
    {
        Locker locker(&lock, threadId);
        Event e(EventType::Free, threadId, addr);
        execCtxt.addEvent(e);
    }

    void PinHandler::handleRoutineEnter(CONTEXT* ctxt, THREADID threadId, int routineId)
    {
        Locker locker(&lock, threadId);
        Event e(EventType::Call, threadId, routineId, (void*)PIN_GetContextReg(ctxt, REG_STACK_PTR));
        execCtxt.addEvent(e);
    }

    void PinHandler::handleRoutineExit(CONTEXT* ctxt, THREADID threadId, int routineId)
    {
        Locker locker(&lock, threadId);
        Event e(EventType::Ret, threadId, routineId, (void*)PIN_GetContextReg(ctxt, REG_STACK_PTR));
        execCtxt.addEvent(e);
    }

    void PinHandler::handleMemoryRead(THREADID threadId, void* addr, size_t size, VOID* ip)
    {
        Locker locker(&lock, threadId);
        Event e(EventType::Read, threadId, addr, size, (uint64_t)ip);
        execCtxt.addEvent(e);
    }

    void PinHandler::handleMemoryWrite(THREADID threadId, void* addr, size_t size, VOID* ip)
    {
        Locker locker(&lock, threadId);
        Event e(EventType::Write, threadId, addr, size, (uint64_t)ip);
        execCtxt.addEvent(e);
    }

    void PinHandler::instrumentRoutine(RTN rtn)
    {
        int id = execCtxt.getRoutineId(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)routineEnter,
                       IARG_PTR, this, IARG_CONTEXT,
                       IARG_THREAD_ID, IARG_UINT32, id, IARG_END);
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)routineExit,
                       IARG_PTR, this, IARG_CONTEXT,
                       IARG_THREAD_ID, IARG_UINT32, id, IARG_END);
    }

    void PinHandler::instrumentInstruction(INS ins)
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
            int id = execCtxt.getRoutineId(rtn);

            INS_InsertPredicatedCall(
                    ins, IPOINT_BEFORE, (AFUNPTR)callInstBefore,
                    IARG_PTR, this,
                    IARG_THREAD_ID, IARG_INST_PTR,
                    IARG_UINT32, id,
                    IARG_END);
            INS_InsertPredicatedCall(
                    next, IPOINT_BEFORE, (AFUNPTR)callInstAfter,
                    IARG_PTR, this,
                    IARG_THREAD_ID, IARG_INST_PTR,
                    IARG_UINT32, id,
                    IARG_END);
        }
    }

    void PinHandler::instrumentImageLoad(IMG img)
    {
        bool external = IMG_Name(img) != binPath;
        for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
        {
            for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn))
            {
                RTN_Open(rtn);
                if (external)
                {
                    instrumentRoutineExternal(rtn);
                }
                else
                {
                    instrumentRoutine(rtn);
                    for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins))
                    {
                        instrumentInstruction(ins);
                    }
                }
                RTN_Close(rtn);
            }
        }
    }

    void PinHandler::instrumentRoutineExternal(RTN rtn)
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

    void PinHandler::handleCallInst(ADDRINT instAddr, THREADID threadId)
    {
        Locker locker(&lock, threadId);
        Event e(EventType::CallInst, threadId, -1, nullptr, instAddr);
        execCtxt.addEvent(e);
    }

    EventManager PinHandler::dumpEvents()
    {
        return execCtxt.dumpEvents();
    }
} //namespace pin
