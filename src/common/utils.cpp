#include "utils.h"

namespace utils
{
    template <>
    void save<std::string>(const std::string& s, std::ostream& out)
    {
        auto sz = s.length();
        out.write((char*)&sz, sizeof(sz));
        if (sz != 0)
        {
            out.write(s.c_str(), sizeof(char)*s.length());
        }
    }

    template <>
    std::string load<std::string>(std::istream& in)
    {
        std::string::size_type sz;
        in.read((char*)&sz, sizeof(sz));
        std::string s;
        s.resize(sz);
        if (sz != 0)
        {
            in.read((char*)s.data(), sz*sizeof(char));
        }
        return s;
    }
} //namespace utils
