#pragma once
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
    };
} //namespace pattern
