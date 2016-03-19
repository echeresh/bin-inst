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
#include "dwarf/dwarfparser.h"
#include "pin/pinhandler.h"
#include "debuginfo/debuginfo.h"
#include "query/querymanager/querymanager.h"

static pin::PinHandler* pinHandler;
static dwarf::DwarfParser* parser;
static dbginfo::DebugContext dbgCtxt;

VOID Instruction(INS ins, VOID *v)
{
    pinHandler->instrumentInstruction(ins);
}

//VOID Trace(TRACE trace, VOID *v)
//{
 //   pinHandler->instrumentTrace(trace);
//}

VOID ImageLoad(IMG img, VOID* v)
{
    pinHandler->instrumentImageLoad(img);
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
}

VOID ThreadFini(THREADID threadId, const CONTEXT* ctxt, INT32 code, VOID *v)
{
}

VOID Fini(INT32 code, VOID *v)
{
    EventManager eventManager = pinHandler->dumpEvents();

    std::ofstream outEvent(EVENT_REF_PATH, std::ios::binary);
    eventManager.save(outEvent);

    std::ofstream outDbg(DEBUG_INFO_PATH, std::ios::binary);
    dbgCtxt.save(outDbg);
    
    delete parser;
    delete pinHandler;
}

int main(int argc, char* argv[])
{
    std::string binPath;
    for (int i = 0; i < argc; i++)
        if (string(argv[i]) == "--")
        {
            assert(i < argc - 1);
            char buf[PATH_MAX];
            assert(realpath(argv[i + 1], buf));
            binPath = string(buf);
        }

    parser = new dwarf::DwarfParser(binPath);
    parser->cuWalk(bind(&dwarf::DwarfParser::walk, parser, placeholders::_1, placeholders::_2));
    dbgCtxt = parser->getDwarfContext().toDbg();

    pinHandler = new pin::PinHandler(binPath, dbgCtxt);

    PIN_InitSymbols();
    if (PIN_Init(argc, argv))
        return -1;

    cout << "======= PIN" << endl;
    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);
    INS_AddInstrumentFunction(Instruction, 0);
    //TRACE_AddInstrumentFunction(Trace, 0);
    IMG_AddInstrumentFunction(ImageLoad, 0);
    PIN_AddFiniUnlockedFunction(Fini, 0);

    PIN_StartProgram();
    return 0;
}
