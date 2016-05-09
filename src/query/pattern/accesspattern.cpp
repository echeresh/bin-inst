#include <cassert>
#include <sstream>
#include "accesspattern.h"

namespace pattern
{
    AccessPattern::AccessPattern(size_t windowSize) :
        windowSize(windowSize)
    {
    }

    void AccessPattern::push_back(const Access& a)
    {
        assert(accesses.size() < windowSize);
        accesses.push_back(a);
    }

    Access AccessPattern::pop_front()
    {
        assert(accesses.size() > 0);
        auto f = accesses.front();
        accesses.pop_front();
        return f;
    }

    bool AccessPattern::isFull() const
    {
        return accesses.size() >= windowSize;
    }

    bool AccessPattern::process(const Access& a)
    {
        processed++;
        if (isFull())
        {
            pop_front();
        }
        push_back(a);

        if (!isFull())
        {
            return false;
        }

        auto* matchImpl = match();
        if (matchImpl)
        {
            matched++;
            auto& first = accesses.front();
            auto& last = accesses.back();
            matches.push_back(MatchInfo(matchImpl, first.addr, last.addr));
            mergeMatches();
        }
        return matchImpl;
    }

    void AccessPattern::mergeMatches()
    {
        if (matches.size() <= 1)
        {
            return;
        }

        auto& prev = matches[matches.size() - 2];
        auto& cur = matches.back();
        if (prev.isMergable(cur))
        {
            prev.update(cur);
            matches.pop_back();
        }
    }

    std::vector<MatchInfo> AccessPattern::getMatches() const
    {
        return matches;
    }

    std::string AccessPattern::str() const
    {
        std::ostringstream oss;
        oss << "[matched " << matched << "/" << processed << "]";
        return oss.str();
    }
}
