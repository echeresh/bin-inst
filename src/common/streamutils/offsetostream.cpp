#include "offsetostream.h"

namespace streamutils
{
    OffsetOstream::OffsetOstream(std::ostream& out) :
        out(out)
    {
    }

    void OffsetOstream::down()
    {
        offset += '\t';
    }

    void OffsetOstream::up()
    {
        offset.resize(offset.size() - 1);
    }

    OffsetOstream& OffsetOstream::operator <<(const std::string& s)
    {
        if (isNewLine)
        {
            out << offset;
        }
        for (int i = 0; i < s.size(); i++)
        {
            out << s[i];
            if (s[i] == '\n' && i != s.size() - 1)
            {
                out << offset;
            }

        }
        isNewLine = s.back() == '\n';
        return *this;
    }

    OffsetOstream& OffsetOstream::operator <<(StdEndLine endLine)
    {
        endLine(out);
        isNewLine = true;
        return *this;
    }
} //namespace streamutils
