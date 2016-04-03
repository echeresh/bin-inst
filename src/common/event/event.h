#pragma once
#include <cassert>
#include <fstream>
#include <string>
#include "access.h"
#include "common/utils.h"

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

struct SourceLocation
{
    std::string fileName;
    int line;

    SourceLocation() = default;
    SourceLocation(const std::string& fileName, int line):
        fileName(fileName),
        line(line)
    {
    }

    operator bool() const
    {
        return !fileName.empty();
    }

    bool operator==(const SourceLocation& sourceLocation) const
    {
        return fileName == sourceLocation.fileName &&
               line == sourceLocation.line;
    }

    void save(std::ostream& out) const
    {
        utils::save(fileName, out);
        utils::save(line, out);
    }

    void load(std::istream& in)
    {
        fileName = utils::load<std::string>(in);
        line = utils::load<int>(in);
    }
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
    int varId;

    std::string str(const EventManager& eventManager) const;
};

struct RoutineEvent
{
    uint64_t t;
    uint32_t threadId;
    int routineId;
    void* stackPointerRegister;
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
    Event(EventType type, uint32_t threadId, int routineId, void* stackPointerRegister, uint64_t instAddr = 0);
    std::string str(const EventManager& eventManager) const;
    Access toAccess() const
    {
        assert(type == EventType::Read || type == EventType::Write);
        return Access((byte*)memoryEvent.addr,
                      memoryEvent.size,
                      type == EventType::Read ? AccessType::Read : AccessType::Write);
    }
    uint32_t getThreadId() const
    {
        switch (type)
        {
            case EventType::CallInst:
            case EventType::Call:
            case EventType::Ret:
                return routineEvent.threadId;
            case EventType::Read:
            case EventType::Write:
            case EventType::Alloc:
            case EventType::Free:
                return memoryEvent.threadId;
        }
        return -1;
    }
};
