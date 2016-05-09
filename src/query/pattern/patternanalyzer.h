#pragma once
#include <string>
#include <vector>
#include "access.h"
#include "consecutivepattern.h"
#include "statpattern.h"

namespace pattern
{
    class PatternAnalyzer
    {
        ConsecutivePattern consecutivePattern;
        StatPattern statPattern;

    public:
        void pushAccess(const Access& a);
        std::string str() const;
        std::vector<MatchInfo> getMatches() const;
    };
} //namespace pattern
