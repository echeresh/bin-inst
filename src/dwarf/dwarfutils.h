#pragma once
#include <cassert>
#include <string>
#include <libdwarf.h>
#include <dwarf.h>

//std::cout << "DWARF: " #call " = " <<  std::to_string(checkedRet) << std::endl;

#define DWARF_CHECK(call) \
    do { \
        int checkedRet = call; \
        assert(checkedRet == DW_DLV_OK); \
    } \
    while (false)

namespace dwarf
{
    const std::string DwarfEmptyName = "[no-name]";

    inline Dwarf_Half getTag(Dwarf_Die die)
    {
        Dwarf_Half tag;
        DWARF_CHECK(dwarf_tag(die, &tag, nullptr));
        return tag;
    }

    inline std::string getTagName(Dwarf_Die die)
    {
        const char* tagName;
        DWARF_CHECK(dwarf_get_TAG_name(getTag(die), &tagName));
        return tagName;
    }

    inline std::string getDieName(Dwarf_Die die)
    {
        char* dieName;
        int err = dwarf_diename(die, &dieName, nullptr);
        if (err != DW_DLV_OK)
        {
            return DwarfEmptyName;
        }
        return dieName;
    }

    inline Dwarf_Attribute getAttr(Dwarf_Die die, Dwarf_Half attr)
    {
        Dwarf_Attribute ret;
        Dwarf_Bool hasAttr;
        DWARF_CHECK(dwarf_hasattr(die, attr, &hasAttr, nullptr));
        if (!hasAttr)
        {
            return nullptr;
        }
        DWARF_CHECK(dwarf_attr(die, attr, &ret, nullptr));
        return ret;
    }

    inline bool isFuncDie(Dwarf_Die die)
    {
        return getTag(die) == DW_TAG_subprogram;
    }
}
