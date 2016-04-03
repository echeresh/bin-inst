#pragma once
#include <set>
#include <vector>
#include "common/event/event.h"
#include "common/debuginfo/debugcontext.h"

class QueryContext
{
    bool threadsEnabled = false;
    std::vector<bool> threads;

    bool funcsEnabled = false;
    std::set<int> funcs;

public:
    QueryContext():
        threads(MAX_THREADS, false)
    {
    }

    QueryContext& acceptThread(int ithr)
    {
        threadsEnabled = true;
        threads[ithr] = true;
        return *this;
    }

    QueryContext& rejectThread(int ithr)
    {
        threadsEnabled = true;
        threads[ithr] = false;
        return *this;
    }

    QueryContext& acceptFunc(const dbginfo::FuncInfo* funcInfo)
    {
        funcsEnabled = true;
        funcs.insert(funcInfo->id);
        return *this;
    }

    QueryContext& rejectFunc(const dbginfo::FuncInfo* funcInfo)
    {
        funcsEnabled = true;
        funcs.erase(funcInfo->id);
        return *this;
    }

    bool accept(const Event& e, const dbginfo::FuncInfo* funcInfo) const
    {
        if (threadsEnabled && !threads[e.getThreadId()])
        {
            return false;
        }
        if (funcsEnabled && (!funcInfo || funcs.find(funcInfo->id) == funcs.end()))
        {
            return false;
        }
        return true;
    }
};
