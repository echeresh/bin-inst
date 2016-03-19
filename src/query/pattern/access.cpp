#include "access.h"
#include <sstream>

namespace pattern
{
    std::string to_string(AccessType accessType)
    {
        switch (accessType)
        {
            case AccessType::Read:
                return "Read";
            case AccessType::Write:
                return "Write";
        }
        return "";
    }

    Access::Access(byte* addr, size_t size, AccessType accessType, const std::string& name) :
        addr(addr),
        size(size),
        accessType(accessType),
        name(name)
    {
    }

    std::string Access::str() const
    {
        std::ostringstream oss;
        oss << *this;
        return oss.str();
    }

    std::ostream& operator <<(std::ostream& out, const Access& a)
    {
        out << "[" << (void*)a.addr << " size: " << a.size
            << " type: " << to_string(a.accessType);
        if (!a.name.empty())
        {
            out << " name: " << a.name;
        }
        out << "]";
        return out;
    }
}
