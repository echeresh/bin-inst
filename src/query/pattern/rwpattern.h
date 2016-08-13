#pragma once
#include <map>
#include <memory>

#include "accesspattern.h"
#include "matchimpl.h"
#include "patterntype.h"

namespace pattern
{
    class RWMatchImpl : public MatchImpl
    {
    public:
        RWMatchImpl():
            MatchImpl(PatternType::RW)
        {
        }

        std::string str() const override
        {
            return "[RWMatchImpl]";
        }
    };

    class RWPattern : public AccessPattern
    {
        static const int WINDOW_SIZE = 2;
        std::unique_ptr<RWMatchImpl> matchImpl;

    public:
        RWPattern():
            AccessPattern(WINDOW_SIZE),
            matchImpl(new RWMatchImpl)
        {
        }

        MatchImpl* match() override
        {
            assert(isFull());
            auto a0 = accesses[0];
            auto a1 = accesses[1];
            if (a0.accessType == a1.accessType && a1.addr == a0.addr)
            {
                return matchImpl.get();
            }
            return nullptr;
        }
    };
} //namespace pattern
