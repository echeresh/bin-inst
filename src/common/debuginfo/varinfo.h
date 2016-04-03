#pragma once
#include <cstdlib>
#include <fstream>
#include <string>
#include "common/utils.h"
#include "debuginfo.h"

namespace dbginfo
{
    struct DebugContext;
    struct FuncInfo;

    struct VarInfo
    {
        int id = -1;
        StorageType type;
        const FuncInfo* parent = nullptr;
        std::string name;
        size_t size;
        ssize_t stackOffset;

        VarInfo() = default;
        VarInfo(StorageType type, const std::string& name, size_t size,
                ssize_t stackOffset, const FuncInfo* parent = nullptr);

        //declare compare operator for storing in set
        bool operator<(const VarInfo& v) const;
        void save(std::ostream& out, const DebugContext& dbgCtxt) const;
        void load(std::istream& in, const DebugContext& dbgCtxt);
    };
} //namespace dbginfo
