#pragma once
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>
#include "locality.h"
#include "lrucache.h"

class TemporalLocality : public Locality
{
    static const size_t DEFAULT_N = 131072;
    size_t N;
    std::vector<std::unique_ptr<LRUCache<uintptr_t>>> caches;
    std::vector<size_t> reuseDiff;
    int totalAccesses = 0;

public:
    TemporalLocality(size_t N = DEFAULT_N) :
        N(N),
        reuseDiff((size_t)(log2(N) + 1))
    {
        size_t cacheSize = 1;
        caches.resize((size_t)(log2(N) + 1));
        for (int i = 0; i < log2(N) + 1; i++)
        {
            caches[i] = std::unique_ptr<LRUCache<uintptr_t>>(new LRUCache<uintptr_t>(cacheSize + 1));
            cacheSize *= 2;
        }
    }

    void add(void* addr) override
    {
        totalAccesses++;
        uintptr_t addr64bit = (uintptr_t)addr / 8;
        bool found = false;
        for (size_t i = 0; i < caches.size(); i++)
        {
            if (!found && caches[i]->contains(addr64bit))
            {
                reuseDiff[i]++;
                found = true;
            }
            caches[i]->put(addr64bit);
        }
    }

    double getValue() const override
    {
        int reuse = 0;
        for (int i = 0; i < log2(N) + 1; i++)
        {
            reuse += reuseDiff[i];
        }
        std::cout << reuse << " " << totalAccesses << std::endl;
        double value = reuseDiff[1] * log2(N);
        for (int i = 2; i < log2(N) + 1; i++)
        {
            std::cout << "-> " << reuseDiff[i] << std::endl;
            value += reuseDiff[i] * (log2(N) - i);
        }
        return value / log2(N) / totalAccesses;
    }
};
