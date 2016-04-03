#pragma once
#include <fstream>
#include <iostream>
#include "accessmatrix.h"
#include "common/callstack.h"
#include "common/debuginfo/debugcontext.h"
#include "common/event/eventmanager.h"
#include "common/streamutils/table.h"
#include "querycontext.h"

class QueryManager
{
    EventManager& eventManager;

public:
    QueryManager(EventManager& eventManager):
        eventManager(eventManager)
    {
    }

    AccessMatrix buildAccessMatrix(const QueryContext& queryContext)
    {
        auto& debugContext = eventManager.getDebugContext();
        AccessMatrix accessMatrix(eventManager);
        eventManager.reset();
        while (eventManager.hasNext())
        {
            Event& e = eventManager.next();
            if (queryContext.accept(e, eventManager.topFuncInfo(e.getThreadId())))
            {
                //std::cout << e.str(eventManager) << std::endl;
                if (e.type == EventType::Read || e.type == EventType::Write)
                {
                    accessMatrix.addMemoryEvent(e);
                    /*auto& me = e.memoryEvent;
                    auto inst = me.instAddr;
                    {
                        auto sourceLoc = debugContext.getInstBinding(inst);
                        if (sourceLoc)
                        {
                            auto* varInfo = debugContext.findVarById(me.varId);
                            std::string varName;
                            if (varInfo)
                            {
                                varName = varInfo->name;
                            }
                            std::cout << (void*)inst << " -> " << sourceLoc.fileName << ":" << sourceLoc.line
                                      << " varId: " << me.varId << " var: " << varName << std::endl;
                        }
                    }*/
                }
            }
        }
        return accessMatrix;
    }
};
