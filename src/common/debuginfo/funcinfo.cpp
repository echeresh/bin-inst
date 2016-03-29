#include <string>
#include "debugcontext.h"
#include "funcinfo.h"
#include "common/utils.h"

namespace dbginfo
{
    FuncInfo::FuncInfo(const std::string& name, ssize_t stackOffset, int id) :
        name(name),
        stackOffset(stackOffset),
        id(id)
    {
    }

    void FuncInfo::save(std::ostream& out, const DebugContext& dbgCtxt) const
    {
        utils::save(name, out);
        utils::save(stackOffset, out);
        utils::save(id, out);
    }

    void FuncInfo::load(std::istream& in, const DebugContext& dbgCtxt)
    {
        name = utils::load<std::string>(in);
        stackOffset = utils::load<ssize_t>(in);
        id = utils::load<int>(in);
    }
} //namespace dbginfo
