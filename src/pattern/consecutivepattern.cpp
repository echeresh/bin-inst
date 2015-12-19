#include <cassert>

#include "consecutivepattern.h"

namespace pattern
{
    ConsecutiveMatchImpl::ConsecutiveMatchImpl(size_t stride) :
        stride(stride)
    {
    }

    bool ConsecutiveMatchImpl::equals(const MatchImpl* matchImpl) const
    {
        auto* consecutiveMatchImpl = (ConsecutiveMatchImpl*)matchImpl;
        return stride == consecutiveMatchImpl->stride;

    }

    std::string ConsecutiveMatchImpl::str() const
    {
        return "[ConsecutiveMatchImpl stride:" + std::to_string((long long)stride) + "]";
    }


    MatchImpl* ConsecutivePattern::getMatchImpl(size_t stride)
    {
        auto it = matchesPool.find(stride);
        if (it == matchesPool.end())
        {
            matchesPool[stride] = std::unique_ptr<ConsecutiveMatchImpl>(new ConsecutiveMatchImpl(stride));
        }
        return matchesPool[stride].get();
    }

    ConsecutivePattern::ConsecutivePattern() :
        AccessPattern(WINDOW_SIZE)
    {
    }

    MatchImpl* ConsecutivePattern::match()
    {
        assert(isFull());
        auto a0 = accesses[0];
        auto a1 = accesses[1];
        if (a0.accessType == a1.accessType)
        {
            return getMatchImpl(a1.addr - a0.addr);
        }
        return nullptr;
    }
} //namespace pattern
