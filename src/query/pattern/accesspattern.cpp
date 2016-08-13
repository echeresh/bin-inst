#include <cassert>
#include <sstream>
#include "accesspattern.h"

namespace pattern
{
    int AccessPattern::MIN_GROUP_SIZE = 3;

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
        if (isFull())
        {
            pop_front();
        }
        push_back(a);

        if (!isFull())
        {
            processed++;
            return false;
        }

        auto* matchImpl = match();
        if (matchImpl)
        {
            matched++;
            auto& first = accesses.front();
            auto& last = accesses.back();
            MatchInfo mi(matchImpl, first.addr, last.addr);
            mi.setIndex(processed);
            matches.push_back(mi);
            mergeMatches();
        }
        processed++;
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
        else if (prev.size() < MIN_GROUP_SIZE)
        {
            matches.erase(matches.begin() + matches.size() - 1);
        }
    }

    std::vector<MatchInfo> AccessPattern::getMatches()
    {
        //remove last machInfo if it has size < MIN_GROUP_SIZE
        if (!matches.empty() && matches.back().size() < MIN_GROUP_SIZE)
        {
            matches.pop_back();
        }
        return matches;
    }

    std::string AccessPattern::str() const
    {
        std::ostringstream oss;
        oss << "[matched " << matched << "/" << processed << "]";
        return oss.str();
    }
} //namespace pattern
