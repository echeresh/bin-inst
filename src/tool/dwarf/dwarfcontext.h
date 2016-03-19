#pragma once
#include <map>
#include <string>
#include <vector>
#include "location.h"
#include "debuginfo/debugcontext.h"

namespace dwarf
{
    enum class StorageType
    {
        Static,
        Auto
    };

    struct VarInfo;

    struct FuncInfo
    {
        int id = -1;
        std::string name;
        std::string linkageName;
        LocInfo location;
        std::vector<const VarInfo*> vars;

        FuncInfo() = default;
        FuncInfo(int id, const std::string& name, const std::string& linkageName,
                 const LocInfo& location);
    };

    struct VarInfo
    {
        int id;
        StorageType type;
        FuncInfo* parent;
        std::string name;
        size_t size;
        LocInfo location;

        VarInfo() = default;
        VarInfo(int id, StorageType type, FuncInfo* parent, const std::string& name, size_t size,
                const LocInfo& location);
    };

    class DwarfContext
    {
        std::map<int, FuncInfo> funcs;
        std::map<int, VarInfo> vars;

        DwarfContext(const DwarfContext& d) = delete;
        DwarfContext& operator=(const DwarfContext& d) = delete;
    public:
        DwarfContext() = default;
        void addFunc(const FuncInfo& f);
        FuncInfo* getFunc(int id);
        void addVar(const VarInfo& v);
        VarInfo* getVar(int id);

        dbginfo::DebugContext toDbg() const;
    };
} //namespace dwarf
