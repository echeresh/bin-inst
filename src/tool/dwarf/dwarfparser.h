#pragma once
#include <string>
#include <libdwarf.h>
#include <dwarf.h>
#include <libelf.h>
#include <functional>
#include "dwarfcontext.h"

namespace dwarf
{
    struct WalkInfo;

    class DwarfParser
    {
        Dwarf_Debug dbg = nullptr;
        DwarfContext dctx;

    public:
        DwarfParser(const std::string& filePath);
        size_t getSize(Dwarf_Die die, const WalkInfo& walkInfo, size_t* pTypeSize);
        void walkTree(Dwarf_Die die, WalkInfo& walkInfo);
        void walk(Dwarf_Die cuDie, Dwarf_Unsigned off);
        void cuWalk(std::function<void(Dwarf_Die, Dwarf_Unsigned)> cuHandler);
        const DwarfContext& getDwarfContext() const;
        ~DwarfParser();
    };
} //namespace dwarf
