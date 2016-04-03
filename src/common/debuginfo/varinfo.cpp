#include "varinfo.h"
#include "debugcontext.h"
#include "funcinfo.h"

namespace dbginfo
{
    VarInfo::VarInfo(StorageType type, const std::string& name, size_t size,
                     ssize_t stackOffset, const FuncInfo* parent) :
        type(type),
        parent(parent),
        name(name),
        size(size),
        stackOffset(stackOffset)
    {
        static int globalId = 0;
        id = globalId++;
    }

    bool VarInfo::operator<(const VarInfo& v) const
    {
        return this < &v;
    }

    void VarInfo::save(std::ostream& out, const DebugContext& dbgCtxt) const
    {
        utils::save(id, out);
        utils::save(type, out);
        utils::save(!parent ? -1 : parent->id, out);
        utils::save(name, out);
        utils::save(size, out);
        utils::save(stackOffset, out);
    }

    void VarInfo::load(std::istream& in, const DebugContext& dbgCtxt)
    {
        id = utils::load<int>(in);
        type = utils::load<StorageType>(in);
        int funcId = utils::load<int>(in);
        if (funcId >= 0)
        {
            parent = dbgCtxt.findFuncById(funcId);
        }
        name = utils::load<std::string>(in);
        size = utils::load<size_t>(in);
        stackOffset = utils::load<ssize_t>(in);
    }
} //namespace dbginfo
