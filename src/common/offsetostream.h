#pragma once
#include <iostream>

class OffsetOstream
{
    std::ostream& out;
    std::string offset;
    bool isNewLine = true;

public:
    OffsetOstream(std::ostream& out = std::cout);
    void down();
    void up();

    template <class T>
    OffsetOstream& operator <<(const T& t)
    {
        if (isNewLine)
        {
            out << offset;
        }
        out << t;
        isNewLine = false;
        return *this;
    }

    OffsetOstream& operator <<(const std::string& s);

    typedef std::basic_ostream<char, std::char_traits<char>>& (*StdEndLine)(std::basic_ostream<char, std::char_traits<char>>&);

    OffsetOstream& operator <<(StdEndLine endLine);
};
