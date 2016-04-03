#pragma once
#include <deque>
#include <vector>
#include <utility>

#include "common/access.h"
#include "matchimpl.h"
#include "matchinfo.h"

namespace pattern
{
    class AccessPattern
    {
        size_t windowSize;
        std::vector<MatchInfo> matches;

        void mergeMatches();

    protected:
        std::deque<Access> accesses;

    public:
        AccessPattern(size_t windowSize);
        void push_back(const Access& a);
        Access pop_front();
        bool isFull() const;
        bool process(const Access& a);
        std::vector<MatchInfo> getMatches() const;
        virtual MatchImpl* match() = 0;
    };
}
