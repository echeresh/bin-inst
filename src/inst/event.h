#pragma once
#include <string>
#include "pin.H"

namespace inst
{
    enum class EventType
    {
        CallInst,
        Call,
        Ret,
        Read,
        Write,
        Alloc,
        Free
    };

    std::string to_string(EventType type);

    struct ExecContext;

    struct MemoryEvent
    {
        uint64_t t;
        THREADID threadId;
        void* addr;
        size_t size;
        ADDRINT instAddr;

        std::string str(const ExecContext& execCtxt) const;
    };

    struct RoutineEvent
    {
        uint64_t t;
        THREADID threadId;
        int routineId;
        void* spValue;
        ADDRINT instAddr;

        std::string str(const ExecContext& execCtxt) const;
    };


    struct Event
    {
        EventType type;
        union
        {
            MemoryEvent memoryEvent;
            RoutineEvent routineEvent;
        };

        Event() = default;
        Event(EventType type, THREADID threadId, void* addr, size_t size = 0, ADDRINT instAddr = 0);
        Event(EventType type, THREADID threadId, int routineId, void* spValue, ADDRINT instAddr = 0);
        std::string str(const ExecContext& execCtxt) const;
    };
} //namespace inst
