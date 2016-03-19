#pragma once

class Locality
{
public:
    virtual void add(void* addr) = 0;
    virtual double getValue() const = 0;
};
