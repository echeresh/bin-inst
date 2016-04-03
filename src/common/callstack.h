#pragma once
#include <sstream>
#include <vector>
#include "debuginfo/debugcontext.h"

struct FuncCall
{
    const dbginfo::FuncInfo* funcInfo;
    void* frameBase;

    FuncCall(const dbginfo::FuncInfo* funcInfo, void* frameBase);
};

struct CallStack
{
    std::vector<FuncCall> calls;

    std::string str() const;
    const FuncCall& top() const;
    void push(const FuncCall& funcCall);
    void pop(const FuncCall& funcCall);
    void pop();
    void clear();
    bool empty() const;
};

struct CallStackGlobal
{
    std::vector<CallStack> callStacks;

    CallStackGlobal(int nThreads);
    CallStack& getCallStack(int threadId);
    std::vector<FuncCall>& getCalls(int threadId);
    void push(int threadId, const FuncCall& funcCall);
    void pop(int threadId, const FuncCall& funcCall);
    void pop(int threadId);
    const FuncCall& top(int threadId) const;
    bool empty(int threadId) const;
    void clear();
};
