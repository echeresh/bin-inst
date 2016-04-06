#pragma once
#include <cstdint>
#include <cstdlib>
#include <vector>
#include "locality.h"

class SpatialLocality : public Locality
{
    static const size_t DEFAULT_WINDOW_SIZE = 32;
    std::vector<size_t> stride;
    std::vector<uintptr_t> window;
    size_t windowIndex = 0;
    size_t accessCount = 0;

public:
    SpatialLocality(size_t windowSize = DEFAULT_WINDOW_SIZE) :
        stride(128),
        window(windowSize)
    {
    }

    void add(void* addr) override
    {
        uintptr_t addr64bit = (uintptr_t)addr / 8;
        size_t minStride = SIZE_MAX;
        for (size_t i = windowIndex, j = 0; j < window.size(); i = (i + 1) % window.size(), j++)
        {
            size_t diff = abs(window[i] - addr64bit);
            if (diff < minStride)
            {
                minStride = diff;
                break;
            }
        }
        if (minStride < stride.size())
        {
            stride[minStride]++;
        }
        window[windowIndex] = addr64bit;
        windowIndex = (windowIndex + 1) % window.size();
        accessCount++;
    }

    double getValue() const override
    {
        double value = 0;
        for (size_t i = 0; i < stride.size(); i++)
        {
            value += stride[i] / (double)(i + 1);
        }
        return value / accessCount;
    }
};
