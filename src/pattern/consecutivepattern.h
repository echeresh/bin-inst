#pragma once
#include <map>
#include <memory>

#include "accesspattern.h"
#include "matchimpl.h"

namespace pattern
{
    class ConsecutiveMatchImpl : public MatchImpl
    {
        size_t stride;

    public:
        ConsecutiveMatchImpl(size_t stride);
        bool equals(const MatchImpl* matchImpl) const override;
        std::string str() const override;
    };

    class ConsecutivePattern : public AccessPattern
    {
        static const int WINDOW_SIZE = 2;
        std::map<size_t, std::unique_ptr<ConsecutiveMatchImpl>> matchesPool;

        MatchImpl* getMatchImpl(size_t stride);

    public:
        ConsecutivePattern();
        MatchImpl* match() override;
    };
} //namespace pattern
