#pragma once
#include <map>
#include <sstream>
#include <vector>
#include "debuginfo/debugcontext.h"

namespace pattern
{
    class PatternInfo
    {
        const dbginfo::DebugContext& debugContext;
        std::map<int, std::vector<MatchInfo>> matchInfos;

    public:
        PatternInfo(const dbginfo::DebugContext& debugContext):
            debugContext(debugContext)
        {
        }

        void addMatches(int varId, const std::vector<MatchInfo>& matchInfos)
        {
            auto& mi = this->matchInfos[varId];
            mi.insert(mi.end(), matchInfos.begin(), matchInfos.end());
        }

        std::string str() const
        {
            std::ostringstream oss;
            for (auto& e: matchInfos)
            {
                auto* varInfo = debugContext.findVarById(e.first);
		if (!e.second.empty())
		{
                    oss << "MatchInfo -- " << varInfo->name << ":" << std::endl;
                    for (auto& mi: e.second)
                    {
                        oss << mi.str() << std::endl;
                    }
                    oss << std::endl;
		}
            }
            return oss.str();
        }
    };
} //namespace pattern
