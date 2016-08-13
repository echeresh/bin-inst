#pragma once
#include <memory>
#include <string>
#include <vector>

#include "access.h"
#include "accesspattern.h"

namespace pattern
{
    class PatternAnalyzer
    {
        std::vector<std::unique_ptr<AccessPattern>> patterns;
        size_t typeSize;

    public:
        PatternAnalyzer(size_t typeSize = 0);
        void pushAccess(const Access& a);
        std::string str() const;
        std::vector<MatchInfo> getMatches() const;
    };
} //namespace pattern
