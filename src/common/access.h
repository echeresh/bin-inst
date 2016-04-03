#pragma once
#include <iostream>

typedef uint8_t byte;

enum class AccessType
{
    None = 0x0,
    Read = 0x1,
    Write = 0x2
};

std::string to_string(AccessType accessType);

struct Access
{
    AccessType accessType;
    byte* addr;
    size_t size;
    //std::string name;

    Access(byte* addr, size_t size, AccessType accessType);
    std::string str() const;
    std::string name() const;
};

std::ostream& operator <<(std::ostream& out, const Access& a);
