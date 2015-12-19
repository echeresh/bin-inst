#pragma once
#include <iostream>

typedef uint8_t byte;

namespace pattern
{
    enum class AccessType
    {
        Read,
        Write
    };

    std::string to_string(AccessType accessType);

    struct Access
    {
        AccessType accessType;
        byte* addr;
        size_t size;
        std::string name;

        Access(byte* addr, size_t size, AccessType accessType, const std::string& name);
        std::string str() const;
    };

    std::ostream& operator <<(std::ostream& out, const Access& a);
}
