#include <cassert>
#include <iostream>
#include <string>
#include "debugcontext.h"
using namespace std;

namespace dbginfo
{
    DebugContext::DebugContext(DebugContext&& d) :
        funcs(std::move(d.funcs)),
        vars(std::move(d.vars))
    {
    }

    DebugContext& DebugContext::operator=(DebugContext&& d)
    {
        funcs = std::move(d.funcs);
        vars = std::move(d.vars);
        return *this;
    }

    const FuncInfo* DebugContext::findFuncByName(const std::string& name) const
    {
        auto it = funcs.find(name);
        return it != funcs.end() ? &it->second : nullptr;
    }

    const FuncInfo* DebugContext::findFuncById(int id) const
    {
        auto it = idFuncs.find(id);
        return it != idFuncs.end() ? it->second : nullptr;
    }

    const FuncInfo* DebugContext::addFunc(const std::string& name, ssize_t stackOffset)
    {
        cout << "ADDED: " << name << endl;
        int id = funcs.size();
        auto ret = funcs.insert(make_pair(name, FuncInfo(name, stackOffset, id)));
        assert(ret.second);
        idFuncs.insert(make_pair(id, &ret.first->second));
        return &ret.first->second;
    }

    const VarInfo* DebugContext::addVar(const VarInfo& varInfo)
    {
        auto ret = vars.insert(varInfo);
        if (varInfo.parent)
            const_cast<FuncInfo*>(varInfo.parent)->vars.push_back(&*ret.first);
        return &*ret.first;
    }

    const VarInfo* DebugContext::findVarByAddress(void* addr) const
    {
        for (auto& v : vars)
            if (v.type == StorageType::Static)
            {
                char* varAddr = (char*)v.stackOffset;
                if (varAddr <= addr && addr < varAddr + v.size)
                    return &v;
            }
        return nullptr;
    }

    void DebugContext::setInstBinding(uint64_t inst, const SourceLocation& sourceLocation)
    {
        assert(sourceLocation);
        auto ret = instBindings.insert(make_pair(inst, sourceLocation));
        if (!ret.second)
        {
            assert(sourceLocation == ret.first->second);
        }
    }

    SourceLocation DebugContext::getInstBinding(uint64_t inst) const
    {
        auto it = instBindings.find(inst);
        return it == instBindings.end() ? SourceLocation() : it->second;
    }

    void DebugContext::save(std::ostream& out) const
    {
        auto nFuncs = funcs.size();
        utils::save(nFuncs, out);
        for (auto& e: funcs)
        {
            utils::save(e.first, out);
            e.second.save(out, *this);
        }
        auto nVars = vars.size();
        utils::save(nVars, out);
        for (auto& varInfo: vars)
        {
            varInfo.save(out, *this);
        }
        utils::save(instBindings.size(), out);
        for (auto e: instBindings)
        {
            utils::save(e.first, out);
            e.second.save(out);
        }
    }

    void DebugContext::load(std::istream& in)
    {
        auto nFuncs = utils::load<std::map<std::string, FuncInfo>::size_type>(in);
        for (int i = 0; i < nFuncs; i++)
        {
            auto name = utils::load<std::string>(in);
            FuncInfo funcInfo;
            funcInfo.load(in, *this);
            auto ret = funcs.insert(make_pair(name, funcInfo));
            assert(ret.second);
            idFuncs.insert(make_pair(funcInfo.id, &ret.first->second));
        }
        auto nVars = utils::load<std::set<VarInfo>::size_type>(in);
        for (int i = 0; i < nVars; i++)
        {
            VarInfo varInfo;
            varInfo.load(in, *this);
            auto ret = vars.insert(varInfo);
            assert(ret.second);
            if (ret.first->parent)
            {
                const_cast<FuncInfo*>(ret.first->parent)->vars.push_back(&*ret.first);
            }
        }
        auto nInstBindings = utils::load<std::map<uint64_t, SourceLocation>::size_type>(in);
        for (int i = 0; i < nInstBindings; i++)
        {
            auto inst = utils::load<uint64_t>(in);
            SourceLocation sourceLoc;
            sourceLoc.load(in);
            instBindings[inst] = sourceLoc;
        }
    }
} //namespace dbginfo
