#pragma once
#include <string>
#include "common/utils.h"

struct SourceLocation
{
    std::string fileName;
    int line;

    SourceLocation() = default;
    SourceLocation(const std::string& fileName, int line):
        fileName(fileName),
        line(line)
    {
    }

    operator bool() const
    {
        return !fileName.empty();
    }

    bool operator==(const SourceLocation& sourceLocation) const
    {
        return fileName == sourceLocation.fileName &&
               line == sourceLocation.line;
    }

    std::string str() const
    {
        return fileName + ":" + std::to_string(line);
    }

    void save(std::ostream& out) const
    {
        utils::save(fileName, out);
        utils::save(line, out);
    }

    void load(std::istream& in)
    {
        fileName = utils::load<std::string>(in);
        line = utils::load<int>(in);
    }
};
