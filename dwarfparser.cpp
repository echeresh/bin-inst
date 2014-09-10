#include <fstream>
#include "dwarfparser.h"
using namespace std;

const string NoName = "[no-name]";

struct OffsetOstream
{
    ostream& out;
    string offset;
    bool isNewLine = true;

    OffsetOstream(ostream& out = cout) : out(out)
    {
    }
    
    void down()
    {
        offset += '\t';
    }
    
    void up()
    {
        offset.resize(offset.size() - 1);
    }
    
    template <class T>
    OffsetOstream& operator<<(const T& t)
    {
        if (isNewLine)
            out << offset;
        out << t;
        isNewLine = false;
        return *this;    
    }
    
    OffsetOstream& operator<<(const string& s)
    {
        if (isNewLine)
            out << offset;
        for (int i = 0; i < s.size(); i++)
        {
            out << s[i];
            if (s[i] == '\n' && i != s.size() - 1)
                out << offset;
            
        }
        isNewLine = s.back() == '\n';
        return *this;    
    }
    
    typedef std::basic_ostream<char, std::char_traits<char>>& (*StdEndLine)(std::basic_ostream<char, std::char_traits<char>>&);

    OffsetOstream& operator<<(StdEndLine endLine)
    {
        endLine(out);
        isNewLine = true;
        return *this;
    }
};

static ofstream dwarfLogRaw("dwarf.log");
static OffsetOstream dwarfLog(dwarfLogRaw);

//#define _VERBOSE
#ifdef _VERBOSE
ostream& trace(dwarfLog);
#else
ostream trace(0);
#endif
#define DWARF_CHECK(call) \
    do \
    { \
        int checkedRet = call; \
        trace << "DWARF: " #call " = " << checkedRet << endl; \
        assert(checkedRet == DW_DLV_OK); \
    } \
    while (false);
    
namespace dwarf
{
    static Dwarf_Half getTag(Dwarf_Die die)
    {   
        Dwarf_Half tag;
        DWARF_CHECK(dwarf_tag(die, &tag, nullptr));
        return tag;
    }

    static string getTagName(Dwarf_Die die)
    {
        const char* tagName;
        DWARF_CHECK(dwarf_get_TAG_name(getTag(die), &tagName));
        return tagName;
    }

    static string getDieName(Dwarf_Die die)
    {
        char* dieName;
        int err = dwarf_diename(die, &dieName, nullptr);
        if (err != DW_DLV_OK)
            return NoName;
        return dieName;
    }

    static Dwarf_Attribute getAttr(Dwarf_Die die, Dwarf_Half attr)
    {
        Dwarf_Attribute ret;
        Dwarf_Bool hasAttr;
        DWARF_CHECK(dwarf_hasattr(die, attr, &hasAttr, nullptr));
        if (!hasAttr)
            return nullptr;
        DWARF_CHECK(dwarf_attr(die, attr, &ret, nullptr));
        return ret;
    }

    static bool isFuncDie(Dwarf_Die die)
    {
        return getTag(die) == DW_TAG_subprogram;
    }
    
    string to_string(LocSource src)
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
                 << " " << to_string(reg) << " " << off << endl;
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

    string LocInfo::str(Dwarf_Debug dbg, Dwarf_Locdesc** loclist, Dwarf_Signed nLoclist)
    {
        ostringstream oss;
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
            dwarfLog << "unknown loc" << endl;
            return;
        }
        dwarfLog << endl;
        for (int i = 0; i < nLoclist; i++)
        {
            if (loclist[i]->ld_cents != 1)
            {
                dwarfLog << "unknown loc: " << endl;
                dwarfLog << str(dbg, loclist, nLoclist) << endl;
                freeLoclist(dbg, loclist, nLoclist);
                return;
            }

            Dwarf_Loc loc = loclist[i]->ld_s[0];
            LocSource locsrc = getLocSource(loc.lr_atom);
            if (locsrc == LocSource::Unknown)
            {
                dwarfLog << "unknown loc: " << endl;
                dwarfLog << str(dbg, loclist, nLoclist) << endl;
                freeLoclist(dbg, loclist, nLoclist);
                return;
            }
            entries.emplace_back((void*)loclist[i]->ld_lopc, (void*)loclist[i]->ld_hipc,
                                 locsrc, loc.lr_number);
        }
        freeLoclist(dbg, loclist, nLoclist);
    }
    
    //------------------------------------------------------------------------------
    //FuncInfo
    //------------------------------------------------------------------------------

    FuncInfo::FuncInfo(int id, const string& name, const string& linkageName,
                       const LocInfo& location) :
        id(id),
        name(name),
        linkageName(linkageName),
        location(location)
    {
        dwarfLog << "FuncInfo: " << name << " " << linkageName << endl;
    }
    
    //------------------------------------------------------------------------------
    //VarInfo
    //------------------------------------------------------------------------------

    VarInfo::VarInfo(int id, StorageType type, FuncInfo* parent, const string& name, size_t size,
                     const LocInfo& location) :
        id(id),
        type(type),
        parent(parent),
        name(name),
        size(size),
        location(location)
    {
        dwarfLog << "VarInfo: " << name << " " << size << endl;
    }
    
    //------------------------------------------------------------------------------
    //DebugContext
    //------------------------------------------------------------------------------

    void DebugContext::addFunc(const FuncInfo& f)
    {
        auto ret = funcs.insert(make_pair(f.id, f));
        assert(ret.second);
    }

    FuncInfo* DebugContext::getFunc(int id)
    {
        auto it = funcs.find(id);
        assert(it != funcs.end());
        return &it->second;
    }

    void DebugContext::addVar(const VarInfo& v)
    {
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
    
    dbg::DebugContext DebugContext::toDbg() const
    {
        dbg::DebugContext ctxt;
        for (auto& fe : funcs)
        {
            auto& f = fe.second;
            string name = f.linkageName.empty() ? f.name : f.linkageName;
            if (name == NoName)
                continue;
            auto& entries = f.location.entries;
            if (entries.empty())
                continue;
            auto& entry = entries.front();
            if (entry.reg != LocSource::RSP)
                continue;
            ssize_t loc = entry.off;
            dbg::FuncInfo funcInfo(name, loc);
            auto* pFuncInfo = ctxt.addFunc(funcInfo);
            for (auto& pv : f.vars)
            {
                auto& v = *pv;
                string name = v.name;
                auto& entries = v.location.entries;
                dbg::StorageType type =
                    v.type == StorageType::Auto ?
                    dbg::StorageType::Auto :
                    dbg::StorageType::Static;
                    
                //if there is loc info with offset from frame base keep only it
                LocEntry entry;
                bool found = false;
                for (auto& e : entries)
                    if (e.reg == LocSource::FrameBase)
                    {
                        entry = e;
                        found = true;
                        break;
                    }
                    else if (e.reg == LocSource::Addr)
                    {
                        if (type != dbg::StorageType::Static)
                            continue;
                        entry = e;
                        found = true;
                        break;
                    }
                if (!found)
                    continue;
                dbg::VarInfo var(type, v.name, v.size, entry.off, pFuncInfo);
                ctxt.addVar(var);
            }
        }
        return ctxt;
    }
    
    //------------------------------------------------------------------------------
    //Parser
    //------------------------------------------------------------------------------

    Parser::Parser(const std::string& filePath)
    {
        int fd = open(filePath.c_str(), O_RDONLY);
        assert(fd != -1);
        Dwarf_Unsigned access = DW_DLC_READ;
        DWARF_CHECK(dwarf_init(fd, access, nullptr, nullptr, &dbg, nullptr));
    }

    size_t Parser::getSize(Dwarf_Die die, const WalkInfo& walkInfo)
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
        DWARF_CHECK(dwarf_formref(attr, &off, nullptr))
        
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
            DWARF_CHECK(dwarf_formref(attr, &off, nullptr))
            
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
        dwarfLog << endl << "size attr is missed tag name: " << getTagName(typeDie)
                 << "; die name: " << getDieName(typeDie) << endl;
        return 0;
    }

    void Parser::walkTree(Dwarf_Die die, WalkInfo& walkInfo)
    {
        Dwarf_Half tag = getTag(die);
        Dwarf_Die child;
        int err;
        dwarfLog << getDieName(die) << " " << getTagName(die) << endl;
            
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
                if (walkInfo.storageType == StorageType::Static)
                {
                    Dwarf_Off id;
                    DWARF_CHECK(dwarf_dieoffset(die, &id, nullptr));
                    dctx.addVar(VarInfo(id, StorageType::Static, nullptr, getDieName(die),
                                        size, LocInfo(dbg, die)));
                }
                else
                {
                    Dwarf_Off id;
                    assert(walkInfo.lastFuncDie);                
                    DWARF_CHECK(dwarf_dieoffset(walkInfo.lastFuncDie, &id, nullptr));
                    auto* func = dctx.getFunc(id);
                    DWARF_CHECK(dwarf_dieoffset(die, &id, nullptr));
                    dctx.addVar(VarInfo(id, StorageType::Auto, func, getDieName(die),
                                        size, LocInfo(dbg, die)));
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
            string linkageName;
            if (attr)
            {
                Dwarf_Half retForm;
                dwarf_whatform(attr, &retForm, nullptr);
                assert(retForm == DW_FORM_strp);
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

    void Parser::walk(Dwarf_Die cuDie, Dwarf_Unsigned off)
    {
        WalkInfo walkInfo;
        walkInfo.cuOffset = off;
        Dwarf_Attribute attr = getAttr(cuDie, DW_AT_language);
        Dwarf_Half retForm;
        dwarf_whatform(attr, &retForm, nullptr);
        assert(retForm == DW_FORM_data1);
        DWARF_CHECK(dwarf_formsdata(attr, &walkInfo.lang, nullptr));

        walkTree(cuDie, walkInfo);
    }

    void Parser::cuWalk(function<void(Dwarf_Die, Dwarf_Unsigned)> cuHandler)
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

    const DebugContext& Parser::getDebugContext() const
    {
        return dctx;
    }

    Parser::~Parser()
    {
        dwarf_elf_handle elfptr;
        DWARF_CHECK(dwarf_get_elf(dbg, &elfptr, nullptr));
        DWARF_CHECK(dwarf_finish(dbg, nullptr));
        //elf_end(elfptr);
    }
}