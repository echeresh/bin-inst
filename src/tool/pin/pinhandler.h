#pragma once
#include <fstream>
#include <vector>
#include <stack>
#include <map>
#include <set>
#include <cstdint>
#include "pin.H"
#include "common/debuginfo/debugcontext.h"
#include "common/event/eventmanager.h"
#include "execcontext.h"

namespace pin
{
    class PinHandler
    {
        const std::string binPath;
        PIN_LOCK lock;

        ExecContext execCtxt;

    public:
        PinHandler(const std::string& binPath, dbginfo::DebugContext& dbgCtxt);
        void handleHeapAlloc(THREADID threadId, void* addr, size_t size);
        void handleHeapFree(THREADID threadId, void* addr);
        void handleRoutineEnter(CONTEXT* ctxt, THREADID threadId, int routineId);
        void handleRoutineExit(CONTEXT* ctxt, THREADID threadId, int routineId);
        void handleMemoryRead(THREADID threadId, void* addr, size_t size, VOID* ip);
        void handleMemoryWrite(THREADID threadId, void* addr, size_t size, VOID* ip);

        void instrumentImageLoad(IMG img);
        void instrumentRoutine(RTN rtn);
        void instrumentTrace(TRACE trace);
        void instrumentInstruction(INS ins);

        void instrumentRoutineExternal(RTN rtn);
        void handleCallInst(ADDRINT instAddr, THREADID threadId);

        EventManager dumpEvents();
    };
} //namespace pin
