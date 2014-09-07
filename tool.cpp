#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <set>
#include <cassert>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include "pin.H"
#include "dwarfparser.h"
#include "exechandler.h"

ExecHandler* execHandler;
DwarfParser* parser;
 
VOID Instruction(INS ins, VOID *v)
{
    execHandler->analyseInstruction(ins);
}

VOID ImageLoad(IMG img, VOID* v)
{    
    for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
        for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn))
            execHandler->analyseRoutine(rtn);
}

VOID ThreadStart(THREADID threadId, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    ADDRINT stackBase = PIN_GetContextReg(ctxt, REG_STACK_PTR);
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

    void* stackEnd = (void*)(stackBase - rlim.rlim_cur);
    cout << "stack end : " << stackEnd << endl;
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
    execHandler->saveMemoryAccesses();
    delete parser;
    delete execHandler;
}

int main(int argc, char* argv[])
{
    //parser = new DwarfParser(argv[argc - 1]);
    parser = new DwarfParser("test");
    parser->cuWalk(bind(&DwarfParser::walk, parser, placeholders::_1, placeholders::_2));
    execHandler = new ExecHandler(parser->getDebugContext());
    PIN_InitSymbols();
    if (PIN_Init(argc, argv))
        return -1;

    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);
    INS_AddInstrumentFunction(Instruction, 0);
    IMG_AddInstrumentFunction(ImageLoad, 0);
    PIN_AddFiniFunction(Fini, 0);
    PIN_StartProgram();
    return 0;
}
