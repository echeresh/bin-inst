#include "event.h"
#include <sstream>
#include "execcontext.h"

namespace inst
{
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

    std::string MemoryEvent::str(const ExecContext& execCtxt) const
    {
        std::ostringstream oss;
        oss << "thread: " << threadId << "; addr: " << addr
            << "; size: " << size << "; t: " << t;
        return oss.str();
    }


    std::string RoutineEvent::str(const ExecContext& execCtxt) const
    {
        std::ostringstream oss;
        auto* rtnInfo = execCtxt.findRoutine(routineId);
        if (rtnInfo)
            oss << rtnInfo->name << "; ";
        oss << "thread: " << threadId << "; t: " << t;
        return oss.str();
    }

    Event::Event(EventType type, THREADID threadId, void* addr, size_t size, ADDRINT instAddr) :
        type(type)
    {
        switch (type)
        {
        case EventType::Read:
        case EventType::Write:
        case EventType::Alloc:
        case EventType::Free:
            memoryEvent.t = rdtsc();
            memoryEvent.threadId = threadId;
            memoryEvent.addr = addr;
            memoryEvent.size = size;
            memoryEvent.instAddr = instAddr;
            break;
        default:
            assert(false);
        }
    }

    Event::Event(EventType type, THREADID threadId, int routineId, void* spValue, ADDRINT instAddr) :
        type(type)
    {
        switch (type)
        {
        case EventType::CallInst:
        case EventType::Call:
        case EventType::Ret:
            routineEvent.t = rdtsc();
            routineEvent.threadId = threadId;
            routineEvent.routineId = routineId;
            routineEvent.spValue = spValue;
            routineEvent.instAddr = instAddr;
            break;
        default:
            assert(false);
        }
    }

    std::string Event::str(const ExecContext& execCtxt) const
    {
        std::ostringstream oss;
        oss << to_string(type) << "[";
        switch (type)
        {
        case EventType::Read:
        case EventType::Write:
        case EventType::Alloc:
        case EventType::Free:
            oss << memoryEvent.str(execCtxt);
            break;
        case EventType::Call:
        case EventType::Ret:
            oss << routineEvent.str(execCtxt);
            break;
        }
        oss << "]";
        return oss.str();
    }
} //namespace inst
