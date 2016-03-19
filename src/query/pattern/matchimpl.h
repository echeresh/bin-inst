#pragma once
#include <string>

namespace pattern
{
    class MatchImpl
    {
    public:
        virtual bool equals(const MatchImpl* matchImpl) const = 0;
        virtual std::string str() const = 0;
    };
}
