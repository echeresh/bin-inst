#include "debuginfo.h"

namespace dbg
{       
    VarInfo::VarInfo(StorageType type, const std::string& name, size_t size,
        ssize_t loc, const FuncInfo* parent) :
        type(type),
        parent(parent),
        name(name),
        size(size),
        loc(loc)
    {
    }
        
    bool VarInfo::operator<(const VarInfo& v) const
    {
        return this < &v;
    }

    FuncInfo::FuncInfo(const std::string& name, ssize_t loc) :
        name(name),
        loc(loc)
    {
    }
    
    //

    DebugContext::DebugContext(DebugContext&& d) :
        funcs(std::move(d.funcs)),
        vars(std::move(d.vars))
    {
    }
        
    DebugContext&  DebugContext::operator=(DebugContext&& d)
    {
        funcs = std::move(d.funcs);
        vars = std::move(d.vars);
        return *this;
    }
    
    const FuncInfo*  DebugContext::findFuncByName(const std::string& name) const
    {
        auto it = funcs.find(name);
        return it != funcs.end() ? &it->second : nullptr;
    }
    
    const FuncInfo*  DebugContext::addFunc(const FuncInfo& f)
    {
        auto ret = funcs.insert(make_pair(f.name, f));
        std::cout << f.name << std::endl;
        assert(ret.second);
        return &ret.first->second;
    }
    
    const VarInfo*  DebugContext::addVar(const VarInfo& f)
    {
        auto ret = vars.insert(f);
        if (f.parent)
            const_cast<FuncInfo*>(f.parent)->vars.push_back(&*ret.first);
        return &*ret.first;
    }
}