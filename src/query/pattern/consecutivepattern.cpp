#include <cassert>

#include "consecutivepattern.h"

namespace pattern
{
    ConsecutiveMatchImpl::ConsecutiveMatchImpl(size_t typeSize):
        typeSize(typeSize),
        MatchImpl(PatternType::Consecutive)
    {
    }

    std::string ConsecutiveMatchImpl::str() const
    {
        return "[ConsecutiveMatchImpl: typeSize = " + std::to_string((long long)typeSize) + "]";
    }

    ConsecutivePattern::ConsecutivePattern(size_t typeSize):
        AccessPattern(WINDOW_SIZE),
        typeSize(typeSize),
        matchImpl(new ConsecutiveMatchImpl(typeSize))
    {
    }

    MatchImpl* ConsecutivePattern::match()
    {
        assert(isFull());
        auto a0 = accesses[0];
        auto a1 = accesses[1];
#if 0
        std::cout << "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" << std::endl;
        std::cout << (void*)a0.addr << std::endl;
        std::cout << (void*)a1.addr << std::endl;
        std::cout << (size_t)(a1.addr - a0.addr) << std::endl;
        std::cout << typeSize << std::endl;
        std::cout << "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB" << std::endl;
#endif
        if (a0.accessType == a1.accessType && a1.addr - a0.addr == typeSize)
        {
            return matchImpl.get();
        }
        return nullptr;
    }
} //namespace pattern
