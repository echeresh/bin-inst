#include "matchimpl.h"

namespace pattern
{
   // std::string MatchImpl::str() const

    MatchImpl::MatchImpl(PatternType patternType):
        patternType(patternType)
    {
    }

    bool MatchImpl::equals(const MatchImpl* matchImpl) const
    {
        return patternType == matchImpl->patternType;
    }
} //namespace pattern
