#include "dwarfparser.h"
using namespace std;

#define _VERBOSE
#ifdef _VERBOSE
ostream& trace(cout);
#else
ostream trace(0);
#endif
#define DWARF_CHECK(call) \
    do \
    { \
        int ret = call; \
        trace << "DWARF: " #call " = " << ret << endl; \
        if (ret != DW_DLV_OK) \
            return -1; \
    } \
    while (false);

static Dwarf_Half getTag(Dwarf_Die die)
{   
    Dwarf_Half tag;
    int err = dwarf_tag(die, &tag, nullptr);
    assert(err == DW_DLV_OK);
    return tag;
}

static string getTagName(Dwarf_Die die)
{
    const char* tagName;
    int err = dwarf_get_TAG_name(getTag(die), &tagName);
    assert(err == DW_DLV_OK);
    return tagName;
}

static string getDieName(Dwarf_Die die)
{
    char* dieName;
    int err = dwarf_diename(die, &dieName, nullptr);
    if (err != DW_DLV_OK)
        return "[no-name]";
    return dieName;
}

static Dwarf_Attribute getAttr(Dwarf_Die die, Dwarf_Half attr)
{
    Dwarf_Attribute ret;
    Dwarf_Bool hasAttr;
    int err = dwarf_hasattr(die, attr, &hasAttr, nullptr);
    assert(err == DW_DLV_OK);
    if (!hasAttr)
        return nullptr;
    err = dwarf_attr(die, attr, &ret, nullptr);
    assert(err == DW_DLV_OK);
    return ret;
}

LocEntry::LocEntry(void* lo, void* hi, LocSource reg, int off) :
    lo(lo), hi(hi), reg(reg), off(off)
{
    cout << endl << "LocEntry: " << lo << " " << hi
         << " " << (int)reg << " " << off << endl;
}

void LocInfo::freeLoclist(Dwarf_Debug dbg, Dwarf_Locdesc** loclist, Dwarf_Signed nLoclist)
{
    for (int i = 0; i < nLoclist; i++)
    {
        dwarf_dealloc(dbg, loclist[i]->ld_s, DW_DLA_LOC_BLOCK);
        dwarf_dealloc(dbg, loclist[i], DW_DLA_LOCDESC);
    }
    dwarf_dealloc(dbg, loclist, DW_DLA_LIST);
}

string LocInfo::str(Dwarf_Debug dbg, Dwarf_Locdesc** loclist, Dwarf_Signed nLoclist)
{
    ostringstream oss;
    for (int i = 0; i < nLoclist; i++)
    {
        oss << (void*)loclist[i]->ld_lopc << " - " << (void*)loclist[i]->ld_hipc << endl;
        for (int j = 0; j < loclist[i]->ld_cents; j++)  
        {
            Dwarf_Loc loc = loclist[i]->ld_s[j];
            const char* opName;
            int err = dwarf_get_OP_name(loc.lr_atom, &opName);
            assert(err == DW_DLV_OK);                      
            oss << "\t" << opName << " " << (long long)loc.lr_number << "; ";
        }
        oss << endl;
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
    if (getTag(die) == DW_TAG_subprogram)
        attr = getAttr(die, DW_AT_frame_base);
    else
        attr = getAttr(die, DW_AT_location);

    if (!attr)
        return;

    Dwarf_Locdesc** loclist;
    Dwarf_Signed nLoclist;
    int err = dwarf_loclist_n(attr, &loclist, &nLoclist, nullptr);
    assert(err == DW_DLV_OK);

    for (int i = 0; i < nLoclist; i++)
    {
        if (loclist[i]->ld_cents != 1)
        {
            cout << "unknown loc: " << endl << str(dbg, loclist, nLoclist) << endl;
            freeLoclist(dbg, loclist, nLoclist);
            return;
        }

        Dwarf_Loc loc = loclist[i]->ld_s[0];
        LocSource locsrc = getLocSource(loc.lr_atom);
        if (locsrc == LocSource::Unknown)
        {
            cout << endl << "unknown loc: " << endl << str(dbg, loclist, nLoclist) << endl;
            freeLoclist(dbg, loclist, nLoclist);
            return;
        }
        entries.emplace_back((void*)loclist[i]->ld_lopc, (void*)loclist[i]->ld_hipc,
                             locsrc, loc.lr_number);
    }
    freeLoclist(dbg, loclist, nLoclist);
}

FuncInfo::FuncInfo(int id, const string& name, const string& mangledName,
         const LocInfo& location) :
    id(id),
    name(name),
    mangledName(mangledName),
    location(location)
{
    cout << "FuncInfo: " << name << " " << mangledName << endl;
}

VarInfo::VarInfo(int id, StorageType type, FuncInfo* parent, const string& name, size_t size,
        const LocInfo& location) :
    id(id),
    type(type),
    parent(parent),
    name(name),
    size(size),
    location(location)
{
    cout << "VarInfo: " << name << " " << size << endl;
}

void DebugContext::addFunc(const FuncInfo& f)
{
    auto ret = funcs.insert(make_pair(f.id, f));
    assert(ret.second);
}

const FuncInfo* DebugContext::findFuncByName(const std::string& name) const
{
    for (auto& e : funcs)
    {
        if (e.second.mangledName != "")
        {
            if (e.second.mangledName == name)
                return &e.second;
        }
        else if (e.second.name == name)
            return &e.second;
    }
    return nullptr;
}

FuncInfo* DebugContext::getFunc(int id)
{
    auto it = funcs.find(id);
    assert(it != funcs.end());
    return &it->second;
}

void DebugContext::addVar(const VarInfo& v)
{
    //cout << "ss " << v.name << " " << v.id << endl;
    auto ret = vars.insert(make_pair(v.id, v));
    assert(ret.second);
    auto& mv = ret.first->second;
    if (mv.parent)
        mv.parent->vars.push_back(&mv);
}

VarInfo* DebugContext::getVar(int id)
{
    auto it = vars.find(id);
    assert(it != vars.end());
    return &it->second;
}    


bool DwarfParser::isFuncDie(Dwarf_Die die)
{
    Dwarf_Half tag;
    int err = dwarf_tag(die, &tag, nullptr);
    assert(err == DW_DLV_OK);
    return tag == DW_TAG_subprogram;
}

DwarfParser::DwarfParser(const char* filePath)
{
    int fd = open(filePath, O_RDONLY);
    assert(fd != -1);
    Dwarf_Unsigned access = DW_DLC_READ;
    int err = dwarf_init(fd, access, nullptr, nullptr, &dbg, nullptr);
    assert(err == DW_DLV_OK);
}

size_t DwarfParser::getSize(Dwarf_Die die, Dwarf_Unsigned cuOffset)
{
    Dwarf_Attribute attr = getAttr(die, DW_AT_type);
    if (!attr)
    {
        //cout << endl << "type attr is missed tag name: " << getTagName(die) << endl;
        return 0;
    }
    
    Dwarf_Half retForm;
    dwarf_whatform(attr, &retForm, nullptr);
    assert(retForm == DW_FORM_ref4);
    
    Dwarf_Off off;
    int err = dwarf_formref(attr, &off, nullptr);
    assert(err == DW_DLV_OK);
    
    Dwarf_Die typeDie;
    err = dwarf_offdie(dbg, cuOffset + off, &typeDie, nullptr);
    Dwarf_Half tag = getTag(typeDie);
    if (tag == DW_TAG_typedef || tag == DW_TAG_const_type ||
        tag == DW_TAG_volatile_type || tag == DW_TAG_restrict_type ||
        tag == DW_TAG_mutable_type)
        return getSize(typeDie, cuOffset);
        
    Dwarf_Bool hasSize;
    switch (tag)
    {
    // case DW_TAG_subrange_type:
        // err = dwarf_hasattr(typeDie, DW_AT_byte_size, &hasSize, nullptr);
        // assert(err == DW_DLV_OK);
        // if (!hasSize)
            // return getSize(typeDie, cuOffset); 
    case DW_TAG_array_type:
    {
        Dwarf_Die child;
        if (dwarf_child(typeDie, &child, nullptr) != DW_DLV_OK)
        {
            cout << "error: size for array type: no child" << endl;
            return 0;
        }
        
        Dwarf_Attribute attr = getAttr(child, DW_AT_type);
        if (!attr)
            return 0;
        Dwarf_Off off;
        int err = dwarf_formref(attr, &off, nullptr);
        assert(err == DW_DLV_OK);
        
        Dwarf_Die typeChildDie;
        err = dwarf_offdie(dbg, cuOffset + off, &typeChildDie, nullptr);
        //cout << "tag: " << getTagName(typeChildDie) << endl;
        assert(getTag(child) == DW_TAG_subrange_type);
        attr = getAttr(child, DW_AT_upper_bound);
        assert(attr);
        Dwarf_Unsigned upperBound;
        dwarf_formudata(attr, &upperBound, nullptr);
        //cout << "length: " << upperBound + 1 << endl;
        return (upperBound + 1)*getSize(typeDie, cuOffset);
    }
    case DW_TAG_base_type:
    case DW_TAG_enumeration_type:
    case DW_TAG_pointer_type:
    case DW_TAG_ptr_to_member_type:
    case DW_TAG_structure_type:
    case DW_TAG_class_type:
    case DW_TAG_union_type:
        err = dwarf_hasattr(typeDie, DW_AT_byte_size, &hasSize, nullptr);
        assert(err == DW_DLV_OK);
        if (hasSize)
        {
            Dwarf_Unsigned size;
            err = dwarf_attr(typeDie, DW_AT_byte_size, &attr, nullptr);
            assert(err == DW_DLV_OK);
            dwarf_formudata(attr, &size, nullptr);
            return size;
        }
    }
    cout << endl << "size attr is missed tag name: " << getTagName(typeDie)
         << "die name: " << getDieName(typeDie) << endl;
    return 0;
}

void DwarfParser::walkTree(Dwarf_Die die, Dwarf_Unsigned cuOffset, WalkInfo& walkInfo)
{
    Dwarf_Half tag = getTag(die);
    Dwarf_Die child;
    int err;
    cout << walkInfo.offset << getDieName(die) << " " << getTagName(die);
        
    Dwarf_Off dieOff;
    err = dwarf_dieoffset(die, &dieOff, nullptr);
    assert(err == DW_DLV_OK);

    if (tag == DW_TAG_variable || tag == DW_TAG_formal_parameter)
    {
        if (!isFuncDie(walkInfo.parentDie))
        {
            Dwarf_Half parentTag;
            const char* parentTagName;
            err = dwarf_tag(walkInfo.parentDie, &parentTag, nullptr);
            assert(err == DW_DLV_OK);
            err = dwarf_get_TAG_name(parentTag, &parentTagName);
            assert(err == DW_DLV_OK);
        }
        size_t size = getSize(die, cuOffset);
        //cout << "size = " << size << endl;
        if (size != 0)
        {
            if (walkInfo.storageType == StorageType::Static)
            {
                Dwarf_Off id;
                err = dwarf_dieoffset(die, &id, nullptr);
                assert(err == DW_DLV_OK);
                dctx.addVar(VarInfo(id, StorageType::Static, nullptr, getDieName(die),
                                    size, LocInfo(dbg, die)));
            }
            else
            {
                Dwarf_Off id;
                assert(walkInfo.lastFuncDie);                
                err = dwarf_dieoffset(walkInfo.lastFuncDie, &id, nullptr);
                assert(err == DW_DLV_OK);
                auto* func = dctx.getFunc(id);
                err = dwarf_dieoffset(die, &id, nullptr);
                assert(err == DW_DLV_OK);
                dctx.addVar(VarInfo(id, StorageType::Auto, func, getDieName(die),
                                    size, LocInfo(dbg, die)));
            }
        }
    }
    cout << endl;
    
    WalkInfo childWalkInfo = walkInfo;
    childWalkInfo.offset += "\t";
    childWalkInfo.parentDie = die;
    if (isFuncDie(die))
    {
        childWalkInfo.lastFuncDie = die;
        Dwarf_Off id;
        err = dwarf_dieoffset(die, &id, nullptr);
        assert(err == DW_DLV_OK);
        char* mangledName = nullptr;
        Dwarf_Bool hasAttr;
        err = dwarf_hasattr(die, DW_AT_MIPS_linkage_name, &hasAttr, NULL);
        assert(err == DW_DLV_OK);
        if (hasAttr)
        {
            Dwarf_Attribute attr;
            err = dwarf_attr(die, DW_AT_MIPS_linkage_name, &attr, NULL);
            assert(err == DW_DLV_OK);
            // Dwarf_Half retForm;
            // err = dwarf_whatform(attr, &retForm, nullptr);
            // assert(err == DW_DLV_OK);
            err = dwarf_formstring(attr, &mangledName, NULL);
            assert(err == DW_DLV_OK);
        }
        dctx.addFunc(FuncInfo(id, getDieName(die), mangledName ? mangledName : "",
                              LocInfo(dbg, die)));
    }

    if (dwarf_child(die, &child, nullptr) != DW_DLV_OK)
        return;
        
    if (tag == DW_TAG_lexical_block || tag == DW_TAG_subprogram)
        childWalkInfo.storageType = StorageType::Auto;
        
    walkTree(child, cuOffset, childWalkInfo);
    while (dwarf_siblingof(dbg, child, &child, nullptr) == DW_DLV_OK)
        walkTree(child, cuOffset, childWalkInfo);
}

void DwarfParser::walk(Dwarf_Die cuDie, Dwarf_Unsigned off)
{
    WalkInfo walkInfo;
    walkTree(cuDie, off, walkInfo);
}

void DwarfParser::cuWalk(function<void(Dwarf_Die, Dwarf_Unsigned)> cuHandler)
{   
    Dwarf_Unsigned cuOffset = 0;
    Dwarf_Unsigned cuNextOffset;
    while (dwarf_next_cu_header_b(dbg, nullptr, nullptr, nullptr, nullptr,
                                  nullptr, nullptr, &cuNextOffset, nullptr) == DW_DLV_OK)
    {
        Dwarf_Die cuDie;
        Dwarf_Half tag;
        int err = dwarf_siblingof(dbg, nullptr, &cuDie, nullptr);        
        assert(err == DW_DLV_OK);
        dwarf_tag(cuDie, &tag, nullptr);
        assert(tag == DW_TAG_compile_unit);
        string offsetStr;
        cuHandler(cuDie, cuOffset);
        cuOffset = cuNextOffset;
    }
}

const DebugContext& DwarfParser::getDebugContext() const
{
    return dctx;
}

DwarfParser::~DwarfParser()
{
    dwarf_elf_handle elfptr;
    int err = dwarf_get_elf(dbg, &elfptr, nullptr);
    assert(err == DW_DLV_OK);
    
    err = dwarf_finish(dbg, nullptr);
    assert(err == DW_DLV_OK);
    //elf_end(elfptr);
}