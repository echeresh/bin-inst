#pragma once
#include <fstream>
#include <map>
#include <set>
#include "funcinfo.h"
#include "varinfo.h"
#include "common/event/event.h"

namespace dbginfo
{
    class DebugContext
    {
        std::map<std::string, FuncInfo> funcs;
        std::map<int, const FuncInfo*> idFuncs;
        std::set<VarInfo> vars;
        std::map<int, const VarInfo*> idVars;
        std::map<uint64_t, SourceLocation> instBindings;

        DebugContext(const DebugContext& d) = delete;
        DebugContext& operator=(const DebugContext& d) = delete;

    public:
        DebugContext() = default;
        DebugContext(DebugContext&& d);
        DebugContext& operator=(DebugContext&& d);
        const FuncInfo* findFuncByName(const std::string& name) const;
        const FuncInfo* findFuncById(int id) const;
        const FuncInfo* addFunc(const FuncInfo& funcInfo);
        const VarInfo* addVar(const VarInfo& f);
        const VarInfo* findVarById(int id) const;
        const VarInfo* findVarByAddress(void* addr) const;
        void setInstBinding(uint64_t inst, const SourceLocation& sourceLocation);
        SourceLocation getInstBinding(uint64_t inst) const;
        void save(std::ostream& out) const;
        void load(std::istream& in);
    };
} //namespace dbginfo
