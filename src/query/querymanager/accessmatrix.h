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
    int accessed = 0;

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

    void merge(const AccessMatrixEntry& ame)
    {
        assert(varInfo->name == ame.varInfo->name &&
               varInfo->srcLoc == ame.varInfo->srcLoc);
        accessed += ame.accessed;
        for (auto& e: ame.threadAccessTypes)
        {
            int at = (int)threadAccessTypes[e.first];
            threadAccessTypes[e.first] = (AccessType)(at | (int)e.second);
        }
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
        entries[me.varId].accessed++;
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

    void merge()
    {
        std::map<std::string, std::vector<std::map<int, AccessMatrixEntry>::iterator>> repeating;
        for (auto it = entries.begin(); it != entries.end(); it++)
        {
            //ensure we known exact value of line
            if (it->second.varInfo->srcLoc.line == -1)
            {
                continue;
            }
            std::string tag = it->second.varInfo->name + it->second.varInfo->srcLoc.str();
            repeating[tag].push_back(it);
        }
        for (auto& e: repeating)
        {
            if (e.second.size() > 1)
            {
                auto first = e.second.front()->second;
                for (size_t i = 1; i < e.second.size(); i++)
                {
                    auto it = e.second[i];
                    first.merge(it->second);
                    entries.erase(it);
                }
            }
        }
    }

    std::string str() const
    {
        streamutils::Table table;
        table.addColumn("File");
        table.addColumn("Variable name");
        table.addColumn("Size");
        table.addColumn("Accessed");
        for (auto ithr: threads)
        {
            table.addColumn(std::string("thread #") + std::to_string(ithr));
        }
        for (auto& e: entries)
        {
            std::vector<std::string> row;
            row.push_back(e.second.varInfo->srcLoc.str());
            row.push_back(e.second.varInfo->name);
            row.push_back(std::to_string(e.second.varInfo->size));
            row.push_back(std::to_string(e.second.accessed));
            for (auto ithr: threads)
            {
                row.push_back(e.second.accessTypeStr(ithr));
            }
            table.addRow(row);
        }
        return table.str();
    }
};
