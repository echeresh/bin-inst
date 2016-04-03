#pragma once
#include <cassert>
#include <map>
#include <string>
#include <vector>
#include "debuginfo/debugcontext.h"
#include "event/eventmanager.h"
#include "streamutils/table.h"

struct AccessMatrixEntry
{
    const dbginfo::VarInfo* varInfo;
    std::map<int, AccessType> threadAccessTypes;

    std::string accessTypeStr(int ithr) const
    {
        auto it = threadAccessTypes.find(ithr);
        auto t = (  it == threadAccessTypes.end()) ? (int)AccessType::None : (int)it->second;
        bool isRead = t & (int)AccessType::Read;
        bool isWrite = t & (int)AccessType::Write;
        if (isRead && isWrite)
        {
            return "R/W";
        }
        if (isRead)
        {
            return "R";
        }
        if (isWrite)
        {
            return "W";
        }
        return "-";
    }
};

class AccessMatrix
{
    int totalThreads;
    const dbginfo::DebugContext& debugContext;
    std::map<int, AccessMatrixEntry> entries;
    std::set<int> threads;

public:
    AccessMatrix(EventManager& eventManager):
        totalThreads(eventManager.getTotalThreads()),
        debugContext(eventManager.getDebugContext())
    {
    }

    void addMemoryEvent(const Event& event)
    {
        assert(event.type == EventType::Read || event.type == EventType::Write);
        auto& me = event.memoryEvent;
        if (me.varId < 0)
        {
            return;
        }
        if (entries.find(me.varId) == entries.end())
        {
            entries[me.varId].varInfo = debugContext.findVarById(me.varId);
            if (!entries[me.varId].varInfo)
            {
                auto* varInfo = debugContext.findVarById(me.varId);
                std::cout << varInfo;
            }
        }
        threads.insert(me.threadId);
        auto& t = entries[me.varId].threadAccessTypes[me.threadId];
        switch (event.type)
        {
            case EventType::Read:
                t = (AccessType)((int)t | (int)AccessType::Read);
                break;
            case EventType::Write:
                t = (AccessType)((int)t | (int)AccessType::Write);
                break;
            default:
                break;
        }
    }

    std::string str() const
    {
        streamutils::Table table;
        table.addColumn("File");
        table.addColumn("Variable name");
        table.addColumn("Size");
        for (auto ithr: threads)
        {
            table.addColumn(std::string("thread #") + std::to_string(ithr));
        }
        for (auto& e: entries)
        {
            std::vector<std::string> row;
            row.push_back("file");
            row.push_back(e.second.varInfo->name);
            row.push_back(std::to_string(e.second.varInfo->size));
            for (auto ithr: threads)
            {
                row.push_back(e.second.accessTypeStr(ithr));
            }
            table.addRow(row);
        }
        return table.str();
    }
};
