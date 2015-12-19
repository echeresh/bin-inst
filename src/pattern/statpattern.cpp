#include "statpattern.h"
#include <algorithm>
#include <cassert>
#include <iomanip>
#include <sstream>
#include <vector>
#include <utility>

namespace pattern
{
    static const int WINDOW_SIZE = 3;
    static const size_t MAX_HALF_OFFSET = 1 << 7;

    StatMatchImpl::StatMatchImpl(size_t hashCode) :
        hashCode(hashCode)
    {
    }

    void StatMatchImpl::increment()
    {
        size++;
    }

    size_t StatMatchImpl::getSize() const
    {
        return size;
    }

    std::string StatMatchImpl::getName() const
    {
        return name;
    }

    void StatMatchImpl::setName(const std::string& name)
    {
        this->name = name;
    }

    bool StatMatchImpl::equals(const MatchImpl* matchImpl) const
    {
        auto* consecutiveMatchImpl = (StatMatchImpl*)matchImpl;
        return hashCode == consecutiveMatchImpl->hashCode;
    }

    std::string StatMatchImpl::str() const
    {
        std::ostringstream oss;
        size_t factor = MAX_HALF_OFFSET * 2 * 2;
        size_t hc = hashCode;
        size_t hcs[WINDOW_SIZE];
        for (int i = 0; i < WINDOW_SIZE; i++)
        {
            size_t ahc = hc % factor;
            hc /= factor;
            hcs[WINDOW_SIZE - i - 1] = ahc;
        }
        for (int i = 0; i < WINDOW_SIZE; i++)
        {
            bool isRead = hcs[i] % 2 == 0;
            long long relativeAddr = (long long)(hcs[i] / 2) - MAX_HALF_OFFSET;
            oss << "[" << (isRead ? "R" : "W") << " " << std::setw(4) << relativeAddr << "] ";
        }
        oss << " " << name;
        return oss.str();
        //return "[StatMatchImpl hashCode:" + std::to_string((long long)hashCode) + "]";
    }

    size_t StatPattern::accessHashCode(const Access& a, byte* baseAddr)
    {
        long long off = (long long)(a.addr - baseAddr);
        assert(off >= 0 && off < MAX_HALF_OFFSET * 2);
        size_t hashCode = 2 * (size_t)off + (a.accessType == AccessType::Read ? 0 : 1);
        return hashCode;
    }

    MatchImpl* StatPattern::getMatchImpl(size_t hashCode)
    {
        auto it = matchesPool.find(hashCode);
        if (it == matchesPool.end())
        {
            matchesPool[hashCode] = std::unique_ptr<StatMatchImpl>(new StatMatchImpl(hashCode));
        }
        return matchesPool[hashCode].get();
    }

    StatPattern::StatPattern() :
        AccessPattern(WINDOW_SIZE)
    {
    }

    MatchImpl* StatPattern::match()
    {
        assert(isFull());
        byte* baseAddr = accesses[0].addr - MAX_HALF_OFFSET;
        size_t hashCode = accessHashCode(accesses[0], baseAddr);
        size_t factor = MAX_HALF_OFFSET * 2 * 2;
        std::string name = accesses[0].name;
        for (int i = 1; i < WINDOW_SIZE; i++)
        {
            long long diff = (long long)accesses[i].addr - (long long)baseAddr;
            if (diff < 0 || diff >= 2 * MAX_HALF_OFFSET)
            {
                hashCode = 0;
                break;
            }
            size_t hc = accessHashCode(accesses[i], baseAddr);
            hashCode = hashCode * factor + hc;
            name += " " + accesses[i].name;
        }
        if (hashCode == 0)
        {
            return nullptr;
        }
        auto* matchImpl = (StatMatchImpl*)getMatchImpl(hashCode);
        if (matchImpl->getName().empty())
        {
            matchImpl->setName(name);
        }
        else
        {
            if (matchImpl->getName() != name)
            {
                return nullptr;
            }
        }
        matchImpl->increment();
        return nullptr;
    }

    StatPattern::~StatPattern()
    {
        std::vector<std::pair<size_t, StatMatchImpl*>> stats;
        for (auto& e : matchesPool)
        {
            stats.push_back(std::make_pair(e.second->getSize(), e.second.get()));
        }
        std::sort(stats.begin(), stats.end(),
            [](const std::pair<size_t, StatMatchImpl*>& a,
               const std::pair<size_t, StatMatchImpl*>& b)
            {
                return a.first > b.first;
            });
        stats.resize(std::min(stats.size(), (size_t)100));
        for (auto& e : stats)
        {
            std::cout << "! " << std::setw(20) << e.first << + " " << e.second->str() << std::endl;
        }
    }
} //namespace pattern
