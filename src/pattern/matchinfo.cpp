#include "matchinfo.h"
#include "matchimpl.h"

namespace pattern
{
    std::ostream& operator <<(std::ostream& out, const MatchInfo& matchInfo)
    {
        out << matchInfo.matchImpl->str()
            << " beginAddr: " << (void*)matchInfo.beginAddr
            << " endAddr: " << (void*)matchInfo.endAddr;
        return out;
    }
}