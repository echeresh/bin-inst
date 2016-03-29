#include <fstream>
#include <iostream>
#include "common/event/eventmanager.h"
#include "querymanager/querymanager.h"
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
    while (eventManager.hasNext())
    {
        auto e = eventManager.next();
        //cout << e.str(eventManager) << endl;
        if (e.type == EventType::Read || e.type == EventType::Write)
        {
            auto inst = e.memoryEvent.instAddr;
            if (inst > 0)
            {
                auto sourceLoc = debugContext.getInstBinding(inst);
                if (sourceLoc)
                {
                    cout << (void*)inst << " -> " << sourceLoc.fileName << ":" << sourceLoc.line << endl;
                }
            }
        }
    }
    return 0;
}
