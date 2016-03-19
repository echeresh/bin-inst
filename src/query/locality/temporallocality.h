#pragma once
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <vector>
#include "locality.h"
#include "lrucache.h"

class TemporalLocality : public Locality
{
    static const size_t DEFAULT_N = 131072;
    size_t N;
    std::vector<std::unique_ptr<LRUCache<uintptr_t>>> caches;
    std::vector<size_t> reuse;

public:
    TemporalLocality(size_t N = DEFAULT_N) :
        N(N),
        reuse((size_t)(log2(N) + 2))
    {
        size_t cacheSize = 1;
        for (int i = 0; i < log2(N) + 2; i++)
        {
            caches[i] = std::unique_ptr<LRUCache<uintptr_t>>(new LRUCache<uintptr_t>(cacheSize));
            cacheSize *= 2;
        }
    }

    void add(void* addr) override
    {
        uintptr_t addr64bit = (uintptr_t)addr / 8;
        bool found = false;
        for (int i = caches.size() - 1; i >= 0; i--)
        {
            if (!found && caches[i]->contains(addr64bit))
            {
                reuse[i]++;
                found = true;
            }
            caches[i]->put(addr64bit);
        }
    }

    double getValue() const override
    {
        double value = 0;
        for (int i = 0; i < log2(N); i++)
        {
            value += reuse[i + 1] * log2(N) - i;
        }
        return value / log2(N);
    }
};
