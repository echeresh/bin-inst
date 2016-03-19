#include "location.h"
#include <cassert>
#include <sstream>
#include "dwarflog.h"
#include "dwarfutils.h"

namespace dwarf
{
    std::string to_string(LocSource src)
    {
        switch (src)
        {
        case LocSource::Addr:
            return "Addr";
        case LocSource::FrameBase:
            return "FrameBase";
        case LocSource::RBP:
            return "RBP";
        case LocSource::RSP:
            return "RSP";
        }
        return "Unknown";
    }

    //------------------------------------------------------------------------------
    //LocEntry
    //------------------------------------------------------------------------------

    LocEntry::LocEntry(void* lo, void* hi, LocSource reg, int off) :
        lo(lo), hi(hi), reg(reg), off(off)
    {
        dwarfLog << "LocEntry: " << lo << " " << hi
                 << " " << to_string(reg) << " " << off << std::endl;
    }

    //------------------------------------------------------------------------------
    //LocInfo
    //------------------------------------------------------------------------------

    void LocInfo::freeLoclist(Dwarf_Debug dbg, Dwarf_Locdesc** loclist, Dwarf_Signed nLoclist)
    {
        for (int i = 0; i < nLoclist; i++)
        {
            dwarf_dealloc(dbg, loclist[i]->ld_s, DW_DLA_LOC_BLOCK);
            dwarf_dealloc(dbg, loclist[i], DW_DLA_LOCDESC);
        }
        dwarf_dealloc(dbg, loclist, DW_DLA_LIST);
    }

    std::string LocInfo::str(Dwarf_Debug dbg, Dwarf_Locdesc** loclist, Dwarf_Signed nLoclist)
    {
        std::ostringstream oss;
        for (int i = 0; i < nLoclist; i++)
        {
            oss << (void*)loclist[i]->ld_lopc << " - " << (void*)loclist[i]->ld_hipc << " : ";
            for (int j = 0; j < loclist[i]->ld_cents; j++)
            {
                Dwarf_Loc loc = loclist[i]->ld_s[j];
                const char* opName;
                int err = dwarf_get_OP_name(loc.lr_atom, &opName);
                assert(err == DW_DLV_OK);
                oss << opName << " " << (long long)loc.lr_number << "; ";
            }
            oss << std::endl;
        }
        return oss.str();
    }

    LocSource LocInfo::getLocSource(Dwarf_Small lr_atom)
    {
        switch (lr_atom)
        {
        case DW_OP_addr:
            return LocSource::Addr;
        case DW_OP_fbreg:
            return LocSource::FrameBase;
        case DW_OP_breg6:
            return LocSource::RBP;
        case DW_OP_breg7:
            return LocSource::RSP;
        }
        return LocSource::Unknown;
    }

    LocInfo::LocInfo(Dwarf_Debug dbg, Dwarf_Die die)
    {
        Dwarf_Attribute attr;
        if (isFuncDie(die))
            attr = getAttr(die, DW_AT_frame_base);
        else
            attr = getAttr(die, DW_AT_location);

        if (!attr)
            return;

        Dwarf_Locdesc** loclist;
        Dwarf_Signed nLoclist;
        Dwarf_Error error;
        int err = dwarf_loclist_n(attr, &loclist, &nLoclist, &error);
        if (err != DW_DLV_OK)
        {
            dwarfLog << "unknown loc" << std::endl;
            return;
        }
        dwarfLog << std::endl;
        for (int i = 0; i < nLoclist; i++)
        {
            if (loclist[i]->ld_cents != 1)
            {
                dwarfLog << "unknown loc: " << std::endl;
                dwarfLog << str(dbg, loclist, nLoclist) << std::endl;
                freeLoclist(dbg, loclist, nLoclist);
                return;
            }

            Dwarf_Loc loc = loclist[i]->ld_s[0];
            LocSource locsrc = getLocSource(loc.lr_atom);
            if (locsrc == LocSource::Unknown)
            {
                dwarfLog << "unknown loc: " << std::endl;
                dwarfLog << str(dbg, loclist, nLoclist) << std::endl;
                freeLoclist(dbg, loclist, nLoclist);
                return;
            }
            entries.emplace_back((void*)loclist[i]->ld_lopc, (void*)loclist[i]->ld_hipc,
                                 locsrc, loc.lr_number);
        }
        freeLoclist(dbg, loclist, nLoclist);
    }
} //namespace dwarf
