#include <iostream>
#include <sstream>
#include "patternanalyzer.h"

namespace pattern
{
    void PatternAnalyzer::pushAccess(const Access& a)
    {
        consecutivePattern.process(a);
        statPattern.process(a);
        //std::cout << "push access " << a.str() << std::endl;
    }

    std::string PatternAnalyzer::str() const
    {
        std::ostringstream oss;
        oss << consecutivePattern.str() << std::endl;
        oss << statPattern.str() << std::endl;
        return oss.str();
    }

    std::vector<MatchInfo> PatternAnalyzer::getMatches() const
    {
        std::vector<MatchInfo> matches;
        auto consecutiveMatches = consecutivePattern.getMatches();
        matches.insert(matches.end(), consecutiveMatches.begin(), consecutiveMatches.end());
        return matches;
    }
} //namespace pattern