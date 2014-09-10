#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <map>
#include <set>

namespace dbg
{
    enum class StorageType
    {
        Static,
        Auto,
        Dynamic
    };
    
    struct FuncInfo;

    struct VarInfo
    {
        StorageType type;
        const FuncInfo* parent;
        std::string name;
        size_t size;
        ssize_t loc;
       
        VarInfo(StorageType type, const std::string& name, size_t size,
                ssize_t loc, const FuncInfo* parent = nullptr);
        VarInfo() = default;
        
        //declare compare operator for storing in set
        bool operator<(const VarInfo& v) const;
    };

    struct FuncInfo
    {
        std::string name;
        std::vector<const VarInfo*> vars;
        ssize_t loc;
        
        FuncInfo(const std::string& name, ssize_t loc);
    };

    class DebugContext
    {
        std::map<std::string, FuncInfo> funcs;
        std::set<VarInfo> vars;
        
        DebugContext(const DebugContext& d) = delete;
        DebugContext& operator=(const DebugContext& d) = delete;
    
    public:
        DebugContext() = default;
        DebugContext(DebugContext&& d);
        DebugContext& operator=(DebugContext&& d);
        const FuncInfo* findFuncByName(const std::string& name) const;
        const FuncInfo* addFunc(const FuncInfo& f);
        const VarInfo* addVar(const VarInfo& f);
    };
}