#pragma once
#include <string>

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

struct EventManager;

struct MemoryEvent
{
    uint64_t t;
    uint32_t threadId;
    void* addr;
    size_t size;
    uint64_t instAddr;

    std::string str(const EventManager& eventManager) const;
};

struct RoutineEvent
{
    uint64_t t;
    uint32_t threadId;
    int routineId;
    void* spValue;
    uint64_t instAddr;

    std::string str(const EventManager& eventManager) const;
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
    Event(EventType type, uint32_t threadId, void* addr, size_t size = 0, uint64_t instAddr = 0);
    Event(EventType type, uint32_t threadId, int routineId, void* spValue, uint64_t instAddr = 0);
    std::string str(const EventManager& eventManager) const;
};
