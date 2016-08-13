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
        int matched = 0;
        int processed = 0;
        size_t windowSize;
        std::vector<MatchInfo> matches;

        void mergeMatches();

    protected:
        static int MIN_GROUP_SIZE;
        std::deque<Access> accesses;

    public:
        AccessPattern(size_t windowSize);
        void push_back(const Access& a);
        Access pop_front();
        bool isFull() const;
        bool process(const Access& a);
        std::vector<MatchInfo> getMatches();
        virtual MatchImpl* match() = 0;
        std::string str() const;
    };
} //namespace pattern
