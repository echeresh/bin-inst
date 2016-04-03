#include <string>
#include "debugcontext.h"
#include "funcinfo.h"
#include "common/utils.h"

namespace dbginfo
{
    FuncInfo::FuncInfo(const std::string& name, ssize_t stackOffset) :
        name(name),
        stackOffset(stackOffset)
    {
        static int globalId = 0;
        id = globalId++;
    }

    void FuncInfo::save(std::ostream& out, const DebugContext& dbgCtxt) const
    {
        utils::save(id, out);
        utils::save(name, out);
        utils::save(stackOffset, out);
    }

    void FuncInfo::load(std::istream& in, const DebugContext& dbgCtxt)
    {
        id = utils::load<int>(in);
        name = utils::load<std::string>(in);
        stackOffset = utils::load<ssize_t>(in);
    }
} //namespace dbginfo
