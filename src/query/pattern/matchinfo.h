#pragma once
#include <cassert>
#include <iostream>

#include "matchimpl.h"

typedef uint8_t byte;

namespace pattern
{
    class MatchInfo
    {
        MatchImpl* matchImpl;
        byte* beginAddr;
        byte* endAddr;

        friend std::ostream& operator <<(std::ostream& out, const MatchInfo& matchInfo);

    public:
        MatchInfo(MatchImpl* matchImpl, byte* beginAddr, byte* endAddr) :
            matchImpl(matchImpl),
            beginAddr(beginAddr),
            endAddr(endAddr)
        {
        }

        void update(MatchInfo& matchInfo)
        {
            assert(isMergable(matchInfo));
            endAddr = matchInfo.endAddr;
        }

        bool isMergable(const MatchInfo& matchInfo) const
        {
            return typeEquals(matchInfo) && endAddr == matchInfo.beginAddr;
        }

        bool typeEquals(const MatchInfo& matchInfo) const
        {
            return matchImpl->equals(matchInfo.matchImpl);
        }
    };
} //namespace pattern
