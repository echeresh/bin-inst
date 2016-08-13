#pragma once
#include <map>
#include <memory>

#include "accesspattern.h"
#include "matchimpl.h"

namespace pattern
{
    class ConsecutiveMatchImpl : public MatchImpl
    {
        size_t typeSize;

    public:
        ConsecutiveMatchImpl(size_t typeSize = 0);
        std::string str() const override;
    };

    class ConsecutivePattern : public AccessPattern
    {
        static const int WINDOW_SIZE = 2;

        size_t typeSize;
        std::unique_ptr<ConsecutiveMatchImpl> matchImpl;

        MatchImpl* getMatchImpl(size_t stride);

    public:
        ConsecutivePattern(size_t typeSize = 0);
        MatchImpl* match() override;
    };
} //namespace pattern
