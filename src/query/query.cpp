#include <fstream>
#include <iostream>
#include "common/event/eventmanager.h"
#include "querymanager/querymanager.h"
#include "querymanager/querycontext.h"
#include "config.h"
using namespace std;

int main()
{
    dbginfo::DebugContext debugContext;
    std::ifstream dbgIn(DEBUG_INFO_PATH, std::ios::binary);
    debugContext.load(dbgIn);

    EventManager eventManager(debugContext);
    std::ifstream eventIn(EVENT_REF_PATH, std::ios::binary);
    eventManager.load(eventIn);

    QueryManager qm(eventManager);
    QueryContext qctxt;
    qctxt.acceptFunc(debugContext.findFuncByName("mul1"));
    auto accMat = qm.buildAccessMatrix(qctxt);
    cout << accMat.str() << endl;
    return 0;
}
