#pragma once
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include "accessmatrix.h"
#include "common/callstack.h"
#include "common/debuginfo/debugcontext.h"
#include "common/event/eventmanager.h"
#include "common/streamutils/table.h"
#include "query/locality/localityinfo.h"
#include "query/locality/spatiallocality.h"
#include "query/locality/temporallocality.h"
#include "query/pattern/patternanalyzer.h"
#include "query/pattern/patterninfo.h"
#include "querycontext.h"

class QueryManager
{
    EventManager& eventManager;
    const dbginfo::DebugContext& debugContext;
    const QueryContext& queryContext;
    std::map<int, pattern::PatternAnalyzer> varAnalyzers;

public:
    QueryManager(EventManager& eventManager, const QueryContext& queryContext):
        eventManager(eventManager),
        debugContext(eventManager.getDebugContext()),
        queryContext(queryContext)
    {
    }

    LocalityInfo getLocalities()
    {
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
                    if (e.memoryEvent.varId >= 0)
                    {
                        spatialLocality.add(e.memoryEvent.addr);
                        temporalLocality.add(e.memoryEvent.addr);
                    }
                }
            }
        }
        return LocalityInfo(spatialLocality.getValue(), temporalLocality.getValue());
    }

    AccessMatrix getAccessMatrix()
    {
        AccessMatrix accessMatrix(eventManager);
        eventManager.reset();
        while (eventManager.hasNext())
        {
            Event& e = eventManager.next();
            //std::cout << "EVENT: " << e.str(eventManager) << std::endl;
            if (queryContext.accept(e, eventManager.topFuncInfo(e.getThreadId())))
            {
                if (e.type == EventType::Read || e.type == EventType::Write)
                {
                    accessMatrix.addMemoryEvent(e);
                }
            }
        }
        accessMatrix.merge();
        return accessMatrix;
    }

    pattern::PatternInfo getAccessPatterns()
    {
        varAnalyzers.clear();
        pattern::PatternInfo patternInfo(debugContext);
        eventManager.reset();
        while (eventManager.hasNext())
        {
            Event& e = eventManager.next();
            if (queryContext.accept(e, eventManager.topFuncInfo(e.getThreadId())))
            {
                if (e.type == EventType::Read || e.type == EventType::Write)
                {
                    auto& me = e.memoryEvent;
                    if (me.varId >= 0)
                    {
                        if (varAnalyzers.find(me.varId) == varAnalyzers.end())
                        {
                            auto* varInfo = debugContext.findVarById(me.varId);
                            assert(varInfo);
                            varAnalyzers.insert(std::make_pair(me.varId, pattern::PatternAnalyzer(varInfo->typeSize)));
                        }
                        varAnalyzers[me.varId].pushAccess(e.toAccess());
                    }
                }
            }
        }

        for (auto& e: varAnalyzers)
        {
            int varId = e.first;
            auto* varInfo = debugContext.findVarById(varId);
            assert(varInfo);
            std::cout << varInfo->name << " : " << e.second.str() << std::endl;
            patternInfo.addMatches(varId, varAnalyzers[varId].getMatches());
        }
        return patternInfo;
    }
};
