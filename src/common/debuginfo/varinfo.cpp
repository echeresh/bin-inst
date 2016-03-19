#include "varinfo.h"
#include "debugcontext.h"
#include "funcinfo.h"

namespace dbginfo
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

    void VarInfo::save(std::ostream& out, const DebugContext& dbgCtxt) const
    {
        utils::save(type, out);
        utils::save(!parent ? -1 : parent->id, out);
        utils::save(name, out);
        utils::save(size, out);
        utils::save(loc, out);
    }

    void VarInfo::load(std::istream& in, const DebugContext& dbgCtxt)
    {
        type = utils::load<StorageType>(in);
        int funcId = utils::load<int>(in);
        if (funcId >= 0)
        {
            parent = dbgCtxt.findFuncById(funcId);
        }
        name = utils::load<std::string>(in);
        size = utils::load<size_t>(in);
        loc = utils::load<ssize_t>(in);
    }
} //namespace dbginfo
