#pragma once
#include <cassert>
#include <iostream>
#include <sstream>
#include "matchimpl.h"

typedef uint8_t byte;

namespace pattern
{
    class MatchInfo
    {
        MatchImpl* matchImpl;
        byte* beginAddr;
        byte* endAddr;
        size_t sz = 0;
        int index = -1;

        friend std::ostream& operator <<(std::ostream& out, const MatchInfo& matchInfo);

    public:
        MatchInfo(MatchImpl* matchImpl, byte* beginAddr, byte* endAddr) :
            matchImpl(matchImpl),
            beginAddr(beginAddr),
            endAddr(endAddr),
            sz(1)
        {
        }

        void update(MatchInfo& matchInfo)
        {
            assert(isMergable(matchInfo));
            endAddr = matchInfo.endAddr;
            sz += matchInfo.sz;
        }

        bool isMergable(const MatchInfo& matchInfo) const
        {
            return typeEquals(matchInfo) && endAddr == matchInfo.beginAddr;
        }

        bool typeEquals(const MatchInfo& matchInfo) const
        {
            return matchImpl->equals(matchInfo.matchImpl);
        }

        int size() const
        {
            return sz;
        }

        void setIndex(int index)
        {
            this->index = index;
        }

        std::string str() const
        {
            std::ostringstream oss;
            oss << matchImpl->str() << " size = " << sz;
            return oss.str();
        }
    };
} //namespace pattern
