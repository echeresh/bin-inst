#pragma once
#include <map>
#include <memory>
#include "accesspattern.h"
#include "matchimpl.h"

namespace pattern
{
    class StatMatchImpl : public MatchImpl
    {
        size_t hashCode;
        size_t size = 0;
        std::string name;

    public:
        StatMatchImpl(size_t hashCode);
        void increment();
        size_t getSize() const;
        std::string getName() const;
        void setName(const std::string& name);
        bool equals(const MatchImpl* matchImpl) const override;
        std::string str() const override;
    };

    class StatPattern : public AccessPattern
    {
        std::map<size_t, std::unique_ptr<StatMatchImpl>> matchesPool;

        MatchImpl* getMatchImpl(size_t hashCode);
        size_t accessHashCode(const Access& a, byte* baseAddr);

    public:
        StatPattern();
        MatchImpl* match() override;
        ~StatPattern();
    };
} //namespace pattern
