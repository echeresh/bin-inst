#pragma once
#include <string>

#include "patterntype.h"

namespace pattern
{
    class MatchImpl
    {
    protected:
        PatternType patternType;

    public:
        MatchImpl(PatternType patternType);
        virtual bool equals(const MatchImpl* matchImpl) const;
        virtual std::string str() const = 0;
    };
}
