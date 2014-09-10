#pragma once
#include <iostream>
#include <iomanip>
#include <string>
#include <cassert>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libdwarf.h>
#include <dwarf.h>
#include <libelf.h>
#include <sstream>
#include <functional>
#include <map>
#include <vector>
#include "debuginfo.h"

namespace dwarf
{
    enum class StorageType
    {
        Static,
        Auto
    };

    enum class LocSource
    {
        RBP,
        RSP,
        FrameBase,
        Addr,
        Unknown
    };
    
    std::string to_string(LocSource src);

    struct WalkInfo
    {
        Dwarf_Die parentDie = nullptr;
        StorageType storageType = StorageType::Static;
        Dwarf_Die lastFuncDie = nullptr;
        Dwarf_Unsigned cuOffset = 0;
        Dwarf_Signed lang;
    };

    struct LocEntry
    {
        void* lo;
        void* hi;
        LocSource reg;
        int off;
        
        LocEntry() = default;
        LocEntry(void* lo, void* hi, LocSource reg, int off);
    };

    struct LocInfo
    {
        //[lopc; hipc)
        std::vector<LocEntry> entries;
        
        LocInfo() = default;
        void freeLoclist(Dwarf_Debug dbg, Dwarf_Locdesc** loclist, Dwarf_Signed nLoclist);
        std::string str(Dwarf_Debug dbg, Dwarf_Locdesc** loclist, Dwarf_Signed nLoclist);
        LocSource getLocSource(Dwarf_Small lr_atom);
        LocInfo(Dwarf_Debug dbg, Dwarf_Die die);
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

    class DebugContext
    {
        std::map<int, FuncInfo> funcs;
        std::map<int, VarInfo> vars;
        
        DebugContext(const DebugContext& d) = delete;
        DebugContext& operator=(const DebugContext& d) = delete;
    public:
        DebugContext() = default;    
        void addFunc(const FuncInfo& f);
        FuncInfo* getFunc(int id);
        void addVar(const VarInfo& v);
        VarInfo* getVar(int id);
        
        dbg::DebugContext toDbg() const;
    };


    class Parser
    {
        Dwarf_Debug dbg = nullptr;
        DebugContext dctx;
        
    public:
        Parser(const std::string& filePath);
        size_t getSize(Dwarf_Die die, const WalkInfo& walkInfo);    
        void walkTree(Dwarf_Die die, WalkInfo& walkInfo);
        void walk(Dwarf_Die cuDie, Dwarf_Unsigned off);
        void cuWalk(std::function<void(Dwarf_Die, Dwarf_Unsigned)> cuHandler);
        const DebugContext& getDebugContext() const;
        ~Parser();
    };
}