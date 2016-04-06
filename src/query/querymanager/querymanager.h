#pragma once
#include <fstream>
#include <iostream>
#include "accessmatrix.h"
#include "common/callstack.h"
#include "common/debuginfo/debugcontext.h"
#include "common/event/eventmanager.h"
#include "common/streamutils/table.h"
#include "query/locality/localityinfo.h"
#include "query/locality/spatiallocality.h"
#include "query/locality/temporallocality.h"
#include "querycontext.h"

class QueryManager
{
    EventManager& eventManager;

public:
    QueryManager(EventManager& eventManager):
        eventManager(eventManager)
    {
    }

    LocalityInfo computeLocality(const QueryContext& queryContext)
    {
        auto& debugContext = eventManager.getDebugContext();
        AccessMatrix accessMatrix(eventManager);
        eventManager.reset();
        SpatialLocality spatialLocality;
        TemporalLocality temporalLocality;
        while (eventManager.hasNext())
        {
            Event& e = eventManager.next();
            if (queryContext.accept(e, eventManager.topFuncInfo(e.getThreadId())))
            {
                if (e.type == EventType::Read || e.type == EventType::Write)
                {
                    spatialLocality.add(e.memoryEvent.addr);
                    temporalLocality.add(e.memoryEvent.addr);
                }
            }
        }
        return LocalityInfo(spatialLocality.getValue(), temporalLocality.getValue());
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
                if (e.type == EventType::Read || e.type == EventType::Write)
                {
                    accessMatrix.addMemoryEvent(e);
                }
            }
        }
        return accessMatrix;
    }
};
