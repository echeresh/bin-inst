#include <fstream>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "dwarfutils.h"
#include "dwarfparser.h"
#include "dwarflog.h"
using namespace std;

namespace dwarf
{
    struct WalkInfo
    {
        Dwarf_Die parentDie = nullptr;
        StorageType storageType = StorageType::Static;
        Dwarf_Die lastFuncDie = nullptr;
        Dwarf_Unsigned cuOffset = 0;
        Dwarf_Signed lang;
        const char* cuName;
    };

    //------------------------------------------------------------------------------
    //DwarfParser
    //------------------------------------------------------------------------------

    DwarfParser::DwarfParser(const std::string& filePath)
    {
        int fd = open(filePath.c_str(), O_RDONLY);
        assert(fd != -1);
        Dwarf_Unsigned access = DW_DLC_READ;
        DWARF_CHECK(dwarf_init(fd, access, nullptr, nullptr, &dbg, nullptr));
    }

    size_t DwarfParser::getSize(Dwarf_Die die, const WalkInfo& walkInfo)
    {
        const size_t PtrSize = 8;
        Dwarf_Attribute attr = getAttr(die, DW_AT_type);
        if (!attr)
        {
            //dwarfLog << endl << "type attr is missed tag name: " << getTagName(die) << endl;
            return 0;
        }

        Dwarf_Half retForm;
        dwarf_whatform(attr, &retForm, nullptr);
        assert(retForm == DW_FORM_ref4);

        Dwarf_Off off;
        DWARF_CHECK(dwarf_formref(attr, &off, nullptr));

        Dwarf_Die typeDie;
        DWARF_CHECK(dwarf_offdie(dbg, walkInfo.cuOffset + off, &typeDie, nullptr));
        Dwarf_Half tag = getTag(typeDie);
        if (tag == DW_TAG_typedef || tag == DW_TAG_const_type ||
            tag == DW_TAG_volatile_type || tag == DW_TAG_restrict_type ||
            tag == DW_TAG_mutable_type)
            return getSize(typeDie, walkInfo);

        Dwarf_Bool hasSize;
        switch (tag)
        {
        // case DW_TAG_subrange_type:
            // err = dwarf_hasattr(typeDie, DW_AT_byte_size, &hasSize, nullptr);
            // assert(err == DW_DLV_OK);
            // if (!hasSize)
                // return getSize(typeDie, walkInfo);
        case DW_TAG_array_type:
        {
            if (getTag(die) == DW_TAG_formal_parameter)
                return PtrSize;
            Dwarf_Die child;
            if (dwarf_child(typeDie, &child, nullptr) != DW_DLV_OK)
            {
                dwarfLog << "error: size for array type: no child" << endl;
                return 0;
            }

            Dwarf_Attribute attr = getAttr(child, DW_AT_type);
            if (!attr)
                return 0;
            Dwarf_Off off;
            DWARF_CHECK(dwarf_formref(attr, &off, nullptr));

            Dwarf_Die typeChildDie;
            DWARF_CHECK(dwarf_offdie(dbg, walkInfo.cuOffset + off, &typeChildDie, nullptr));
            assert(getTag(child) == DW_TAG_subrange_type);
            attr = getAttr(child, DW_AT_upper_bound);
            //Fortran uses array type without upper_bound
            //for formal parameters
            if (!attr)
                return PtrSize;

            Dwarf_Unsigned upperBound;
            dwarf_formudata(attr, &upperBound, nullptr);
            if (walkInfo.lang != DW_LANG_Fortran77 &&
                walkInfo.lang != DW_LANG_Fortran90 &&
                walkInfo.lang != DW_LANG_Fortran95)
                upperBound++;
            size_t elemCount = upperBound;
            {
                while (dwarf_siblingof(dbg, child, &child, nullptr) == DW_DLV_OK)
                {
                    attr = getAttr(child, DW_AT_upper_bound);
                    assert(attr);
                    dwarf_formudata(attr, &upperBound, nullptr);
                    if (walkInfo.lang != DW_LANG_Fortran77 &&
                        walkInfo.lang != DW_LANG_Fortran90 &&
                        walkInfo.lang != DW_LANG_Fortran95)
                        upperBound++;
                    elemCount *= upperBound;
                }
            }
            return elemCount*getSize(typeDie, walkInfo);
        }
        case DW_TAG_base_type:
        case DW_TAG_enumeration_type:
        case DW_TAG_pointer_type:
        case DW_TAG_ptr_to_member_type:
        case DW_TAG_structure_type:
        case DW_TAG_class_type:
        case DW_TAG_union_type:
        case DW_TAG_string_type:
            DWARF_CHECK(dwarf_hasattr(typeDie, DW_AT_byte_size, &hasSize, nullptr));
            if (hasSize)
            {
                Dwarf_Unsigned size;
                DWARF_CHECK(dwarf_attr(typeDie, DW_AT_byte_size, &attr, nullptr));
                dwarf_formudata(attr, &size, nullptr);
                return size;
            }
        }
        dwarfLog << std::endl << "size attr is missed tag name: " << getTagName(typeDie)
                 << "; die name: " << getDieName(typeDie) << std::endl;
        return 0;
    }

    void DwarfParser::walkTree(Dwarf_Die die, WalkInfo& walkInfo)
    {
        Dwarf_Half tag = getTag(die);
        Dwarf_Die child;
        int err;
        dwarfLog << getDieName(die) << " " << getTagName(die) << std::endl;

        Dwarf_Off dieOff;
        DWARF_CHECK(dwarf_dieoffset(die, &dieOff, nullptr));

        if (tag == DW_TAG_variable || tag == DW_TAG_formal_parameter)
        {
            if (!isFuncDie(walkInfo.parentDie))
            {
                Dwarf_Half parentTag;
                const char* parentTagName;
                DWARF_CHECK(dwarf_tag(walkInfo.parentDie, &parentTag, nullptr));
                DWARF_CHECK(dwarf_get_TAG_name(parentTag, &parentTagName));
            }
            size_t size = getSize(die, walkInfo);
            //dwarfLog << "size = " << size << endl;
            if (size != 0)
            {
                Dwarf_Unsigned declLine = -1;
                auto declLineAttr = getAttr(die, DW_AT_decl_line);
                Dwarf_Error error;
                dwarf_formudata(declLineAttr, &declLine, &error);
                if (walkInfo.storageType == StorageType::Static)
                {
                    Dwarf_Off id;
                    DWARF_CHECK(dwarf_dieoffset(die, &id, nullptr));
                    dctx.addVar(VarInfo(id, StorageType::Static, nullptr, getDieName(die),
                                        size, LocInfo(dbg, die), SourceLocation(walkInfo.cuName, declLine)));
                }
                else
                {
                    Dwarf_Off id;
                    assert(walkInfo.lastFuncDie);
                    DWARF_CHECK(dwarf_dieoffset(walkInfo.lastFuncDie, &id, nullptr));
                    auto* func = dctx.getFunc(id);
                    DWARF_CHECK(dwarf_dieoffset(die, &id, nullptr));
                    dctx.addVar(VarInfo(id, StorageType::Auto, func, getDieName(die),
                                        size, LocInfo(dbg, die), SourceLocation(walkInfo.cuName, declLine)));
                }
            }
        }
        dwarfLog << endl;

        WalkInfo childWalkInfo = walkInfo;
        childWalkInfo.parentDie = die;
        if (isFuncDie(die))
        {
            childWalkInfo.lastFuncDie = die;
            Dwarf_Off id;
            err = dwarf_dieoffset(die, &id, nullptr);
            assert(err == DW_DLV_OK);

            //gcc uses DW_AT_MIPS_linkage_name attribute to set linkage name
            Dwarf_Attribute attr = getAttr(die, DW_AT_linkage_name);
            if (!attr)
                attr = getAttr(die, DW_AT_MIPS_linkage_name);

            string name = getDieName(die);
            std::cout << name << std::endl;
            string linkageName;
            if (attr)
            {
                Dwarf_Half retForm;
                dwarf_whatform(attr, &retForm, nullptr);
                std::cout << retForm << std::endl;
                assert(retForm == DW_FORM_strp || retForm == DW_FORM_string);
                char* linkageNamePtr = nullptr;
                dwarf_formstring(attr, &linkageNamePtr, NULL);
                linkageName = linkageNamePtr;
            }
            dctx.addFunc(FuncInfo(id, name, linkageName, LocInfo(dbg, die)));
        }

        if (dwarf_child(die, &child, nullptr) != DW_DLV_OK)
            return;

        if (tag == DW_TAG_lexical_block || tag == DW_TAG_subprogram)
            childWalkInfo.storageType = StorageType::Auto;
        else if (tag == DW_TAG_common_block)
            childWalkInfo.storageType = StorageType::Static;

        dwarfLog.down();
        walkTree(child, childWalkInfo);
        while (dwarf_siblingof(dbg, child, &child, nullptr) == DW_DLV_OK)
            walkTree(child, childWalkInfo);
        dwarfLog.up();
    }

    void DwarfParser::walk(Dwarf_Die cuDie, Dwarf_Unsigned off)
    {
        WalkInfo walkInfo;
        walkInfo.cuOffset = off;
        Dwarf_Attribute attr = getAttr(cuDie, DW_AT_language);
        Dwarf_Half retForm;
        dwarf_whatform(attr, &retForm, nullptr);
        assert(retForm == DW_FORM_data1);
        DWARF_CHECK(dwarf_formsdata(attr, &walkInfo.lang, nullptr));
        char* cuName;
        DWARF_CHECK(dwarf_diename(cuDie, &cuName, nullptr));
        walkInfo.cuName = cuName;
        walkTree(cuDie, walkInfo);
    }

    void DwarfParser::cuWalk(function<void(Dwarf_Die, Dwarf_Unsigned)> cuHandler)
    {
        Dwarf_Unsigned cuOffset = 0;
        Dwarf_Unsigned cuNextOffset;
        while (dwarf_next_cu_header_b(dbg, nullptr, nullptr, nullptr, nullptr,
                                      nullptr, nullptr, &cuNextOffset, nullptr) == DW_DLV_OK)
        {
            Dwarf_Die cuDie;
            DWARF_CHECK(dwarf_siblingof(dbg, nullptr, &cuDie, nullptr));
            assert(getTag(cuDie) == DW_TAG_compile_unit);
            string offsetStr;
            cuHandler(cuDie, cuOffset);
            cuOffset = cuNextOffset;
        }
    }

    const DwarfContext& DwarfParser::getDwarfContext() const
    {
        return dctx;
    }

    DwarfParser::~DwarfParser()
    {
        dwarf_elf_handle elfptr;
        DWARF_CHECK(dwarf_get_elf(dbg, &elfptr, nullptr));
        DWARF_CHECK(dwarf_finish(dbg, nullptr));
        //elf_end(elfptr);
    }
} //namespace dwarf
