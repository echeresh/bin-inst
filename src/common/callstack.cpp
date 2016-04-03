#include <cassert>
#include "callstack.h"

//------------------------------------------------------------------------------
//FuncCall
//------------------------------------------------------------------------------

FuncCall::FuncCall(const dbginfo::FuncInfo* funcInfo, void* frameBase) :
    funcInfo(funcInfo),
    frameBase(frameBase)
{
}

//------------------------------------------------------------------------------
//CallStack
//------------------------------------------------------------------------------

void CallStack::push(const FuncCall& funcCall)
{
    //cleanup call stack
    while (!calls.empty() && calls.back().frameBase <= funcCall.frameBase)
    {
        calls.pop_back();
    }
    calls.push_back(funcCall);
}

std::string CallStack::str() const
{
    std::ostringstream oss;
    for (int i = 0; i < (int)calls.size(); i++)
    {
        oss << calls[i].funcInfo->name << " frame base: " << calls[i].frameBase << std::endl;
    }
    return oss.str();
}

const FuncCall& CallStack::top() const
{
    return calls.back();
}

void CallStack::pop(const FuncCall& funcCall)
{
    auto& topCall = top();
    assert(topCall.funcInfo == funcCall.funcInfo);
    assert(topCall.frameBase == funcCall.frameBase);
    pop();
}

void CallStack::pop()
{
    calls.pop_back();
}

void CallStack::clear()
{
    calls.clear();
}

bool CallStack::empty() const
{
    return calls.empty();
}

//------------------------------------------------------------------------------
//CallStackGlobal
//------------------------------------------------------------------------------

CallStackGlobal::CallStackGlobal(int nThreads): callStacks(nThreads)
{
}

CallStack& CallStackGlobal::getCallStack(int threadId)
{
    return callStacks[threadId];
}

std::vector<FuncCall>& CallStackGlobal::getCalls(int threadId)
{
    return callStacks[threadId].calls;
}

void CallStackGlobal::push(int threadId, const FuncCall& funcCall)
{
    callStacks[threadId].push(funcCall);
}

void CallStackGlobal::pop(int threadId, const FuncCall& funcCall)
{
    callStacks[threadId].pop(funcCall);
}

void CallStackGlobal::pop(int threadId)
{
    callStacks[threadId].pop();
}

const FuncCall& CallStackGlobal::top(int threadId) const
{
    return callStacks[threadId].top();
}

void CallStackGlobal::clear()
{
    for (auto& c: callStacks)
    {
        c.clear();
    }
}

bool CallStackGlobal::empty(int threadId) const
{
    return callStacks[threadId].empty();
}
