#pragma once
#include <fstream>
#include <string>
#include <vector>
#include "debuginfo.h"
#include "varinfo.h"

namespace dbginfo
{
    struct DebugContext;

    struct FuncInfo
    {
        std::string name;
        std::vector<const VarInfo*> vars;
        ssize_t loc; //TODO: refactor
        int id;

        FuncInfo() = default;
        FuncInfo(const std::string& name, ssize_t loc, int id);
        void save(std::ostream& out, const DebugContext& dbgCtxt) const;
        void load(std::istream& in, const DebugContext& dbgCtxt);
    };
} //namespace dbginfo
