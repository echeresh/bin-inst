#include <cassert>
#include <sstream>
#include "common/utils.h"
#include "event.h"
#include "eventmanager.h"

std::string to_string(EventType type)
{
    switch (type)
    {
    case EventType::CallInst:
        return "CallInst";
    case EventType::Call:
        return "Call";
    case EventType::Ret:
        return "Ret";
    case EventType::Read:
        return "Read";
    case EventType::Write:
        return "Write";
    case EventType::Alloc:
        return "Alloc";
    case EventType::Free:
        return "Free";
    }
   return "Unknown EventType: " + std::to_string((int)type);
}

std::string MemoryEvent::str(const EventManager& eventManager) const
{
    std::ostringstream oss;
    oss << "thread: " << threadId << "; addr: " << addr
        << "; size: " << size << "; inst: " << (void*)instAddr << "; t: " << t;
    return oss.str();
}

std::string RoutineEvent::str(const EventManager& eventManager) const
{
    std::ostringstream oss;
    auto* funcInfo = eventManager.getDebugContext().findFuncById(routineId);
    if (funcInfo)
        oss << funcInfo->name << "; ";
    oss << "thread: " << threadId << "; inst: " << (void*)instAddr << "; t: " << t;
    return oss.str();
}

Event::Event(EventType type, uint32_t threadId, void* addr, size_t size, uint64_t instAddr) :
    type(type)
{
    switch (type)
    {
        case EventType::Read:
        case EventType::Write:
        case EventType::Alloc:
        case EventType::Free:
            memoryEvent.t = utils::rdtsc();
            memoryEvent.threadId = threadId;
            memoryEvent.addr = addr;
            memoryEvent.size = size;
            memoryEvent.instAddr = instAddr;
            break;
        default:
            assert(false);
    }
}

Event::Event(EventType type, uint32_t threadId, int routineId, void* stackPointerRegister, uint64_t instAddr) :
    type(type)
{
    switch (type)
    {
        case EventType::CallInst:
        case EventType::Call:
        case EventType::Ret:
            routineEvent.t = utils::rdtsc();
            routineEvent.threadId = threadId;
            routineEvent.routineId = routineId;
            routineEvent.stackPointerRegister = stackPointerRegister;
            routineEvent.instAddr = instAddr;
            break;
        default:
            assert(false);
    }
}

std::string Event::str(const EventManager& eventManager) const
{
    std::ostringstream oss;
    oss << to_string(type) << "[";
    switch (type)
    {
        case EventType::Read:
        case EventType::Write:
        case EventType::Alloc:
        case EventType::Free:
            oss << memoryEvent.str(eventManager);
            break;
        case EventType::Call:
        case EventType::Ret:
            oss << routineEvent.str(eventManager);
            break;
    }
    oss << "]";
    return oss.str();
}
