#include <utility>
#include "dwarfcontext.h"
#include "dwarflog.h"
#include "dwarfutils.h"

namespace dwarf
{
    //------------------------------------------------------------------------------
    //FuncInfo
    //------------------------------------------------------------------------------

    FuncInfo::FuncInfo(int id, const std::string& name, const std::string& linkageName,
                       const LocInfo& location) :
        id(id),
        name(name),
        linkageName(linkageName),
        location(location)
    {
        dwarfLog << "FuncInfo: " << name << " " << linkageName << std::endl;
    }

    //------------------------------------------------------------------------------
    //VarInfo
    //------------------------------------------------------------------------------

    VarInfo::VarInfo(int id, StorageType type, FuncInfo* parent, const std::string& name, size_t size,
                     size_t typeSize, const LocInfo& location, const SourceLocation& srcLoc) :
        id(id),
        type(type),
        parent(parent),
        name(name),
        size(size),
        typeSize(typeSize),
        location(location),
        srcLoc(srcLoc)
    {
        dwarfLog << "VarInfo: " << name << " " << size << " " << typeSize << std::endl;
    }

    //------------------------------------------------------------------------------
    //DwarfContext
    //------------------------------------------------------------------------------

    void DwarfContext::addFunc(const FuncInfo& f)
    {
        auto ret = funcs.insert(std::make_pair(f.id, f));
        assert(ret.second);
    }

    FuncInfo* DwarfContext::getFunc(int id)
    {
        auto it = funcs.find(id);
        assert(it != funcs.end());
        return &it->second;
    }

    void DwarfContext::addVar(const VarInfo& v)
    {
        auto ret = vars.insert(std::make_pair(v.id, v));
        assert(ret.second);
        auto& mv = ret.first->second;
        if (mv.parent)
        {
            mv.parent->vars.push_back(&mv);
        }
    }

    VarInfo* DwarfContext::getVar(int id)
    {
        auto it = vars.find(id);
        assert(it != vars.end());
        return &it->second;
    }

    dbginfo::DebugContext DwarfContext::toDbg() const
    {
        dbginfo::DebugContext ctxt;
        for (auto& fe : funcs)
        {
            auto& f = fe.second;
            std::string name = f.linkageName.empty() ? f.name : f.linkageName;
            if (name == DwarfEmptyName)
            {
                continue;
            }
            auto& entries = f.location.entries;
            if (entries.empty())
            {
                continue;
            }
            auto& entry = entries.front();
            if (entry.reg != LocSource::RSP)
            {
                continue;
            }
            ssize_t loc = entry.off;
            auto* pFuncInfo = ctxt.addFunc(dbginfo::FuncInfo(name, loc));
            for (auto& pv : f.vars)
            {
                auto& v = *pv;
                std::string name = v.name;
                auto& entries = v.location.entries;
                dbginfo::StorageType type =
                    v.type == StorageType::Auto ?
                    dbginfo::StorageType::Auto :
                    dbginfo::StorageType::Static;

                //if there is loc info with offset from frame base keep only it
                LocEntry entry;
                bool found = false;
                for (auto& e : entries)
                {
                    if (e.reg == LocSource::FrameBase)
                    {
                        entry = e;
                        found = true;
                        break;
                    }
                    else if (e.reg == LocSource::Addr)
                    {
                        if (type != dbginfo::StorageType::Static)
                        {
                            continue;
                        }
                        entry = e;
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    continue;
                }
                dbginfo::VarInfo var(type, v.name, v.size, v.typeSize, entry.off, v.srcLoc, pFuncInfo);
                ctxt.addVar(var);
            }
        }
        //global variables (without parent)
        for (auto& ve : vars)
        {
            auto& v = ve.second;
            if (!v.parent)
            {
                assert(v.type == StorageType::Static);
                auto& entries = v.location.entries;
                if (entries.empty())
                {
                    continue;
                }
                auto& entry = entries.front();
                dbginfo::VarInfo var(dbginfo::StorageType::Static, v.name, v.size,
                                     v.typeSize, entry.off, v.srcLoc);
                ctxt.addVar(var);
            }
        }
        return ctxt;
    }
} //namespace dwarf
