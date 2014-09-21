#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <set>
#include <cassert>
#include <thread>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include "pin.H"
#include "dwarfparser.h"
#include "exechandler.h"
#include "debuginfo.h"

ExecHandler* execHandler;
dwarf::Parser* parser;
 
// VOID Instruction(INS ins, VOID *v)
// {
    // execHandler->instrumentInstruction(ins);
// }

// VOID Trace(TRACE trace, VOID *v)
// {
    // execHandler->instrumentTrace(trace);
// }

VOID ImageLoad(IMG img, VOID* v)
{
    execHandler->instrumentImageLoad(img);
}

VOID ThreadStart(THREADID threadId, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    //ADDRINT stackBase = PIN_GetContextReg(ctxt, REG_STACK_PTR);
    struct rlimit rlim;
    if (getrlimit(RLIMIT_STACK, &rlim))
    {
        cerr << "\n Failed to getrlimit()\n";
        PIN_ExitProcess(-1);
    }

    if (rlim.rlim_cur == RLIM_INFINITY)
    {
        cerr << "\n Need a finite stack size. Dont use unlimited.\n";
        PIN_ExitProcess(-1);
    }

    //void* stackEnd = (void*)(stackBase - rlim.rlim_cur);
    //cout << "stack end : " << stackEnd << endl;
    // MemoryAllocation ma;
    // ma.lo = (void*)stackEnd;
    // ma.hi = (void*)stackBase;
    // ma.threadId = threadId;
    // memInfos[threadId].stack = ma;
    // memInfos[threadId].threadId = threadId;
}

VOID ThreadFini(THREADID threadId, const CONTEXT* ctxt, INT32 code, VOID *v)
{
}

VOID Fini(INT32 code, VOID *v)
{
    cout << "Fini" << endl;
    cout.flush();
    // printStacks(cout);
    // printHeapAllocs(cout);
    // for (size_t i = 0; i < memInfos.size() && memInfos[i].threadId != -1; i++)
    // {
        // int totalWrites = 0;
        // int totalReads = 0;
        // for (auto& e : memInfos[i].heapWriteAccesses)
            // totalWrites += e.second.size();
        // for (auto& e : memInfos[i].heapReadAccesses)
            // totalReads += e.second.size();
        // cout << "thread #" << i << " total write blocks: " << totalWrites << endl;
        // cout << "thread #" << i << " total read blocks:  " << totalReads << endl;
    // }
    //execHandler->saveMemoryAccesses();
    delete parser;
    delete execHandler;
}

dbg::DebugContext dbgCtxt;

int main(int argc, char* argv[])
{
    string binPath;
    for (int i = 0; i < argc; i++)
        if (string(argv[i]) == "--")
        {
            assert(i < argc - 1);
            char buf[PATH_MAX];
            assert(realpath(argv[i + 1], buf));
            binPath = string(buf);            
        }

    parser = new dwarf::Parser(binPath);
    parser->cuWalk(bind(&dwarf::Parser::walk, parser, placeholders::_1, placeholders::_2));
    dbgCtxt = parser->getDebugContext().toDbg();
    
    execHandler = new ExecHandler(binPath, dbgCtxt);

    PIN_InitSymbols();
    if (PIN_Init(argc, argv))
        return -1;

    cout << "======= PIN" << endl;
    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);
    //INS_AddInstrumentFunction(Instruction, 0);
    //TRACE_AddInstrumentFunction(Trace, 0);
    IMG_AddInstrumentFunction(ImageLoad, 0);
    PIN_AddFiniUnlockedFunction(Fini, 0);
   
    PIN_StartProgram();
    return 0;
}
