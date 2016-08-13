#include <iostream>
#include <sstream>

#include "consecutivepattern.h"
#include "patternanalyzer.h"
#include "rwpattern.h"
#include "statpattern.h"

namespace pattern
{
    PatternAnalyzer::PatternAnalyzer(size_t typeSize):
        typeSize(typeSize)
    {
        patterns.push_back(std::unique_ptr<AccessPattern>(new ConsecutivePattern(typeSize)));
        patterns.push_back(std::unique_ptr<AccessPattern>(new StatPattern));
        patterns.push_back(std::unique_ptr<AccessPattern>(new RWPattern));
    }

    void PatternAnalyzer::pushAccess(const Access& a)
    {
        for (auto& p: patterns)
        {
            p->process(a);
        }
    }

    std::string PatternAnalyzer::str() const
    {
        std::ostringstream oss;
        for (auto& p: patterns)
        {
            oss << p->str() << std::endl;
        }
        return oss.str();
    }

    std::vector<MatchInfo> PatternAnalyzer::getMatches() const
    {
        std::vector<MatchInfo> matches;
        for (auto& p: patterns)
        {
            auto patternMatches = p->getMatches();
            matches.insert(matches.end(), patternMatches.begin(), patternMatches.end());
        }
        return matches;
    }
} //namespace pattern
