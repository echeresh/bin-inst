#include <fstream>
#include <iostream>
#include "common/event/eventmanager.h"
#include "config.h"
#include "querymanager/querymanager.h"
#include "querymanager/querycontext.h"
using namespace std;

int main()
{
    dbginfo::DebugContext debugContext;
    std::ifstream dbgIn(DEBUG_INFO_PATH, std::ios::binary);
    debugContext.load(dbgIn);

    EventManager eventManager(debugContext);
    std::ifstream eventIn(EVENT_REF_PATH, std::ios::binary);
    eventManager.load(eventIn);

    QueryContext qctxt;
    QueryManager qm(eventManager, qctxt);

    //qctxt.acceptFunc(debugContext.findFuncByName("mul0"));
    auto accMat = qm.getAccessMatrix();
    auto localityInfo = qm.getLocalities();
    auto patternInfo = qm.getAccessPatterns();

    cout << accMat.str() << endl;
    cout << localityInfo.str() << endl;
    cout << patternInfo.str() << endl;

    
    return 0;
}
