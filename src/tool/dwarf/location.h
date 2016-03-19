#pragma once
#include <string>
#include <vector>
#include <libdwarf.h>
#include <dwarf.h>

namespace dwarf
{
    enum class LocSource
    {
        RBP,
        RSP,
        FrameBase,
        Addr,
        Unknown
    };

    std::string to_string(LocSource src);

    struct LocEntry
    {
        void* lo;
        void* hi;
        LocSource reg;
        int off;

        LocEntry() = default;
        LocEntry(void* lo, void* hi, LocSource reg, int off);
    };

    struct LocInfo
    {
        //[lopc; hipc)
        std::vector<LocEntry> entries;

        LocInfo() = default;
        void freeLoclist(Dwarf_Debug dbg, Dwarf_Locdesc** loclist, Dwarf_Signed nLoclist);
        std::string str(Dwarf_Debug dbg, Dwarf_Locdesc** loclist, Dwarf_Signed nLoclist);
        LocSource getLocSource(Dwarf_Small lr_atom);
        LocInfo(Dwarf_Debug dbg, Dwarf_Die die);
    };
} //namespace dwarf
