#include "patternanalyzer.h"
#include <iostream>

namespace pattern
{
    void PatternAnalyzer::pushAccess(const Access& a)
    {
        consecutivePattern.process(a);
        statPattern.process(a);
        //std::cout << "push access " << a.str() << std::endl;
    }

} //namespace pattern