﻿/*
'C'-style script compiler development file. (c) 2000,2001 Chris Jones
SCOM is a script compiler for the 'C' language.

BIRD'S EYE OVERVIEW - IMPLEMENTATION

General:
Functions have names of the form AaaAaa or AaaAaa_BbbBbb
where the component parts are camelcased. This means that function AaaAaa_BbbBbb is a
subfunction of function AaaAaa that is exclusively called by function AaaAaa.

The Parser does does NOT get the sequence of tokens in a pipe from the scanning step, i.e.,
it does NOT read the symbols one-by-one. To the contrary, the logic reads back and forth in
the token sequence.

(Nearly) All parser functions return an error code that is negative iff an error has been
encountered. In case of an error, they call Error() and return with a negative integer.

The Parser runs in two phases.
The first phase runs quickly through the tokenized source and collects the headers
of the local functions.

The second phase has the following main components:
    Declaration parsing
    Command parsing
        Functions that process the keyword Kkk are called ParseKkk()

    Code nesting and compound statements
        In ParseWhile() etc., DealWithEndOf..(), and class AGS::Parser::NestingStack.

    Expression parsing
        In ParseExpression()
        Note that "++" and "--" are treated as assignment symbols, not as operators.

    Memory access
        In AccessData()
        In order to read data or write to data, the respective piece of data must
        be located first. This also encompasses literals of the program code.
        Note that "." and "[]" are not treated as normal operators (operators like +).
        The memory offset of struct components in relation to the
        location of the respective struct is calculated at compile time, whereas array
        offsets are calculated at run time.

Notes on how nested statements are handled:
    When handling nested constructs, the parser sometimes generates and emits some code,
    then rips it out of the codebase and stores it internally, then later on, retrieves
    it and emits it into the codebase again.

Oldstyle strings, string literals, string buffers:
    If a "string" is declared, 200 bytes of memory are reserved on the stack (local) or in
    global memory (global). This is called a "string buffer". Whenever oldstyle strings or
    literal strings are used, they are referred to by the address of their first byte.
    The only way of modifying a string buffer is by functions. However, string buffer assignments
    are handled with inline code. The compiler doesn't attempt in any way to prevent buffer underruns or overruns.


MEMORY LAYOUT

Global variables go into their own dedicated memory block and are addressed relatively to the beginning of that block.
	This block is initialized with constant values at the start of the game. So it is possible to set the start value of
    globals to some constant value, but it is not possible to _calculate_ it at the start of the runtime.
    In particular, initial pointer values and initial String values can only be given as null because any
    other value would entail a runtime computation.

Literal strings go into their own, dedicated memory block and are also addressed relatively to the beginning of that block.
	The scanner populates this memory block; for the parser, the whole block is treated as constant and read-only.

Imported values are treated as if they were global values. However, their exact location is only computed at runtime by the
    linker. For the purposes of the parser, imported values are assigned an ordinal number #0, #1, #2 etc. and are referenced
    by their ordinal number.

Local variables go into a memory block, the "local memory block", that is reserved on the stack.
 	They are addressed relatively to the start of that block. The start of this block can always be determined at
 	compile time by subtracting a specific offset from the stack pointer, namely offset_to_local_var_block.
 	This offset changes in the course of the compilation but can always be determined at compile time.
 	The space for local variables is reserved on the stack at runtime when the respective function is called.
    Therefore, local variables can be initialized to any value that can be computed,
    they aren't restricted to compile time constants as the global variables are.

A local variable is declared within a nesting of { ... } in the program code;
    It becomes valid at the point of declaration and it becomes invalid when the closing '}' to the
    innermost open '{' is encountered. In the course of reading the program from beginning to end,
    the open '{' that have not yet been closed form a stack called the "nesting stack".
    Whenever a '{' is encountered, the nesting stack gets an additional level; whenever a '}' is encountered,
    the topmost level is popped from the stack.
        Side Note: Compound statements can have a body that is NOT surrounded with braces, e.g.,
        "if (foo) i++;" instead of "if (foo) { i++; }". In this case the nesting stack is
        still extended by one level before the compound statement body is processed and
        reduced by one level afterwards.
  
    The depth of the stack plus 1 is called the nested depth or scope. Each local variable is assigned
 	the nested depth of its point of declaration.

    When program flow passes a closing '}' then all the variables with higher nested depth can be freed.
    This shortens the local memory block from the end; its start remains unchanged.
    "continue", "break" and "return" statements can break out of several '}' at once. In this case,
    all their respective variables must be freed.

    Class NestingStack keeps information on the nested level of code.
    For each nested level, the class keeps, amongst others, the location in the bytecode
    of the start of the construct and the location of a Bytecode jump to its end.

Parameters of a function are local variables; they are assigned the nested depth 1.
    Only parameters can have the nested depth 1.
    The first parameter of a function is also the first parameter in the local variable block. To make this happen,
    the parameters must be pushed back-to-front onto the stack when the function is called,
    i.e. the last function parameter must be pushed first.

    Global, imported variables, literal constants and strings are valid from the point of declaration onwards
    until the end of the compilation unit; their assigned nested depth is 0.

Whilst a function is running, its local variable block is as follows:
((lower memory addresses))
		parameter1					<- SP - offset_to_local_var_block
		parameter2
		parameter3
		...
		(return address of the current function)
		variable21 with scope 2
		variable22 with scope 2
		variable23 with scope 2
		...
		variable31 with scope 3
		variable32 with scope 3
		...
		variable41 with scope 4
		...
		(pushed value1)
		(pushed value2)
		...
		(pushed valueN)				<- SP
((higher memory addresses))

Classic arrays and Dynarrays, pointers and managed structs:
    Memory that is allocated with "new" is allocated dynamically by the Engine. The compiler need not be concerned how it is done.
	The compiler also needn't concern itself with freeing the dynamic memory itself; this is the Engine's job, too.
    However, the compiler must declare that a memory cell shall hold a pointer to dynamic memory, by using the opcode MEMWRITEPTR.
    And when a memory cell is no longer reserved for pointers, this must be declared as well, using the opcode MEMZEROPTR.
		Side note: Before a function is called, all its parameters are "pushed" to the stack using the PUSHREG opcode.
		So when some parameters are pointers then the fact that the respective memory cells contain a pointer isn't declared yet.
		So first thing at the start of the function, all pointer parameters must be read with normal non-...PTR opcodes
        and then written into the same place as before using the special opcode MEMWRITEPTR.
		Side note: When a '}' is reached and local pointer variables are concerned, it isn't enough to just shorten the
		local memory block. On all such pointer variables, MEMZEROPTR must be applied first to declare that the respective memory
	    cells won't necessarily contain pointers any more.

    Any address that should hold a pointer must be manipulated using the SCMD_...PTR form of the opcodes

    There are only two kinds of dynamic memory blocks:
        memory blocks that do not contain any pointers to dynamic memory whatsoever
        memory blocks that completely consist of pointers to dynamic memory ONLY.
    (This is an engine restriction pertaining to memory serialization, not a compiler restriction.)

    A Dynarray of primitives (e.g., int[]) is represented in memory as a pointer to a memory
    block that comprises all the elements, one after the other.
    [*]->[][]...[]
    A Dynarray of structs must be a dynarray of managed structs. It is represented in
    memory as a pointer to a block of pointers, each of which points to one element.
    [*]->[*][*]...[*]
          |  |     |
          V  V ... V
         [] [] ... []
    In contrast to a dynamic array, a classic array is never managed.
    A classic array of primitives (e.g., int[12]) or of non-managed structs is represented
    in memory as a block of those elements.
    [][]...[]
    A classic array of managed structs is a classic array of pointers,
    each of which points to a memory block that contains one element.
    [*][*]...[*]
     |  |     |
     V  V ... V
    [] [] ... []

Pointers are exclusively used for managed memory. If managed structs are manipulated,
    pointers MUST ALWAYS be used; for un-managed structs, pointers MAY NEVER be used.
    However as an exception, in import statements non-pointed managed structs can be used, too.
    That means that the compiler can deduce whether a pointer is expected by
    looking at the keyword "managed" of the struct alone -- except in global import declarations.
    Blocks of primitive vartypes can be allocated as managed memory, in which case pointers
    MUST be used. Again, the compiler can deduce from the declaration that a pointer MUST be
    used in this case.
*/


#include <string>
#include <limits>
#include <fstream>
#include <cmath>

#include "util/string.h"

#include "script/cc_options.h"
#include "script/script_common.h"
#include "script/cc_error.h"

#include "cc_internallist.h"
#include "cc_symboltable.h"

#include "cs_parser_common.h"
#include "cs_scanner.h"
#include "cs_parser.h"


// Declared in Common/script/script_common.h 
// Defined in Common/script/script_common.cpp
extern int currentline;

char ccCopyright[] = "ScriptCompiler32 v" SCOM_VERSIONSTR " (c) 2000-2007 Chris Jones and 2011-2020 others";

// Receives the section name in case of errors
static char SectionNameBuffer[256];

std::map<TypeQualifier, std::string> AGS::Parser::_tq2String;

bool AGS::Parser::IsIdentifier(AGS::Symbol symb)
{
    if (symb <= kKW_LastPredefined || symb > static_cast<decltype(symb)>(_sym.entries.size()))
        return false;
    std::string name = _sym.GetName(symb);
    if (name.size() == 0)
        return false;
    if ('0' <= name[0] && name[0] <= '9')
        return false;
    for (size_t idx = 0; idx < name.size(); ++idx)
    {
        char const &ch = name[idx];
        // don't use "is.." functions, these are locale dependent
        if ('0' <= ch && ch <= '9') continue;
        if ('A' <= ch && ch <= 'Z') continue;
        if ('a' <= ch && ch <= 'z') continue;
        if ('_' == ch) continue;
        return false;
    }
    return true;
}

std::string const AGS::Parser::TypeQualifierSet2String(TypeQualifierSet tqs) const
{
    std::string ret;

    for (auto tq_it = _tq2String.begin(); _tq2String.end() != tq_it; tq_it++)
        if (tqs[tq_it->first])
            ret += tq_it->second + " ";
    if (ret.length() > 0)
        ret.pop_back();
    return ret;
}

ErrorType AGS::Parser::String2Int(std::string const &str, int &val)
{
    const bool is_neg = (0 == str.length() || '-' == str.at(0));
    errno = 0;
    char *endptr = 0;
    const long longValue = strtol(str.c_str(), &endptr, 10);
    if ((longValue == LONG_MIN && errno == ERANGE) ||
        (is_neg && (endptr[0] != '\0')) ||
        (longValue < INT_MIN))
    {
        Error("Literal value '%s' is too low (min. is '%d')", str.c_str(), INT_MIN);
        return kERR_UserError;
    }

    if ((longValue == LONG_MAX && errno == ERANGE) ||
        ((!is_neg) && (endptr[0] != '\0')) ||
        (longValue > INT_MAX))
    {
        Error("Literal value %s is too high (max. is %d)", str.c_str(), INT_MAX);
        return kERR_UserError;
    }

    val = static_cast<int>(longValue);
    return kERR_None;
}

ErrorType AGS::Parser::String2Float(std::string const &float_as_string, float &f)
{
    char *endptr;
    char const *instring = float_as_string.c_str();
    double const d = strtod(instring, &endptr);
    if (endptr != instring + float_as_string.length())
    {   // The scanner ought to prevent that
        Error("!Illegal floating point literal '%s'", instring);
        return kERR_InternalError;
    }
    if (HUGE_VAL == d)
    {
        Error("Floating point literal '%s' is out of range", instring);
        return kERR_UserError;
    }
    f = static_cast<float>(d);
    return kERR_None;
}



AGS::Symbol AGS::Parser::MangleStructAndComponent(AGS::Symbol stname, AGS::Symbol component)
{
    std::string fullname_str = _sym.GetName(stname) + "::" + _sym.GetName(component);
    return _sym.FindOrAdd(fullname_str);
}

ErrorType AGS::Parser::SkipTo(SymbolList const &stoplist, SrcList &source)
{
    int delimeter_nesting_depth = 0;
    for (; !source.ReachedEOF(); source.GetNext())
    {
        // Note that the scanner/tokenizer has already verified
        // that all opening symbols get closed and 
        // that we don't have (...] or similar in the input
        Symbol const next_sym = _src.PeekNext();
        switch (next_sym)
        {
        case kKW_OpenBrace:
        case kKW_OpenBracket:
        case kKW_OpenParenthesis:
        {
            ++delimeter_nesting_depth;
            continue;
        }
        case kKW_CloseBrace:
        case kKW_CloseBracket:
        case kKW_CloseParenthesis:
        {
            if (--delimeter_nesting_depth < 0)
                return kERR_None;
            continue;
        }
        }
        if (0 < delimeter_nesting_depth)
            continue;

        for (auto it = stoplist.begin(); it != stoplist.end(); ++it)
            if (next_sym == *it)
                return kERR_None;
    }
    return kERR_UserError;
}

ErrorType AGS::Parser::SkipToClose(Predefined closer)
{
    SkipTo(SymbolList{}, _src);
    if (closer == _src.GetNext())
        return kERR_None;
    
    Error("!Unexpected closing symbol");
    return kERR_InternalError;
}

ErrorType AGS::Parser::Expect(Symbol expected, Symbol actual, std::string const &custom_msg)
{
    if (actual == expected)
        return kERR_None;

    if ("" != custom_msg)
        Error(
            (custom_msg + ", found %s instead").c_str(),
            _sym.GetName(actual).c_str());
    else
        Error(
            "Expected '%s', found '%s' instead",
            _sym.GetName(expected).c_str(),
            _sym.GetName(actual).c_str());
    return kERR_UserError;
}

ErrorType AGS::Parser::Expect(std::vector<Symbol> const &expected, Symbol actual)
{
    for (size_t expected_idx = 0; expected_idx < expected.size(); expected_idx++)
        if (actual == expected[expected_idx])
            return kERR_None;
    std::string errmsg = "Expected ";
    for (size_t expected_idx = 0; expected_idx < expected.size(); expected_idx++)
    {
        errmsg += "'" + _sym.GetName(expected[expected_idx]) + "'";
        if (expected_idx + 2 < expected.size())
            errmsg += ", ";
        else if (expected_idx + 2 == expected.size())
            errmsg += " or";
    }
    errmsg += ", found '%s' instead";
    Error(errmsg.c_str(), _sym.GetName(actual).c_str());
    return kERR_UserError;
}
            

AGS::Parser::NestingStack::NestingInfo::NestingInfo(NSType stype, ::ccCompiledScript &scrip)
    : Type(stype)
    , OldDefinitions({})
    , Start(BackwardJumpDest{ scrip })
    , JumpOut(ForwardJump{ scrip })
    , SwitchExprVartype(0)
    , SwitchDefault({ scrip })
    , SwitchJumptable({ scrip })
    , Chunks({})
{
}

// For assigning unique IDs to chunks
int AGS::Parser::NestingStack::_chunkIdCtr = 0;

AGS::Parser::NestingStack::NestingStack(::ccCompiledScript &scrip)
    :_scrip(scrip)
{
    // Push first record on stack so that it isn't empty
    Push(NSType::kNone);
}

bool AGS::Parser::NestingStack::AddOldDefinition(Symbol s, SymbolTableEntry const &entry)
{
    if (_stack.back().OldDefinitions.count(s) > 0)
        return true;

    _stack.back().OldDefinitions.emplace(s, entry);
    return false;
}

// Rip the code that has already been generated, starting from codeoffset, out of scrip
// and move it into the vector at list, instead.
void AGS::Parser::NestingStack::YankChunk(size_t src_line, CodeLoc code_start, size_t fixups_start, int &id)
{
    Chunk item;
    item.SrcLine = src_line;

    size_t const codesize = std::max<int>(0, _scrip.codesize);
    for (size_t code_idx = code_start; code_idx < codesize; code_idx++)
        item.Code.push_back(_scrip.code[code_idx]);

    size_t numfixups = std::max<int>(0, _scrip.numfixups);
    for (size_t fixups_idx = fixups_start; fixups_idx < numfixups; fixups_idx++)
    {
        CodeLoc const code_idx = _scrip.fixups[fixups_idx];
        item.Fixups.push_back(code_idx - code_start);
        item.FixupTypes.push_back(_scrip.fixuptypes[fixups_idx]);
    }
    item.Id = id = ++_chunkIdCtr;

    _stack.back().Chunks.push_back(item);

    // Cut out the code that has been pushed
    _scrip.codesize = code_start;
    _scrip.numfixups = static_cast<decltype(_scrip.numfixups)>(fixups_start);
}

// Copy the code in the chunk to the end of the bytecode vector 
void AGS::Parser::NestingStack::WriteChunk(size_t level, size_t chunk_idx, int &id)
{
    Chunk const item = Chunks(level).at(chunk_idx);
    id = item.Id;

    // Add a line number opcode so that runtime errors
    // can show the correct originating source line.
    if (0u < item.Code.size() && SCMD_LINENUM != item.Code[0u] && 0u < item.SrcLine)
        _scrip.write_lineno(item.SrcLine);

    // The fixups are stored relative to the start of the insertion,
    // so remember what that is
    size_t const start_of_insert = _scrip.codesize;
    size_t const code_size = item.Code.size();
    for (size_t code_idx = 0u; code_idx < code_size; code_idx++)
        _scrip.write_code(item.Code[code_idx]);

    size_t const fixups_size = item.Fixups.size();
    for (size_t fixups_idx = 0u; fixups_idx < fixups_size; fixups_idx++)
        _scrip.add_fixup(
            item.Fixups[fixups_idx] + start_of_insert,
            item.FixupTypes[fixups_idx]);

    // Make the last emitted source line number invalid so that the next command will
    // generate a line number opcode first
    _scrip.last_emitted_lineno = INT_MAX;
}

AGS::Parser::FuncCallpointMgr::FuncCallpointMgr(Parser &parser)
    : _parser(parser)
{ }

void AGS::Parser::FuncCallpointMgr::Reset()
{
    _funcCallpointMap.clear();
}

ErrorType AGS::Parser::FuncCallpointMgr::TrackForwardDeclFuncCall(Symbol func, CodeLoc loc, size_t in_source)
{
    // Patch callpoint in when known
    CodeCell const callpoint = _funcCallpointMap[func].Callpoint;
    if (callpoint >= 0)
    {
        _parser._scrip.code[loc] = callpoint;
        return kERR_None;
    }

    // Callpoint not known, so remember this location
    PatchInfo pinfo;
    pinfo.ChunkId = kCodeBaseId;
    pinfo.Offset = loc;
    pinfo.InSource = in_source;
    _funcCallpointMap[func].List.push_back(pinfo);

    return kERR_None;
}

ErrorType AGS::Parser::FuncCallpointMgr::UpdateCallListOnYanking(AGS::CodeLoc chunk_start, size_t chunk_len, int id)
{
    size_t const chunk_end = chunk_start + chunk_len;

    for (CallMap::iterator func_it = _funcCallpointMap.begin();
        func_it != _funcCallpointMap.end();
        ++func_it)
    {
        PatchList &pl = func_it->second.List;
        size_t const pl_size = pl.size();
        for (size_t pl_idx = 0; pl_idx < pl_size; ++pl_idx)
        {
            PatchInfo &patch_info = pl[pl_idx];
            if (kCodeBaseId != patch_info.ChunkId)
                continue;
            if (patch_info.Offset < chunk_start || patch_info.Offset >= static_cast<decltype(patch_info.Offset)>(chunk_end))
                continue; // This address isn't yanked

            patch_info.ChunkId = id;
            patch_info.Offset -= chunk_start;
        }
    }

    return kERR_None;
}

ErrorType AGS::Parser::FuncCallpointMgr::UpdateCallListOnWriting(AGS::CodeLoc start, int id)
{
    for (CallMap::iterator func_it = _funcCallpointMap.begin();
        func_it != _funcCallpointMap.end();
        ++func_it)
    {
        PatchList &pl = func_it->second.List;
        size_t const size = pl.size();
        for (size_t pl_idx = 0; pl_idx < size; ++pl_idx)
        {
            PatchInfo &patch_info = pl[pl_idx];
            if (patch_info.ChunkId != id)
                continue; // Not our concern this time

            // We cannot repurpose patch_info since it may be written multiple times.
            PatchInfo cb_patch_info;
            cb_patch_info.ChunkId = kCodeBaseId;
            cb_patch_info.Offset = patch_info.Offset + start;
            pl.push_back(cb_patch_info);
        }
    }

    return kERR_None;
}

ErrorType AGS::Parser::FuncCallpointMgr::SetFuncCallpoint(AGS::Symbol func, AGS::CodeLoc dest)
{
    _funcCallpointMap[func].Callpoint = dest;
    PatchList &pl = _funcCallpointMap[func].List;
    size_t const pl_size = pl.size();
    bool yanked_patches_exist = false;
    for (size_t pl_idx = 0; pl_idx < pl_size; ++pl_idx)
        if (kCodeBaseId == pl[pl_idx].ChunkId)
        {
            _parser._scrip.code[pl[pl_idx].Offset] = dest;
            pl[pl_idx].ChunkId = kPatchedId;
        }
        else if (kPatchedId != pl[pl_idx].ChunkId)
        {
            yanked_patches_exist = true;
        }
    if (!yanked_patches_exist)
        pl.clear();
    return kERR_None;
}

ErrorType AGS::Parser::FuncCallpointMgr::CheckForUnresolvedFuncs()
{
    for (auto fcm_it = _funcCallpointMap.begin(); fcm_it != _funcCallpointMap.end(); ++fcm_it)
    {
        PatchList &pl = fcm_it->second.List;
        size_t const pl_size = pl.size();
        for (size_t pl_idx = 0; pl_idx < pl_size; ++pl_idx)
        {
            if (kCodeBaseId != pl[pl_idx].ChunkId)
                continue;
            _parser._src.SetCursor(pl[pl_idx].InSource);
            _parser.Error(
                _parser.ReferenceMsgSym("The called function '%s()' isn't defined with body nor imported", fcm_it->first).c_str(),
                _parser._sym.GetName(fcm_it->first).c_str());
            return kERR_InternalError;
        }
    }
    return kERR_None;
}

AGS::Parser::FuncCallpointMgr::CallpointInfo::CallpointInfo()
    : Callpoint(-1)
{ }

AGS::Parser::RestorePoint::RestorePoint(::ccCompiledScript &scrip)
    : _scrip(scrip)
{
    _restoreLoc = _scrip.codesize;
    _lastEmittedSrcLineno = _scrip.last_emitted_lineno;
}

void AGS::Parser::RestorePoint::Restore()
{
    _scrip.codesize = _restoreLoc;
    _scrip.last_emitted_lineno = _lastEmittedSrcLineno;
}

AGS::Parser::BackwardJumpDest::BackwardJumpDest(::ccCompiledScript &scrip)
    : _scrip(scrip)
    , _dest(-1)
    , _lastEmittedSrcLineno(INT_MAX)
{ }

void AGS::Parser::BackwardJumpDest::Set(CodeLoc cl)
{
    _dest = (cl >= 0) ? cl : _scrip.codesize;
    _lastEmittedSrcLineno = _scrip.last_emitted_lineno;
}

void AGS::Parser::BackwardJumpDest::WriteJump(CodeCell jump_op, size_t cur_line)
{
    if (SCMD_LINENUM != _scrip.code[_dest] &&
        _scrip.last_emitted_lineno != _lastEmittedSrcLineno)
    {
        _scrip.write_lineno(cur_line);
    }
    _scrip.write_cmd(jump_op, _scrip.RelativeJumpDist(_scrip.codesize + 1, _dest));
}

AGS::Parser::ForwardJump::ForwardJump(::ccCompiledScript &scrip)
    : _scrip(scrip)
    , _lastEmittedSrcLineno(INT_MAX)
{ }

void AGS::Parser::ForwardJump::AddParam(int offset)
{
    // If the current value for the last emitted lineno doesn't match the
    // saved value then the saved value won't work for all jumps so it
    // must be set to invalid.
    if (_jumpDestParamLocs.empty())
        _lastEmittedSrcLineno = _scrip.last_emitted_lineno;
    else if (_lastEmittedSrcLineno != _scrip.last_emitted_lineno)
        _lastEmittedSrcLineno = INT_MAX;
    _jumpDestParamLocs.push_back(_scrip.codesize + offset);
}

void AGS::Parser::ForwardJump::Patch(size_t cur_line)
{
    if (!_jumpDestParamLocs.empty())
    {
        // There are two ways of reaching the bytecode that will be emitted next:
        // through the jump or from the previous bytecode command. If the source line
        // of both isn't identical then a line opcode must be emitted next.
        if (cur_line != _scrip.last_emitted_lineno || cur_line != _lastEmittedSrcLineno)
            _scrip.last_emitted_lineno = INT_MAX;
    }
    for (auto loc = _jumpDestParamLocs.cbegin(); loc != _jumpDestParamLocs.cend(); loc++)
        _scrip.code[*loc] = _scrip.RelativeJumpDist(*loc, _scrip.codesize);
    _jumpDestParamLocs.clear();
}

AGS::Parser::ImportMgr::ImportMgr()
    : _scrip(nullptr)
{ }

void AGS::Parser::ImportMgr::Init(ccCompiledScript *scrip)
{
    _importIdx.clear();
    _scrip = scrip;
    for (int import_idx = 0; import_idx < scrip->numimports; import_idx++)
        _importIdx[scrip->imports[import_idx]] = import_idx;
}

bool AGS::Parser::ImportMgr::IsDeclaredImport(std::string s)
{
    return (_importIdx.end() != _importIdx.find(s));
}

int AGS::Parser::ImportMgr::FindOrAdd(std::string s)
{
    auto it = _importIdx.find(s);
    if (_importIdx.end() != it)
        return it->second;
    // Cache miss
    int idx = _scrip->add_new_import(s.c_str());
    _importIdx[s] = idx;
    return idx;
}

AGS::Parser::MemoryLocation::MemoryLocation(Parser &parser)
    : _parser(parser)
    , _ScType (ScT::kNone)
    , _startOffs(0u)
    , _componentOffs (0u)
{
}

ErrorType AGS::Parser::MemoryLocation::SetStart(ScopeType type, size_t offset)
{
    if (ScT::kNone != _ScType)
    {
        _parser.Error("!Memory location object doubly initialized ");
        return kERR_InternalError;
    }
    _ScType = type;
    _startOffs = offset;
    _componentOffs = 0;
    return kERR_None;
}

ErrorType AGS::Parser::MemoryLocation::MakeMARCurrent(size_t lineno, ccCompiledScript &scrip)
{
    switch (_ScType)
    {
    default:
        // The start offset is already reached (e.g., when a Dynpointer chain is dereferenced) 
        // but the component offset may need to be processed.
        if (_componentOffs > 0)
            scrip.write_cmd(SCMD_ADD, SREG_MAR, _componentOffs);
        break;

    case ScT::kGlobal:
        scrip.refresh_lineno(lineno);
        scrip.write_cmd(SCMD_LITTOREG, SREG_MAR, _startOffs + _componentOffs);
        scrip.fixup_previous(Parser::kFx_GlobalData);
        break;

    case ScT::kImport:
        // Have to convert the import number into a code offset first.
        // Can only then add the offset to it.
        scrip.refresh_lineno(lineno);
        scrip.write_cmd(SCMD_LITTOREG, SREG_MAR, _startOffs);
        scrip.fixup_previous(Parser::kFx_Import);
        if (_componentOffs != 0)
            scrip.write_cmd(SCMD_ADD, SREG_MAR, _componentOffs);
        break;

    case ScT::kLocal:
        scrip.refresh_lineno(lineno);
        CodeCell const offset = scrip.offset_to_local_var_block - _startOffs - _componentOffs;
        if (offset < 0)
        {   // Must be a bug: That memory is unused.
            _parser.Error("!Trying to emit the negative offset %d to the top-of-stack", (int) offset);
            return kERR_InternalError;
        }

        scrip.write_cmd(SCMD_LOADSPOFFS, offset);
        break;
    }
    Reset();
    return kERR_None;
}

void AGS::Parser::MemoryLocation::Reset()
{
    _ScType = ScT::kNone;
    _startOffs = 0u;
    _componentOffs = 0u;
}

AGS::Parser::Parser(SrcList &src, ::ccCompiledScript &scrip, ::SymbolTable &symt, MessageHandler &mh)
    : _nest(scrip)
    , _pp(PP::kPreAnalyze)
    , _sym(symt)
    , _src(src)
    , _scrip(scrip)
    , _msg_handler(mh)
    , _fcm(*this)
    , _fim(*this)
{
    _importMgr.Init(&scrip);
    _givm.clear();
    _lastEmittedSectionId = 0;
    _lastEmittedLineno = 0;

    if (_tq2String.empty())
        _tq2String =
            {
                { TQ::kAttribute, "attribute", },
                { TQ::kAutoptr, "autoptr", },
                { TQ::kBuiltin, "builtin", },
                { TQ::kConst, "const", },
                { TQ::kImport, "import", },
                { TQ::kManaged, "managed", },
                { TQ::kProtected, "protected", },
                { TQ::kReadonly, "readonly", },
                { TQ::kStatic, "static", },
                { TQ::kStringstruct, "stringstruct", },
                { TQ::kWriteprotected, "writeprotected", },
            };
}

void AGS::Parser::SetDynpointerInManagedVartype(Vartype &vartype)
{
    if (_sym.IsManagedVartype(vartype))
        vartype = _sym.VartypeWith(VTT::kDynpointer, vartype);
}

size_t AGS::Parser::StacksizeOfLocals(size_t from_level)
{
    size_t total_size = 0;
    for (size_t level = from_level; level <= _nest.TopLevel(); level++)
    {
        std::map<Symbol, SymbolTableEntry> const &od = _nest.GetOldDefinitions(level);
        for (auto symbols_it = od.cbegin(); symbols_it != od.end(); symbols_it++)
        {
            Symbol const s = symbols_it->first;
            if (SymT::kLocalVar == _sym.GetSymbolType(s))
                total_size += _sym.GetSize(s);
        }
    }
    return total_size;
}

// Does vartype v contain releasable pointers?
// Also determines whether vartype contains standard (non-dynamic) arrays.
bool AGS::Parser::ContainsReleasableDynpointers(AGS::Vartype vartype)
{
    if (_sym.IsDynVartype(vartype))
        return true;
    if (_sym.IsArrayVartype(vartype))
        return ContainsReleasableDynpointers(_sym.GetVartype(vartype));
    if (!_sym.IsStructVartype(vartype))
        return false; // Atomic non-structs can't have pointers

    std::vector<Symbol> compo_list;
    _sym.GetComponentsOfStruct(vartype, compo_list);
    for (size_t cl_idx = 0; cl_idx < compo_list.size(); cl_idx++)
        if (ContainsReleasableDynpointers(_sym.GetVartype(compo_list[cl_idx])))
            return true;

    return false;
}

// We're at the end of a block and releasing a standard array of pointers.
// MAR points to the array start. Release each array element (pointer).
ErrorType AGS::Parser::FreeDynpointersOfStdArrayOfDynpointer(size_t num_of_elements, bool &clobbers_ax)
{
    if (num_of_elements == 0)
        return kERR_None;

    if (num_of_elements < 4)
    {
        WriteCmd(SCMD_MEMZEROPTR);
        for (size_t loop = 1; loop < num_of_elements; ++loop)
        {
            WriteCmd(SCMD_ADD, SREG_MAR, SIZE_OF_DYNPOINTER);
            WriteCmd(SCMD_MEMZEROPTR);
        }
        return kERR_None;
    }

    clobbers_ax = true;
    WriteCmd(SCMD_LITTOREG, SREG_AX, num_of_elements);

    BackwardJumpDest loop_start(_scrip);
    loop_start.Set();
    WriteCmd(SCMD_MEMZEROPTR);
    WriteCmd(SCMD_ADD, SREG_MAR, SIZE_OF_DYNPOINTER);
    WriteCmd(SCMD_SUB, SREG_AX, 1);
    loop_start.WriteJump(SCMD_JNZ, _src.GetLineno());
    return kERR_None;
}

// We're at the end of a block and releasing all the pointers in a struct.
// MAR already points to the start of the struct.
void AGS::Parser::FreeDynpointersOfStruct(Vartype struct_vtype, bool &clobbers_ax)
{
    std::vector<Symbol> compo_list;
    _sym.GetComponentsOfStruct(struct_vtype, compo_list);
    for (int cl_idx = 0; cl_idx < static_cast<int>(compo_list.size()); cl_idx++) // note "int"!
    {
        if (ContainsReleasableDynpointers(_sym.GetVartype(compo_list[cl_idx])))
            continue;
        // Get rid of this component
        compo_list[cl_idx] = compo_list.back();
        compo_list.pop_back();
        cl_idx--; // this might make the var negative so it needs to be int
    }

    size_t offset_so_far = 0;
    for (auto compo_it = compo_list.cbegin(); compo_it != compo_list.cend(); ++compo_it)
    {
        SymbolTableEntry &entry = _sym[*compo_it];

        // Let MAR point to the component
        size_t const diff = entry.SOffset - offset_so_far;
        if (diff > 0)
            WriteCmd(SCMD_ADD, SREG_MAR, diff);
        offset_so_far = entry.SOffset;

        if (_sym.IsDynVartype(entry.Vartype))
        {
            WriteCmd(SCMD_MEMZEROPTR);
            continue;
        }

        if (compo_list.back() != *compo_it)
            PushReg(SREG_MAR);
        if (entry.IsArrayVartype(_sym))
            FreeDynpointersOfStdArray(*compo_it, clobbers_ax);
        else if (entry.IsStructVartype(_sym))
            FreeDynpointersOfStruct(entry.Vartype, clobbers_ax);
        if (compo_list.back() != *compo_it)
            PopReg(SREG_MAR);
    }
}

// We're at the end of a block and we're releasing a standard array of struct.
// MAR points to the start of the array. Release all the pointers in the array.
void AGS::Parser::FreeDynpointersOfStdArrayOfStruct(AGS::Symbol struct_vtype, SymbolTableEntry &entry, bool &clobbers_ax)
{
    clobbers_ax = true;

    // AX will be the index of the current element
    WriteCmd(SCMD_LITTOREG, SREG_AX, entry.NumArrayElements(_sym));

    BackwardJumpDest loop_start(_scrip);
    loop_start.Set();
    PushReg(SREG_MAR);
    PushReg(SREG_AX); // FreeDynpointersOfStruct might call funcs that clobber AX
    FreeDynpointersOfStruct(struct_vtype, clobbers_ax);
    PopReg(SREG_AX);
    PopReg(SREG_MAR);
    WriteCmd(SCMD_ADD, SREG_MAR, _sym.GetSize(struct_vtype));
    WriteCmd(SCMD_SUB, SREG_AX, 1);
    loop_start.WriteJump(SCMD_JNZ, _src.GetLineno());
    return;
}

// We're at the end of a block and releasing a standard array. MAR points to the start.
// Release the pointers that the array contains.
void AGS::Parser::FreeDynpointersOfStdArray(Symbol the_array, bool &clobbers_ax)
{
    int const num_of_elements = _sym.NumArrayElements(the_array);
    if (num_of_elements < 1)
        return;
    Vartype const element_vartype =
        _sym.GetVartype(_sym.GetVartype(the_array));
    if (_sym.IsDynpointerVartype(element_vartype))
    {
        FreeDynpointersOfStdArrayOfDynpointer(num_of_elements, clobbers_ax);
        return;
    }

    if (_sym.IsStructVartype(element_vartype))
        FreeDynpointersOfStdArrayOfStruct(element_vartype, _sym[the_array], clobbers_ax);

    return;
}

// Note: Currently, the structs/arrays that are pointed to cannot contain
// pointers in their turn.
// If they do, we need a solution at runtime to chase the pointers to release;
// we can't do it at compile time. Also, the pointers might form "rings"
// (e.g., A contains a field that points to B; B contains a field that
// points to A), so we can't rely on reference counting for identifying
// _all_ the unreachable memory chunks. (If nothing else points to A or B,
// both are unreachable so _could_ be released, but they still point to each
// other and so have a reference count of 1; the reference count will never reach 0).

ErrorType AGS::Parser::FreeDynpointersOfLocals0(size_t from_level, bool &clobbers_ax, bool &clobbers_mar)
{
    for (size_t level = from_level; level <= _nest.TopLevel(); level++)
    {
        std::map<Symbol, SymbolTableEntry> const &od = _nest.GetOldDefinitions(level);
        for (auto symbols_it = od.cbegin(); symbols_it != od.end(); symbols_it++)
        {
            Symbol const s = symbols_it->first;
            Vartype const s_vartype = _sym.GetVartype(s);
            if (!ContainsReleasableDynpointers(s_vartype))
                continue;

            // Set MAR to the start of the construct that contains releasable pointers
            WriteCmd(SCMD_LOADSPOFFS, _scrip.offset_to_local_var_block - _sym[s].SOffset);
            clobbers_mar = true;
            if (_sym.IsDynVartype(s_vartype))
                WriteCmd(SCMD_MEMZEROPTR);
            else if (_sym.IsArrayVartype(s_vartype))
                FreeDynpointersOfStdArray(s, clobbers_ax);
            else if (_sym.IsStructVartype(s_vartype))
                FreeDynpointersOfStruct(s_vartype, clobbers_ax);
        }
    }
    return kERR_None;
}

// Free the pointers of any locals that have a nesting depth higher than from_level
ErrorType AGS::Parser::FreeDynpointersOfLocals(size_t from_level)
{
    bool dummy_bool;
    return FreeDynpointersOfLocals0(from_level, dummy_bool, dummy_bool);
}

ErrorType AGS::Parser::FreeDynpointersOfAllLocals_DynResult(void)
{
    // The return value AX might point to a local dynamic object. So if we
    // now free the dynamic references and we don't take precautions,
    // this dynamic memory might drop its last reference and get
    // garbage collected in consequence. AX would have a dangling pointer.
    // We only need these precautions if there are local dynamic objects.
    RestorePoint rp_before_precautions(_scrip);

    // Allocate a local dynamic pointer to hold the return value.
    PushReg(SREG_AX);
    WriteCmd(SCMD_LOADSPOFFS, SIZE_OF_DYNPOINTER);
    WriteCmd(SCMD_MEMINITPTR, SREG_AX);

    RestorePoint rp_before_freeing(_scrip);
    bool dummy_bool;
    bool mar_may_be_clobbered = false;
    ErrorType retval = FreeDynpointersOfLocals0(0u, dummy_bool, mar_may_be_clobbered);
    if (retval < 0) return retval;
    bool const no_precautions_were_necessary = rp_before_freeing.IsEmpty();

    // Now release the dynamic pointer with a special opcode that prevents 
    // memory de-allocation as long as AX still has this pointer, too
    if (mar_may_be_clobbered)
        WriteCmd(SCMD_LOADSPOFFS, SIZE_OF_DYNPOINTER);
    WriteCmd(SCMD_MEMREADPTR, SREG_AX);
    WriteCmd(SCMD_MEMZEROPTRND); // special opcode
    PopReg(SREG_BX); // do NOT pop AX here
    if (no_precautions_were_necessary)
        rp_before_precautions.Restore();
    return kERR_None;
}

// Free all local Dynpointers taking care to not clobber AX
ErrorType AGS::Parser::FreeDynpointersOfAllLocals_KeepAX(void)
{
    RestorePoint rp_before_free(_scrip);
    bool clobbers_ax = false;
    bool dummy_bool;
    ErrorType retval = FreeDynpointersOfLocals0(0u, clobbers_ax, dummy_bool);
    if (retval < 0) return retval;
    if (!clobbers_ax)
        return kERR_None;

    // We should have saved AX, so redo this
    rp_before_free.Restore();
    PushReg(SREG_AX);
    retval = FreeDynpointersOfLocals0(0u, clobbers_ax, dummy_bool);
    if (retval < 0) return retval;
    PopReg(SREG_AX);

    return kERR_None;
}

ErrorType AGS::Parser::RemoveLocalsFromSymtable(size_t from_level)
{
    SymbolTableEntry const empty = {};

    size_t const last_level = _nest.TopLevel();
    for (size_t level = from_level; level <= last_level; level++)
    {
        std::map<Symbol, SymbolTableEntry> const &od = _nest.GetOldDefinitions(level);
        for (auto symbols_it = od.begin(); symbols_it != od.end(); symbols_it++)
        {
            Symbol const s = symbols_it->first;
            if (SymT::kLocalVar != _sym.GetSymbolType(s))
                continue;

            if (SymT::kNoType != symbols_it->second.SType)
            {   // Restore the old definition that we've stashed
                _sym[s] = symbols_it->second;
                continue;
            }

            std::string const sname = _sym[s].SName;
            _sym[s] = empty;
            _sym[s].SName = sname;
        }
    }
    return kERR_None;
}

ErrorType AGS::Parser::HandleEndOfDo()
{
    ErrorType retval = Expect(
        kKW_While,
        _src.GetNext(),
        "Expected the 'while' of a 'do ... while(...)' statement");
    if (retval < 0) return retval;

    retval = ParseParenthesizedExpression();
    if (retval < 0) return retval;

    retval = Expect(kKW_Semicolon, _src.GetNext());
    if (retval < 0) return retval;

    // Jump back to the start of the loop while the condition is true
    _nest.Start().WriteJump(SCMD_JNZ, _src.GetLineno());
    // Jumps out of the loop should go here
    _nest.JumpOut().Patch(_src.GetLineno());
    _nest.Pop();

    return kERR_None;
}

ErrorType AGS::Parser::HandleEndOfElse()
{
    _nest.JumpOut().Patch(_src.GetLineno());
    _nest.Pop();
    return kERR_None;
}

ErrorType AGS::Parser::HandleEndOfSwitch()
{
    // If there was no terminating break at the last switch-case, 
    // write a jump to the jumpout point to prevent a fallthrough into the jumptable
    CodeLoc const lastcmd_loc = _scrip.codesize - 2;
    if (SCMD_JMP != _scrip.code[lastcmd_loc])
    {
        WriteCmd(SCMD_JMP, -77);
        _nest.JumpOut().AddParam();
    }

    // We begin the jump table
    _nest.SwitchJumptable().Patch(_src.GetLineno());

    // Get correct comparison operation: Don't compare strings as pointers but as strings
    CodeCell const eq_opcode =
        _sym.IsAnyStringVartype(_nest.SwitchExprVartype()) ? SCMD_STRINGSEQUAL : SCMD_ISEQUAL;

    const size_t number_of_cases = _nest.Chunks().size();
    for (size_t cases_idx = 0; cases_idx < number_of_cases; ++cases_idx)
    {
        int id;
        CodeLoc const codesize = _scrip.codesize;
        // Emit the code for the case expression of the current case. Result will be in AX
        _nest.WriteChunk(cases_idx, id);
        _fcm.UpdateCallListOnWriting(codesize, id);
        _fim.UpdateCallListOnWriting(codesize, id);
        
        WriteCmd(eq_opcode, SREG_AX, SREG_BX);
        _nest.SwitchCases().at(cases_idx).WriteJump(SCMD_JNZ, _src.GetLineno());
    }

    if (INT_MAX != _nest.SwitchDefault().Get())
        _nest.SwitchDefault().WriteJump(SCMD_JMP, _src.GetLineno());

    _nest.JumpOut().Patch(_src.GetLineno());
    _nest.Pop();
    return kERR_None;
}

ErrorType AGS::Parser::IntLiteralOrConst2Value(AGS::Symbol symb, bool is_negative, std::string const &errorMsg, int &the_value)
{
    SymbolType const stype = _sym.GetSymbolType(symb);
    if (SymT::kConstant == stype)
    {
        the_value = _sym[symb].SOffset;
        if (is_negative)
            the_value = -the_value;
        return kERR_None;
    }

    if (SymT::kLiteralInt == stype)
    {
        std::string literal = _sym.GetName(symb);
        if (is_negative)
            literal = '-' + literal;

        return String2Int(literal, the_value);
    }

    if (!errorMsg.empty())
        Error(errorMsg.c_str());
    return kERR_UserError;
}

ErrorType AGS::Parser::FloatLiteral2Value(Symbol symb, bool is_negative, std::string const &errorMsg, float &the_value)
{
    SymbolType const stype = _sym.GetSymbolType(symb);
    if (SymT::kLiteralFloat == stype)
    {
        std::string literal = _sym.GetName(symb);
        if (is_negative)
            literal = '-' + literal;

        return String2Float(literal, the_value);
    }

    if (!errorMsg.empty())
        Error(errorMsg.c_str());
    return kERR_UserError;
}

// We're parsing a parameter list and we have accepted something like "(...int i"
// We accept a default value clause like "= 15" if it follows at this point.
ErrorType AGS::Parser::ParseParamlist_Param_DefaultValue(AGS::Vartype param_type, SymbolTableEntry::ParamDefault &default_value)
{
    if (SymT::kAssign != _sym.GetSymbolType(_src.PeekNext()))
    {
        default_value.Type = SymbolTableEntry::kDT_None; // No default value given
        return kERR_None;
    }

    _src.GetNext();   // Eat '='

    Symbol default_value_symbol = _src.GetNext(); // can also be "-"
    bool default_is_negative = false;
    if (_sym.Find("-") == default_value_symbol)
    {
        default_is_negative = true;
        default_value_symbol = _src.GetNext();
    }

    if (_sym.IsDynVartype(param_type))
    {
        default_value.Type = SymbolTableEntry::kDT_Dyn;
        default_value.DynDefault = nullptr;

        if (kKW_Null == default_value_symbol)
            return kERR_None;
        if (!default_is_negative && _sym.Find("0") == default_value_symbol)
        {
            Warning("Found '0' as a default for a dynamic object (prefer 'null')");
            return kERR_None;
        }
        
        Error("Expected the parameter default 'null'");
        return kERR_UserError;
    }

    if (_sym.IsAnyIntegerVartype(param_type))
    {
        default_value.Type = SymbolTableEntry::kDT_Int;
        return IntLiteralOrConst2Value(
            default_value_symbol,
            default_is_negative,
            "Expected an integer literal or constant as parameter default",
            default_value.IntDefault);
    }

    if (kKW_Float != param_type)
    {
        Error("Parameter cannot have any default value");
        return kERR_UserError;
    }

    default_value.Type = SymbolTableEntry::kDT_Float;
    if (!default_is_negative && _sym.Find("0") == default_value_symbol)
    {
        default_value.FloatDefault = 0.0f;
        Warning("Found '0' as a default for a float value (prefer '0.0')");
        return kERR_None;
    }

    return FloatLiteral2Value(
        default_value_symbol,
        default_is_negative,
        "Expected a float literal as a parameter default",
        default_value.FloatDefault);
}

ErrorType AGS::Parser::ParseDynArrayMarkerIfPresent(AGS::Vartype &vartype)
{
    if (kKW_OpenBracket != _src.PeekNext())
        return kERR_None;
    _src.GetNext(); // Eat '['
    ErrorType retval = Expect(kKW_CloseBracket, _src.GetNext());
    if (retval < 0) return retval;

    vartype = _sym.VartypeWith(VTT::kDynarray, vartype);
    return kERR_None;
}

// Copy so that the forward decl can be compared afterwards to the real one     
void AGS::Parser::CopyKnownSymInfo(SymbolTableEntry &entry, SymbolTableEntry &known_info)
{
    known_info.SType = SymT::kNoType;
    if (SymT::kNoType == entry.SType)
        return; // there is no info yet

    known_info = entry;

    // Kill the defaults so we can check whether this defn replicates them exactly.
    size_t const num_of_params = entry.GetNumOfFuncParams();

    SymbolTableEntry::ParamDefault deflt{};
    deflt.Type = SymbolTableEntry::kDT_None;
    entry.FuncParamDefaultValues.assign(num_of_params + 1, deflt);
    return;
}


// extender function, eg. function GoAway(this Character *someone)
// We've just accepted something like "int func(", we expect "this" --OR-- "static" (!)
// We'll accept something like "this Character *"
ErrorType AGS::Parser::ParseFuncdecl_ExtenderPreparations(bool is_static_extender, AGS::Symbol &struct_of_func, AGS::Symbol &name_of_func, TypeQualifierSet &tqs)
{
    if (is_static_extender)
        tqs[TQ::kStatic] = true;

    _src.GetNext(); // Eat "this" or "static"
    struct_of_func = _src.GetNext();
    if (!_sym.IsStructVartype(struct_of_func))
    {
        Error("Expected a struct type instead of '%s'", _sym.GetName(struct_of_func).c_str());
        return kERR_UserError;
    }

    name_of_func = MangleStructAndComponent(struct_of_func, name_of_func);

    if (kKW_Dynpointer == _src.PeekNext())
    {
        if (is_static_extender)
        {
            Error("Unexpected '*' after 'static' in static extender function");
            return kERR_UserError;
        }
        _src.GetNext(); // Eat '*'
    }

    // If a function is defined with the Extender mechanism, it needn't have a declaration
    // in the struct defn. So pretend that this declaration has happened.
    _sym[name_of_func].Parent = struct_of_func;
    SetFlag(_sym[name_of_func].Flags, kSFLG_StructMember, true);

    Symbol const punctuation = _src.PeekNext();
    ErrorType retval = Expect(SymbolList{ kKW_Comma, kKW_CloseParenthesis }, punctuation);
    if (retval < 0) return retval;
    if (kKW_Comma == punctuation)
        _src.GetNext(); // Eat ','

    return kERR_None;
}

ErrorType AGS::Parser::ParseVarname(bool accept_member_access, Symbol &structname, Symbol &varname)
{
    varname = _src.GetNext();
    if (varname <= kKW_LastPredefined)
    {
        Error("Expected an identifier, found '%s' instead", _sym.GetName(varname).c_str());
        return kERR_UserError;
    }

    if (!accept_member_access)
    {
        if (0 != structname)
            return kERR_None;

        if (SymT::kVartype == _sym.GetSymbolType(varname))
        {
            std::string msg =
                ReferenceMsgSym("'%s' is already in use as a type name", varname);
            Error(msg.c_str(), _sym.GetName(varname).c_str());
            return kERR_UserError;
        }
        return kERR_None;
    }

    if (kKW_ScopeRes != _src.PeekNext())
        return kERR_None; // done

    if (!accept_member_access)
    {
        Error("Cannot use '::' here");
        return kERR_UserError;
    }

    // We are accepting "struct::member"; so varname isn't the var name yet: it's the struct name.
    structname = varname;
    _src.GetNext(); // Eat "::"
    Symbol membername = _src.GetNext();

// change varname to be the full function name
varname = MangleStructAndComponent(structname, membername);
if (varname < 0)
{
    Error("'%s' does not contain a function '%s'",
        _sym.GetName(structname).c_str(),
        _sym.GetName(membername).c_str());
    return kERR_UserError;
}

return kERR_None;
}

ErrorType AGS::Parser::ParseParamlist_ParamType(AGS::Vartype &vartype)
{
    if (kKW_Void == vartype)
    {
        Error("A function parameter must not have the type 'void'");
        return kERR_UserError;
    }
    SetDynpointerInManagedVartype(vartype);
    ErrorType retval = EatDynpointerSymbolIfPresent(vartype);
    if (retval < 0) return retval;

    if (PP::kMain == _pp && !_sym.IsManagedVartype(vartype) && _sym.IsStructVartype(vartype))
    {
        Error("'%s' is non-managed; a non-managed struct cannot be passed as parameter", _sym.GetName(vartype).c_str());
        return kERR_UserError;
    }
    return kERR_None;
}


// We're accepting a parameter list. We've accepted something like "int".
// We accept a param name such as "i" if present
ErrorType AGS::Parser::ParseParamlist_Param_Name(bool body_follows, AGS::Symbol &param_name)
{
    param_name = kKW_NoSymbol;

    if (PP::kPreAnalyze == _pp || !body_follows)
    {
        // Ignore the parameter name when present, it won't be used later on (in this phase)
        Symbol const nextsym = _src.PeekNext();
        if (IsIdentifier(nextsym))
            _src.GetNext();
        return kERR_None;
    }

    Symbol no_struct = 0;
    ErrorType retval = ParseVarname(false, no_struct, param_name);
    if (retval < 0) return retval;

    switch (_sym.GetSymbolType(param_name))
    {
    default:
        Error(
            ReferenceMsgSym("Parameter '%s' is already in use", param_name).c_str(),
            _sym.GetName(param_name).c_str());
        return kERR_UserError;

    case SymT::kFunction:
        Warning(
            ReferenceMsgSym("This hides the function '%s()'", param_name).c_str(),
            _sym.GetName(param_name).c_str());
        return kERR_None;

    case SymT::kGlobalVar:
        return kERR_None;

    case SymT::kLocalVar:
        Error(
            ReferenceMsgSym("The name '%s' is already in use as a parameter", param_name).c_str(),
            _sym.GetName(param_name).c_str());
        return kERR_UserError;

    case SymT::kNoType:
        return kERR_None;

    case SymT::kVartype:
        Warning(
            ReferenceMsgSym("This hides the type '%s'", param_name).c_str(),
            _sym.GetName(param_name).c_str());
        return kERR_None;
    }
    // Can't reach.
}

ErrorType AGS::Parser::ParseParamlist_Param_AsVar2Sym(AGS::Symbol param_name, AGS::Vartype param_vartype, bool param_is_const, int param_idx)
{
    SymbolTableEntry &param_entry = _sym[param_name];
    
   if (param_is_const)
    {
        param_entry.TypeQualifiers[TQ::kReadonly] = true;
        param_entry.Vartype = _sym.VartypeWith(VTT::kConst, param_entry.Vartype);
    }
    // the parameters are pushed backwards, so the top of the
    // stack has the first parameter. The + 1 is because the
    // call will push the return address onto the stack as well
    param_entry.SOffset = _scrip.offset_to_local_var_block - (param_idx + 1) * SIZE_OF_STACK_CELL;
    _sym.SetDeclared(param_name, _src.GetCursor());
    return kERR_None;
}

ErrorType AGS::Parser::ParseParamlist_Param(Symbol name_of_func, bool body_follows, Vartype param_vartype, bool param_is_const, size_t param_idx)
{
    ErrorType retval = ParseParamlist_ParamType(param_vartype);
    if (retval < 0) return retval;
    if (param_is_const)
        param_vartype = _sym.VartypeWith(VTT::kConst, param_vartype);

    Symbol param_name;
    retval = ParseParamlist_Param_Name(body_follows, param_name);
    if (retval < 0) return retval;

    retval = ParseDynArrayMarkerIfPresent(param_vartype);
    if (retval < 0) return retval;

    SymbolTableEntry::ParamDefault param_default;
    retval = ParseParamlist_Param_DefaultValue(param_vartype, param_default);
    if (retval < 0) return retval;

    _sym[name_of_func].FuncParamVartypes.push_back(param_vartype);
    _sym[name_of_func].FuncParamDefaultValues.push_back(param_default);
    
    if (PP::kMain != _pp || !body_follows)
        return kERR_None;

    // All function parameters correspond to local variables.
    // A body will follow, so we need to enter this parameter as a variable into the symbol table
    retval = ParseVardecl_Var2SymTable(param_name, param_vartype, ScT::kLocal);
    if (retval < 0) return retval;
    // Set the offset, make read-only if required
    return ParseParamlist_Param_AsVar2Sym(param_name, param_vartype, param_is_const, param_idx);
}

ErrorType AGS::Parser::ParseFuncdecl_Paramlist(AGS::Symbol funcsym, bool body_follows)
{
    _sym[funcsym].SScope = false;
    _sym[funcsym].FuncParamVartypes.resize(1u); // [0] is the return type; leave that
    _sym[funcsym].FuncParamDefaultValues.resize(1u);
    bool param_is_const = false;
    size_t param_idx = 0;
    while (!_src.ReachedEOF())
    {
        Symbol const cursym = _src.GetNext();
        if (kKW_CloseParenthesis == cursym)
            return kERR_None;   // empty parameter list

        if (SymT::kVartype == _sym.GetSymbolType(cursym))
        {
            if (param_idx == 0 && kKW_Void == cursym && kKW_CloseParenthesis == _src.PeekNext())
            {   // explicitly empty parameter list, "(void)"
                _src.GetNext(); // Eat ')'
                return kERR_None;
            }

            if ((++param_idx) >= MAX_FUNCTION_PARAMETERS)
            {
                Error("Too many parameters defined for function (max. allowed: %u)", MAX_FUNCTION_PARAMETERS - 1u);
                return kERR_UserError;
            }

            ErrorType retval = ParseParamlist_Param(funcsym, body_follows, cursym, param_is_const, _sym[funcsym].FuncParamVartypes.size());
            if (retval < 0) return retval;

            param_is_const = false; // modifier has been used up
            Symbol const nextsym = _src.GetNext();
            if (kKW_Comma != nextsym && kKW_CloseParenthesis != nextsym)
            {
                Error("Expected ',' or ')' or an identifier, found '%s' instead", _sym.GetName(nextsym).c_str());
                return kERR_UserError;
            }
            if (kKW_CloseParenthesis == nextsym)
                return kERR_None;
            continue;
        }

        if (kKW_Const == cursym)
        {
            // check in main compiler phase that type must follow
            if (PP::kMain == _pp && SymT::kVartype != _sym.GetSymbolType(_src.PeekNext()))
            {
                Error(
                    "Expected a type after 'const', found '%s' instead",
                    _sym.GetName(_src.PeekNext()).c_str());
                return kERR_UserError;
            }
            param_is_const = true;
            continue;
        }

        if (kKW_Varargs == cursym)
        {
            _sym[funcsym].SScope = true;
            return Expect(kKW_CloseParenthesis, _src.GetNext(), "Expected ')' following the '...'");
        }
        
        Error("Unexpected '%s' in parameter list", _sym.GetName(cursym).c_str());
        return kERR_UserError;
    } // while
    // Can't happen
    Error("!End of input when processing parameter list");
    return kERR_InternalError;
}
void AGS::Parser::ParseFuncdecl_MasterData2Sym(TypeQualifierSet tqs, Vartype return_vartype, Symbol struct_of_function, Symbol name_of_function, bool body_follows)
{
    SymbolTableEntry &entry = _sym[name_of_function];
    entry.SType = SymT::kFunction;
    entry.FuncParamVartypes[0] = return_vartype;
    entry.TypeQualifiers = tqs;
    // "autoptr", "managed" and "builtin" are aspects of the vartype, not of the entity returned.
    entry.TypeQualifiers[TQ::kAutoptr] = false;
    entry.TypeQualifiers[TQ::kManaged] = false;
    entry.TypeQualifiers[TQ::kBuiltin] = false;

    // Do not set Extends and the component flag here.
    // They are used to denote functions that were either declared in a struct defn or as extender

    if (PP::kPreAnalyze == _pp)
    {
        // Encode in entry.SOffset the type of function declaration
        FunctionType ft = kFT_PureForward;
        if (tqs[TQ::kImport])
            ft = kFT_Import;
        if (body_follows)
            ft = kFT_LocalBody;
        if (_sym[name_of_function].SOffset < ft)
            _sym[name_of_function].SOffset = ft;
    }
    return;
}

ErrorType AGS::Parser::ParseFuncdecl_CheckThatKIM_CheckDefaults(SymbolTableEntry const &this_entry, SymbolTableEntry const &known_info, bool body_follows)
{
    if (body_follows)
    {
        // If none of the parameters have a default, we'll let this through.
        bool has_default = false;
        for (size_t param_idx = 1; param_idx <= this_entry.GetNumOfFuncParams(); ++param_idx)
            if (this_entry.HasParamDefault(param_idx))
            {
                has_default = true;
                break;
            }
        if (!has_default)
            return kERR_None;
    }

    // this is 1 .. GetNumOfFuncArgs(), INCLUSIVE, because param 0 is the return type
    for (size_t param_idx = 1; param_idx <= this_entry.GetNumOfFuncParams(); ++param_idx)
    {
        if ((this_entry.HasParamDefault(param_idx) == known_info.HasParamDefault(param_idx)) &&
            (this_entry.HasParamDefault(param_idx) == false ||
                this_entry.FuncParamDefaultValues[param_idx] ==
                known_info.FuncParamDefaultValues[param_idx]))
            continue;

        std::string errstr1 = "In this declaration, parameter #<1> <2>; ";
        errstr1.replace(errstr1.find("<1>"), 3, std::to_string(param_idx));
        if (!this_entry.HasParamDefault(param_idx))
            errstr1.replace(errstr1.find("<2>"), 3, "doesn't have a default value");
        else
            errstr1.replace(errstr1.find("<2>"), 3, "has the default "
                + this_entry.FuncParamDefaultValues[param_idx].ToString());

        std::string errstr2 = "in a declaration elsewhere, that parameter <2>";
        if (!known_info.HasParamDefault(param_idx))
            errstr2.replace(errstr2.find("<2>"), 3, "doesn't have a default value");
        else
            errstr2.replace(errstr2.find("<2>"), 3, "has the default "
                + known_info.FuncParamDefaultValues[param_idx].ToString());
        errstr1 += errstr2;
        Error(ReferenceMsgLoc(errstr1, known_info.Declared).c_str());
        return kERR_UserError;
    }
    return kERR_None;
}

// there was a forward declaration -- check that the real declaration matches it
ErrorType AGS::Parser::ParseFuncdecl_CheckThatKnownInfoMatches(SymbolTableEntry &this_entry, SymbolTableEntry const &known_info, bool body_follows)
{
    if (SymT::kNoType == known_info.SType)
        return kERR_None; // We don't have any known info

    if (known_info.SType != this_entry.SType)
    {
        std::string msg = ReferenceMsgLoc(
            "'%s' is declared as a function here but differently elsewhere",
            known_info.Declared);
        Error(msg.c_str(), this_entry.SName.c_str());
        return kERR_UserError;
    }

    TypeQualifierSet known_tq = known_info.TypeQualifiers;
    known_tq[TQ::kImport] = false;
    TypeQualifierSet this_tq = this_entry.TypeQualifiers;
    this_tq[TQ::kImport] = false;
    if (known_tq != this_tq)
    {
        std::string const ki_tq = TypeQualifierSet2String(known_tq);
        std::string const te_tq = TypeQualifierSet2String(this_tq);
        std::string msg = ReferenceMsgLoc(
            "'%s' has the qualifiers '%s' here but '%s' elsewhere",
            known_info.Declared);
        Error(msg.c_str(), this_entry.SName.c_str(), te_tq.c_str(), ki_tq.c_str());
        return kERR_UserError;
    }

    if (known_info.GetNumOfFuncParams() != this_entry.GetNumOfFuncParams())
    {
        std::string msg = ReferenceMsgLoc(
            "Function '%s' is declared with %d mandatory parameters here, %d mandatory parameters elswehere",
            known_info.Declared);
        Error(msg.c_str(), this_entry.SName.c_str(), this_entry.GetNumOfFuncParams(), known_info.GetNumOfFuncParams());
        return kERR_UserError;
    }
    if (known_info.IsVarargsFunc() != this_entry.IsVarargsFunc())
    {
        std::string te =
            this_entry.IsVarargsFunc() ?
            "is declared to accept additional parameters here" :
            "is declared to not accept additional parameters here";
        std::string ki =
            known_info.IsVarargsFunc() ?
            "to accepts additional parameters elsewhere" :
            "to not accept additional parameters elsewhere";
        std::string const msg =
            ReferenceMsgLoc(
                "Function '%s' %s, %s",
                known_info.Declared);
        Error(msg.c_str(), this_entry.SName.c_str(), te.c_str(), ki.c_str());
        return kERR_UserError;
    }

    if (known_info.FuncParamVartypes.at(0) != this_entry.FuncParamVartypes.at(0))
    {
        std::string msg = ReferenceMsgLoc(
            "Return type of '%s' is declared as '%s' here, as '%s' elsewhere",
            known_info.Declared);
        Error(
            msg.c_str(),
            this_entry.SName.c_str(),
            _sym.GetName(this_entry.FuncParamVartypes.at(0)).c_str(),
            _sym.GetName(known_info.FuncParamVartypes.at(0)).c_str());

        return kERR_UserError;
    }

    for (size_t param_idx = 1; param_idx <= this_entry.GetNumOfFuncParams(); param_idx++)
    {
        if (known_info.FuncParamVartypes.at(param_idx) != this_entry.FuncParamVartypes.at(param_idx))
        {
            std::string msg = ReferenceMsgLoc(
                "For function '%s': Type of parameter #%d is %s here, %s in a declaration elsewhere",
                known_info.Declared);
            Error(
                msg.c_str(),
                this_entry.SName.c_str(),
                param_idx,
                _sym.GetName(this_entry.FuncParamVartypes.at(param_idx)).c_str(),
                _sym.GetName(known_info.FuncParamVartypes.at(param_idx)).c_str());
            return kERR_UserError;
        }
    }

    // Check that the defaults match
    ErrorType retval = ParseFuncdecl_CheckThatKIM_CheckDefaults(this_entry, known_info, body_follows);
    if (retval < 0) return retval;

    return kERR_None;
}

// Enter the function in the imports[] or functions[] array; get its index   
ErrorType AGS::Parser::ParseFuncdecl_EnterAsImportOrFunc(AGS::Symbol name_of_func, bool body_follows, bool func_is_import, AGS::CodeLoc &function_soffs, int &function_idx)
{
    if (body_follows)
    {
        // Index of the function in the ccCompiledScript::functions[] array
        function_soffs = _scrip.add_new_function(_sym.GetName(name_of_func), &function_idx);
        if (function_soffs < 0)
        {
            Error("Max. number of functions exceeded");
            return kERR_UserError;
        }
        _fcm.SetFuncCallpoint(name_of_func, function_soffs);
        return kERR_None;
    }

    if (!func_is_import)
    {
        function_soffs = -1; // forward decl; callpoint is unknown yet
        return kERR_None;
    }

    // Index of the function in the ccScript::imports[] array
    function_soffs = _importMgr.FindOrAdd(_sym.GetName(name_of_func));
    return kERR_None;
}


// We're at something like "int foo(", directly before the "("
// Get the symbol after the corresponding ")"
ErrorType AGS::Parser::ParseFuncdecl_DoesBodyFollow(bool &body_follows)
{
    int const cursor = _src.GetCursor();

    ErrorType retval = SkipToClose(kKW_CloseParenthesis);
    if (retval < 0) return retval;
    body_follows = (kKW_OpenBrace == _src.PeekNext());

    _src.SetCursor(cursor);
    return kERR_None;
}

ErrorType AGS::Parser::ParseFuncdecl_Checks(TypeQualifierSet tqs, Symbol struct_of_func, Symbol name_of_func, Vartype return_vartype, bool body_follows, bool no_loop_check)
{
    if (0 >= struct_of_func && tqs[TQ::kProtected])
    {
        Error(
            "Function '%s' isn't a struct component and so cannot be 'protected'",
            _sym.GetName(name_of_func).c_str());
        return kERR_UserError;
    }

    if (!body_follows && no_loop_check)
    {
        Error("Can only use 'noloopcheck' when a function body follows the definition");
        return kERR_UserError;
    }

    SymbolType const stype = _sym[name_of_func].SType;
    if (SymT::kFunction != stype && SymT::kNoType != stype)
    {
        Error(
            ReferenceMsgSym("'%s' is defined elsewhere as a non-function", name_of_func).c_str(),
            _sym.GetName(name_of_func).c_str());
        return kERR_UserError;
    }

    if (!_sym.IsManagedVartype(return_vartype) && _sym.IsStructVartype(return_vartype))
    {
        Error("Can only return a struct when it is 'managed'");
        return kERR_UserError;
    }

    if (PP::kPreAnalyze == _pp && body_follows && kFT_LocalBody == _sym[name_of_func].SOffset)
    {
        Error(
            ReferenceMsgSym("Function '%s' is also defined with body elsewhere", name_of_func).c_str(),
            _sym.GetName(name_of_func).c_str());
        return kERR_UserError;
    }

    if (PP::kMain == _pp && 0 < struct_of_func && struct_of_func != _sym[name_of_func].Parent)
    {
        // Functions only get this if they are declared in a struct or as extender
        std::string component = _sym.GetName(name_of_func);
        component.erase(0, component.rfind(':') + 1);
        Error(
            ReferenceMsgSym("Function '%s' has not been declared within struct '%s' as a component", struct_of_func).c_str(),
            component.c_str(), _sym.GetName(struct_of_func).c_str());
        return kERR_UserError;
    }

    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseFuncdecl_HandleFunctionOrImportIndex(TypeQualifierSet tqs, Symbol struct_of_func, Symbol name_of_func, bool body_follows)
{
    if (PP::kMain == _pp)
    {
        // Get start offset and function index
        int function_idx = -1; // Index in the _scrip.functions[] array
        int func_startoffs;
        ErrorType retval = ParseFuncdecl_EnterAsImportOrFunc(name_of_func, body_follows, tqs[TQ::kImport], func_startoffs, function_idx);
        if (retval < 0) return retval;
        _sym[name_of_func].SOffset = func_startoffs;
        if (function_idx >= 0)
            _scrip.functions[function_idx].NumOfParams =
            _sym[name_of_func].GetNumOfFuncParams();
    }

    if (!tqs[TQ::kImport])
        return kERR_None;

    // Imported functions

    _sym[name_of_func].TypeQualifiers[TQ::kImport] = true;

    if (PP::kPreAnalyze == _pp)
    {
        _sym[name_of_func].SOffset = kFT_Import;
        return kERR_None;
    }

    if (struct_of_func > 0)
    {
        char appendage[10];
        sprintf(appendage, "^%d", _sym[name_of_func].GetNumOfFuncParams() + 100 * _sym[name_of_func].SScope);
        strcat(_scrip.imports[_sym[name_of_func].SOffset], appendage);
    }

    _fim.SetFuncCallpoint(name_of_func, _sym[name_of_func].SOffset);
    return kERR_None;
}

// We're at something like "int foo(", directly before the "("
// This might or might not be within a struct defn
// An extender func param, if any, has already been resolved
ErrorType AGS::Parser::ParseFuncdecl(size_t declaration_start, TypeQualifierSet tqs, Vartype return_vartype, Symbol struct_of_func, Symbol name_of_func, bool no_loop_check, bool &body_follows)
{
    ErrorType retval = ParseFuncdecl_DoesBodyFollow(body_follows);
    if (retval < 0) return retval;

    retval = ParseFuncdecl_Checks(tqs, struct_of_func, name_of_func, return_vartype, body_follows, no_loop_check);
    if (retval < 0) return retval;
   
    // A forward decl can be written with the
    // "import" keyword (when allowed in the options). This isn't an import
    // proper, so reset the "import" flag in this case.
    if (tqs[TQ::kImport] &&   // This declaration has 'import'
        SymT::kFunction == _sym.GetSymbolType(name_of_func) &&
        !_sym[name_of_func].TypeQualifiers[TQ::kImport]) // but symbol table hasn't 'import'
    {
        if (0 != ccGetOption(SCOPT_NOIMPORTOVERRIDE))
        {
            std::string const msg = ReferenceMsgSym(
                "In here, a function with a local body must not have an \"import\" declaration",
                name_of_func);
            Error(msg.c_str());
            return kERR_UserError;
        }
        tqs[TQ::kImport] = false;
    }

    if (PP::kMain == _pp && body_follows)
    {
        // All the parameters that will be defined as local variables go on nesting level 1.
        _nest.Push(NSType::kParameters);
        // When this function is called, first all the parameters are pushed on the stack
        // and then the address to which the function should return after it has finished.
        // So the first parameter isn't on top of the stack but one address below that
        _scrip.offset_to_local_var_block += SIZE_OF_STACK_CELL;
    }

    // Copy all known info about the function so that we can check whether this declaration is compatible
    SymbolTableEntry known_info;
    CopyKnownSymInfo(_sym[name_of_func], known_info);
    
    ParseFuncdecl_MasterData2Sym(tqs, return_vartype, struct_of_func, name_of_func, body_follows);

    retval = ParseFuncdecl_Paramlist(name_of_func, body_follows);
    if (retval < 0) return retval;

    retval = ParseFuncdecl_CheckThatKnownInfoMatches(_sym[name_of_func], known_info, body_follows);
    if (retval < 0) return retval;

    // copy the default values from the function prototype into the symbol table
    if (known_info.SType != SymT::kNoType)
        _sym[name_of_func].FuncParamDefaultValues.assign(
            known_info.FuncParamDefaultValues.begin(),
            known_info.FuncParamDefaultValues.end());

    retval = ParseFuncdecl_HandleFunctionOrImportIndex(tqs, struct_of_func, name_of_func, body_follows);
    if (retval < 0) return retval;

    _sym.SetDeclared(name_of_func, declaration_start);
    return kERR_None;
}


// interpret the float as if it were an int (without converting it really);
// return that int
int AGS::Parser::InterpretFloatAsInt(float floatval)
{
    float *floatptr = &floatval; // Get pointer to the float
    int *intptr = reinterpret_cast<int *>(floatptr); // pretend that it points to an int
    return *intptr; // return the int that the pointer points to
}

ErrorType AGS::Parser::IndexOfLeastBondingOperator(SrcList &expression, int &idx)
{
    size_t nesting_depth = 0;

    int largest_prio_found = INT_MIN; // note: largest number == lowest priority
    bool largest_is_binary = true;
    int index_of_largest_prio = -1;
    bool encountered_operand = false;

    expression.StartRead();
    while (!expression.ReachedEOF())
    {
        Symbol const current_sym = expression.GetNext();
        SymbolType current_sym_type = _sym.GetSymbolType(current_sym);
        if (kKW_New == current_sym ||
            kKW_Tern == current_sym ||
            SymT::kOperator == current_sym_type)
        {
            current_sym_type = SymT::kOperator;
        }
        else if (kKW_CloseBracket == current_sym ||
            kKW_CloseParenthesis == current_sym)
        {
            encountered_operand = true;
            if (nesting_depth > 0)
                nesting_depth--;
            continue;
        }
        else if (kKW_OpenBracket == current_sym ||
            kKW_OpenParenthesis == current_sym)
        {
            nesting_depth++;
            continue;
        }
        else
        {
            encountered_operand = true;
        }

        // Continue if we aren't at zero nesting depth, since ()[] take priority
        if (nesting_depth > 0)
            continue;

        if (current_sym_type != SymT::kOperator)
            continue;

        // a binary operator has an operand to its left
        bool const is_binary = encountered_operand;
        encountered_operand = false;

        Symbol const current_op = current_sym;
        int const current_prio =
            is_binary ? _sym.BinaryOpPrio(current_op) : _sym.UnaryOpPrio(current_op);
        if (current_prio < 0)
        {
            Error(
                "'%s' cannot be used as %s operator",
                _sym.GetName(current_op).c_str(),
                is_binary ? "binary" : "unary");
            return kERR_UserError;
        }
        if (current_prio < largest_prio_found)
            continue; // can't be lowest priority

        // remember this and continue looking
        largest_prio_found = current_prio;
        // The cursor has already moved to the next symbol, so the index is one less
        index_of_largest_prio = expression.GetCursor() - 1;
        largest_is_binary = is_binary;
    }

    // unary operators are prefix, so if the least binding operator
    // turns out to be unary and not in first position, it must be
    // a chain of unary operators and the first should be evaluated
    // first
    idx = largest_is_binary ? index_of_largest_prio : 0;
    return kERR_None;
}

// Change the generic opcode to the one that is correct for the vartypes
// Also check whether the operator can handle the types at all
ErrorType AGS::Parser::GetOpcodeValidForVartype(Vartype vartype1, Vartype vartype2, CodeCell &opcode)
{
    if (kKW_Float == vartype1 || kKW_Float == vartype2)
    {
        switch (opcode)
        {
        default:
            Error("The operator cannot be applied to float values");
            return kERR_UserError;
        case SCMD_ADD:      opcode = SCMD_FADD; break;
        case SCMD_ADDREG:   opcode = SCMD_FADDREG; break;
        case SCMD_DIVREG:   opcode = SCMD_FDIVREG; break;
        case SCMD_GREATER:  opcode = SCMD_FGREATER; break;
        case SCMD_GTE:      opcode = SCMD_FGTE; break;
        case SCMD_ISEQUAL:  break;
        case SCMD_LESSTHAN: opcode = SCMD_FLESSTHAN; break;
        case SCMD_LTE:      opcode = SCMD_FLTE; break;
        case SCMD_MULREG:   opcode = SCMD_FMULREG; break;
        case SCMD_NOTEQUAL: break;
        case SCMD_SUB:      opcode = SCMD_FSUB; break;
        case SCMD_SUBREG:   opcode = SCMD_FSUBREG; break;
        }
        return kERR_None;
    }

    bool const iatos1 = _sym.IsAnyStringVartype(vartype1);
    bool const iatos2 = _sym.IsAnyStringVartype(vartype2);

    if (iatos1 || iatos2)
    {
        switch (opcode)
        {
        default:
            Error("Operator cannot be applied to string type values");
            return kERR_UserError;
        case SCMD_ISEQUAL:  opcode = SCMD_STRINGSEQUAL; break;
        case SCMD_NOTEQUAL: opcode = SCMD_STRINGSNOTEQ; break;
        }
        if (kKW_Null == vartype1 || kKW_Null == vartype2)
            return kERR_None;

        if (iatos1 != iatos2)
        {
            Error("A string type value cannot be compared to a value that isn't a string type");
            return kERR_UserError;
        }
        return kERR_None;
    }

    if (((_sym.IsDynpointerVartype(vartype1) || kKW_Null == vartype1) &&
        (_sym.IsDynpointerVartype(vartype2) || kKW_Null == vartype2)) ||
        ((_sym.IsDynarrayVartype(vartype1) || kKW_Null == vartype1) &&
        (_sym.IsDynarrayVartype(vartype2) || kKW_Null == vartype2)))
    {
        switch (opcode)
        {
        default:
            Error("The operator cannot be applied to managed types");
            return kERR_UserError;
        case SCMD_ISEQUAL:  return kERR_None;
        case SCMD_NOTEQUAL: return kERR_None;
        }
    }

    // Other combinations of managed types won't mingle
    if (_sym.IsDynpointerVartype(vartype1) || _sym.IsDynpointerVartype(vartype2))
    {
        Error("The operator cannot be applied to values of these types");
        return kERR_UserError;
    }

    ErrorType retval = IsVartypeMismatch(vartype1, kKW_Int, true);
    if (retval < 0) return retval;
    return IsVartypeMismatch(vartype2, kKW_Int, true);
}

// Check for a type mismatch in one direction only
bool AGS::Parser::IsVartypeMismatch_Oneway(AGS::Vartype vartype_is, AGS::Vartype vartype_wants_to_be) const
{
    // cannot convert 'void' to anything
    if (kKW_Void == vartype_is || kKW_Void == vartype_wants_to_be)
        return true;

    // Don't convert if no conversion is called for
    if (vartype_is == vartype_wants_to_be)
        return false;


    // Can convert null to dynpointer or dynarray
    if (kKW_Null == vartype_is)
        return
            !_sym.IsDynpointerVartype(vartype_wants_to_be) &&
            !_sym.IsDynarrayVartype(vartype_wants_to_be);

    // can convert String * to const string
    if (_sym.GetStringStructSym() == _sym.VartypeWithout(VTT::kDynpointer, vartype_is) &&
        kKW_String == _sym.VartypeWithout(VTT::kConst, vartype_wants_to_be))
    {
        return false;
    }

    // can convert string or const string to String *
    if (kKW_String == _sym.VartypeWithout(VTT::kConst, vartype_is) &&
        _sym.GetStringStructSym() == _sym.VartypeWithout(VTT::kDynpointer, vartype_wants_to_be))
    {
        return false;
    }

    // Note: CanNOT convert String * or const string to string;
    // a function that has a string parameter may modify it, but a String or const string may not be modified.

    if (_sym.IsOldstring(vartype_is) != _sym.IsOldstring(vartype_wants_to_be))
        return true;

    // Note: the position of this test is important.
    // Don't "group" string tests "together" and move this test above or below them.
    // cannot convert const to non-const
    if (_sym.IsConstVartype(vartype_is) && !_sym.IsConstVartype(vartype_wants_to_be))
        return true;

    if (_sym.IsOldstring(vartype_is))
        return false;

    // From here on, don't mind constness or dynarray-ness
    vartype_is = _sym.VartypeWithout(VTT::kConst, vartype_is);
    vartype_is = _sym.VartypeWithout(VTT::kDynarray, vartype_is);
    vartype_wants_to_be = _sym.VartypeWithout(VTT::kConst, vartype_wants_to_be);
    vartype_wants_to_be = _sym.VartypeWithout(VTT::kDynarray, vartype_wants_to_be);

    // floats cannot mingle with other types
    if ((vartype_is == kKW_Float) != (vartype_wants_to_be == kKW_Float))
        return true;

    // Can convert short, char etc. into int
    if (_sym.IsAnyIntegerVartype(vartype_is) && kKW_Int == vartype_wants_to_be)
        return false;

    // Checks to do if at least one is dynarray
    if (_sym.IsDynarrayVartype(vartype_is) || _sym.IsDynarrayVartype(vartype_wants_to_be))
    {
        // BOTH sides must be dynarray 
        if (_sym.IsDynarrayVartype(vartype_is) != _sym.IsDynarrayVartype(vartype_wants_to_be))
            return false;

        // The underlying core vartypes must be identical:
        // A dynarray contains a sequence of elements whose size are used
        // to index the individual element, so no extending elements
        Symbol const target_core_vartype = _sym.VartypeWithout(VTT::kDynarray, vartype_wants_to_be);
        Symbol const current_core_vartype = _sym.VartypeWithout(VTT::kDynarray, vartype_is);
        return current_core_vartype != target_core_vartype;
    }

    // Checks to do if at least one is dynpointer
    if (_sym.IsDynpointerVartype(vartype_is) || _sym.IsDynpointerVartype(vartype_wants_to_be))
    {
        // BOTH sides must be dynpointer
        if (_sym.IsDynpointerVartype(vartype_is) != _sym.IsDynpointerVartype(vartype_wants_to_be))
            return true;

        // Core vartypes need not be identical here: check against extensions
        Symbol const target_core_vartype = _sym.VartypeWithout(VTT::kDynpointer, vartype_wants_to_be);
        Symbol current_core_vartype = _sym.VartypeWithout(VTT::kDynpointer, vartype_is);
        while (current_core_vartype != target_core_vartype)
        {
            current_core_vartype = _sym[current_core_vartype].Parent;
            if (current_core_vartype == 0)
                return true;
        }
        return false;
    }

    // Checks to do if at least one is a struct or an array
    if (_sym.IsStructVartype(vartype_is) || _sym.IsStructVartype(vartype_wants_to_be) ||
        _sym.IsArrayVartype(vartype_is) || _sym.IsArrayVartype(vartype_wants_to_be))
        return (vartype_is != vartype_wants_to_be);

    return false;
}

// Check whether there is a type mismatch; if so, give an error
ErrorType AGS::Parser::IsVartypeMismatch(AGS::Vartype vartype_is, AGS::Vartype vartype_wants_to_be, bool orderMatters)
{
    if (!IsVartypeMismatch_Oneway(vartype_is, vartype_wants_to_be))
        return kERR_None;
    if (!orderMatters && !IsVartypeMismatch_Oneway(vartype_wants_to_be, vartype_is))
        return kERR_None;


    Error(
        "Type mismatch: cannot convert '%s' to '%s'",
        _sym.GetName(vartype_is).c_str(),
        _sym.GetName(vartype_wants_to_be).c_str());
    return kERR_UserError;
}

// returns whether the vartype of the opcode is always bool
bool AGS::Parser::IsBooleanOpcode(CodeCell opcode)
{
    if (opcode >= SCMD_ISEQUAL && opcode <= SCMD_OR)
        return true;

    if (opcode >= SCMD_FGREATER && opcode <= SCMD_FLTE)
        return true;

    if (opcode == SCMD_STRINGSNOTEQ || opcode == SCMD_STRINGSEQUAL)
        return true;

    return false;
}

// If we need a String but AX contains a string, 
// then convert AX into a String object and set its type accordingly
void AGS::Parser::ConvertAXStringToStringObject(AGS::Vartype wanted_vartype)
{
    if (kKW_String == _sym.VartypeWithout(VTT::kConst, _scrip.ax_vartype) &&
        _sym.GetStringStructSym() == _sym.VartypeWithout(VTT::kDynpointer, wanted_vartype))
    {
        WriteCmd(SCMD_CREATESTRING, SREG_AX); // convert AX
        _scrip.ax_vartype = _sym.VartypeWith(VTT::kDynpointer, _sym.GetStringStructSym());
    }
}

int AGS::Parser::GetReadCommandForSize(int the_size)
{
    switch (the_size)
    {
    default: return SCMD_MEMREAD;
    case 1:  return SCMD_MEMREADB;
    case 2:  return SCMD_MEMREADW;
    }
}

int AGS::Parser::GetWriteCommandForSize(int the_size)
{
    switch (the_size)
    {
    default: return SCMD_MEMWRITE;
    case 1:  return SCMD_MEMWRITEB;
    case 2:  return SCMD_MEMWRITEW;
    }
}

ErrorType AGS::Parser::HandleStructOrArrayResult(AGS::Vartype &vartype, AGS::Parser::ValueLocation &vloc)
{
    if (_sym.IsArrayVartype(vartype))
    {
        Error("Cannot access array as a whole (did you forget to add \"[0]\"?)");
        return kERR_UserError;
    }

    if (_sym.IsAtomic(vartype) && _sym.IsStructVartype(vartype))
    {
        if (_sym.IsManagedVartype(vartype))
        {
            // Interpret the memory address as the result
            vartype = _sym.VartypeWith(VTT::kDynpointer, vartype);
            WriteCmd(SCMD_REGTOREG, SREG_MAR, SREG_AX);
            vloc = kVL_AX_is_value;
            _scrip.ax_vartype = vartype;
            return kERR_None;
        }

        Error("Cannot access non-managed struct as a whole");
        return kERR_UserError;
    }

    return kERR_None;
}

ErrorType AGS::Parser::ResultToAX(ValueLocation &vloc, ScopeType &scope_type, AGS::Vartype &vartype)
{
    if (kVL_MAR_pointsto_value != vloc)
        return kERR_None; // So it's already in AX 

    _scrip.ax_vartype = vartype;
    _scrip.ax_scope_type = scope_type;

    if (kKW_String == _sym.VartypeWithout(VTT::kConst, vartype))
        WriteCmd(SCMD_REGTOREG, SREG_MAR, SREG_AX);
    else
        WriteCmd(
            _sym.IsDynVartype(vartype) ? SCMD_MEMREADPTR : GetReadCommandForSize(_sym.GetSize(vartype)),
            SREG_AX);
    vloc = kVL_AX_is_value;
    return kERR_None;
}

ErrorType AGS::Parser::ParseExpression_CheckArgOfNew(Vartype new_vartype)
{
    if (SymT::kVartype != _sym.GetSymbolType(new_vartype))
    {
        Error("Expected a type after 'new', found '%s' instead", _sym.GetName(new_vartype).c_str());
        return kERR_UserError;
    }

    if (SymT::kUndefinedStruct == _sym.GetSymbolType(new_vartype))
    {
        Error(
            "The struct '%s' hasn't been completely defined yet",
            _sym.GetName(new_vartype).c_str());
        return kERR_UserError;
    }

    if (!_sym.IsAnyIntegerVartype(new_vartype) && !_sym.IsManagedVartype(new_vartype))
    {
        Error("Can only use integer or managed types with 'new'");
        return kERR_UserError;
    }

    // Note: While it is an error to use a built-in type with new, it is
    // allowed to use a built-in type with new[].
    return kERR_None;
}

ErrorType AGS::Parser::ParseExpression_New(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, AGS::Vartype &vartype)
{
    expression.StartRead();
    expression.GetNext(); // Eat "new"

    if (expression.ReachedEOF())
    {
        Error("Expected a type after 'new' but didn't find any");
        return kERR_UserError;
    }
    Vartype const argument_vartype = expression.GetNext();

    ErrorType retval = ParseExpression_CheckArgOfNew(argument_vartype);
    if (retval < 0) return retval;

    bool const is_managed = !_sym.IsAnyIntegerVartype(argument_vartype);
    bool const with_bracket_expr = !expression.ReachedEOF(); // "new FOO[BAR]"

    Vartype element_vartype = 0;
    if (with_bracket_expr)
    {
        // Note that in AGS, you can write "new Struct[]" but what you mean then is "new Struct*[]".
        retval = EatDynpointerSymbolIfPresent(argument_vartype);
        if (retval < 0) return retval;

        retval = AccessData_ReadBracketedIntExpression(expression);
        if (retval < 0) return retval;
        element_vartype = is_managed ? _sym.VartypeWith(VTT::kDynpointer, argument_vartype) : argument_vartype;
        vartype = _sym.VartypeWith(VTT::kDynarray, element_vartype);
    }
    else
    {
        if (_sym.IsBuiltin(argument_vartype))
        {   
            Error("Expected '[' after the built-in type '%s'", _sym.GetName(argument_vartype).c_str());
            return kERR_UserError;
        }
        if (!is_managed)
        {
            Error("Expected '[' after the integer type '%s'", _sym.GetName(argument_vartype).c_str());
            return kERR_UserError;
        }
        element_vartype = argument_vartype;
        vartype = _sym.VartypeWith(VTT::kDynpointer, argument_vartype);
    }

    size_t const element_size = _sym.GetSize(element_vartype);
    if (0 == element_size)
    {   // The Engine really doesn't like that (division by zero error)
        Error("!Trying to emit allocation of zero dynamic memory");
        return kERR_InternalError;
    }

    if (with_bracket_expr)
        WriteCmd(SCMD_NEWARRAY, SREG_AX, element_size, is_managed);
    else
        WriteCmd(SCMD_NEWUSEROBJECT, SREG_AX, element_size);

    _scrip.ax_scope_type = scope_type = ScT::kGlobal;
    _scrip.ax_vartype = vartype;
    vloc = kVL_AX_is_value;
    return kERR_None;
}

// We're parsing an expression that starts with '-' (unary minus)
ErrorType AGS::Parser::ParseExpression_UnaryMinus(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, AGS::Vartype &vartype)
{
    if (expression.Length() < 2)
    {
        Error(
            "Expected a term after '%s' but didn't find any",
            _sym.GetName(expression[0]).c_str());
        return kERR_UserError;
    }

    expression.EatFirstSymbol(); // Eat '-'
    if (expression.Length() == 1)
    {
        expression.StartRead();
        SymbolType const stype = _sym.GetSymbolType(expression.PeekNext());
        if (SymT::kConstant == stype || SymT::kLiteralInt == stype)
            return AccessData_IntLiteralOrConst(true, expression, vartype);
        if (SymT::kLiteralFloat == stype)
            return AccessData_FloatLiteral(true, expression, vartype);
    };

    // parse the rest of the expression into AX
    ErrorType retval = ParseExpression_Term(expression, vloc, scope_type, vartype);
    if (retval < 0) return retval;
    retval = ResultToAX(vloc, scope_type, vartype);
    if (retval < 0) return retval;

    CodeCell opcode = SCMD_SUBREG; 
    retval = GetOpcodeValidForVartype(_scrip.ax_vartype, _scrip.ax_vartype, opcode);
    if (retval < 0) return retval;

    // Calculate 0 - AX
    // The binary representation of 0.0 is identical to the binary representation of 0
    // so this will work for floats as well as for ints.
    WriteCmd(SCMD_LITTOREG, SREG_BX, 0);
    WriteCmd(opcode, SREG_BX, SREG_AX);
    WriteCmd(SCMD_REGTOREG, SREG_BX, SREG_AX);
    vloc = kVL_AX_is_value;
    return kERR_None;
}

// We're parsing an expression that starts with '!' (boolean NOT) or '~' (bitwise Negate)
ErrorType AGS::Parser::ParseExpression_Negate(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, AGS::Vartype &vartype)
{
    Symbol const op_sym = expression[0];
    if (expression.Length() < 2)
    {
        Error(
            "Expected a term after '%s' but didn't find any",
            _sym.GetName(op_sym).c_str());
        return kERR_UserError;
    }

    SrcList after_not = SrcList(expression, 1, expression.Length() - 1);
    ErrorType retval = ParseExpression_Term(after_not, vloc, scope_type, vartype);
    if (retval < 0) return retval;
    retval = ResultToAX(vloc, scope_type, vartype);
    if (retval < 0) return retval;

    if (!_sym.IsAnyIntegerVartype(_scrip.ax_vartype))
    {
        Error(
            "Expected an integer expression after '%s' but found type %s",
            _sym.GetName(op_sym).c_str(),
            _sym.GetName(_scrip.ax_vartype).c_str());
        return kERR_UserError;
    }
    
    bool const bitwise_negation = (0 != _sym.GetName(op_sym).compare("!"));
    if (bitwise_negation)
    {
        // There isn't any opcode for this, so calculate -1 - AX
        WriteCmd(SCMD_LITTOREG, SREG_BX, -1);
        WriteCmd(SCMD_SUBREG, SREG_BX, SREG_AX);
        WriteCmd(SCMD_REGTOREG, SREG_BX, SREG_AX);
    }
    else
    {
        WriteCmd(SCMD_NOTREG, SREG_AX);
    }

    vloc = kVL_AX_is_value;
    vartype = _scrip.ax_vartype = kKW_Int;
    return kERR_None;
}

// The least binding operator is the first thing in the expression
// This means that the op must be an unary op.
ErrorType AGS::Parser::ParseExpression_Unary(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, AGS::Vartype &vartype)
{
    Symbol const first_op = expression[0];

    if (kKW_New == first_op) // we're parsing something like "new foo"
        return ParseExpression_New(expression, vloc, scope_type, vartype);

    CodeCell const opcode = _sym.GetOperatorOpcode(first_op);
    if (SCMD_SUBREG == opcode) // we're parsing something like "- foo"
        return ParseExpression_UnaryMinus(expression, vloc, scope_type, vartype);

    if (SCMD_NOTREG == opcode) // we're parsing something like "! foo"
        return ParseExpression_Negate(expression, vloc, scope_type, vartype);

    // All the other operators need a non-empty left hand side
    Error("Unexpected operator '%s' without a preceding expression", _sym.GetName(first_op).c_str());
    return kERR_UserError;
}

// The least binding operator is '?'
ErrorType AGS::Parser::ParseExpression_Ternary(size_t tern_idx, SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, AGS::Vartype &vartype)
{
    // First term ends before the '?'
    SrcList term1 = SrcList(expression, 0, tern_idx);

    // Second term begins after the '?', we don't know how long it is yet
    SrcList after_term1 = SrcList(expression, tern_idx + 1, expression.Length() - (tern_idx + 1));

    // Find beginning of third term
    after_term1.StartRead();
    SkipTo(SymbolList{ kKW_Colon }, after_term1);
    if (after_term1.ReachedEOF() || kKW_Colon != after_term1.PeekNext())
    {
        expression.SetCursor(tern_idx);
        Error("Didn't find the matching ':' to '?'");
        return kERR_UserError;
    }
    size_t const term3_start = after_term1.GetCursor() + 1;
    SrcList term3 = SrcList(after_term1, term3_start, after_term1.Length() - term3_start);
    SrcList term2 = SrcList(after_term1, 0u, after_term1.GetCursor());

    Vartype term1_vartype, term2_vartype, term3_vartype;
    ScopeType term1_scope_type, term2_scope_type, term3_scope_type;

    // First term of ternary
    ErrorType retval = ParseExpression_Term(term1, vloc, term1_scope_type, term1_vartype);
    if (retval < 0) return retval;
    ResultToAX(vloc, term1_scope_type, term1_vartype);
    if (!term1.ReachedEOF())
    {
        Error("!Unexpected '%s' after 1st term of ternary", _sym.GetName(term1.GetNext()).c_str());
        return kERR_InternalError;
    }

    // We jump either to the start of the third term or to the end of the ternary
    // expression. We don't know where this is yet, thus -77. This is just a
    // random number that's easy to spot in debugging outputs (where it's a clue
    // that it probably hasn't been replaced by a proper value). Don't use for anything.
    WriteCmd(
        (term2.Length() > 0) ? SCMD_JZ : SCMD_JNZ,
        -77);
    ForwardJump test_jumpdest(_scrip);
    test_jumpdest.AddParam();

    // Second term of ternary
    bool const second_term_exists = (term2.Length() > 0);
    if (second_term_exists)
    {
        retval = ParseExpression_Term(term2, vloc, term2_scope_type, term2_vartype);
        if (retval < 0) return retval;
        if (!term2.ReachedEOF())
        {
            Error("!Unexpected '%s' after 1st term of ternary", _sym.GetName(term2.GetNext()).c_str());
            return kERR_InternalError;
        }
        ResultToAX(vloc, term2_scope_type, term2_vartype);
        if (_sym.IsAnyStringVartype(term2_vartype))
        {
            ConvertAXStringToStringObject(_sym.GetStringStructSym());
            term2_vartype = _scrip.ax_vartype;
        }
        // Jump to the end of the ternary expression;
        // We don't know the dest yet, thus the placeholder value -77. Don't
        // test for this random magic number or use it in code
        WriteCmd(SCMD_JMP, -77);
    }
    else
    {
        // Take the first expression as the result of the missing second expression
        // No code is generated; instead, the conditional jump after the test goes
        // to the end of the expression if the test does NOT yield zero
        term2_vartype = term1_vartype;
        term2_scope_type = term1_scope_type;
        if (_sym.IsAnyStringVartype(term2_vartype))
        {
            ConvertAXStringToStringObject(_sym.GetStringStructSym());
            term2_vartype = _scrip.ax_vartype;
        }
    }
    ForwardJump jumpdest_after_term2(_scrip); // only valid if second_term_exists
    jumpdest_after_term2.AddParam();

    // Third term of ternary
    if (0 == term3.Length())
    {
        expression.SetCursor(tern_idx);
        Error("The third expression of this ternary is empty");
        return kERR_UserError;
    }
    if (second_term_exists)
        test_jumpdest.Patch(_src.GetLineno());

    retval = ParseExpression_Term(term3, vloc, term3_scope_type, term3_vartype);
    if (retval < 0) return retval;
    ResultToAX(vloc, term3_scope_type, term3_vartype);
    if (_sym.IsAnyStringVartype(term3_vartype))
    {
        ConvertAXStringToStringObject(_sym.GetStringStructSym());
        term3_vartype = _scrip.ax_vartype;
    }

    if (second_term_exists)
        jumpdest_after_term2.Patch(_src.GetLineno());
    else
        test_jumpdest.Patch(_src.GetLineno());

    scope_type =
        (ScT::kLocal == term2_scope_type || ScT::kLocal == term3_scope_type) ?
        ScT::kLocal : ScT::kGlobal;

    if (!IsVartypeMismatch_Oneway(term2_vartype, term3_vartype))
    {
        vartype = _scrip.ax_vartype = term3_vartype;
        return kERR_None;
    }
    if (!IsVartypeMismatch_Oneway(term3_vartype, term2_vartype))
    {
        vartype = _scrip.ax_vartype = term2_vartype;
        return kERR_None;
    }

    term3.SetCursor(0);
    Error("An expression of type '%s' is incompatible with an expression of type '%s'",
        _sym.GetName(term2_vartype).c_str(), _sym.GetName(term3_vartype).c_str());
    return kERR_UserError;
}

// The least binding operator has a left-hand and a right-hand side, e.g. "foo + bar"
ErrorType AGS::Parser::ParseExpression_Binary(size_t op_idx, SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, AGS::Vartype &vartype)
{
    // process the left hand side
    // This will be in vain if we find out later on that there isn't any right hand side,
    // but doing the left hand side first means that any errors will be generated from left to right
    Vartype vartype_lhs = 0;
    SrcList lhs = SrcList(expression, 0, op_idx);
    ErrorType retval = ParseExpression_Term(lhs, vloc, scope_type, vartype_lhs);
    if (retval < 0) return retval;
    retval = ResultToAX(vloc, scope_type, vartype_lhs);
    if (retval < 0) return retval;
    if (!lhs.ReachedEOF())
    {
        Error("!Unexpected '%s' after LHS of binary expression", _sym.GetName(lhs.GetNext()).c_str());
        return kERR_InternalError;
    }

    ForwardJump to_exit(_scrip);
    Symbol const operator_sym = expression[op_idx];
    int const opcode = _sym.GetOperatorOpcode(operator_sym);

    if (SCMD_AND == opcode)
    {
        // "&&" operator lazy evaluation: if AX is 0 then the AND has failed, 
        // so just jump directly past the AND instruction;
        // AX will still be 0 so that will do as the result of the calculation
        WriteCmd(SCMD_JZ, -77);
        // We don't know the end of the instruction yet, so remember the location we need to patch
        to_exit.AddParam();
    }
    else if (SCMD_OR == opcode)
    {
        // "||" operator lazy evaluation: if AX is non-zero then the OR has succeeded, 
        // so just jump directly past the OR instruction; 
        // AX will still be non-zero so that will do as the result of the calculation
        WriteCmd(SCMD_JNZ, -77);
        // We don't know the end of the instruction yet, so remember the location we need to patch
        to_exit.AddParam();
    }

    PushReg(SREG_AX);
    SrcList rhs = SrcList(expression, op_idx + 1, expression.Length());
    if (0 == rhs.Length())
    {
        // there is no right hand side for the expression
        Error("Binary operator '%s' doesn't have a right hand side", _sym.GetName(expression[op_idx]).c_str());
        return kERR_UserError;
    }

    retval = ParseExpression_Term(rhs, vloc, scope_type, vartype);
    if (retval < 0) return retval;
    retval = ResultToAX(vloc, scope_type, vartype);
    if (retval < 0) return retval;

    PopReg(SREG_BX); // Note, we pop to BX although we have pushed AX
    // now the result of the left side is in BX, of the right side is in AX

    // Check whether the left side type and right side type match either way
    retval = IsVartypeMismatch(vartype_lhs, vartype, false);
    if (retval < 0) return retval;

    int actual_opcode = opcode;
    retval = GetOpcodeValidForVartype(vartype_lhs, vartype, actual_opcode);
    if (retval < 0) return retval;

    WriteCmd(actual_opcode, SREG_BX, SREG_AX);
    WriteCmd(SCMD_REGTOREG, SREG_BX, SREG_AX);
    vloc = kVL_AX_is_value;

    to_exit.Patch(_src.GetLineno());

    // Operators like == return a bool (in our case, that's an int);
    // other operators like + return the type that they're operating on
    if (IsBooleanOpcode(actual_opcode))
        _scrip.ax_vartype = vartype = kKW_Int;

    return kERR_None;
}

ErrorType AGS::Parser::ParseExpression_BinaryOrTernary(size_t op_idx, SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, AGS::Vartype &vartype)
{
    Symbol const operator_sym = expression[op_idx];
    if (kKW_Tern == operator_sym)
        return ParseExpression_Ternary(op_idx, expression, vloc, scope_type, vartype);
    return ParseExpression_Binary(op_idx, expression, vloc, scope_type, vartype);
}

ErrorType AGS::Parser::ParseExpression_InParens(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, AGS::Vartype &vartype)
{
    // find the corresponding closing parenthesis
    size_t const bp_start = 1;
    expression.SetCursor(bp_start); // Skip the '('
    SkipTo(SymbolList{}, expression);
    size_t const bp_end = expression.GetCursor();
    
    SrcList between_parens = SrcList(expression, bp_start, bp_end - bp_start);
    ErrorType retval = ParseExpression_Term(between_parens, vloc, scope_type, vartype);
    if (retval < 0) return retval;

    if (!between_parens.ReachedEOF())
    {
        Error("Expected ')', found '%s' instead.", _sym.GetName(between_parens.GetNext()).c_str());
        return kERR_UserError;
    }

    expression.GetNext(); // Eat ')'
    return kERR_None;
}

// We're in the parameter list of a function call, and we have less parameters than declared.
// Provide defaults for the missing values
ErrorType AGS::Parser::AccessData_FunctionCall_ProvideDefaults(int num_func_args, size_t num_supplied_args, AGS::Symbol funcSymbol, bool func_is_import)
{
    for (size_t arg_idx = num_func_args; arg_idx > num_supplied_args; arg_idx--)
    {
        if (!_sym[funcSymbol].HasParamDefault(arg_idx))
        {
            Error("Function call parameter # %d isn't provided and doesn't have any default value", arg_idx);
            return kERR_UserError;
        }

        // push the default value onto the stack
        WriteCmd(
            SCMD_LITTOREG,
            SREG_AX,
            _sym[funcSymbol].FuncParamDefaultValues[arg_idx].ToInt32());

        if (func_is_import)
            WriteCmd(SCMD_PUSHREAL, SREG_AX);
        else
            PushReg(SREG_AX);
    }
    return kERR_None;
}

void AGS::Parser::DoNullCheckOnStringInAXIfNecessary(AGS::Vartype valTypeTo)
{

    if (_sym.GetStringStructSym() == _sym.VartypeWithout(VTT::kDynpointer, _scrip.ax_vartype) &&
        kKW_String == _sym.VartypeWithout(VTT::kConst, valTypeTo))
        WriteCmd(SCMD_CHECKNULLREG, SREG_AX);
}

std::string const AGS::Parser::ReferenceMsgLoc(std::string const &msg, size_t declared)
{
    if (SymbolTableEntry::kNoSrcLocation == declared)
        return msg;

    int const section_id = _src.GetSectionIdAt(declared);
    std::string const &section = _src.SectionId2Section(section_id);

    int const line = _src.GetLinenoAt(declared);
    if (line <= 0 || (!section.empty() && '_' == section[0]))
        return msg;

    std::string tpl;
    if (_src.GetSectionId() != section_id)
        tpl = ". See <1> line <2>";
    else if (_src.GetLineno() != line)
        tpl = ". See line <2>";
    else
        tpl = ". See the current line";
    size_t const loc1 = tpl.find("<1>");
    if (std::string::npos != loc1)
        tpl.replace(tpl.find("<1>"), 3, section);
    size_t const loc2 = tpl.find("<2>");
    if (std::string::npos != loc2)
        tpl.replace(tpl.find("<2>"), 3, std::to_string(line));
    return msg + tpl;
}

std::string const AGS::Parser::ReferenceMsgSym(std::string const &msg, AGS::Symbol symb)
{
    return ReferenceMsgLoc(msg, _sym.GetDeclared(symb));
}

ErrorType AGS::Parser::AccessData_FunctionCall_PushParams(SrcList &parameters, size_t closed_paren_idx, size_t num_func_args, size_t num_supplied_args, AGS::Symbol funcSymbol, bool func_is_import)
{
    size_t param_num = num_supplied_args + 1;
    size_t start_of_current_param = 0;
    int end_of_current_param = closed_paren_idx;  // can become < 0, points to (last symbol of parameter + 1)
    // Go backwards through the parameters since they must be pushed that way
    do
    {
        // Find the start of the next parameter
        param_num--;
        int paren_nesting_depth = 0;
        for (size_t paramListIdx = end_of_current_param - 1; true; paramListIdx--)
        {
            // going backwards so ')' increases the depth level
            Symbol const &idx = parameters[paramListIdx];
            if (kKW_CloseParenthesis == idx)
                paren_nesting_depth++;
            if (kKW_OpenParenthesis == idx)
                paren_nesting_depth--;
            if ((paren_nesting_depth == 0 && kKW_Comma == idx) ||
                (paren_nesting_depth < 0 && kKW_OpenParenthesis == idx))
            {
                start_of_current_param = paramListIdx + 1;
                break;
            }
            if (paramListIdx == 0)
                break; // Don't put this into the for header!
        }

        if (end_of_current_param < 0 || static_cast<size_t>(end_of_current_param) < start_of_current_param)
        {   
            Error("!Parameter length is negative");
            return kERR_InternalError;
        }

        // Compile the parameter
        ValueLocation vloc;
        ScopeType scope_type;
        Vartype vartype;

        SrcList current_param = SrcList(parameters, start_of_current_param, end_of_current_param - start_of_current_param);
        ErrorType retval = ParseExpression_Term(current_param, vloc, scope_type, vartype);
        if (retval < 0) return retval;
        retval = ResultToAX(vloc, scope_type, vartype);
        if (retval < 0) return retval;

        if (param_num <= num_func_args) // we know what type to expect
        {
            // If we need a string object ptr but AX contains a normal string, convert AX
            Vartype const param_vartype = _sym[funcSymbol].FuncParamVartypes[param_num];
            ConvertAXStringToStringObject(param_vartype);
            vartype = _scrip.ax_vartype;
            // If we need a normal string but AX contains a string object ptr, 
            // check that this ptr isn't null
            DoNullCheckOnStringInAXIfNecessary(param_vartype);

            if (IsVartypeMismatch(vartype, param_vartype, true))
                return kERR_UserError;
        }

        // Note: We push the parameters, which is tantamount to writing them
        // into memory with SCMD_MEMWRITE. The called function will use them
        // as local variables. However, if a parameter is managed, then its 
        // memory must be written with SCMD_MEMWRITEPTR, not SCMD_MEMWRITE 
        // as we do here. So to compensate, the called function will have to 
        // read each pointer variable with SCMD_MEMREAD and then write it
        // back with SCMD_MEMWRITEPTR.

        if (func_is_import)
            WriteCmd(SCMD_PUSHREAL, SREG_AX);
        else
            PushReg(SREG_AX);

        end_of_current_param = start_of_current_param - 1;
    }
    while (end_of_current_param > 0);

    return kERR_None;
}


// Count parameters, check that all the parameters are non-empty; find closing paren
ErrorType AGS::Parser::AccessData_FunctionCall_CountAndCheckParm(SrcList &parameters, AGS::Symbol name_of_func, size_t &index_of_close_paren, size_t &num_supplied_args)
{
    size_t paren_nesting_depth = 1;
    num_supplied_args = 1;
    size_t param_idx;
    bool found_param_symbol = false;

    for (param_idx = 1; param_idx < parameters.Length(); param_idx++)
    {
        Symbol const &idx = parameters[param_idx];

        if (kKW_OpenParenthesis == idx)
            paren_nesting_depth++;
        if (kKW_CloseParenthesis == idx)
        {
            paren_nesting_depth--;
            if (paren_nesting_depth == 0)
                break;
        }

        if (paren_nesting_depth == 1 && kKW_Comma == idx)
        {
            num_supplied_args++;
            if (found_param_symbol)
                continue;

            Error("Argument %d in function call is empty", num_supplied_args - 1);
            return kERR_UserError;
        }
        found_param_symbol = true;
    }

    // Special case: "()" means 0 arguments
    if (num_supplied_args == 1 &&
        parameters.Length() > 1 &&
        kKW_CloseParenthesis == parameters[1])
    {
        num_supplied_args = 0;
    }

    index_of_close_paren = param_idx;

    if (kKW_CloseParenthesis != parameters[index_of_close_paren])
    {
        Error("!Missing ')' at the end of the parameter list");
        return kERR_InternalError;
    }

    if (index_of_close_paren > 0 &&
        kKW_Comma == parameters[index_of_close_paren - 1])
    {
        Error("Last argument in function call is empty");
        return kERR_UserError;
    }

    if (paren_nesting_depth > 0)
    {
        Error("!Parser confused near '%s'", _sym.GetName(name_of_func).c_str());
        return kERR_InternalError;
    }

    return kERR_None;
}

// We are processing a function call. General the actual function call
void AGS::Parser::AccessData_GenerateFunctionCall(Symbol name_of_func, size_t num_args, bool func_is_import)
{
    if (func_is_import)
    {
        // tell it how many args for this call (nested imported functions cause stack problems otherwise)
        WriteCmd(SCMD_NUMFUNCARGS, num_args);
    }

    // Load function address into AX
    WriteCmd(SCMD_LITTOREG, SREG_AX, _sym[name_of_func].SOffset);

    if (func_is_import)
    {   
        _scrip.fixup_previous(kFx_Import); 
        if (!_importMgr.IsDeclaredImport(_sym.GetName(name_of_func)))
            _fim.TrackForwardDeclFuncCall(name_of_func, _scrip.codesize - 1, _src.GetCursor());

        WriteCmd(SCMD_CALLEXT, SREG_AX); // Do the call
        // At runtime, we will arrive here when the function call has returned: Restore the stack
        if (num_args > 0)
            WriteCmd(SCMD_SUBREALSTACK, num_args);
        return;
    }

    // Func is non-import
    _scrip.fixup_previous(kFx_Code);
    if (_fcm.IsForwardDecl(name_of_func))
        _fcm.TrackForwardDeclFuncCall(name_of_func, _scrip.codesize - 1, _src.GetCursor());

    WriteCmd(SCMD_CALL, SREG_AX);  // Do the call

    // At runtime, we will arrive here when the function call has returned: Restore the stack
    if (num_args > 0)
    {
        size_t const size_of_passed_args = num_args * SIZE_OF_STACK_CELL;
        WriteCmd(SCMD_SUB, SREG_SP, size_of_passed_args);
        _scrip.offset_to_local_var_block -= size_of_passed_args;
    }
}

// We are processing a function call.
// Get the parameters of the call and push them onto the stack.
ErrorType AGS::Parser::AccessData_PushFunctionCallParams(Symbol name_of_func, bool func_is_import, SrcList &parameters, size_t &actual_num_args)
{
    size_t const num_func_args = _sym[name_of_func].GetNumOfFuncParams();

    size_t num_supplied_args = 0;
    size_t closed_paren_idx;
    ErrorType retval = AccessData_FunctionCall_CountAndCheckParm(parameters, name_of_func, closed_paren_idx, num_supplied_args);
    if (retval < 0) return retval;

    // Push default parameters onto the stack when applicable
    // This will give an error if there aren't enough default parameters
    if (num_supplied_args < num_func_args)
    {
        retval = AccessData_FunctionCall_ProvideDefaults(num_func_args, num_supplied_args, name_of_func, func_is_import);
        if (retval < 0) return retval;
    }
    if (num_supplied_args > num_func_args && !_sym[name_of_func].IsVarargsFunc())
    {
        Error("Expected just %d parameters but found %d", num_func_args, num_supplied_args);
        return kERR_UserError;
    }
    // ASSERT at this point, the number of parameters is okay

    // Push the explicit arguments of the function
    if (num_supplied_args > 0)
    {
        retval = AccessData_FunctionCall_PushParams(parameters, closed_paren_idx, num_func_args, num_supplied_args, name_of_func, func_is_import);
        if (retval < 0) return retval;
    }

    actual_num_args = std::max(num_supplied_args, num_func_args);
    parameters.SetCursor(closed_paren_idx + 1); // Go to the end of the parameter list
    return kERR_None;
}

ErrorType AGS::Parser::AccessData_FunctionCall(Symbol name_of_func, SrcList &expression, MemoryLocation &mloc, Vartype &rettype)
{
    if (kKW_OpenParenthesis != expression[1])
    {
        Error("Expected '('");
        return kERR_UserError;
    }

    expression.EatFirstSymbol();

    bool const func_is_import = _sym[name_of_func].TypeQualifiers[TQ::kImport];
    // If function uses normal stack, we need to do stack calculations to get at certain elements
    bool const func_uses_normal_stack = !func_is_import;
    bool const called_func_uses_this =
        std::string::npos != _sym.GetName(name_of_func).find("::") &&
        !_sym[name_of_func].TypeQualifiers[TQ::kStatic];
    bool const calling_func_uses_this = (0 != _sym.GetVartype(kKW_This));
    bool mar_pushed = false;
    bool op_pushed = false;

    if (calling_func_uses_this)
    {
        // Save OP since we need it after the func call
        // We must do this no matter whether the callED function itself uses "this"
        // because a called function that doesn't might call a function that does.
        PushReg(SREG_OP);
        op_pushed = true;
    }

    if (called_func_uses_this)
    {
        // MAR contains the address of "outer"; this is what will be used for "this" in the called function.
        ErrorType retval = mloc.MakeMARCurrent(_src.GetLineno(), _scrip);
        if (retval < 0) return retval;

        // Parameter processing might entail calling yet other functions, e.g., in "f(...g(x)...)".
        // So we can't emit SCMD_CALLOBJ here, before parameters have been processed.
        // Save MAR because parameter processing might clobber it 
        PushReg(SREG_MAR);
        mar_pushed = true;
    }

    size_t num_args = 0;
    ErrorType retval = AccessData_PushFunctionCallParams(name_of_func, func_is_import, expression, num_args);
    if (retval < 0) return retval;

    if (called_func_uses_this)
    {
        if (0 == num_args)
        {   // MAR must still be current, so undo the unneeded PUSH above.
            _scrip.offset_to_local_var_block -= SIZE_OF_STACK_CELL;
            _scrip.codesize -= 2;
            mar_pushed = false;
        }
        else
        {   // Recover the value of MAR from the stack. It's in front of the parameters.
            WriteCmd(
                SCMD_LOADSPOFFS,
                (1 + (func_uses_normal_stack ? num_args : 0)) * SIZE_OF_STACK_CELL);
            WriteCmd(SCMD_MEMREAD, SREG_MAR);
        }
        WriteCmd(SCMD_CALLOBJ, SREG_MAR);
    }

    AccessData_GenerateFunctionCall(name_of_func, num_args, func_is_import);

    // function return type
    rettype = _scrip.ax_vartype = _sym[name_of_func].FuncParamVartypes[0];
    _scrip.ax_scope_type = ScT::kLocal;

    if (mar_pushed)
        PopReg(SREG_MAR);
    if (op_pushed)
        PopReg(SREG_OP);

    MarkAcessed(name_of_func);
    return kERR_None;
}

ErrorType AGS::Parser::ParseExpression_NoOps(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, AGS::Vartype &vartype)
{
    Symbol const first_sym = expression[0];
    SymbolType const first_sym_type = _sym.GetSymbolType(first_sym);
    if (kKW_OpenParenthesis == first_sym)
        return ParseExpression_InParens(expression, vloc, scope_type, vartype);

    if (SymT::kOperator != first_sym_type)
        return AccessData(false, expression, vloc, scope_type, vartype);

    Error("Expected '(' or an operator, found '%s' instead", _sym.GetName(first_sym).c_str());
    return kERR_UserError;
}

ErrorType AGS::Parser::ParseExpression_Term(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, AGS::Vartype &vartype)
{
    if (expression.Length() == 0)
    {
        Error("!Cannot parse empty subexpression");
        return kERR_InternalError;
    }

    Symbol const first_sym = expression[0];
    if (kKW_CloseParenthesis == first_sym || kKW_CloseBracket == first_sym || kKW_CloseBrace == first_sym)
    {   // Shouldn't happen: the scanner sees to it that nesting symbols match
        Error(
            "!Unexpected '%s' at start of expression",
            _sym.GetName(first_sym).c_str());
        return kERR_InternalError;
    }

    int least_binding_op_idx;
    ErrorType retval = IndexOfLeastBondingOperator(expression, least_binding_op_idx);  // can be < 0
    if (retval < 0) return retval;

    if (0 == least_binding_op_idx)
        retval = ParseExpression_Unary(expression, vloc, scope_type, vartype);
    else if (0 < least_binding_op_idx)
        retval = ParseExpression_BinaryOrTernary(static_cast<size_t>(least_binding_op_idx), expression, vloc, scope_type, vartype);
    else
        retval = ParseExpression_NoOps(expression, vloc, scope_type, vartype);
    if (retval < 0) return retval;

    if (!expression.ReachedEOF())
    {
        // e.g. "4 3" or "(5) 3".
        // This is most probably due to the user having forgotten an operator
        Error(
            "Expected an operator, found '%s' instead",
            _sym.GetName(expression.GetNext()).c_str());
        return kERR_UserError;
    }
    return HandleStructOrArrayResult(vartype, vloc);
}

// symlist starts a sequence of bracketed expressions; parse it
ErrorType AGS::Parser::AccessData_ReadIntExpression(SrcList &expression)
{
    ValueLocation vloc;
    ScopeType scope_type;
    Vartype vartype;
    ErrorType retval = ParseExpression_Term(expression, vloc, scope_type, vartype);
    if (retval < 0) return retval;
    retval = ResultToAX(vloc, scope_type, vartype);
    if (retval < 0) return retval;

    return IsVartypeMismatch(vartype, kKW_Int, true);
}

// We access a variable or a component of a struct in order to read or write it.
// This is a simple member of the struct.
ErrorType AGS::Parser::AccessData_StructMember(AGS::Symbol component, bool writing, bool access_via_this, SrcList &expression, AGS::Parser::MemoryLocation &mloc, AGS::Vartype &vartype)
{
    expression.GetNext(); // Eat component
    SymbolTableEntry &entry = _sym[component];

    if (writing && entry.TypeQualifiers[TQ::kWriteprotected] && !access_via_this)
    {
        Error(
            "Writeprotected component '%s' must not be modified from outside",
            _sym.GetName(component).c_str());
        return kERR_UserError;
    }
    if (entry.TypeQualifiers[TQ::kProtected] && !access_via_this)
    {
        Error(
            "Protected component '%s' must not be accessed from outside",
            _sym.GetName(component).c_str());
        return kERR_UserError;
    }

    mloc.AddComponentOffset(entry.SOffset);
    vartype = _sym.GetVartype(component);
    return kERR_None;
}

// Get the symbol for the get or set function corresponding to the attribute given.
ErrorType AGS::Parser::ConstructAttributeFuncName(AGS::Symbol attribsym, bool writing, bool indexed, AGS::Symbol &func)
{
    std::string member_str = _sym.GetName(attribsym);
    // If "::" in the name, take the part after the last "::"
    size_t const m_access_position = member_str.rfind("::");
    if (std::string::npos != m_access_position)
        member_str = member_str.substr(m_access_position + 2);
    char const *stem_str = writing ? "set" : "get";
    char const *indx_str = indexed ? "i_" : "_";
    std::string func_str = stem_str + (indx_str + member_str);
    func = _sym.FindOrAdd(func_str);
    return kERR_None;
}

// We call the getter or setter of an attribute
ErrorType AGS::Parser::AccessData_CallAttributeFunc(bool is_setter, SrcList &expression, AGS::Vartype &vartype)
{
    Symbol const component = expression.GetNext();
    Symbol const struct_of_component =
        FindStructOfComponent(vartype, component);
    if (0 == struct_of_component)
    {
        Error(
            "Struct '%s' does not have an attribute named '%s'",
            _sym.GetName(vartype).c_str(),
            _sym.GetName(component).c_str());
        return kERR_UserError;
    }
    Symbol const name_of_attribute = MangleStructAndComponent(struct_of_component, component);

    bool const attrib_uses_this =
        !_sym[name_of_attribute].TypeQualifiers[TQ::kStatic];
    bool const call_is_indexed =
        (kKW_OpenBracket == expression.PeekNext());
    bool const attrib_is_indexed =
        _sym.IsDynarrayVartype(name_of_attribute);

    if (call_is_indexed && !attrib_is_indexed)
    {
        Error("Unexpected '[' after non-indexed attribute %s", _sym.GetName(name_of_attribute).c_str());
        return kERR_UserError;
    }
    else if (!call_is_indexed && attrib_is_indexed)
    {
        Error("'[' expected after indexed attribute but not found", _sym.GetName(name_of_attribute).c_str());
        return kERR_UserError;
    }

    // Get the appropriate access function (as a symbol)
    Symbol name_of_func = kKW_NoSymbol;
    ErrorType retval = ConstructAttributeFuncName(component, is_setter, attrib_is_indexed, name_of_func);
    if (retval < 0) return retval;
    name_of_func = MangleStructAndComponent(struct_of_component, name_of_func);
    if (name_of_func < 0) return retval;

    bool const func_is_import = _sym[name_of_func].TypeQualifiers[TQ::kImport];

    if (attrib_uses_this)
        PushReg(SREG_OP); // is the current this ptr, must be restored after call

    size_t num_of_args = 0;
    if (is_setter)
    {
        if (func_is_import)
            WriteCmd(SCMD_PUSHREAL, SREG_AX);
        else
            PushReg(SREG_AX);
        ++num_of_args;
    }

    if (call_is_indexed)
    {
        // The index to be set is in the [...] clause; push it as the first parameter
        if (attrib_uses_this)
            PushReg(SREG_MAR); // must not be clobbered
        retval = AccessData_ReadBracketedIntExpression(expression);
        if (retval < 0) return retval;

        if (attrib_uses_this)
            PopReg(SREG_MAR);

        if (func_is_import)
            WriteCmd(SCMD_PUSHREAL, SREG_AX);
        else
            PushReg(SREG_AX);
        ++num_of_args;
    }

    if (attrib_uses_this)
        WriteCmd(SCMD_CALLOBJ, SREG_MAR); // make MAR the new this ptr

    AccessData_GenerateFunctionCall(name_of_func, num_of_args, func_is_import);

    if (attrib_uses_this)
        PopReg(SREG_OP); // restore old this ptr after the func call

    // attribute return type
    _scrip.ax_scope_type = ScT::kLocal;
    _scrip.ax_vartype = vartype = _sym[name_of_func].FuncParamVartypes[0];

    MarkAcessed(name_of_func);
    return kERR_None;
}


// Location contains a pointer to another address. Get that address.
ErrorType AGS::Parser::AccessData_Dereference(ValueLocation &vloc, AGS::Parser::MemoryLocation &mloc)
{
    if (kVL_AX_is_value == vloc)
    {
        WriteCmd(SCMD_REGTOREG, SREG_AX, SREG_MAR);
        WriteCmd(SCMD_CHECKNULL);
        vloc = kVL_MAR_pointsto_value;
        mloc.Reset();
    }
    else
    {
        ErrorType retval = mloc.MakeMARCurrent(_src.GetLineno(), _scrip);
        if (retval < 0) return retval;
        // Note: We need to check here whether m[MAR] == 0, but CHECKNULL
        // checks whether MAR == 0. So we need to do MAR := m[MAR] first.
        WriteCmd(SCMD_MEMREADPTR, SREG_MAR);
        WriteCmd(SCMD_CHECKNULL);
    }
    return kERR_None;
}

ErrorType AGS::Parser::AccessData_ProcessArrayIndexConstant(size_t idx, Symbol index_symbol, bool negate, size_t num_array_elements, size_t element_size, MemoryLocation &mloc)
{
    int array_index = -1;
    std::string msg = "Error parsing array index #<1>";
    msg.replace(msg.find("<1>"), 3, std::to_string(idx + 1));
    ErrorType retval = IntLiteralOrConst2Value(index_symbol, negate, msg.c_str(), array_index);
    if (retval < 0) return retval;
    if (array_index < 0)
    {
        Error(
            "Array index #%u is %d, thus out of bounds (minimum is 0)",
            idx + 1u,
            array_index);
        return kERR_UserError;
    }
    if (num_array_elements > 0 && static_cast<size_t>(array_index) >= num_array_elements)
    {
        Error(
            "Array index #%u is %d, thus out of bounds (maximum is %u)",
            idx + 1u,
            array_index,
            num_array_elements - 1u);
        return kERR_UserError;
    }

    mloc.AddComponentOffset(array_index * element_size);
    return kERR_None;
}

ErrorType AGS::Parser::AccessData_ProcessCurrentArrayIndex(size_t idx, size_t dim, size_t factor, bool is_dynarray, SrcList &expression, MemoryLocation &mloc)
{
    // Get the index
    size_t const index_start = expression.GetCursor();
    SkipTo(SymbolList{ kKW_Comma, kKW_CloseBracket }, expression);
    size_t const index_end = expression.GetCursor();
    SrcList current_index = SrcList(expression, index_start, index_end - index_start);
    if (0 == current_index.Length())
    {
        Error("Empty array index is not supported");
        return kERR_UserError;
    }

    // If the index is a literal or constant or a negation thereof, process it at compile time
    if (1 == current_index.Length())
    {
        Symbol const index_sym = current_index[0];
        SymbolType const index_sym_type = _sym.GetSymbolType(index_sym);
        if (SymT::kLiteralInt == index_sym_type || SymT::kConstant == index_sym_type)
            return AccessData_ProcessArrayIndexConstant(idx, index_sym, false, dim, factor, mloc);
    }
    if (2 == current_index.Length())
    {
        Symbol const op_sym = current_index[0];
        Symbol const index_sym = current_index[1];
        SymbolType const index_sym_type = _sym.GetSymbolType(index_sym);
        if (SymT::kOperator == _sym[op_sym].SType && SCMD_SUBREG ==_sym[op_sym].OperatorOpcode &&
            (SymT::kLiteralInt == index_sym_type || SymT::kConstant == index_sym_type))
            return AccessData_ProcessArrayIndexConstant(idx, index_sym, true, dim, factor, mloc);
    }

    ErrorType retval = mloc.MakeMARCurrent(_src.GetLineno(), _scrip);
    if (retval < 0) return retval;
    PushReg(SREG_MAR);
    
    retval = AccessData_ReadIntExpression(current_index);
    if (retval < 0) return retval;

    PopReg(SREG_MAR);
    

    // Note: DYNAMICBOUNDS compares the offset into the memory block;
    // it mustn't be larger than the size of the allocated memory. 
    // On the other hand, CHECKBOUNDS checks the index; it mustn't be
    //  larger than the maximum given. So dynamic bounds must be checked
    // after the multiplication; static bounds before the multiplication.
    // For better error messages at runtime, don't do CHECKBOUNDS after the multiplication.
    if (!is_dynarray)
        WriteCmd(SCMD_CHECKBOUNDS, SREG_AX, dim);
    if (factor != 1)
        WriteCmd(SCMD_MUL, SREG_AX, factor);
    if (is_dynarray)
        WriteCmd(SCMD_DYNAMICBOUNDS, SREG_AX);
    WriteCmd(SCMD_ADDREG, SREG_MAR, SREG_AX);
    return kERR_None;
}

// We're processing some struct component or global or local variable.
// If an array index follows, parse it and shorten symlist accordingly
ErrorType AGS::Parser::AccessData_ProcessAnyArrayIndex(ValueLocation vloc_of_array, SrcList &expression, ValueLocation &vloc, AGS::Parser::MemoryLocation &mloc, AGS::Vartype &vartype)
{
    if (kKW_OpenBracket != expression.PeekNext())
        return kERR_None;
    expression.GetNext(); // Eat '['

    bool const is_dynarray = _sym.IsDynarrayVartype(vartype);
    bool const is_array = _sym.IsArrayVartype(vartype);
    if (!is_dynarray && !is_array)
    {
        Error("Array index is only legal after an array expression");
        return kERR_UserError;
    }

    Vartype const element_vartype = _sym[vartype].Vartype;
    size_t const element_size = _sym.GetSize(element_vartype);
    std::vector<size_t> dim_sizes;
    std::vector<size_t> dynarray_dims = { 0, };
    std::vector<size_t> &dims = is_dynarray ? dynarray_dims : _sym[vartype].Dims;
    vartype = element_vartype;

    if (is_dynarray)
        AccessData_Dereference(vloc, mloc);

    // Number of dimensions and the the size of the dimension for each dimension
    size_t const num_of_dims = dims.size();
    dim_sizes.resize(num_of_dims);
    size_t factor = element_size;
    for (int dim_idx = num_of_dims - 1; dim_idx >= 0; dim_idx--) // yes, "int"
    {
        dim_sizes[dim_idx] = factor;
        factor *= dims[dim_idx];
    }

    for (size_t dim_idx = 0; dim_idx < num_of_dims; dim_idx++)
    {
        ErrorType retval = AccessData_ProcessCurrentArrayIndex(dim_idx, dims[dim_idx], dim_sizes[dim_idx], is_dynarray, expression, mloc);
        if (retval < 0) return retval;

        Symbol divider = expression.PeekNext();
        retval = Expect(SymbolList{ kKW_CloseBracket, kKW_Comma }, divider);
        if (retval < 0) return retval;

        if (kKW_CloseBracket == divider)
        {
            expression.GetNext(); // Eat ']'
            divider = expression.PeekNext();
        }
        if (kKW_Comma == divider || kKW_OpenBracket == divider)
        {
            if (num_of_dims == dim_idx + 1)
            {
                Error("Expected %d indexes, found more", num_of_dims);
                return kERR_UserError;
            }
            expression.GetNext(); // Eat ',' or '['
            continue;
        }
        if (num_of_dims != dim_idx + 1)
        {
            Error("Expected %d indexes, but only found %d", num_of_dims, dim_idx + 1);
            return kERR_UserError;
        }
    }
    return kERR_None;
}

ErrorType AGS::Parser::AccessData_GlobalOrLocalVar(bool is_global, bool writing, SrcList &expression, AGS::Parser::MemoryLocation &mloc, AGS::Vartype &vartype)
{
    Symbol varname = expression.GetNext();
    SymbolTableEntry &entry = _sym[varname];
    CodeCell const soffs = entry.SOffset;

    if (writing && entry.TypeQualifiers[TQ::kReadonly])
    {
        Error("Cannot write to readonly '%s'", _sym.GetName(varname).c_str());
        return kERR_UserError;
    }

    if (entry.TypeQualifiers[TQ::kImport])
        mloc.SetStart(ScT::kImport, soffs);
    else
        mloc.SetStart(is_global ? ScT::kGlobal : ScT::kLocal, soffs);

    vartype = _sym.GetVartype(varname);

    // Process an array index if it follows
    ValueLocation vl_dummy = kVL_MAR_pointsto_value;
    return AccessData_ProcessAnyArrayIndex(kVL_MAR_pointsto_value, expression, vl_dummy, mloc, vartype);
}

ErrorType AGS::Parser::AccessData_Static(SrcList &expression, MemoryLocation &mloc, AGS::Vartype &vartype)
{
    vartype = expression[0];
    expression.EatFirstSymbol(); // Eat vartype
    mloc.Reset();
    return kERR_None;
}

ErrorType AGS::Parser::AccessData_FloatLiteral(bool negate, SrcList &expression, AGS::Vartype &vartype)
{
    float f;
    ErrorType retval = String2Float(_sym.GetName(expression.GetNext()), f);
    if (retval < 0) return retval;

    if (negate)
        f = -f;
    int const i = InterpretFloatAsInt(f);

    WriteCmd(SCMD_LITTOREG, SREG_AX, i);
    _scrip.ax_vartype = vartype = kKW_Float;
    _scrip.ax_scope_type = ScT::kGlobal;
    return kERR_None;
}

ErrorType AGS::Parser::AccessData_IntLiteralOrConst(bool negate, SrcList &expression, AGS::Vartype &vartype)
{
    int literal;
    
    ErrorType retval = IntLiteralOrConst2Value(expression.GetNext(), negate, "Error parsing integer value", literal);
    if (retval < 0) return retval;

    WriteCmd(SCMD_LITTOREG, SREG_AX, literal);
    _scrip.ax_vartype = vartype = kKW_Int;
    _scrip.ax_scope_type = ScT::kGlobal;
    return kERR_None;
}

ErrorType AGS::Parser::AccessData_Null(SrcList &expression, AGS::Vartype &vartype)
{
    expression.GetNext(); // Eat 'null'

    WriteCmd(SCMD_LITTOREG, SREG_AX, 0);
    _scrip.ax_vartype = vartype = kKW_Null;
    _scrip.ax_scope_type = ScT::kGlobal;

    return kERR_None;
}

ErrorType AGS::Parser::AccessData_StringLiteral(SrcList &expression, AGS::Vartype &vartype)
{
    WriteCmd(SCMD_LITTOREG, SREG_AX, _sym[expression.GetNext()].SOffset);
    _scrip.fixup_previous(kFx_String);
    _scrip.ax_vartype = vartype = _sym.VartypeWith(VTT::kConst, kKW_String);

    return kERR_None;
}

ErrorType AGS::Parser::AccessData_FirstClause(bool writing, SrcList &expression, ValueLocation &vloc, ScopeType &return_scope_type, AGS::Parser::MemoryLocation &mloc, AGS::Vartype &vartype, bool &implied_this_dot, bool &static_access)
{
    if (expression.Length() < 1)
    {
        Error("!Empty variable");
        return kERR_InternalError;
    }
    expression.StartRead();

    implied_this_dot = false;

    Symbol const first_sym = expression.PeekNext();

    if (kKW_This == first_sym)
    {
        expression.GetNext(); // Eat 'this'
        vartype = _sym.GetVartype(kKW_This);
        if (0 == vartype)
        {
            Error("'this' is only legal in non-static struct functions");
            return kERR_UserError;
        }
        vloc = kVL_MAR_pointsto_value;
        WriteCmd(SCMD_REGTOREG, SREG_OP, SREG_MAR);
        WriteCmd(SCMD_CHECKNULL);
        mloc.Reset();
        if (kKW_Dot == expression.PeekNext())
        {
            expression.GetNext(); // Eat '.'
            // Going forward, we must "imply" "this." since we've just gobbled it.
            implied_this_dot = true;
        }
        
        return kERR_None;
    }

    switch (_sym.GetSymbolType(first_sym))
    {
    default:
    {
        // If this unknown symbol can be interpreted as a component of this,
        // treat it that way.
        vartype = _sym.GetVartype(kKW_This);
        Symbol const thiscomponent = MangleStructAndComponent(vartype, first_sym);
        if (SymT::kNoType != _sym[thiscomponent].SType)
        {            
            vloc = kVL_MAR_pointsto_value;
            WriteCmd(SCMD_REGTOREG, SREG_OP, SREG_MAR);
            WriteCmd(SCMD_CHECKNULL);
            mloc.Reset();

            // Going forward, the code should imply "this."
            // with the '.' already read in.
            implied_this_dot = true;
            // Then the component needs to be read again.
            expression.BackUp();
            return kERR_None;
        }

        Error("Unexpected '%s'", _sym.GetName(expression.GetNext()).c_str());
        return kERR_UserError;
    }

    case SymT::kConstant:
        if (writing) break; // to error msg
        return_scope_type = ScT::kGlobal;
        vloc = kVL_AX_is_value;
        return AccessData_IntLiteralOrConst(false, expression, vartype);

    case SymT::kFunction:
    {
        return_scope_type = ScT::kGlobal;
        vloc = kVL_AX_is_value;
        ErrorType retval = AccessData_FunctionCall(first_sym, expression, mloc, vartype);
        if (retval < 0) return retval;
        if (_sym.IsDynarrayVartype(vartype))
            return AccessData_ProcessAnyArrayIndex(vloc, expression, vloc, mloc, vartype);
        return kERR_None;
    }

    case SymT::kGlobalVar:
    {
        return_scope_type = ScT::kGlobal;
        vloc = kVL_MAR_pointsto_value;
        bool const is_global = true;
        MarkAcessed(first_sym);
        return AccessData_GlobalOrLocalVar(is_global, writing, expression, mloc, vartype);
    }

    case SymT::kKeyword:
        if (writing) break; // to error msg
        if (kKW_Null != first_sym) break; // to error msg
        return_scope_type = ScT::kGlobal;
        vloc = kVL_AX_is_value;
        return AccessData_Null(expression, vartype);

    case SymT::kLiteralFloat:
        if (writing) break; // to error msg
        return_scope_type = ScT::kGlobal;
        vloc = kVL_AX_is_value;
        return AccessData_FloatLiteral(false, expression, vartype);

    case SymT::kLiteralInt:
        if (writing) break; // to error msg
        return_scope_type = ScT::kGlobal;
        vloc = kVL_AX_is_value;
        return AccessData_IntLiteralOrConst(false, expression, vartype);

    case SymT::kLiteralString:
        if (writing) break; // to error msg
        return_scope_type = ScT::kGlobal;
        vloc = kVL_AX_is_value;
        return AccessData_StringLiteral(expression, vartype);

    case SymT::kLocalVar:
    {
        // Parameters can be returned although they are local because they are allocated
        // outside of the function proper. The return scope type for them is global.
        return_scope_type = _sym[first_sym].IsParameter() ? ScT::kGlobal : ScT::kLocal;
        vloc = kVL_MAR_pointsto_value;
        bool const is_global = false;
        return AccessData_GlobalOrLocalVar(is_global, writing, expression, mloc, vartype);
    }

    case SymT::kVartype:
        return_scope_type = ScT::kGlobal;
        static_access = true;
        return AccessData_Static(expression, mloc, vartype);
    }

    Error("Cannot assign a value to '%s'", _sym.GetName(expression[0]).c_str());
    return kERR_UserError;
}

// We're processing a STRUCT.STRUCT. ... clause.
// We've already processed some structs, and the type of the last one is vartype.
// Now we process a component of vartype.
ErrorType AGS::Parser::AccessData_SubsequentClause(bool writing, bool access_via_this, bool static_access, SrcList &expression, ValueLocation &vloc, ScopeType &return_scope_type, MemoryLocation &mloc, AGS::Vartype &vartype)
{
    Symbol const next_sym = expression.PeekNext();

    Symbol const component = FindComponentInStruct(vartype, next_sym);
    SymbolType const component_type = (component) ? _sym.GetSymbolType(component) : SymT::kNoType;

    if (static_access && !_sym[component].TypeQualifiers[TQ::kStatic])
    {
        Error("Must specify a specific struct for non-static component %s", _sym.GetName(component).c_str());
        return kERR_UserError;
    }

    ErrorType retval;
    switch (component_type)
    {
    default:
        Error(
            "Expected a component of '%s', found '%s' instead",
            _sym.GetName(vartype).c_str(),
            _sym.GetName(next_sym).c_str());
        return kERR_UserError;

    case SymT::kAttribute:
    {   // make MAR point to the struct of the attribute
        retval = mloc.MakeMARCurrent(_src.GetLineno(), _scrip);
        if (retval < 0) return retval;
        if (writing)
        {
            // We cannot process the attribute here so return to the assignment that
            // this attribute was originally called from
            vartype = _sym.GetVartype(component);
            vloc = kVL_Attribute;
            return kERR_None;
        }
        vloc = kVL_AX_is_value;
        bool const is_setter = false;
        return AccessData_CallAttributeFunc(is_setter, expression, vartype);
    }

    case SymT::kFunction:
    {
        vloc = kVL_AX_is_value;
        return_scope_type = ScT::kLocal;
        SrcList start_of_funccall = SrcList(expression, expression.GetCursor(), expression.Length());
        retval = AccessData_FunctionCall(component, start_of_funccall, mloc, vartype);
        if (retval < 0) return retval;
        if (_sym.IsDynarrayVartype(vartype))
            return AccessData_ProcessAnyArrayIndex(vloc, expression, vloc, mloc, vartype);
        return kERR_None;
    }

    case SymT::kStructComponent:
        vloc = kVL_MAR_pointsto_value;
        retval = AccessData_StructMember(component, writing, access_via_this, expression, mloc, vartype);
        if (retval < 0) return retval;
        return AccessData_ProcessAnyArrayIndex(vloc, expression, vloc, mloc, vartype);
    }

    // Can't reach
}

Symbol AGS::Parser::FindStructOfComponent(Vartype strct, Symbol component)
{
    do
    {
        Symbol symb = MangleStructAndComponent(strct, component);
        if (SymT::kNoType != _sym.GetSymbolType(symb))
            return strct;
        strct = _sym[strct].Parent;
    }
    while (strct > 0);
    return 0;
}

Symbol AGS::Parser::FindComponentInStruct(AGS::Vartype strct, AGS::Symbol component)
{
    do
    {
        Symbol ret = MangleStructAndComponent(strct, component);
        if (SymT::kNoType != _sym.GetSymbolType(ret))
            return ret;
        strct = _sym[strct].Parent;
    }
    while (strct > 0);
    return 0;
}

// We are in a STRUCT.STRUCT.STRUCT... cascade.
// Check whether we have passed the last dot
ErrorType AGS::Parser::AccessData_IsClauseLast(SrcList &expression, bool &is_last)
{
    size_t const cursor = expression.GetCursor();
    SkipTo(SymbolList{ kKW_Dot },  expression);
    is_last = (kKW_Dot != expression.PeekNext());
    expression.SetCursor(cursor);
    return kERR_None;
}

// Access a variable, constant, literal, func call, struct.component.component cascade, etc.
// Result is in AX or m[MAR], dependent on vloc. Type is in vartype.
// At end of function, symlist and symlist_len will point to the part of the symbol string
// that has not been processed yet
// NOTE: If this selects an attribute for writing, then the corresponding function will
// _not_ be called and symlist[0] will be the attribute.
ErrorType AGS::Parser::AccessData(bool writing, SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, AGS::Vartype &vartype)
{
    if (0 == expression.Length())
    {
        Error("!empty expression");
        return kERR_InternalError;
    }
    
    // For memory accesses, we set the MAR register lazily so that we can
    // accumulate offsets at runtime instead of compile time.
    // This struct tracks what we will need to do to set the MAR register.
    MemoryLocation mloc = MemoryLocation(*this);

    bool clause_is_last = false;
    ErrorType retval = AccessData_IsClauseLast(expression, clause_is_last);
    if (retval < 0) return retval;

    bool implied_this_dot = false; // only true when "this." is implied
    bool static_access = false; // only true when a vartype has just been parsed

    // If we are reading, then all the accesses are for reading.
    // If we are writing, then all the accesses except for the last one
    // are for reading and the last one will be for writing.
    retval = AccessData_FirstClause((writing && clause_is_last), expression, vloc, scope_type, mloc, vartype, implied_this_dot, static_access);
    if (retval < 0) return retval;

    Vartype outer_vartype = 0;

    // If the previous function has assumed a "this." that isn't there,
    // then a '.' won't be coming up but the while body must be executed anyway.
    while (kKW_Dot == expression.PeekNext() || implied_this_dot)
    {
        if (!implied_this_dot)
            expression.GetNext(); // Eat '.'
        // Note: do not reset "implied_this_dot" here, it's still needed.

        // Here, if kVL_MAR_pointsto_value == vloc then the first byte of outer is at m[MAR + mar_offset].
        // We accumulate mar_offset at compile time as long as possible to save computing.
        outer_vartype = vartype;

        // Note: A DynArray can't be directly in front of a '.' (need a [...] first)
        if (_sym.IsDynpointerVartype(vartype))
        {
            retval = AccessData_Dereference(vloc, mloc);
            if (retval < 0) return retval;
            vartype = _sym.VartypeWithout(VTT::kDynpointer, vartype);
        }

        if (!_sym.IsStructVartype(vartype) || !_sym.IsAtomic(vartype))
        {
            if (_sym.IsArrayVartype(vartype) || _sym.IsDynarrayVartype(vartype))
                Error("Expected a struct in front of '.' but found an array instead");
            else        
                Error(
                    "Expected a struct in front of '.' but found an expression of type '%s' instead",
                    _sym.GetName(outer_vartype).c_str());
            return kERR_UserError;
        }

        if (expression.ReachedEOF())
        {
            Error("Expected struct component after '.' but did not find it");
            return kERR_UserError;
        }

        retval = AccessData_IsClauseLast(expression, clause_is_last);
        if (retval < 0) return retval;

        // If we are reading, then all the accesses are for reading.
        // If we are writing, then all the accesses except for the last one
        // are for reading and the last one will be for writing.
        retval = AccessData_SubsequentClause((clause_is_last && writing), implied_this_dot, static_access, expression, vloc, scope_type, mloc, vartype);
        if (retval < 0) return retval;

        // Next component access, if there is any, is dependent on
        // the current access, no longer on "this".
        implied_this_dot = false;
        // Next component access, if there is any, won't be static.
        static_access = false;
    }

    if (kVL_Attribute == vloc)
    {
        // Caller will do the assignment
        // For this to work, the caller must know the type of the struct
        // in which the attribute resides
        vartype = _sym.BaseVartype(outer_vartype);
        return kERR_None;
    }

    if (kVL_AX_is_value == vloc)
    {
        _scrip.ax_vartype = vartype;
        _scrip.ax_scope_type = scope_type;
        return kERR_None;
    }

    return mloc.MakeMARCurrent(_src.GetLineno(), _scrip);
}

// In order to avoid push AX/pop AX, find out common cases that don't clobber AX
bool AGS::Parser::AccessData_MayAccessClobberAX(SrcList &expression)
{
    SymbolType const type_of_first = _sym.GetSymbolType(expression[0]);
    if (SymT::kGlobalVar != type_of_first && SymT::kLocalVar != type_of_first)
        return true;

    if (1 == expression.Length())
        return false;

    for (size_t idx = 0; idx < expression.Length() - 3; idx += 2)
    {
        if (kKW_Dot != expression[idx + 1])
            return true;
        Symbol const compo = MangleStructAndComponent(expression[0], expression[2]);
        if (SymT::kStructComponent != _sym.GetSymbolType(compo))
            return true;
    }
    return false;
}

// Insert Bytecode for:
// Copy at most OLDSTRING_LENGTH - 1 bytes from m[MAR...] to m[AX...]
// Stop when encountering a 0
void AGS::Parser::AccessData_StrCpy()
{
    WriteCmd(SCMD_REGTOREG, SREG_AX, SREG_CX); // CX = dest
    WriteCmd(SCMD_REGTOREG, SREG_MAR, SREG_BX); // BX = src
    WriteCmd(SCMD_LITTOREG, SREG_DX, STRINGBUFFER_LENGTH - 1); // DX = count
    CodeLoc const loop_start = _scrip.codesize; // Label LOOP_START
    WriteCmd(SCMD_REGTOREG, SREG_BX, SREG_MAR); // AX = m[BX]
    WriteCmd(SCMD_MEMREAD, SREG_AX);
    WriteCmd(SCMD_REGTOREG, SREG_CX, SREG_MAR); // m[CX] = AX
    WriteCmd(SCMD_MEMWRITE, SREG_AX);
    WriteCmd(SCMD_JZ, -77);  // if (AX == 0) jumpto LOOP_END
    CodeLoc const jumpout_pos = _scrip.codesize - 1;
    WriteCmd(SCMD_ADD, SREG_BX, 1); // BX++, CX++, DX--
    WriteCmd(SCMD_ADD, SREG_CX, 1);
    WriteCmd(SCMD_SUB, SREG_DX, 1);
    WriteCmd(SCMD_REGTOREG, SREG_DX, SREG_AX); // if (DX != 0) jumpto LOOP_START
    WriteCmd(
        SCMD_JNZ,
        ccCompiledScript::RelativeJumpDist(_scrip.codesize + 1, loop_start));
    WriteCmd(SCMD_ADD, SREG_CX, 1); // Force a 0-terminated dest string
    WriteCmd(SCMD_REGTOREG, SREG_CX, SREG_MAR);
    WriteCmd(SCMD_LITTOREG, SREG_AX, 0);
    WriteCmd(SCMD_MEMWRITE, SREG_AX);
    CodeLoc const loop_end = _scrip.codesize; // Label LOOP_END
    _scrip.code[jumpout_pos] = ccCompiledScript::RelativeJumpDist(jumpout_pos, loop_end);
}

// We are typically in an assignment LHS = RHS; the RHS has already been
// evaluated, and the result of that evaluation is in AX.
// Store AX into the memory location that corresponds to LHS, or
// call the attribute function corresponding to LHS.
ErrorType AGS::Parser::AccessData_AssignTo(SrcList &expression)
{
    // We'll evaluate expression later on which moves the cursor,
    // so save it here and restore later on
    size_t const end_of_rhs_cursor = _src.GetCursor();

    // AX contains the result of evaluating the RHS of the assignment
    // Save on the stack so that it isn't clobbered
    Vartype rhsvartype = _scrip.ax_vartype;
    ScopeType rhs_scope_type = _scrip.ax_scope_type;
    // Save AX unless we are sure that it won't be clobbered
    bool const may_clobber = AccessData_MayAccessClobberAX(expression);
    if (may_clobber)
        PushReg(SREG_AX);

    bool const writing = true;
    ValueLocation vloc;
    Vartype lhsvartype;
    ScopeType lhs_scope_type;
    ErrorType retval = AccessData(writing, expression, vloc, lhs_scope_type, lhsvartype);
    if (retval < 0) return retval;

    if (kVL_AX_is_value == vloc)
    {
        if (!_sym.IsManagedVartype(lhsvartype))
        {
            Error("Cannot modify this value");
            return kERR_UserError;
        }
        WriteCmd(SCMD_REGTOREG, SREG_AX, SREG_MAR);
        WriteCmd(SCMD_CHECKNULL);
        vloc = kVL_MAR_pointsto_value;
    }

    if (may_clobber)
        PopReg(SREG_AX);
    _scrip.ax_vartype = rhsvartype;
    _scrip.ax_scope_type = rhs_scope_type;

    if (kVL_Attribute == vloc)
    {
        // We need to call the attribute setter 
        Vartype struct_of_attribute = lhsvartype;

        bool const is_setter = true;
        retval =  AccessData_CallAttributeFunc(is_setter, expression, struct_of_attribute);
        if (retval < 0) return retval;
        _src.SetCursor(end_of_rhs_cursor); // move cursor back to end of RHS
        return kERR_None;
    }

    // MAR points to the value

    if (kKW_String == lhsvartype && kKW_String == _sym.VartypeWithout(VTT::kConst, rhsvartype))
    {
        // copy the string contents over.
        AccessData_StrCpy();
        _src.SetCursor(end_of_rhs_cursor); // move cursor back to end of RHS
        return kERR_None;
    }

    ConvertAXStringToStringObject(lhsvartype);
    rhsvartype = _scrip.ax_vartype;
    if (IsVartypeMismatch_Oneway(rhsvartype, lhsvartype))
    {
        Error(
            "Cannot assign a type '%s' value to a type '%s' variable",
            _sym.GetName(rhsvartype).c_str(),
            _sym.GetName(lhsvartype).c_str());
        return kERR_UserError;
    }

    CodeCell const opcode =
        _sym.IsDynVartype(lhsvartype) ?
        SCMD_MEMWRITEPTR : GetWriteCommandForSize(_sym.GetSize(lhsvartype));
    WriteCmd(opcode, SREG_AX);
    _src.SetCursor(end_of_rhs_cursor); // move cursor back to end of RHS
    return kERR_None;
}

ErrorType AGS::Parser::SkipToEndOfExpression()
{
    int nesting_depth = 0;

    // The ':' in an "a ? b : c" construct can also be the end of a label, and in AGS,
    // expressions are allowed for labels. So we must take care that label ends aren't
    // mistaken for expression parts. For this, tern_depth counts the number of
    // unmatched '?' on the outer level. If this is non-zero, then any arriving 
    // ':' will be interpreted as part of a ternary.
    int tern_depth = 0;

    Symbol peeksym;
    while (0 <= (peeksym = _src.PeekNext())) // note assignment in while condition
    {
        // Skip over parts that are enclosed in braces, brackets, or parens
        if (kKW_OpenParenthesis == peeksym || kKW_OpenBracket == peeksym || kKW_OpenBrace == peeksym)
            ++nesting_depth;
        else if (kKW_CloseParenthesis == peeksym || kKW_CloseBracket == peeksym || kKW_CloseBrace == peeksym)
            if (--nesting_depth < 0)
                break; // this symbol can't be part of the current expression
        if (nesting_depth > 0)
        {
            _src.GetNext();
            continue;
        }

        if (kKW_Colon == peeksym)
        {
            // This is only allowed if it can be matched to an open tern
            if (--tern_depth < 0)
                break;

            _src.GetNext(); // Eat ':'
            continue;
        }

        if (kKW_Dot == peeksym)
        {
            _src.GetNext(); // Eat '.'
            _src.GetNext(); // Eat following symbol
            continue;
        }

        if (kKW_New == peeksym)
        {   // Only allowed if a type follows   
            _src.GetNext(); // Eat 'new'
            Symbol const sym_after_new = _src.PeekNext();
            SymbolType const type_of_sym_after = _sym.GetSymbolType(sym_after_new);
            if (SymT::kVartype == type_of_sym_after || SymT::kUndefinedStruct == type_of_sym_after)
            {
                _src.GetNext(); // Eat symbol after 'new'
                continue;
            }
            _src.BackUp(); // spit out 'new'
            break;
        }

        if (kKW_Null == peeksym)
        {   // Allowed.
            _src.GetNext(); // Eat 'null'
            continue;
        }

        if (kKW_Tern == peeksym)
        {
            tern_depth++;
            _src.GetNext(); // Eat '?'
            continue;
        }

        if (_sym.IsVartype(peeksym))
        {   // Only allowed if a dot follows
            _src.GetNext(); // Eat the vartype
            Symbol const nextsym = _src.PeekNext();
            if (kKW_Dot == nextsym)
            {
                _src.GetNext(); // Eat '.'
                continue;
            }
            _src.BackUp(); // spit out the vartype
            break;
        }

        // Apart from the exceptions above, all symbols with types beyond
        // SymT::kLastInExpression can't be part of an expression
        if (_sym.GetSymbolType(peeksym) > kLastInExpression)
            break;
        _src.GetNext(); // Eat the peeked symbol
    }

    if (nesting_depth > 0)
    {
        Error("Unexpected end of input");
        return kERR_UserError;
    }
    return kERR_None;
}

// evaluate the supplied expression, putting the result into AX
// returns 0 on success or -1 if compile error
// leaves targ pointing to last token in expression, so do getnext() to get the following ; or whatever
ErrorType AGS::Parser::ParseExpression()
{
    size_t const expr_start = _src.GetCursor();
    ErrorType retval = SkipToEndOfExpression();
    if (retval < 0) return retval;
    SrcList expression = SrcList(_src, expr_start, _src.GetCursor() - expr_start);
    if (0 == expression.Length())
    {
        Error("!Empty expression");
        return kERR_InternalError;
    }
    
    ValueLocation vloc;
    ScopeType scope_type;
    Vartype vartype;

    retval = ParseExpression_Term(expression, vloc, scope_type, vartype);
    if (retval < 0) return retval;

    return ResultToAX(vloc, scope_type, vartype);
}

ErrorType AGS::Parser::AccessData_ReadBracketedIntExpression(SrcList &expression)
{
    ErrorType retval = Expect(kKW_OpenBracket, expression.GetNext());
    if (retval < 0) return retval;

    size_t start = expression.GetCursor();
    SkipTo(SymbolList{}, expression);
    SrcList in_brackets = SrcList(expression, start, expression.GetCursor() - start);

    retval = AccessData_ReadIntExpression(in_brackets);
    if (retval < 0) return retval;

    if (!in_brackets.ReachedEOF())
    {
        Error("Expected ']', found '%s' instead", _sym.GetName(in_brackets.GetNext()).c_str());
        return kERR_UserError;
    }
    return Expect(kKW_CloseBracket, expression.GetNext());
}

ErrorType AGS::Parser::ParseParenthesizedExpression()
{
    ErrorType retval = Expect(kKW_OpenParenthesis, _src.GetNext());
    if (retval < 0) return retval;
    
    retval = ParseExpression();
    if (retval < 0) return retval;

    return Expect(kKW_CloseParenthesis, _src.GetNext());
}

// We are parsing the left hand side of a += or similar statement.
ErrorType AGS::Parser::ParseAssignment_ReadLHSForModification(SrcList &lhs, ValueLocation &vloc, AGS::Vartype &lhstype)
{
    ScopeType scope_type;

    bool const writing = false; // reading access
    ErrorType retval = AccessData(writing, lhs, vloc, scope_type, lhstype);
    if (retval < 0) return retval;
    if (!lhs.ReachedEOF())
    {
        Error("!Unexpected symbols following expression");
        return kERR_InternalError;
    }

    if (kVL_MAR_pointsto_value == vloc)
    {
        // write memory to AX
        _scrip.ax_vartype = lhstype;
        _scrip.ax_scope_type = scope_type;
        WriteCmd(
            GetReadCommandForSize(_sym.GetSize(lhstype)),
            SREG_AX);
    }
    return kERR_None;
}

// "var = expression"; lhs is the variable
ErrorType AGS::Parser::ParseAssignment_Assign(SrcList &lhs)
{
    ErrorType retval = ParseExpression(); // RHS of the assignment
    if (retval < 0) return retval;
    
    return AccessData_AssignTo(lhs);
}

// We compile something like "var += expression"
ErrorType AGS::Parser::ParseAssignment_MAssign(AGS::Symbol ass_symbol, SrcList &lhs)
{
    // Parse RHS
    ErrorType retval = ParseExpression();
    if (retval < 0) return retval;

    PushReg(SREG_AX);
    Vartype rhsvartype = _scrip.ax_vartype;

    // Parse LHS (moves the cursor to end of LHS, so save it and restore it afterwards)
    ValueLocation vloc;
    Vartype lhsvartype;
    size_t const end_of_rhs_cursor = _src.GetCursor();
    retval = ParseAssignment_ReadLHSForModification(lhs, vloc, lhsvartype); 
    if (retval < 0) return retval;
    _src.SetCursor(end_of_rhs_cursor); // move cursor back to end of RHS

    // Use the operator on LHS and RHS
    CodeCell opcode = _sym.GetOperatorOpcode(ass_symbol);
    retval = GetOpcodeValidForVartype(lhsvartype, rhsvartype, opcode);
    if (retval < 0) return retval;
    PopReg(SREG_BX);
    WriteCmd(opcode, SREG_AX, SREG_BX);

    if (kVL_MAR_pointsto_value == vloc)
    {
        // Shortcut: Write the result directly back to memory
        CodeCell memwrite = GetWriteCommandForSize(_sym.GetSize(lhsvartype));
        WriteCmd(memwrite, SREG_AX);
        return kERR_None;
    }

    // Do a conventional assignment
    return AccessData_AssignTo(lhs);
}

// "var++" or "var--"
ErrorType AGS::Parser::ParseAssignment_SAssign(AGS::Symbol ass_symbol, SrcList &lhs)
{
    ValueLocation vloc;
    Vartype lhsvartype;
    ErrorType retval = ParseAssignment_ReadLHSForModification(lhs, vloc, lhsvartype);
    if (retval < 0) return retval;

    // increment or decrement AX, using the correct opcode
    CodeCell opcode = _sym.GetOperatorOpcode(ass_symbol);
    retval = GetOpcodeValidForVartype(lhsvartype, lhsvartype, opcode);
    if (retval < 0) return retval;
    WriteCmd(opcode, SREG_AX, 1);

    if (kVL_MAR_pointsto_value == vloc)
    {
        _src.GetNext(); // Eat ++ or --
        // write AX back to memory
        Symbol memwrite = GetWriteCommandForSize(_sym.GetSize(lhsvartype));
        WriteCmd(memwrite, SREG_AX);
        return kERR_None;
    }

    retval = ParseAssignment_Assign(lhs); // moves cursor to end of LHS
    if (retval < 0) return retval; 
    _src.GetNext(); // Eat ++ or --
    return kERR_None;
}

// We've read a variable or selector of a struct into symlist[], the last identifying component is in cursym.
// An assignment symbol is following. Compile the assignment.
ErrorType AGS::Parser::ParseAssignment(AGS::Symbol ass_symbol, SrcList &lhs)
{
    switch (_sym.GetSymbolType(ass_symbol))
    {
    default: // can't happen
        Error("!Illegal assignment symbol found");
        return kERR_InternalError;

    case SymT::kAssign:
        return ParseAssignment_Assign(lhs);

    case SymT::kAssignMod:
        return ParseAssignment_MAssign(ass_symbol, lhs);

    case SymT::kAssignSOp:
        return ParseAssignment_SAssign(ass_symbol, lhs);
    }
}

ErrorType AGS::Parser::ParseVardecl_InitialValAssignment_Float(bool is_neg, void *& initial_val_ptr)
{
    // initialize float
    if (_sym.GetSymbolType(_src.PeekNext()) != SymT::kLiteralFloat)
    {
        Error("Expected floating point value after '='");
        return kERR_UserError;
    }

    float float_init_val = static_cast<float>(atof(_sym.GetName(_src.GetNext()).c_str()));
    if (is_neg)
        float_init_val = -float_init_val;

    // Allocate space for one long value
    initial_val_ptr = malloc(sizeof(long));
    if (!initial_val_ptr)
    {
        Error("Out of memory");
        return kERR_UserError;
    }

    // Interpret the float as an int; move that into the allocated space
    (static_cast<long *>(initial_val_ptr))[0] = InterpretFloatAsInt(float_init_val);

    return kERR_None;
}

ErrorType AGS::Parser::ParseVardecl_InitialValAssignment_OldString(void *&initial_val_ptr)
{
    Symbol literal_sym = _src.GetNext();
    if (SymT::kLiteralString != _sym.GetSymbolType(literal_sym))
    {
        Error("Expected a literal string");
        return kERR_UserError;
    }
    std::string literal = _sym.GetName(literal_sym);
    if (literal.length() >= STRINGBUFFER_LENGTH)
    {
        Error(
            "Initializer string is too long (max. chars allowed: %d",
            STRINGBUFFER_LENGTH - 1);
        return kERR_UserError;
    }
    initial_val_ptr = malloc(STRINGBUFFER_LENGTH);
    if (!initial_val_ptr)
    {
        Error("Out of memory");
        return kERR_UserError;
    }
    std::strncpy(
        static_cast<char *>(initial_val_ptr), literal.c_str(),
        STRINGBUFFER_LENGTH);
    return kERR_None;
}

ErrorType AGS::Parser::ParseVardecl_InitialValAssignment_Inttype(bool is_neg, void *&initial_val_ptr)
{
    // Initializer for an integer value
    int int_init_val;
    ErrorType retval = IntLiteralOrConst2Value(_src.GetNext(), is_neg, "Expected integer value after '='", int_init_val);
    if (retval < 0) return retval;

    // Allocate space for one long value
    initial_val_ptr = malloc(sizeof(long));
    if (!initial_val_ptr)
    {
        Error("Out of memory");
        return kERR_UserError;
    }
    // Convert int to long; move that into the allocated space
    (reinterpret_cast<long *>(initial_val_ptr))[0] = int_init_val;

    return kERR_None;
}

// if initial_value is non-null, it returns malloc'd memory that must be free
ErrorType AGS::Parser::ParseVardecl_InitialValAssignment(AGS::Symbol varname, void *&initial_val_ptr)
{
    initial_val_ptr = nullptr;
    _src.GetNext(); // Eat '='

    if (_sym.IsManagedVartype(varname))
    {
        Error("Cannot assign an initial value to a managed type or String");
        return kERR_UserError;
    }

    if (_sym.IsStructVartype(varname))
    {
        Error("Cannot initialize struct type");
        return kERR_UserError;
    }

    if (kKW_String == _sym.GetVartype(varname))
        return ParseVardecl_InitialValAssignment_OldString(initial_val_ptr);

    // accept leading '-' if present
    bool is_neg = false;
    if (_src.PeekNext() == _sym.Find("-"))
    {
        is_neg = true;
        _src.GetNext();
    }

    // Do actual assignment
    if (_sym.GetVartype(varname) == kKW_Float)
        return ParseVardecl_InitialValAssignment_Float(is_neg, initial_val_ptr);
    return ParseVardecl_InitialValAssignment_Inttype(is_neg, initial_val_ptr);
}

// Move variable information into the symbol table
ErrorType AGS::Parser::ParseVardecl_Var2SymTable(Symbol var_name, AGS::Vartype vartype, ScopeType scope_type)
{
    SymbolTableEntry &var_entry = _sym[var_name];

    if (ScT::kLocal == scope_type)
    {
        if (_nest.TopLevel() == var_entry.SScope)
        {
            Error(
                ReferenceMsgSym("'%s' has already been defined in this scope", var_name).c_str(),
                _sym.GetName(var_name).c_str());
            return kERR_UserError;
        }
        if (SymbolTableEntry::kParameterSScope == var_entry.SScope && SymbolTableEntry::kFunctionSScope == _nest.TopLevel())
        {
            Error(
                ReferenceMsgSym("'%s' has already been defined as a parameter", var_name).c_str(),
                _sym.GetName(var_name).c_str());
            return kERR_UserError;
        }
        if (_nest.AddOldDefinition(var_name, var_entry))
        {
            Error("!AddOldDefinition: Storage place occupied");
            return kERR_InternalError;
        }
    }

    var_entry.SType = (scope_type == ScT::kLocal) ? SymT::kLocalVar : SymT::kGlobalVar;
    var_entry.Vartype = vartype;
    var_entry.SScope = _nest.TopLevel();
    _sym.SetDeclared(var_name, _src.GetCursor());
    return kERR_None;
}

ErrorType AGS::Parser::ParseVardecl_CheckIllegalCombis(AGS::Vartype vartype, ScopeType scope_type)
{
    if (vartype == kKW_String && ccGetOption(SCOPT_OLDSTRINGS) == 0)
    {
        Error("Type 'string' is no longer supported; use String instead");
        return kERR_UserError;
    }

    if (vartype == kKW_String && ScT::kImport == scope_type)
    {
        // cannot import because string is really char *, and the pointer won't resolve properly
        Error("Cannot import string; use char[] instead");
        return kERR_UserError;
    }

    if (vartype == kKW_Void)
    {
        Error("'void' is not a valid type in this context");
        return kERR_UserError;
    }

    return kERR_None;
}

// there was a forward declaration -- check that the real declaration matches it
ErrorType AGS::Parser::ParseVardecl_CheckThatKnownInfoMatches(SymbolTableEntry *this_entry, SymbolTableEntry *known_info, bool body_follows)
{
    if (SymT::kNoType == known_info->SType)
        return kERR_None; // We don't have any known info

    if (known_info->SType != this_entry->SType)
    {
        Error(ReferenceMsgLoc("This variable is declared as %s elsewhere", known_info->Declared).c_str(),
            (SymT::kFunction == known_info->SType)  ? "function" :
            (SymT::kGlobalVar == known_info->SType) ? "global variable" :
            (SymT::kLocalVar == known_info->SType)  ? "local variable" : "another entity");
        return kERR_UserError;
    }

    TypeQualifierSet known_tq = known_info->TypeQualifiers;
    known_tq[TQ::kImport] = false;
    TypeQualifierSet this_tq = this_entry->TypeQualifiers;
    this_tq[TQ::kImport] = false;
    if (known_tq != this_tq)
    {
        std::string const ki_tq = TypeQualifierSet2String(known_tq);
        std::string const te_tq = TypeQualifierSet2String(this_tq);
        std::string msg = ReferenceMsgLoc(
            "The variable '%s' has the qualifiers '%s' here, but '%s' elsewhere",
            known_info->Declared);
        Error(msg.c_str(), te_tq.c_str(), ki_tq.c_str());
        return kERR_UserError;
    }

    if (known_info->Vartype != this_entry->Vartype)
    {
        // This will check the array lengths, too
        std::string msg = ReferenceMsgLoc(
            "This variable is declared as %s here, as %s elsewhere",
            known_info->Declared);
        Error(
            msg.c_str(),
            _sym.GetName(this_entry->Vartype).c_str(),
            _sym.GetName(known_info->Vartype).c_str());
        return kERR_UserError;
    }

    if (known_info->GetSize(_sym) != this_entry->GetSize(_sym))
    {
        std::string msg = ReferenceMsgLoc(
            "Size of this variable is %d here, %d declared elsewhere",
            known_info->Declared);
        Error(
            msg.c_str(),
            this_entry->GetSize(_sym), known_info->GetSize(_sym));
        return kERR_UserError;
    }

    return kERR_None;
}

ErrorType AGS::Parser::ParseVardecl_GlobalImport(AGS::Symbol var_name, bool has_initial_assignment)
{
    if (has_initial_assignment)
    {
        Error("Imported variables cannot have any initial assignment");
        return kERR_UserError;
    }

    if (_givm[var_name])
        return kERR_None; // Skip this since the global non-import decl will come later

    _sym[var_name].TypeQualifiers[TQ::kImport] = true;
    _sym[var_name].SOffset = _scrip.add_new_import(_sym.GetName(var_name).c_str());
    if (_sym[var_name].SOffset == -1)
    {
        Error("!Import table overflow");
        return kERR_InternalError;
    }

    return kERR_None;
}

ErrorType AGS::Parser::ParseVardecl_GlobalNoImport(AGS::Symbol var_name, AGS::Vartype vartype, bool has_initial_assignment, void *&initial_val_ptr)
{
    if (has_initial_assignment)
    {
        ErrorType retval = ParseVardecl_InitialValAssignment(var_name, initial_val_ptr);
        if (retval < 0) return retval;
    }
    SymbolTableEntry &entry = _sym[var_name];
    entry.Vartype = vartype;
    size_t const var_size = _sym.GetSize(vartype);
    entry.SOffset = _scrip.add_global(var_size, initial_val_ptr);
    if (entry.SOffset < 0)
    {
        Error("!Cannot allocate global variable");
        return kERR_InternalError;
    }
    return kERR_None;
}

ErrorType AGS::Parser::ParseVardecl_Local(AGS::Symbol var_name, AGS::Vartype vartype, bool has_initial_assignment)
{
    size_t const var_size = _sym.GetSize(vartype);
    bool const is_dyn = _sym.IsDynVartype(vartype);

    _sym[var_name].SOffset = _scrip.offset_to_local_var_block;

    if (!has_initial_assignment)
    {
        // Initialize the variable with binary zeroes.
        WriteCmd(SCMD_LOADSPOFFS, 0);
        if (is_dyn)
            WriteCmd(SCMD_MEMZEROPTR);
        else
            WriteCmd(SCMD_ZEROMEMORY, var_size);
        WriteCmd(SCMD_ADD, SREG_SP, var_size);
        _scrip.offset_to_local_var_block += var_size;
        return kERR_None;
    }

    // "readonly" vars can't be assigned to, so don't use standard assignment function here.
    _src.GetNext(); // Eat '='
    ErrorType retval = ParseExpression();
    if (retval < 0) return retval;

    // Vartypes must match. This is true even if the lhs is readonly.
    // As a special case, a string may be assigned a const string because the const string will be copied, not modified.
    Vartype const lhsvartype = vartype;
    Vartype const rhsvartype = _scrip.ax_vartype;

    if (IsVartypeMismatch_Oneway(rhsvartype, lhsvartype) &&
        !(kKW_String == _sym.VartypeWithout(VTT::kConst, rhsvartype) &&
          kKW_String == _sym.VartypeWithout(VTT::kConst, lhsvartype)))
    {
        Error(
            "Cannot assign a type '%s' value to a type '%s' variable",
            _sym.GetName(rhsvartype).c_str(),
            _sym.GetName(lhsvartype).c_str());
        return kERR_UserError;
    }

    if (SIZE_OF_INT == var_size && !is_dyn)
    {
        // This PUSH moves the result of the initializing expression into the
        // new variable and reserves space for this variable on the stack.
        PushReg(SREG_AX);
        return kERR_None;
    }

    ConvertAXStringToStringObject(vartype);
    WriteCmd(SCMD_LOADSPOFFS, 0);
    if (kKW_String == _sym.VartypeWithout(VTT::kConst, lhsvartype))
        AccessData_StrCpy();
    else
        WriteCmd(
            is_dyn ? SCMD_MEMWRITEPTR : GetWriteCommandForSize(var_size),
            SREG_AX);
    WriteCmd(SCMD_ADD, SREG_SP, var_size);
    _scrip.offset_to_local_var_block += var_size;
    return kERR_None;
}

ErrorType AGS::Parser::ParseVardecl0(AGS::Symbol var_name, AGS::Vartype vartype, ScopeType scope_type)
{
    Symbol next_sym = _src.PeekNext();
    if (kKW_OpenBracket == next_sym)
    {
        ErrorType retval = ParseArray(var_name, vartype);
        if (retval < 0) return retval;
        next_sym = _src.PeekNext();
    }

    // Enter the variable into the symbol table
    ErrorType retval = ParseVardecl_Var2SymTable(var_name, vartype, scope_type);
    if (retval < 0) return retval;

    bool const has_initial_assignment = (kKW_Assign == next_sym);

    switch (scope_type)
    {
    default:
        Error("!Wrong scope type");
        return kERR_InternalError;

    case ScT::kGlobal:
    {
        void *initial_val_ptr = nullptr;
        ErrorType retval = ParseVardecl_GlobalNoImport(var_name, vartype, has_initial_assignment, initial_val_ptr);
        if (initial_val_ptr) free(initial_val_ptr);
        return retval;
    }

    case ScT::kImport:
        return ParseVardecl_GlobalImport(var_name, has_initial_assignment);

    case ScT::kLocal:
        return ParseVardecl_Local(var_name, vartype, has_initial_assignment);
    }
}

// wrapper around ParseVardecl0() 
ErrorType AGS::Parser::ParseVardecl(AGS::Symbol var_name, AGS::Vartype vartype, ScopeType scope_type)
{
    ErrorType retval = ParseVardecl_CheckIllegalCombis(vartype, scope_type);
    if (retval < 0) return retval;

    if (ScT::kLocal == scope_type)
    {
        switch (_sym.GetSymbolType(var_name))
        {
        default:
            Error(
                ReferenceMsgSym("'%s' is already in use elsewhere", var_name).c_str(),
                 _sym.GetName(var_name).c_str());
            return kERR_UserError;

        case SymT::kFunction:
            Warning(
                ReferenceMsgSym("This hides the function '%s()'", var_name).c_str(),
                _sym.GetName(var_name).c_str());
            break;

        case SymT::kGlobalVar:
        case SymT::kLocalVar:
            break;

        case SymT::kNoType:
            break;

        case SymT::kVartype:
            Error(
                ReferenceMsgSym("'%s' is in use as a type elsewhere", var_name).c_str(),
                _sym.GetName(var_name).c_str());
            return kERR_UserError;
        }
    }

    SymbolTableEntry known_info;
    if (ScT::kGlobal == scope_type)
        CopyKnownSymInfo(_sym[var_name], known_info);
    
    retval = ParseVardecl0(var_name, vartype, scope_type);
    if (retval < 0) return retval;

    if (ScT::kGlobal == scope_type)
        return ParseVardecl_CheckThatKnownInfoMatches(&_sym[var_name], &known_info, false);
    return kERR_None;
}

ErrorType AGS::Parser::ParseFuncBodyStart(Symbol struct_of_func, AGS::Symbol name_of_func)
{
    _nest.Push(NSType::kFunction);

    // write base address of function for any relocation needed later
    WriteCmd(SCMD_THISBASE, _scrip.codesize);
    SymbolTableEntry &entry = _sym[name_of_func];
    if (FlagIsSet(entry.Flags, kSFLG_NoLoopCheck))
    {
        WriteCmd(SCMD_LOOPCHECKOFF);
        SetFlag(entry.Flags, kSFLG_NoLoopCheck, false);
    }

    // If there are dynpointer parameters, then the caller has simply "pushed" them onto the stack.
    // We catch up here by reading each dynpointer and writing it again using MEMINITPTR
    // to declare that the respective cells will from now on be used for dynpointers.
    size_t const num_params = _sym[name_of_func].GetNumOfFuncParams();
    for (size_t param_idx = 1; param_idx <= num_params; param_idx++) // skip return value param_idx == 0
    {
        Vartype const param_vartype = _sym[name_of_func].FuncParamVartypes[param_idx];
        if (!_sym.IsDynVartype(param_vartype))
            continue;

        // The return address is on top of the stack, so the nth param is at (n+1)th position
        WriteCmd(SCMD_LOADSPOFFS, SIZE_OF_STACK_CELL * (param_idx + 1));
        WriteCmd(SCMD_MEMREAD, SREG_AX); // Read the address that is stored there
        // Create a dynpointer that points to the same object as m[AX] and store it in m[MAR]
        WriteCmd(SCMD_MEMINITPTR, SREG_AX);
    }

    SymbolTableEntry &this_entry = _sym[kKW_This];
    this_entry.Vartype = 0;
    if (struct_of_func > 0 && !_sym[name_of_func].TypeQualifiers[TQ::kStatic])
    {
        // Declare "this" but do not allocate memory for it
        this_entry.SType = SymT::kLocalVar;
        this_entry.Vartype = struct_of_func; // Don't declare this as dynpointer
        this_entry.SScope = 0u;
        this_entry.TypeQualifiers = {};
        this_entry.TypeQualifiers[TQ::kReadonly] = true;
        this_entry.Flags = kSFLG_Accessed | kSFLG_StructVartype;
        this_entry.SOffset = 0;
    }
    return kERR_None;
}

ErrorType AGS::Parser::HandleEndOfFuncBody(Symbol &struct_of_current_func, Symbol &name_of_current_func)
{
    // Free all the dynpointers in parameters and locals. 
    FreeDynpointersOfLocals(1u);
    // Pop the local variables proper from the stack but leave the parameters.
    // This is important because the return address is directly above the parameters;
    // we need the return address to return. (The caller will pop the parameters later.)
    RemoveLocalsFromStack(SymbolTableEntry::kFunctionSScope);
    // All the function variables, _including_ the parameters, become invalid.
    RemoveLocalsFromSymtable(SymbolTableEntry::kParameterSScope);

    // Function has ended. Set AX to 0 unless the function doesn't return any value.
    if (kKW_Void != _sym[name_of_current_func].FuncParamVartypes.at(0))
        WriteCmd(SCMD_LITTOREG, SREG_AX, 0);

    // We've just finished the body of the current function.
    name_of_current_func = kKW_NoSymbol;
    struct_of_current_func = kKW_NoSymbol;

    _nest.Pop();    // End function variables nesting
    _nest.JumpOut().Patch(_src.GetLineno());
    _nest.Pop();    // End function parameters nesting

    WriteCmd(SCMD_RET);
    // This has popped the return address from the stack, 
    // so adjust the offset to the start of the parameters.
    _scrip.offset_to_local_var_block -= SIZE_OF_STACK_CELL;

    return kERR_None;
}

void AGS::Parser::ParseStruct_SetTypeInSymboltable(AGS::Symbol stname, TypeQualifierSet tqs)
{
    SymbolTableEntry &entry = _sym[stname];

    entry.SType = SymT::kVartype;
    entry.Parent = 0;
    entry.SSize = 0;

    SetFlag(entry.Flags, kSFLG_StructVartype, true);
    if (tqs[TQ::kManaged])
        SetFlag(entry.Flags, kSFLG_StructManaged, true);
    if (tqs[TQ::kBuiltin])
        SetFlag(entry.Flags, kSFLG_StructBuiltin, true);
    if (tqs[TQ::kAutoptr])
        SetFlag(entry.Flags, kSFLG_StructAutoPtr, true);

    _sym.SetDeclared(stname, _src.GetCursor());
}

// We have accepted something like "struct foo" and are waiting for "extends"
ErrorType AGS::Parser::ParseStruct_ExtendsClause(AGS::Symbol stname, size_t &size_so_far)
{
    _src.GetNext(); // Eat "extends"
    Symbol const parent = _src.GetNext(); // name of the extended struct

    if (PP::kPreAnalyze == _pp)
        return kERR_None; // No further analysis necessary in first phase

    if (SymT::kVartype != _sym.GetSymbolType(parent))
    {
        Error("Expected a struct type here");
        return kERR_UserError;
    }
    if (!_sym.IsStructVartype(parent))
    {
        Error("Must extend a struct type");
        return kERR_UserError;
    }
    if (!_sym.IsManagedVartype(parent) && _sym.IsManagedVartype(stname))
    {
        Error("Managed struct cannot extend the unmanaged struct '%s'", _sym.GetName(parent).c_str());
        return kERR_UserError;
    }
    if (_sym.IsManagedVartype(parent) && !_sym.IsManagedVartype(stname))
    {
        Error("Unmanaged struct cannot extend the managed struct '%s'", _sym.GetName(parent).c_str());
        return kERR_UserError;
    }
    if (_sym.IsBuiltin(parent) && !_sym.IsBuiltin(stname))
    {
        Error("The built-in type '%s' cannot be extended by a concrete struct. Use extender methods instead", _sym.GetName(parent).c_str());
        return kERR_UserError;
    }
    size_so_far = _sym.GetSize(parent);
    _sym[stname].Parent = parent;
    return kERR_None;
}

// Check whether the qualifiers that accumulated for this decl go together
ErrorType AGS::Parser::Parse_CheckTQ(TypeQualifierSet tqs, bool in_func_body, bool in_struct_decl)
{
    if (in_struct_decl)
    {
        TypeQualifier error_tq = TQ::kNone;
        if (tqs[(error_tq = TQ::kBuiltin)] ||
            tqs[(error_tq = TQ::kStringstruct)])
        {
            Error("'%s' is illegal in a struct declaration", _tq2String[error_tq].c_str());
            return kERR_UserError;
        }
    }
    else // !in_struct_decl
    {
        TypeQualifier error_tq = TQ::kNone;
        if (tqs[(error_tq = TQ::kAttribute)] ||
            tqs[(error_tq = TQ::kProtected)] ||
            tqs[(error_tq = TQ::kWriteprotected)])
        {
            Error("'%s' is only legal in a struct declaration", _tq2String[error_tq].c_str());
            return kERR_UserError;
        }
    }

    if (in_func_body)
    {
        TypeQualifier error_tq = TQ::kNone;
        if (tqs[(error_tq = TQ::kAutoptr)] ||
            tqs[(error_tq = TQ::kBuiltin)] ||
            tqs[(error_tq = TQ::kImport)] ||
            tqs[(error_tq = TQ::kManaged)] ||
            tqs[(error_tq = TQ::kStatic)] ||
            tqs[(error_tq = TQ::kStringstruct)])
        {
            Error("'%s' is illegal in a function body", _tq2String[error_tq].c_str());
            return kERR_UserError;
        }
    }

    // Keywords that never go together
    if (1 < tqs[TQ::kProtected] + tqs[TQ::kWriteprotected] + tqs[TQ::kReadonly])
    {
        Error("Can only use one out of 'protected', 'readonly', and 'writeprotected'");
        return kERR_UserError;
    }

    if (tqs[TQ::kAutoptr])
    {
        if (!tqs[TQ::kBuiltin] || !tqs[TQ::kManaged])
        {
            Error("'autoptr' must be combined with 'builtin' and 'managed'");
            return kERR_UserError;
        }
    }

    if (tqs[TQ::kStringstruct] && (!tqs[TQ::kAutoptr]))
    {
        Error("'stringstruct' must be combined with 'autoptr'");
        return kERR_UserError;
    }

    if (tqs[TQ::kConst])
    {
        Error("'const' can only be used for a function parameter (use 'readonly' instead)");
        return kERR_UserError;
    }

    if (tqs[TQ::kImport] && tqs[TQ::kStringstruct])
    {
        Error("Cannot combine 'import' and 'stringstruct'");
        return kERR_UserError;
    }

    return kERR_None;
}

ErrorType AGS::Parser::Parse_CheckEmpty(TypeQualifierSet tqs)
{
    for (auto it = _tq2String.cbegin(); it != _tq2String.cend(); it++)
    {
        if (!tqs[it->first])
            continue;
        Error("Unexpected '%s' before a command", it->second.c_str());
        return kERR_UserError;
    }
    return kERR_None;
}

ErrorType AGS::Parser::ParseQualifiers(TypeQualifierSet &tqs)
{
    bool istd_found = false;
    bool itry_found = false;
    tqs = {};
    while (!_src.ReachedEOF())
    {
        Symbol peeksym = _src.PeekNext();
        switch (peeksym)
        {
        default: return kERR_None;
        case kKW_Attribute:      tqs[TQ::kAttribute] = true; break;
        case kKW_Autoptr:        tqs[TQ::kAutoptr] = true; break;
        case kKW_Builtin:        tqs[TQ::kBuiltin] = true; break;
        case kKW_Const:          tqs[TQ::kConst] = true; break;
        case kKW_ImportStd:      tqs[TQ::kImport] = true; istd_found = true;  break;
        case kKW_ImportTry:      tqs[TQ::kImport] = true; itry_found = true;  break;
        case kKW_Internalstring: tqs[TQ::kStringstruct] = true; break;
        case kKW_Managed:        tqs[TQ::kManaged] = true; break;
        case kKW_Protected:      tqs[TQ::kProtected] = true; break;
        case kKW_Readonly:       tqs[TQ::kReadonly] = true; break;
        case kKW_Static:         tqs[TQ::kStatic] = true; break;
        case kKW_Writeprotected: tqs[TQ::kWriteprotected] = true; break;
        } // switch (_sym.GetSymbolType(peeksym))

        _src.GetNext();
        if (istd_found && itry_found)
        {
            Error("Cannot both use 'import' and '_tryimport'");
            return kERR_UserError;
        }
    };

    return kERR_None;
}

ErrorType AGS::Parser::ParseStruct_CheckComponentVartype(Symbol stname, AGS::Vartype vartype)
{
    if (vartype == stname && !_sym.IsManagedVartype(vartype))
    {
        // cannot do "struct A { A varname; }", this struct would be infinitely large
        Error("Struct '%s' cannot be a member of itself", _sym.GetName(vartype).c_str());
        return kERR_UserError;
    }

    SymbolType const vartype_type = _sym.GetSymbolType(vartype);
    if (vartype_type == SymT::kNoType)
    {
        Error(
            "Type '%s' is undefined",
            _sym.GetName(vartype).c_str());
        return kERR_UserError;
    }
    if (SymT::kVartype != vartype_type && SymT::kUndefinedStruct != vartype_type)
    {
        std::string const msg = ReferenceMsgSym(
            "'%s' should be a typename but is in use differently",
            vartype);
        Error(msg.c_str(), _sym.GetName(vartype).c_str());
        return kERR_UserError;
    }

    return kERR_None;
}

// check that we haven't extended a struct that already contains a member with the same name
ErrorType AGS::Parser::ParseStruct_CheckForCompoInAncester(AGS::Symbol orig, AGS::Symbol compo, AGS::Symbol current_struct)
{
    if (current_struct <= 0)
        return kERR_None;
    Symbol const member = MangleStructAndComponent(current_struct, compo);
    if (SymT::kNoType != _sym.GetSymbolType(member))
    {
        Error(
            ReferenceMsgSym(
                "The struct '%s' extends '%s', and '%s' is already defined",
                member).c_str(),
            _sym.GetName(orig).c_str(),
            _sym.GetName(current_struct).c_str(),
            _sym.GetName(member).c_str());
        return kERR_UserError;
    }

    return ParseStruct_CheckForCompoInAncester(orig, compo, _sym[current_struct].Parent);
}

ErrorType AGS::Parser::ParseStruct_FuncDecl(Symbol struct_of_func, Symbol name_of_func, TypeQualifierSet tqs, Vartype vartype)
{
    if (tqs[TQ::kWriteprotected])
    {
        Error("'writeprotected' does not apply to functions");
        return kERR_UserError;
    }

    size_t const declaration_start = _src.GetCursor();
    _src.GetNext(); // Eat '('

    SetFlag(_sym[name_of_func].Flags, kSFLG_StructMember, true);
    _sym[name_of_func].Parent = struct_of_func;

    bool body_follows;
    ErrorType retval = ParseFuncdecl(declaration_start, tqs, vartype, struct_of_func, name_of_func, false, body_follows);
    if (retval < 0) return retval;
    if (body_follows)
    {
        Error("Cannot code a function body within a struct definition");
        return kERR_UserError;
    }

    return Expect(kKW_Semicolon, _src.PeekNext());
}

ErrorType AGS::Parser::ParseStruct_Attribute_CheckFunc(Symbol name_of_func, bool is_setter, bool is_indexed, AGS::Vartype vartype)
{
    SymbolTableEntry &entry = _sym[name_of_func];
    size_t const num_parameters_wanted = (is_indexed ? 1 : 0) + (is_setter ? 1 : 0);
    if (num_parameters_wanted != entry.GetNumOfFuncParams())
    {
        std::string const msg = ReferenceMsgSym(
            "The attribute function '%s' should have %d parameter(s) but is declared with %d parameter(s) instead",
            name_of_func);
        Error(msg.c_str(), entry.SName.c_str(), num_parameters_wanted, entry.GetNumOfFuncParams());
        return kERR_UserError;
    }

    Vartype const ret_vartype = is_setter ? kKW_Void : vartype;
    if (entry.FuncParamVartypes[0] != ret_vartype)
    {
        std::string const msg = ReferenceMsgSym(
            "The attribute function '%s' must return type '%s' but returns '%s' instead",
            name_of_func);
        Error(msg.c_str(), entry.SName.c_str(),
            _sym.GetName(ret_vartype).c_str(),
            _sym.GetName(entry.FuncParamVartypes[0]).c_str());
        return kERR_UserError;
    }

    size_t p_idx = 1;
    if (is_indexed)
    {
        if (entry.FuncParamVartypes[p_idx] != kKW_Int)
        {
            std::string const msg = ReferenceMsgSym(
                "Parameter #%d of attribute function '%s' must have type integer but doesn't.",
                name_of_func);
            Error(msg.c_str(), p_idx, entry.SName.c_str());
            return kERR_UserError;
        }
        p_idx++;
    }

    if (is_setter && entry.FuncParamVartypes[p_idx] != vartype)
    {
        std::string const msg = ReferenceMsgSym(
            "Parameter #d of attribute function '%s' must have type '%s'",
            name_of_func);
        Error(msg.c_str(), p_idx, entry.SName.c_str(), _sym.GetName(vartype).c_str());
        return kERR_UserError;
    }

    return kERR_None;
}

ErrorType AGS::Parser::ParseStruct_Attribute_ParamList(Symbol struct_of_func, Symbol name_of_func, bool is_setter, bool is_indexed, Vartype vartype)
{
    SymbolTableEntry &entry = _sym[name_of_func];
    size_t const num_param = is_indexed + is_setter;

    entry.FuncParamVartypes.resize(num_param + 1);

    size_t p_idx = 1;
    if (is_indexed)
        entry.FuncParamVartypes[p_idx++] = kKW_Int;
    if (is_setter)
        entry.FuncParamVartypes[p_idx] = vartype;
    SymbolTableEntry::ParamDefault deflt = {};
    entry.FuncParamDefaultValues.assign(entry.FuncParamVartypes.size(), deflt);
    return kERR_None;
}

// We are processing an attribute.
// This corresponds to a getter func and a setter func, declare one of them
ErrorType AGS::Parser::ParseStruct_Attribute_DeclareFunc(TypeQualifierSet tqs, Symbol struct_of_func, Symbol name_of_func, bool is_setter, bool is_indexed, Vartype vartype)
{
    // If this symbol has been defined before, check whether the definitions clash
    SymbolType const stype = _sym[name_of_func].SType;
    if (SymT::kFunction != stype && SymT::kNoType != stype)
    {
        std::string msg = ReferenceMsgSym(
            "Attribute uses '%s' as a function, this clashes with a declaration elsewhere",
            name_of_func);
        Error(msg.c_str(), _sym[name_of_func].SName.c_str());
        return kERR_UserError;
    }
    if (SymT::kFunction == stype)
    {
        ErrorType retval = ParseStruct_Attribute_CheckFunc(name_of_func, is_setter, is_indexed, vartype);
        if (retval < 0) return retval;
    }

    tqs[TQ::kImport] = true; // Assume that attribute functions are imported
    if (tqs[TQ::kImport] &&
        SymT::kFunction == _sym.GetSymbolType(name_of_func) &&
        !_sym[name_of_func].TypeQualifiers[TQ::kImport])
    {
        if (0 != ccGetOption(SCOPT_NOIMPORTOVERRIDE))
        {
            std::string const msg = ReferenceMsgSym(
                "In here, attribute functions may not be defined locally",
                name_of_func);
            Error(msg.c_str());
            return kERR_UserError;
        }
        tqs[TQ::kImport] = false;
    }

    // Store the fact that this function has been declared within the struct declaration
    _sym[name_of_func].Parent = struct_of_func;
    SetFlag(_sym[name_of_func].Flags, kSFLG_StructMember, true);

    Vartype const return_vartype = is_setter ? kKW_Void : vartype;
    tqs[TQ::kAttribute] = false;
    ParseFuncdecl_MasterData2Sym(tqs, return_vartype, struct_of_func, name_of_func, false);

    ErrorType retval = ParseStruct_Attribute_ParamList(struct_of_func, name_of_func, is_setter, is_indexed, vartype);
    if (retval < 0) return retval;

    // When the function is defined, it won't have "attribute" set so don't set "attribute" here
   
    bool const body_follows = false; // we are within a struct definition
    return ParseFuncdecl_HandleFunctionOrImportIndex(tqs, struct_of_func, name_of_func, body_follows);
}

// We're in a struct declaration, parsing a struct attribute
ErrorType AGS::Parser::ParseStruct_Attribute(TypeQualifierSet tqs, Symbol stname, Symbol vname, Vartype vartype)
{
    size_t const declaration_start = _src.GetCursor();
    // "readonly" means that there isn't a setter function. The individual vartypes are not readonly.
    bool const attrib_is_readonly = tqs[TQ::kReadonly];
    tqs[TQ::kReadonly] = false;

    bool attrib_is_indexed = false;

    if (kKW_OpenBracket == _src.PeekNext())
    {
        attrib_is_indexed = true;
        _src.GetNext();
        if (kKW_CloseBracket != _src.GetNext())
        {
            Error("Cannot specify array size for attribute");
            return kERR_UserError;
        }
    }

    _sym[vname].SType = SymT::kAttribute;
    if (attrib_is_indexed)
        _sym[vname].Vartype = _sym.VartypeWith(VTT::kDynarray, _sym[vname].Vartype);

    // Declare attribute getter, e.g. get_ATTRIB()
    Symbol attrib_func = kKW_NoSymbol;
    bool const get_func_is_setter = false;
    ErrorType retval = ConstructAttributeFuncName(vname, get_func_is_setter, attrib_is_indexed, attrib_func);
    if (retval < 0) return retval;
    Symbol const get_func_name = MangleStructAndComponent(stname, attrib_func);
    retval = ParseStruct_Attribute_DeclareFunc(tqs, stname, get_func_name, get_func_is_setter, attrib_is_indexed, vartype);
    if (retval < 0) return retval;
    _sym.SetDeclared(get_func_name, declaration_start);

    if (attrib_is_readonly)
        return kERR_None;

    // Declare attribute setter, e.g. set_ATTRIB(value)
    bool const set_func_is_setter = true;
    retval = ConstructAttributeFuncName(vname, set_func_is_setter, attrib_is_indexed, attrib_func);
    if (retval < 0) return retval;
    Symbol const set_func_name = MangleStructAndComponent(stname, attrib_func);
    retval = ParseStruct_Attribute_DeclareFunc(tqs, stname, set_func_name, set_func_is_setter, attrib_is_indexed, vartype);
    if (retval < 0) return retval;
    _sym.SetDeclared(set_func_name, declaration_start);

    return kERR_None;
}

// We're parsing an array var.
ErrorType AGS::Parser::ParseArray(AGS::Symbol vname, AGS::Vartype &vartype)
{
    _src.GetNext(); // Eat '['

    if (PP::kPreAnalyze == _pp)
    {
        // Skip the sequence of [...]
        while (true)
        {
            ErrorType retval = SkipToClose(kKW_CloseBracket);
            if (retval < 0) return retval;
            if (kKW_OpenBracket != _src.PeekNext())
                return kERR_None;
            _src.GetNext(); // Eat '['
        }
    }

    if (kKW_CloseBracket == _src.PeekNext())
    {
        // Dynamic array
        _src.GetNext(); // Eat ']'
        if (vartype == kKW_String)
        {
            Error("Dynamic arrays of old-style strings are not supported");
            return kERR_UserError;
        }
        if (!_sym.IsAnyIntegerVartype(vartype) && !_sym.IsManagedVartype(vartype) && kKW_Float != vartype)
        {
            Error("Can only have dynamic arrays of integer types, float or managed structs. '%s' isn't any of this.", _sym.GetName(vartype).c_str());
            return kERR_UserError;
        }
        vartype = _sym.VartypeWith(VTT::kDynarray, vartype);
        return kERR_None;
    }

    std::vector<size_t> dims;

    // Static array
    while (true)
    {
        Symbol const dim_symbol = _src.GetNext();

        int dimension_as_int;
        ErrorType retval = IntLiteralOrConst2Value(dim_symbol, false, "Expected a constant integer value for array dimension", dimension_as_int);
        if (retval < 0) return retval;

        if (dimension_as_int < 1)
        {
            Error("Array dimension must be at least 1, found %d instead", dimension_as_int);
            return kERR_UserError;
        }

        dims.push_back(dimension_as_int);

        Symbol const punctuation = _src.GetNext();
        retval = Expect(SymbolList{ kKW_Comma, kKW_CloseBracket }, punctuation);
        if (retval < 0) return retval;
        if (kKW_Comma == punctuation)
            continue;
        if (kKW_OpenBracket != _src.PeekNext())
            break;
        _src.GetNext(); // Eat '['
    }
    vartype = _sym.VartypeWithArray(dims, vartype);
    return kERR_None;
}

// We're inside a struct decl, processing a member variable or a member attribute
ErrorType AGS::Parser::ParseStruct_VariableOrAttributeDefn(TypeQualifierSet tqs, Vartype vartype, Symbol stname, Symbol vname, size_t &size_so_far)
{
    if (PP::kMain == _pp)
    {
        if (_sym.IsBuiltin(vartype) && !_sym.IsDynVartype(vartype))
        {
            Error("'%s' is a builtin non-managed struct; struct members of that type are not supported",
                _sym.GetName(vartype).c_str());
            return kERR_UserError;
        }

        if (tqs[TQ::kImport] && !tqs[TQ::kAttribute])
        {
            // member variable cannot be an import
            Error("Can't import struct component variables; import the whole struct instead");
            return kERR_UserError;
        }

        if (_sym.IsManagedVartype(vartype) && _sym.IsManagedVartype(stname) && !tqs[TQ::kAttribute])
        {
            // This is an Engine restriction
            Error("Cannot currently have managed variable components in managed struct");
            return kERR_UserError;
        }

        SymbolTableEntry &entry = _sym[vname];
        entry.SType = SymT::kStructComponent;
        entry.Parent = stname;  // save which struct it belongs to
        entry.SOffset = size_so_far;
        entry.Vartype = vartype;
        entry.TypeQualifiers = tqs;
        // "autoptr", "managed" and "builtin" are aspects of the vartype, not of the variable having the vartype
        entry.TypeQualifiers[TQ::kAutoptr] = false;
        entry.TypeQualifiers[TQ::kManaged] = false;
        entry.TypeQualifiers[TQ::kBuiltin] = false;
    }

    if (tqs[TQ::kAttribute])
        return ParseStruct_Attribute(tqs, stname, vname, vartype);

    if (_src.PeekNext() == kKW_OpenBracket)
    {
        Vartype vartype = _sym[vname].Vartype;
        ErrorType retval = ParseArray(vname, vartype);
        if (retval < 0) return retval;
        _sym[vname].Vartype = vartype;
    }

    size_so_far += _sym.GetSize(vname);
    return kERR_None;
}

ErrorType AGS::Parser::ParseStruct_MemberDefn(Symbol name_of_struct, TypeQualifierSet tqs, Vartype vartype, size_t &size_so_far)
{
    // Get the variable or function name.
    Symbol component;
    ErrorType retval = ParseVarname(false, name_of_struct, component);
    if (retval < 0) return retval;

    Symbol const var_or_func_name = MangleStructAndComponent(name_of_struct, component);
    bool const is_function = (kKW_OpenParenthesis == _src.PeekNext());

    // In here, all struct members get this flag, functions included
    // This flag shows that the respective member has been declared within a struct xx {  }
    SetFlag(_sym[var_or_func_name].Flags, kSFLG_StructMember, true);
    _sym[var_or_func_name].Parent = name_of_struct;
    _sym[name_of_struct].Children.push_back(var_or_func_name);

    if (is_function)
        return ParseStruct_FuncDecl(name_of_struct, var_or_func_name, tqs, vartype);

    size_t const declaration_start = _src.GetCursor();
    if (_sym.IsDynarrayVartype(vartype)) // e.g., int [] zonk;
    {
        Error("Expected '('");
        return kERR_UserError;
    }

    if (PP::kMain == _pp)
    {
        if (SymT::kNoType != _sym.GetSymbolType(var_or_func_name))
        {
            std::string const msg = ReferenceMsgSym(
                "'%s' is already defined", var_or_func_name);
            Error(msg.c_str(), _sym.GetName(var_or_func_name).c_str());
            return kERR_UserError;
        }

        // Mustn't be in any ancester
        retval = ParseStruct_CheckForCompoInAncester(name_of_struct, component, _sym[name_of_struct].Parent);
        if (retval < 0) return retval;
    }

    retval =  ParseStruct_VariableOrAttributeDefn(tqs, vartype, name_of_struct, var_or_func_name, size_so_far);
    if (retval < 0) return retval;

    _sym.SetDeclared(var_or_func_name, declaration_start);
    return kERR_None;
}

ErrorType AGS::Parser::EatDynpointerSymbolIfPresent(Vartype vartype)
{
    if (kKW_Dynpointer != _src.PeekNext())
        return kERR_None;

    if (PP::kPreAnalyze == _pp || _sym.IsManagedVartype(vartype))
    {
        _src.GetNext(); // Eat '*'
        return kERR_None;
    }

    Error("Cannot use '*' on the non-managed type '%s'", _sym.GetName(vartype).c_str());
    return kERR_UserError;
}

ErrorType AGS::Parser::ParseStruct_Vartype(Symbol name_of_struct, TypeQualifierSet tqs, Vartype vartype, size_t &size_so_far)
{
    if (PP::kMain == _pp)
    {   // Check for illegal struct member types
        ErrorType retval = ParseStruct_CheckComponentVartype(name_of_struct, vartype);
        if (retval < 0) return retval;
    }

    SetDynpointerInManagedVartype(vartype);
    ErrorType retval = EatDynpointerSymbolIfPresent(vartype);
    if (retval < 0) return retval;

    // "int [] func(...)"
    retval = ParseDynArrayMarkerIfPresent(vartype);
    if (retval < 0) return retval;

    // "TYPE noloopcheck foo(...)"
    if (kKW_Noloopcheck == _src.PeekNext())
    {
        Error("Cannot use 'noloopcheck' here");
        return kERR_UserError;
    }  

    // We've accepted a type expression and are now reading vars or one func that should have this type.
    while (true)
    {
        retval = ParseStruct_MemberDefn(name_of_struct, tqs, vartype, size_so_far);
        if (retval < 0) return retval;

        Symbol const punctuation = _src.GetNext();
        retval = Expect(SymbolList{ kKW_Comma, kKW_Semicolon }, punctuation);
        if (retval < 0) return retval;
        if (kKW_Semicolon == punctuation)
            return kERR_None;
    }
}

// Handle a "struct" definition; we've already eaten the keyword "struct"
ErrorType AGS::Parser::ParseStruct(TypeQualifierSet tqs, Symbol &struct_of_current_func, Symbol &name_of_current_func)
{
    size_t const start_of_struct_decl = _src.GetCursor();

    // get token for name of struct
    Symbol const stname = _src.GetNext();

    if (SymT::kNoType !=  _sym.GetSymbolType(stname) &&
        SymT::kUndefinedStruct !=  _sym.GetSymbolType(stname))
    {
        std::string const msg = ReferenceMsgSym("'%s' is already defined", stname);
        Error(msg.c_str(), _sym.GetName(stname).c_str());
        return kERR_UserError;
    }

    ParseStruct_SetTypeInSymboltable(stname, tqs);

    // Declare the struct type that implements new strings
    if (tqs[TQ::kStringstruct])
    {
        if (_sym.GetStringStructSym() > 0 && stname != _sym.GetStringStructSym())
        {
            Error("The stringstruct type is already defined to be %s", _sym.GetName(_sym.GetStringStructSym()).c_str());
            return kERR_UserError;
        }
        _sym.SetStringStructSym(stname);
    }

    size_t size_so_far = 0; // Will sum up the size of the struct

    if (kKW_Extends == _src.PeekNext())
    {
        ErrorType retval = ParseStruct_ExtendsClause(stname, size_so_far);
        if (retval < 0) return retval;
    }

    // forward-declaration of struct type
    if (kKW_Semicolon == _src.PeekNext())
    {
        if (!tqs[TQ::kManaged])
        {
            Error("Forward-declared structs must be 'managed'");
            return kERR_UserError;
        }
        _src.GetNext(); // Eat ';'
        SymbolTableEntry &entry = _sym[stname];
        entry.SType = SymT::kUndefinedStruct;
        SetFlag(entry.Flags, kSFLG_StructManaged, true);
        entry.SSize = 0;
        return kERR_None;
    }

    ErrorType retval = Expect(kKW_OpenBrace, _src.GetNext());
    if (retval < 0) return retval;

    // Declaration of the components
    while (kKW_CloseBrace != _src.PeekNext())
    {
        currentline = _src.GetLinenoAt(_src.GetCursor());
        TypeQualifierSet tqs = {};
        retval = ParseQualifiers(tqs);
        if (retval < 0) return retval;
        bool const in_func_body = false;
        bool const in_struct_decl = true;
        retval = Parse_CheckTQ(tqs, in_func_body, in_struct_decl);
        if (retval < 0) return retval;

        Vartype vartype = _src.GetNext();

        retval = ParseStruct_Vartype(stname, tqs, vartype, size_so_far);
        if (retval < 0) return retval;
    }

    if (PP::kMain == _pp)
    {
        // round up size to nearest multiple of STRUCT_ALIGNTO
        if (0 != (size_so_far % STRUCT_ALIGNTO))
            size_so_far += STRUCT_ALIGNTO - (size_so_far % STRUCT_ALIGNTO);
        _sym[stname].SSize = size_so_far;
    }

    _src.GetNext(); // Eat '}'

    Symbol const nextsym = _src.PeekNext();
    if (kKW_Semicolon == nextsym)
    {
        if (tqs[TQ::kReadonly])
        {
            // Only now do we find out that there isn't any following declaration
            // so "readonly" was incorrect. 
            // Back up the cursor for the error message.
            _src.SetCursor(start_of_struct_decl);
            Error("'readonly' can only be used in a variable declaration");
            return kERR_UserError;
        }
        _src.GetNext(); // Eat ';'
        return kERR_None;
    }

    // If this doesn't seem to be a declaration at first glance,
    // warn that the user might have forgotten a ';'.
    SymbolType type_of_next = _sym.GetSymbolType(nextsym);
    if (SymT::kNoType != type_of_next &&
        SymT::kFunction != type_of_next &&
        SymT::kGlobalVar != type_of_next &&
        SymT::kLocalVar != type_of_next &&
        kKW_Noloopcheck != nextsym && 
        kKW_Dynpointer != nextsym)
    {
        Error("Unexpected '%s' (did you forget a ';'?)", _sym.GetName(nextsym).c_str());
        return kERR_UserError;
    }

    // Take struct that has just been defined as the vartype of a declaration
    return ParseVartype(stname, tqs, struct_of_current_func, name_of_current_func);
}

// We've accepted something like "enum foo { bar"; '=' follows
ErrorType AGS::Parser::ParseEnum_AssignedValue(int &current_constant_value)
{
    _src.GetNext(); // eat "="

    // Get the value of the item
    Symbol item_value = _src.GetNext(); // may be '-', too
    bool is_neg = false;
    if (item_value == _sym.Find("-"))
    {
        is_neg = true;
        item_value = _src.GetNext();
    }

    return IntLiteralOrConst2Value(item_value, is_neg, "Expected integer or integer constant after '='", current_constant_value);
}

void AGS::Parser::ParseEnum_Item2Symtable(AGS::Symbol enum_name, AGS::Symbol item_name, int current_constant_value)
{
    SymbolTableEntry &entry = _sym[item_name];

    entry.SType = SymT::kConstant;
    entry.Vartype = enum_name;
    entry.SScope = 0u;
    entry.TypeQualifiers = {}; TQ::kReadonly;
    entry.TypeQualifiers[TQ::kReadonly] = true;
    entry.Parent = enum_name;
    // SOffset is unused for a constant, so in a gratuitous hack we use it to store the enum's value
    entry.SOffset = current_constant_value;
    if (PP::kMain == _pp)
        _sym.SetDeclared(item_name, _src.GetCursor());

    _sym[enum_name].Children.push_back(item_name);
}

ErrorType AGS::Parser::ParseEnum_Name2Symtable(AGS::Symbol enumName)
{
    SymbolTableEntry &entry = _sym[enumName];

    if (SymT::kNoType != entry.SType)
    {
        std::string msg = ReferenceMsgLoc("'%s' is already defined", entry.Declared);
        Error(msg.c_str(), _sym.GetName(enumName).c_str());
        return kERR_UserError;
    }

    entry.SType = SymT::kVartype;
    entry.SSize = SIZE_OF_INT;
    entry.Vartype = kKW_Int;

    return kERR_None;
}

// enum eEnumName { value1, value2 };
// We've already eaten "enum"
ErrorType AGS::Parser::ParseEnum(TypeQualifierSet tqs, Symbol &struct_of_current_func, Symbol &name_of_current_func)
{
    size_t const start_of_enum_decl = _src.GetCursor();
    if (kKW_NoSymbol !=  name_of_current_func)
    {
        Error("Enum declaration is not allowed within a function body");
        return kERR_UserError;
    }
    if (tqs[TQ::kBuiltin])
    {
        Error("'builtin' can only be used in a struct declaration");
        return kERR_UserError;
    }

    // Get name of the enum, enter it into the symbol table
    Symbol enum_name = _src.GetNext();
    ErrorType retval = ParseEnum_Name2Symtable(enum_name);
    if (retval < 0) return retval;

    retval = Expect(kKW_OpenBrace, _src.GetNext());
    if (retval < 0) return retval;

    int current_constant_value = 0;

    while (true)
    {
        Symbol item_name = _src.GetNext();
        if (kKW_CloseBrace == item_name)
            break; // item list empty or ends with trailing ','

        if (PP::kMain == _pp)
        {
            if (SymT::kConstant == _sym.GetSymbolType(item_name))
            {
                Error(
                    ReferenceMsgSym("'%s' is already defined as a constant or enum value", item_name).c_str(),
                    _sym.GetName(item_name).c_str());
                return kERR_UserError;
            }
            if (SymT::kNoType != _sym.GetSymbolType(item_name))
            {
                Error("Expected '}' or an unused identifier, found '%s' instead", _sym.GetName(item_name).c_str());
                return kERR_UserError;
            }
        }

        current_constant_value++;

        Symbol const punctuation = _src.PeekNext();
        retval = Expect(SymbolList{ kKW_Comma, kKW_Assign, kKW_CloseBrace }, punctuation);
        if (retval < 0) return retval;

        if (kKW_Assign == punctuation)
        {
            // the value of this entry is specified explicitly
            retval = ParseEnum_AssignedValue(current_constant_value);
            if (retval < 0) return retval;
        }

        // Enter this enum item as a constant int into the _sym table
        ParseEnum_Item2Symtable(enum_name, item_name, current_constant_value);

        Symbol const comma_or_brace = _src.GetNext();
        retval = Expect(SymbolList{ kKW_Comma, kKW_CloseBrace }, comma_or_brace);
        if (retval < 0) return retval;
        if (kKW_Comma == comma_or_brace)
            continue;
        break;
    }

    Symbol const nextsym = _src.PeekNext();
    if (kKW_Semicolon == nextsym)
    {
        _src.GetNext(); // Eat ';'
        if (tqs[TQ::kReadonly])
        {
            // Only now do we find out that there isn't any following declaration
            // so "readonly" was incorrect. 
            // Back up the cursor for the error message.
            _src.SetCursor(start_of_enum_decl);
            Error("'readonly' can only be used in a variable declaration");
            return kERR_UserError;
        }
        return kERR_None;
    }

    // If this doesn't seem to be a declaration at first glance,
    // warn that the user might have forgotten a ';'.
    
    SymbolType type_of_next = _sym.GetSymbolType(nextsym);
    if (SymT::kNoType != type_of_next &&
        SymT::kFunction != type_of_next &&
        SymT::kGlobalVar != type_of_next &&
        SymT::kLocalVar != type_of_next &&
        kKW_Noloopcheck != nextsym &&
        kKW_Dynpointer != nextsym)
    {
        Error("Unexpected '%s' (did you forget a ';'?)", _sym.GetName(nextsym).c_str());
        return kERR_UserError;
    }

    // Take enum that has just been defined as the vartype of a declaration
    return ParseVartype(enum_name, tqs, struct_of_current_func, name_of_current_func);
}

ErrorType AGS::Parser::ParseExport()
{
    if (PP::kPreAnalyze == _pp)
    {
        SkipTo(SymbolList{ kKW_Semicolon }, _src);
        _src.GetNext(); // Eat ';'
        return kERR_None;
    }

    // export specified symbols
    while (true) 
    {
        Symbol const export_sym = _src.GetNext();
        SymbolType const export_type = _sym.GetSymbolType(export_sym);
        if ((export_type != SymT::kGlobalVar) && (export_type != SymT::kFunction))
        {
            Error("Can only export global variables and functions, not '%s'", _sym.GetName(export_sym).c_str());
            return kERR_UserError;
        }
        if (_sym.IsImport(export_sym))
        {
            Error("Cannot export the imported '%s'", _sym.GetName(export_sym).c_str());
            return kERR_UserError;
        }
        if (kKW_String == _sym.GetVartype(export_sym))
        {
            Error("Cannot export 'string'; use char[200] instead");
            return kERR_UserError;
        }
        // if all functions are being exported anyway, don't bother doing it now
        if (!(0 != ccGetOption(SCOPT_EXPORTALL) && SymT::kFunction == export_type))
        {
            ErrorType retval = static_cast<ErrorType>(_scrip.add_new_export(
                _sym.GetName(export_sym).c_str(),
                (SymT::kGlobalVar == export_type) ? EXPORT_DATA : EXPORT_FUNCTION,
                _sym[export_sym].SOffset,
                _sym[export_sym].GetNumOfFuncParams() + 100 * _sym[export_sym].SScope));
            if (retval < 0) return retval;
        }

        Symbol const punctuation = _src.GetNext();
        ErrorType retval = Expect(SymbolList{ kKW_Comma, kKW_Semicolon,  }, punctuation);
        if (retval < 0) return retval;
        if (kKW_Semicolon == punctuation)
            break;
    }

    return kERR_None;
}
ErrorType AGS::Parser::ParseVartype_CheckForIllegalContext()
{
    NSType const ns_type = _nest.Type();
    if (NSType::kSwitch == ns_type)
    {
        Error("Cannot use declarations directly within a switch body. (Put \"{ ... }\" around the case statements)");
        return kERR_UserError;
    }

    if (NSType::kBraces == ns_type || NSType::kFunction == ns_type || NSType::kNone == ns_type)
        return kERR_None;

    Error("A declaration cannot be the sole body of an 'if', 'else' or loop clause");
    return kERR_UserError;
}

ErrorType AGS::Parser::ParseVartype_CheckIllegalCombis(bool is_function, AGS::TypeQualifierSet tqs)
{
    if (tqs[TQ::kStatic] && !is_function)
    {
        Error("'static' can only be applied to functions that are members of a struct");
        return kERR_UserError;
    }

    // Note: 'protected' is valid for struct functions; those can be defined directly,
    // as in int strct::function(){} or extender, as int function(this strct){}
    // We can't know at this point whether the function is extender, so we can't
    // check  at this point whether 'protected' is allowed.

    if (tqs[TQ::kReadonly] && is_function)
    {
        Error("Readonly cannot be applied to a function");
        return kERR_UserError;
    }

    if (tqs[TQ::kWriteprotected] && is_function)
    {
        Error("'writeprotected' cannot be applied to a function");
        return kERR_UserError;
    }

    return kERR_None;
}

ErrorType AGS::Parser::ParseVartype_FuncDecl(TypeQualifierSet tqs, Vartype vartype, Symbol struct_name, Symbol func_name, bool no_loop_check, Symbol &struct_of_current_func, Symbol &name_of_current_func, bool &body_follows)
{
    size_t const declaration_start = _src.GetCursor();
    _src.GetNext(); // Eat '('

    if (0 >= struct_name)
    {
        bool const func_is_static_extender = (kKW_Static == _src.PeekNext());
        bool const func_is_extender = func_is_static_extender || (kKW_This == _src.PeekNext());

        if (func_is_extender)
        {
            // Rewrite extender function as a component function of the corresponding struct.
            ErrorType retval = ParseFuncdecl_ExtenderPreparations(func_is_static_extender, struct_name, func_name, tqs);
            if (retval < 0) return retval;
        }

    }

    // Do not set .Extends or the StructComponent flag here. These denote that the
    // func has been either declared within the struct definition or as extender.

    ErrorType retval = ParseFuncdecl(declaration_start, tqs, vartype, struct_name, func_name, false, body_follows);
    if (retval < 0) return retval;
        
    if (!body_follows)
        return kERR_None;

    if (0 < name_of_current_func)
    {
        Error(
            ReferenceMsgSym("Function bodies cannot nest, but the body of function %s is still open. (Did you forget a '}'?)", func_name).c_str(),
            _sym.GetName(name_of_current_func).c_str());
        return kERR_UserError;
    }

    if (no_loop_check)
        SetFlag(_sym[func_name].Flags, kSFLG_NoLoopCheck, true);

    // We've started a function, remember what it is.
    name_of_current_func = func_name;
    struct_of_current_func = struct_name;
    return kERR_None;
}

ErrorType AGS::Parser::ParseVartype_VarDecl_PreAnalyze(AGS::Symbol var_name, ScopeType scope_type)
{
    if (0 != _givm.count(var_name))
    {
        if (_givm[var_name])
        {
            Error("'%s' is already defined as a global non-import variable", _sym.GetName(var_name).c_str());
            return kERR_UserError;
        }
        else if (ScT::kGlobal == scope_type && 0 != ccGetOption(SCOPT_NOIMPORTOVERRIDE))
        {
            Error("'%s' is defined as an import variable; that cannot be overridden here", _sym.GetName(var_name).c_str());
            return kERR_UserError;
        }
    }
    _givm[var_name] = (ScT::kGlobal == scope_type);

    // Apart from this, we aren't interested in var defns at this stage, so skip this defn
    SkipTo(SymbolList{ kKW_Comma, kKW_Semicolon }, _src);
    return kERR_None;
}

ErrorType AGS::Parser::ParseVartype_VarDecl(Symbol var_name, ScopeType scope_type, TypeQualifierSet tqs, Vartype vartype)
{
    if (PP::kPreAnalyze == _pp)
        return ParseVartype_VarDecl_PreAnalyze(var_name, scope_type);

    _sym[var_name].TypeQualifiers = tqs;
    // "autoptr", "managed" and "builtin" are aspects of the vartype, not of the variable having the vartype.
    _sym[var_name].TypeQualifiers[TQ::kAutoptr] = false;
    _sym[var_name].TypeQualifiers[TQ::kManaged] = false;
    _sym[var_name].TypeQualifiers[TQ::kBuiltin] = false;
    if (tqs[TQ::kStatic])
    {
        Error("'static' cannot be used in a variable declaration");
        return kERR_UserError;
    }
    ErrorType retval = Parse_CheckTQ(tqs, (_nest.TopLevel() > SymbolTableEntry::kParameterSScope), std::string::npos != _sym.GetName(var_name).rfind(':'));
    if (retval < 0) return retval;

    // parse the definition
    return ParseVardecl(var_name, vartype, scope_type);
}

// We accepted a variable type such as "int", so what follows is a function or variable declaration
ErrorType AGS::Parser::ParseVartype(Vartype vartype, TypeQualifierSet tqs, Symbol &struct_of_current_func, AGS::Symbol &name_of_current_func)
{
    if (_src.ReachedEOF())
    {
        Error("Unexpected end of input (did you forget ';'?)");
        return kERR_UserError;
    }
    if (tqs[TQ::kBuiltin])
    {
        Error("'builtin' can only be used in a struct declaration");
        return kERR_UserError;
    }

    // Don't define variable or function where illegal in context.
    ErrorType retval = ParseVartype_CheckForIllegalContext();
    if (retval < 0) return retval;

    ScopeType const scope_type = 
        (kKW_NoSymbol != name_of_current_func) ? ScT::kLocal :
        (tqs[TQ::kImport]) ? ScT::kImport : ScT::kGlobal;

    // Only imply a pointer for a managed entity if it isn't imported.
    if ((ScT::kImport == scope_type && kKW_Dynpointer == _src.PeekNext()) ||
        (ScT::kImport != scope_type && _sym.IsManagedVartype(vartype)))
    {
        vartype = _sym.VartypeWith(VTT::kDynpointer, vartype);
    }

    retval = EatDynpointerSymbolIfPresent(vartype);
    if (retval < 0) return retval;

    // "int [] func(...)"
    retval = ParseDynArrayMarkerIfPresent(vartype);
    if (retval < 0) return retval;

    // Look for "noloopcheck"; if present, gobble it and set the indicator
    // "TYPE noloopcheck foo(...)"
    bool const no_loop_check = (kKW_Noloopcheck == _src.PeekNext());
    if (no_loop_check)
        _src.GetNext();

    // We've accepted a vartype expression and are now reading vars or one func that should have this type.
    while(true)
    {
        // Get the variable or function name.
        Symbol var_or_func_name = kKW_NoSymbol;
        Symbol struct_name = kKW_NoSymbol;
        retval = ParseVarname(true, struct_name, var_or_func_name);
        if (retval < 0) return retval;

        bool const is_function = (kKW_OpenParenthesis == _src.PeekNext());

        // certain qualifiers, such as "static" only go with certain kinds of definitions.
        retval = ParseVartype_CheckIllegalCombis(is_function, tqs);
        if (retval < 0) return retval;

        if (is_function)
        {
            // Do not set .Extends or the StructComponent flag here. These denote that the
            // func has been either declared within the struct definition or as extender,
            // so they are NOT set unconditionally
            bool body_follows = false;
            retval = ParseVartype_FuncDecl(tqs, vartype, struct_name, var_or_func_name, no_loop_check, struct_of_current_func, name_of_current_func, body_follows);
            if (retval < 0) return retval;
            if (body_follows)
                return kERR_None;
        }
        else if (_sym.IsDynarrayVartype(vartype) || no_loop_check) // e.g., int [] zonk;
        {
            Error("Expected '('");
            return kERR_UserError;
        }
        else
        {
            if (kKW_NoSymbol != struct_name)
            {
                Error("Variable may not contain '::'");
                return kERR_UserError;
            }
            retval = ParseVartype_VarDecl(var_or_func_name, scope_type, tqs, vartype);
            if (retval < 0) return retval;
        }

        Symbol const punctuation = _src.GetNext();
        retval = Expect(SymbolList{ kKW_Comma, kKW_Semicolon }, punctuation);
        if (retval < 0) return retval;
        if (kKW_Semicolon == punctuation)
            return kERR_None;
    }
}

ErrorType AGS::Parser::HandleEndOfCompoundStmts()
{
    ErrorType retval;
    while (_nest.TopLevel() > SymbolTableEntry::kFunctionSScope)
        switch (_nest.Type())
        {
        default:
            Error("!Nesting of unknown type ends");
            return kERR_InternalError;

        case NSType::kBraces:
        case NSType::kSwitch:
            // The body of those statements can only be closed by an explicit '}'.
            // So that means that there cannot be any more non-braced compound statements to close here.
            return kERR_None;

        case NSType::kDo:
            retval = HandleEndOfDo();
            if (retval < 0) return retval;
            break;

        case NSType::kElse:
            retval = HandleEndOfElse();
            if (retval < 0) return retval;
            break;

        case NSType::kIf:
        {
            bool else_follows;
            retval = HandleEndOfIf(else_follows);
            if (retval < 0 || else_follows)
                return retval;
            break;
        }

        case NSType::kWhile:
            retval = HandleEndOfWhile();
            if (retval < 0) return retval;
            break;
        } // switch (nesting_stack->Type())

    return kERR_None;
}

ErrorType AGS::Parser::ParseReturn(AGS::Symbol name_of_current_func)
{
    Symbol const functionReturnType = _sym[name_of_current_func].FuncParamVartypes[0];

    if (kKW_Semicolon != _src.PeekNext())
    {
        if (functionReturnType == kKW_Void)
        {
            Error("Cannot return value from void function");
            return kERR_UserError;
        }

        // parse what is being returned
        ErrorType retval = ParseExpression();
        if (retval < 0) return retval;

        // If we need a string object ptr but AX contains a normal string, convert AX
        ConvertAXStringToStringObject(functionReturnType);

        // check return type is correct
        retval = IsVartypeMismatch(_scrip.ax_vartype, functionReturnType, true);
        if (retval < 0) return retval;

        if (_sym.IsOldstring(_scrip.ax_vartype) &&
            (ScT::kLocal == _scrip.ax_scope_type))
        {
            Error("Cannot return local string from function");
            return kERR_UserError;
        }
    }
    else if (_sym.IsAnyIntegerVartype(functionReturnType))
    {
        WriteCmd(SCMD_LITTOREG, SREG_AX, 0);
    }
    else if (kKW_Void != functionReturnType)
    {
        Error("Must return a '%s' value from function", _sym.GetName(functionReturnType).c_str());
        return kERR_UserError;
    }

    ErrorType retval = Expect(kKW_Semicolon, _src.GetNext());
    if (retval < 0) return retval;

    // If locals contain pointers, free them
    if (_sym.IsDynVartype(functionReturnType))
        FreeDynpointersOfAllLocals_DynResult(); // Special protection for result needed
    else if (kKW_Void != functionReturnType)
        FreeDynpointersOfAllLocals_KeepAX();
    else 
        FreeDynpointersOfLocals(0u);

    size_t const save_offset = _scrip.offset_to_local_var_block;
    // Pop the local variables proper from the stack but leave the parameters.
    // This is important because the return address is directly above the parameters;
    // we need the return address to return. (The caller will pop the parameters later.)
    RemoveLocalsFromStack(SymbolTableEntry::kFunctionSScope);
  
    // Jump to the exit point of the function
    WriteCmd(SCMD_JMP, 0);
    _nest.JumpOut(SymbolTableEntry::kParameterSScope).AddParam();

    // The locals only disappear if control flow actually follows the "return"
    // statement. Otherwise, below the statement, the locals remain on the stack.
    // So restore the offset_to_local_var_block.
    _scrip.offset_to_local_var_block = save_offset;
    return kERR_None;
}

// Evaluate the header of an "if" clause, e.g. "if (i < 0)".
ErrorType AGS::Parser::ParseIf()
{
    ErrorType retval = ParseParenthesizedExpression();
    if (retval < 0) return retval;

    _nest.Push(NSType::kIf);

    // The code that has just been generated has put the result of the check into AX
    // Generate code for "if (AX == 0) jumpto X", where X will be determined later on.
    WriteCmd(SCMD_JZ, -77);
    _nest.JumpOut().AddParam();

    return kERR_None;
}

ErrorType AGS::Parser::HandleEndOfIf(bool &else_follows)
{
    if (kKW_Else != _src.PeekNext())
    {
        else_follows = false;
        _nest.JumpOut().Patch(_src.GetLineno());
        _nest.Pop();
        return kERR_None;
    }

    else_follows = true;
    _src.GetNext(); // Eat "else"
    // Match the 'else' clause that is following to this 'if' stmt:
    // So we're at the end of the "then" branch. Jump out.
    _scrip.write_cmd(SCMD_JMP, -77);
    // So now, we're at the beginning of the "else" branch.
    // The jump after the "if" condition should go here.
    _nest.JumpOut().Patch(_src.GetLineno());
    // Mark the  out jump after the "then" branch, above, for patching.
    _nest.JumpOut().AddParam();
    // To prevent matching multiple else clauses to one if
    _nest.SetType(NSType::kElse);
    return kERR_None;
}

// Evaluate the head of a "while" clause, e.g. "while (i < 0)" 
ErrorType AGS::Parser::ParseWhile()
{
    // point to the start of the code that evaluates the condition
    CodeLoc const condition_eval_loc = _scrip.codesize;

    ErrorType retval = ParseParenthesizedExpression();
    if (retval < 0) return retval;

    _nest.Push(NSType::kWhile);

    // Now the code that has just been generated has put the result of the check into AX
    // Generate code for "if (AX == 0) jumpto X", where X will be determined later on.
    WriteCmd(SCMD_JZ, -77);
    _nest.JumpOut().AddParam();
    _nest.Start().Set(condition_eval_loc);

    return kERR_None;
}

ErrorType AGS::Parser::HandleEndOfWhile()
{
    // if it's the inner level of a 'for' loop,
    // drop the yanked chunk (loop increment) back in
    if (_nest.ChunksExist())
    {
        int id;
        CodeLoc const write_start = _scrip.codesize;
        _nest.WriteChunk(0u, id);
        _fcm.UpdateCallListOnWriting(write_start, id);
        _fim.UpdateCallListOnWriting(write_start, id);
        _nest.Chunks().clear();
    }

    // jump back to the start location
    _nest.Start().WriteJump(SCMD_JMP, _src.GetLineno());

    // This ends the loop
    _nest.JumpOut().Patch(_src.GetLineno());
    _nest.Pop();

    if (NSType::kFor != _nest.Type())
        return kERR_None;

    // This is the outer level of the FOR loop.
    // It can contain defns, e.g., "for (int i = 0;...)".
    // (as if it were surrounded in braces). Free these definitions
    return HandleEndOfBraceCommand();
}

ErrorType AGS::Parser::ParseDo()
{
    _nest.Push(NSType::kDo);
    _nest.Start().Set();
    return kERR_None;
}

ErrorType AGS::Parser::HandleEndOfBraceCommand()
{
    size_t const depth = _nest.TopLevel();
    FreeDynpointersOfLocals(depth);
    RemoveLocalsFromStack(depth);
    RemoveLocalsFromSymtable(depth);
    _nest.Pop();
    return kERR_None;
}

ErrorType AGS::Parser::ParseAssignmentOrExpression(AGS::Symbol cursym)
{    
    // Get expression
    _src.BackUp(); // Expression starts with cursym: the symbol in front of the cursor.
    size_t const expr_start = _src.GetCursor();
    ErrorType retval = SkipToEndOfExpression();
    if (retval < 0) return retval;
    SrcList expression = SrcList(_src, expr_start, _src.GetCursor() - expr_start);

    if (expression.Length() == 0)
    {
        Error("Unexpected symbol '%s' at start of statement", _sym.GetName(_src.GetNext()).c_str());
        return kERR_UserError;
    }

    Symbol const nextsym = _src.PeekNext();
    SymbolType const nexttype = _sym.GetSymbolType(nextsym);
    if (SymT::kAssign == nexttype || SymT::kAssignMod == nexttype || SymT::kAssignSOp == nexttype)
    {
        _src.GetNext(); // Eat assignment symbol
        return ParseAssignment(nextsym, expression);
    }

    // So this must be an isolated expression such as a function call. 
    ValueLocation vloc;
    ScopeType scope_type;
    Vartype vartype;
    retval = ParseExpression_Term(expression, vloc, scope_type, vartype);
    if (retval < 0) return retval;
    return ResultToAX(vloc, scope_type, vartype);
}

ErrorType AGS::Parser::ParseFor_InitClauseVardecl()
{
    Vartype vartype = _src.GetNext();
    SetDynpointerInManagedVartype(vartype);
    ErrorType retval = EatDynpointerSymbolIfPresent(vartype);
    if (retval < 0) return retval;

    while (true)
    {
        Symbol varname = _src.GetNext();
        Symbol const nextsym = _src.PeekNext();
        if (kKW_ScopeRes == nextsym || kKW_OpenParenthesis == nextsym)
        {
            Error("Function definition not allowed in for loop initialiser");
            return kERR_UserError;
        }

        retval = ParseVardecl(varname, vartype, ScT::kLocal);
        if (retval < 0) return retval;

        Symbol const punctuation = _src.PeekNext();
        retval = Expect(SymbolList{ kKW_Comma, kKW_Semicolon }, punctuation);
        if (retval < 0) return retval;
        if (kKW_Comma == punctuation)
            _src.GetNext(); // Eat ','
        if (kKW_Semicolon == punctuation)
            return kERR_None;
    }
}

// The first clause of a 'for' header
ErrorType AGS::Parser::ParseFor_InitClause(AGS::Symbol peeksym)
{
    if (kKW_Semicolon == peeksym)
        return kERR_None; // Empty init clause
    if (SymT::kVartype == _sym.GetSymbolType(peeksym))
        return ParseFor_InitClauseVardecl();
    return ParseAssignmentOrExpression(_src.GetNext());
}

ErrorType AGS::Parser::ParseFor_WhileClause()
{
    // Make the last emitted line number invalid so that a linenumber bytecode is emitted
    _scrip.last_emitted_lineno = INT_MAX;
    if (kKW_Semicolon == _src.PeekNext())
    {
        // Not having a while clause is tantamount to the while condition "true".
        // So let's write "true" to the AX register.
        WriteCmd(SCMD_LITTOREG, SREG_AX, 1);
        return kERR_None;
    }

    return ParseExpression();
}

ErrorType AGS::Parser::ParseFor_IterateClause()
{
    if (kKW_CloseParenthesis == _src.PeekNext())
        return kERR_None; // iterate clause is empty

    return ParseAssignmentOrExpression(_src.GetNext());
}

ErrorType AGS::Parser::ParseFor()
{
    // "for (I; E; C) {...}" is equivalent to "{ I; while (E) {...; C} }"
    // We implement this with TWO levels of the nesting stack.
    // The outer level contains "I"
    // The inner level contains "while (E) { ...; C}"

    // Outer level
    _nest.Push(NSType::kFor);

    ErrorType retval = Expect(kKW_OpenParenthesis, _src.GetNext());
    if (retval < 0) return retval;

    Symbol const peeksym = _src.PeekNext();
    if (kKW_CloseParenthesis == peeksym)
    {
        Error("Empty parentheses '()' aren't allowed after 'for' (write 'for(;;)' instead");
        return kERR_UserError;
    }

    // Initialization clause (I)
    retval = ParseFor_InitClause(peeksym);
    if (retval < 0) return retval;

    retval = Expect(kKW_Semicolon, _src.GetNext(), "Expected ';' after for loop initializer clause");
    if (retval < 0) return retval;

    // Remember where the code of the while condition starts.
    CodeLoc const while_cond_loc = _scrip.codesize;

    retval = ParseFor_WhileClause();
    if (retval < 0) return retval;

    retval = Expect(kKW_Semicolon, _src.GetNext(), "Expected ';' after for loop while clause");
    if (retval < 0) return retval;

    // Remember where the code of the iterate clause starts.
    CodeLoc const iterate_clause_loc = _scrip.codesize;
    size_t const iterate_clause_fixups_start = _scrip.numfixups;
    size_t const iterate_clause_lineno = _src.GetLineno();

    retval = ParseFor_IterateClause();
    if (retval < 0) return retval;

    retval = Expect(kKW_CloseParenthesis, _src.GetNext(), "Expected ')' after for loop iterate clause");
    if (retval < 0) return retval;

    // Inner nesting level
    _nest.Push(NSType::kWhile);
    _nest.Start().Set(while_cond_loc);

    // We've just generated code for getting to the next loop iteration.
     // But we don't need that code right here; we need it at the bottom of the loop.
     // So rip it out of the bytecode base and save it into our nesting stack.
    int id;
    size_t const yank_size = _scrip.codesize - iterate_clause_loc;
    _nest.YankChunk(iterate_clause_lineno, iterate_clause_loc, iterate_clause_fixups_start, id);
    _fcm.UpdateCallListOnYanking(iterate_clause_loc, yank_size, id);
    _fim.UpdateCallListOnYanking(iterate_clause_loc, yank_size, id);

    // Code for "If the expression we just evaluated is false, jump over the loop body."
    WriteCmd(SCMD_JZ, -77);
    _nest.JumpOut().AddParam();

    return kERR_None;
}

ErrorType AGS::Parser::ParseSwitch()
{
    // Get the switch expression
    ErrorType retval = ParseParenthesizedExpression();
    if (retval < 0) return retval;

    // Remember the type of this expression to enforce it later
    Vartype const switch_expr_vartype = _scrip.ax_vartype;

    // Copy the result to the BX register, ready for case statements
    WriteCmd(SCMD_REGTOREG, SREG_AX, SREG_BX);

    retval = Expect(kKW_OpenBrace, _src.GetNext());
    if (retval < 0) return retval;

    _nest.Push(NSType::kSwitch);
    _nest.SetSwitchExprVartype(switch_expr_vartype);
    _nest.SwitchDefault().Set(INT_MAX); // no default case encountered yet

    // Jump to the jump table
    _scrip.write_cmd(SCMD_JMP, -77);
    _nest.SwitchJumptable().AddParam();

    // Check that "default" or "case" follows
    if (_src.ReachedEOF())
    {
        Error("Unexpected end of input");
        return kERR_UserError;
    }

    return Expect(SymbolList{ kKW_Default, kKW_Case, kKW_CloseBrace }, _src.PeekNext());
}

ErrorType AGS::Parser::ParseSwitchLabel(Symbol case_or_default)
{
    if (NSType::kSwitch != _nest.Type())
    {
        Error("'%s' is only allowed directly within a 'switch' block", _sym.GetName(case_or_default).c_str());
        return kERR_UserError;
    }

    if (kKW_Default == case_or_default)
    {
        if (INT_MAX != _nest.SwitchDefault().Get())
        {
            Error("This switch block already has a 'default' label");
            return kERR_UserError;
        }
        _nest.SwitchDefault().Set();
    }
    else // "case"
    {
        CodeLoc const start_of_code_loc = _scrip.codesize;
        size_t const start_of_fixups = _scrip.numfixups;
        size_t const start_of_code_lineno = _src.GetLineno();

        PushReg(SREG_BX);   // Result of the switch expression

        ErrorType retval = ParseExpression(); // case n: label expression
        if (retval < 0) return retval;

        // Vartypes of the "case" expression and the "switch" expression must match
        retval = IsVartypeMismatch(_scrip.ax_vartype, _nest.SwitchExprVartype(), false);
        if (retval < 0) return retval;

        PopReg(SREG_BX);

        // rip out the already generated code for the case/switch and store it with the switch
        int id;
        size_t const yank_size = _scrip.codesize - start_of_code_loc;
        _nest.YankChunk(start_of_code_lineno, start_of_code_loc, start_of_fixups, id);
        _fcm.UpdateCallListOnYanking(start_of_code_loc, yank_size, id);
        _fim.UpdateCallListOnYanking(start_of_code_loc, yank_size, id);

        BackwardJumpDest case_code_start(_scrip);
        case_code_start.Set();
        _nest.SwitchCases().push_back(case_code_start);
    }

    return Expect(kKW_Colon, _src.GetNext());
}

ErrorType AGS::Parser::RemoveLocalsFromStack(size_t nesting_level)
{
    size_t const size_of_local_vars = StacksizeOfLocals(nesting_level);
    if (size_of_local_vars > 0)
    {
        _scrip.offset_to_local_var_block -= size_of_local_vars;
        WriteCmd(SCMD_SUB, SREG_SP, size_of_local_vars);
    }
    return kERR_None;
}

ErrorType AGS::Parser::ParseBreak()
{
    ErrorType retval = Expect(kKW_Semicolon, _src.GetNext());
    if (retval < 0) return retval;

    // Find the (level of the) looping construct to which the break applies
    // Note that this is similar, but _different_ from "continue".
    size_t nesting_level;
    for (nesting_level = _nest.TopLevel(); nesting_level > 0; nesting_level--)
    {
        NSType const ltype = _nest.Type(nesting_level);
        if (NSType::kDo == ltype || NSType::kSwitch == ltype || NSType::kWhile == ltype)
            break;
    }

    if (0u == nesting_level)
    {
        Error("'break' is only valid inside a loop or a switch statement block");
        return kERR_UserError;
    }

    size_t const save_offset = _scrip.offset_to_local_var_block;
    FreeDynpointersOfLocals(nesting_level + 1);
    RemoveLocalsFromStack(nesting_level + 1);
    
    // Jump out of the loop or switch
    WriteCmd(SCMD_JMP, -77);
    _nest.JumpOut(nesting_level).AddParam();

    // The locals only disappear if control flow actually follows the "break"
    // statement. Otherwise, below the statement, the locals remain on the stack.
    // So restore the offset_to_local_var_block.
    _scrip.offset_to_local_var_block = save_offset;
    return kERR_None;
}

ErrorType AGS::Parser::ParseContinue()
{
    ErrorType retval = Expect(kKW_Semicolon, _src.GetNext());
    if (retval < 0) return retval;

    // Find the level of the looping construct to which the break applies
    // Note that this is similar, but _different_ from "break".
    size_t nesting_level;
    for (nesting_level = _nest.TopLevel(); nesting_level > 0; nesting_level--)
    {
        NSType const ltype = _nest.Type(nesting_level);
        if (NSType::kDo == ltype || NSType::kWhile == ltype)
            break;
    }

    if (nesting_level == 0)
    {
        Error("'continue' is only valid inside a loop");
        return kERR_UserError;
    }

    size_t const save_offset = _scrip.offset_to_local_var_block;
    FreeDynpointersOfLocals(nesting_level + 1);
    RemoveLocalsFromStack(nesting_level + 1);

    // if it's a for loop, drop the yanked loop increment chunk in
    if (_nest.ChunksExist(nesting_level))
    {
        int id;
        CodeLoc const write_start = _scrip.codesize;
        _nest.WriteChunk(nesting_level, 0u, id);
        _fcm.UpdateCallListOnWriting(write_start, id);
        _fim.UpdateCallListOnWriting(write_start, id);
    }

    // Jump to the start of the loop
    _nest.Start(nesting_level).WriteJump(SCMD_JMP, _src.GetLineno());

    // The locals only disappear if control flow actually follows the "continue"
    // statement. Otherwise, below the statement, the locals remain on the stack.
     // So restore the offset_to_local_var_block.
    _scrip.offset_to_local_var_block = save_offset;
    return kERR_None;
}

ErrorType AGS::Parser::ParseCloseBrace()
{
    if (NSType::kSwitch == _nest.Type())
        return HandleEndOfSwitch();
    return HandleEndOfBraceCommand();
}

ErrorType AGS::Parser::ParseCommand(Symbol leading_sym, Symbol &struct_of_current_func, AGS::Symbol &name_of_current_func)
{
    ErrorType retval;

    // NOTE that some branches of this switch will leave
    // the whole function, others will continue after the switch.
    switch (leading_sym)
    {
    default:
    {
        // No keyword, so it should be an assignment or an isolated expression
        retval = ParseAssignmentOrExpression(leading_sym);
        if (retval < 0) return retval;
        retval = Expect(kKW_Semicolon, _src.GetNext());
        if (retval < 0) return retval;
        break;
    }

    case kKW_Break:
        retval = ParseBreak();
        if (retval < 0) return retval;
        break;

    case kKW_Case:
        retval = ParseSwitchLabel(leading_sym);
        if (retval < 0) return retval;
        break;

    case kKW_CloseBrace:
        // Note that the scanner has already made sure that every close brace has an open brace
        if (SymbolTableEntry::kFunctionSScope >= _nest.TopLevel())
            return HandleEndOfFuncBody(struct_of_current_func, name_of_current_func);

        retval = ParseCloseBrace();
        if (retval < 0) return retval;
        break;

    case kKW_Continue:
        retval = ParseContinue();
        if (retval < 0) return retval;
        break;

    case kKW_Default:
        retval = ParseSwitchLabel(leading_sym);
        if (retval < 0) return retval;
        break;

    case kKW_Do:
        return ParseDo();

    case kKW_Else:
        Error("Cannot find any 'if' clause that matches this 'else'");
        return kERR_UserError;

    case kKW_For:
        return ParseFor();

    case kKW_If:
        return ParseIf();

    case kKW_OpenBrace:
        if (SymbolTableEntry::kParameterSScope == _nest.TopLevel())
             return ParseFuncBodyStart(struct_of_current_func, name_of_current_func);
        _nest.Push(NSType::kBraces);
        return kERR_None;

    case kKW_Return:
        retval = ParseReturn(name_of_current_func);
        if (retval < 0) return retval;
        break;

    case kKW_Switch:
        retval = ParseSwitch();
        if (retval < 0) return retval;
        break;

    case kKW_While:
        // This cannot be the end of a do...while() statement
        // because that would have been handled in HandleEndOfDo()
        return ParseWhile();
    }

    // This statement may be the end of some unbraced
    // compound statements, e.g. "while (...) if (...) i++";
    // Pop the nesting levels of such statements and handle
    // the associated jumps.
    return HandleEndOfCompoundStmts();
}

void AGS::Parser::HandleSrcSectionChangeAt(size_t pos)
{
    size_t const src_section_id = _src.GetSectionIdAt(pos);
    if (src_section_id == _lastEmittedSectionId)
        return;

    if (PP::kMain == _pp)
        _scrip.start_new_section(_src.SectionId2Section(src_section_id));
    _lastEmittedSectionId = src_section_id;
}

ErrorType AGS::Parser::ParseInput()
{
    Parser::NestingStack nesting_stack(_scrip);
    size_t nesting_level = 0;

    // We start off in the global data part - no code is allowed until a function definition is started
    Symbol struct_of_current_func = kKW_NoSymbol; // non-zero only when a struct member function is open
    Symbol name_of_current_func = kKW_NoSymbol;

    // Collects vartype qualifiers such as 'readonly'
    TypeQualifierSet tqs = {};

    while (!_src.ReachedEOF())
    {
        size_t const next_pos = _src.GetCursor();
        HandleSrcSectionChangeAt(next_pos);
        currentline = _src.GetLinenoAt(next_pos);

        ErrorType retval = ParseQualifiers(tqs);
        if (retval < 0) return retval;

        Symbol const leading_sym = _src.GetNext();
        switch (leading_sym)
        {
        default:    // Construct does not begin with a keyword
        {
            SymbolType const leading_type = _sym.GetSymbolType(leading_sym);
            if (SymT::kNoType == leading_type)
            {   
                if (struct_of_current_func > 0)
                {
                    Symbol const mangled = MangleStructAndComponent(struct_of_current_func, leading_sym);
                    if (SymT::kNoType != _sym.GetSymbolType(mangled))
                        break; // "this" can be implied, so treat as a command
                }
                Error("Unexpected token '%s'", _sym.GetName(leading_sym).c_str());
                return kERR_UserError;
            }

            if (SymT::kVartype == leading_type)
            {
                if (kKW_Dot == _src.PeekNext())
                    break; // this refers to a static struct component, so treat as a command

                // We can't check yet whether the TQS are legal because we don't know whether the
                // var / func names are composite.
                Vartype const vartype = leading_sym;
                ErrorType retval = ParseVartype(vartype, tqs, struct_of_current_func, name_of_current_func);
                if (retval < 0) return retval;
                continue;
            }

            break; // Treat as a command
        }

        case kKW_Enum:
            retval = Parse_CheckTQ(tqs, (name_of_current_func > 0), false);
            if (retval < 0) return retval;
            retval = ParseEnum(tqs, struct_of_current_func, name_of_current_func);
            if (retval < 0) return retval;
            continue;

        case kKW_Export:
            retval = Parse_CheckEmpty(tqs); // No qualifiers in front of 'export' allowed
            if (retval < 0) return retval;
            retval = ParseExport();
            if (retval < 0) return retval;
            continue;

        case kKW_OpenBrace:
            if (PP::kMain == _pp)
                break; // treat as a command, handled below the switch

            retval = SkipToClose(kKW_CloseBrace);
            if (retval < 0) return retval;
            name_of_current_func = kKW_NoSymbol;
            struct_of_current_func = kKW_NoSymbol;
            continue;

        case kKW_Struct:
            retval = Parse_CheckTQ(tqs, (name_of_current_func > 0), false);
            if (retval < 0) return retval;
            retval = ParseStruct(tqs, struct_of_current_func, name_of_current_func);
            if (retval < 0) return retval;
            continue;
        } // switch (symType)

        // Commands are only allowed within a function
        if (kKW_NoSymbol == name_of_current_func)
        {
            Error("'%s' is illegal outside a function", _sym.GetName(leading_sym).c_str());
            return kERR_UserError;
        }

        // No qualifiers in front of a command allowed
        retval = Parse_CheckEmpty(tqs); 
        if (retval < 0) return retval;

        retval = ParseCommand(leading_sym, struct_of_current_func, name_of_current_func);
        if (retval < 0) return retval;
    } // while (!targ.reached_eof())

    return kERR_None;
}

ErrorType AGS::Parser::Parse_ReinitSymTable(const ::SymbolTable &sym_after_scanning)
{
    size_t const size_of_sym_after_scanning = sym_after_scanning.entries.size();
    SymbolTableEntry empty;

    for (size_t sym_idx = kKW_LastPredefined + 1; sym_idx < _sym.entries.size(); sym_idx++)
    {
        SymbolTableEntry &s_entry = _sym[sym_idx];
        if (SymT::kFunction == s_entry.SType)
        {
            s_entry.TypeQualifiers[TQ::kImport] = (kFT_Import == s_entry.SOffset);
            s_entry.SOffset = 0;
            continue;
        }
        if (sym_idx < size_of_sym_after_scanning)
        {
            s_entry = sym_after_scanning.entries[sym_idx];
            continue;
        }

        empty.SName = s_entry.SName;
        s_entry = empty;
    }

    // This has invalidated the symbol table caches, so kill them
    _sym.ResetCaches();

    return kERR_None;
}

ErrorType AGS::Parser::Parse_BlankOutUnusedImports()
{
    for (size_t entries_idx = 0; entries_idx < _sym.entries.size(); entries_idx++)
    {
        SymbolType const stype = _sym.GetSymbolType(entries_idx);
        // Don't mind attributes - they are shorthand for the respective getter
        // and setter funcs. If _those_ are unused, then they will be caught
        // in the same that way normal functions are.
        if (SymT::kFunction != stype && SymT::kGlobalVar != stype)
            continue;

        if (_sym[entries_idx].TypeQualifiers[TQ::kImport] &&
            !FlagIsSet(_sym[entries_idx].Flags, kSFLG_Accessed))
            _scrip.imports[_sym[entries_idx].SOffset][0] = '\0';
    }

    return kERR_None;
}

void AGS::Parser::MessageWithPosition(MessageHandler::Severity sev, int section_id, size_t lineno, char const *descr, ...)
{
    va_list vlist1, vlist2;
    va_start(vlist1, descr);
    va_copy(vlist2, vlist1);
    char *message = new char[vsnprintf(nullptr, 0, descr, vlist1) + 1];
    vsprintf(message, descr, vlist2);
    va_end(vlist2);
    va_end(vlist1);

    _msg_handler.AddMessage(
        sev,
        _src.SectionId2Section(section_id),
        lineno,
        message);

    delete[] message;
}

void AGS::Parser::Error(char const *descr, ...)
{
    // ErrorWithPosition() can't be called with a va_list and doesn't have a variadic variant,
    // so convert all the parameters into a single C string here
    va_list vlist1, vlist2;
    va_start(vlist1, descr);
    va_copy(vlist2, vlist1);
    char *message = new char[vsnprintf(nullptr, 0, descr, vlist1) + 1];
    vsprintf(message, descr, vlist2);
    va_end(vlist2);
    va_end(vlist1);

    _msg_handler.AddMessage(
        MessageHandler::kSV_Error,
        _src.SectionId2Section(_src.GetSectionId()),
        _src.GetLineno(),
        message);
    delete[] message;
}

void AGS::Parser::Warning(char const *descr, ...)
{
    va_list vlist1, vlist2;
    va_start(vlist1, descr);
    va_copy(vlist2, vlist1);
    char *message = new char[vsnprintf(nullptr, 0, descr, vlist1) + 1];
    vsprintf(message, descr, vlist2);
    va_end(vlist2);
    va_end(vlist1);

    _msg_handler.AddMessage(
        MessageHandler::kSV_Warning,
        _src.SectionId2Section(_src.GetSectionId()),
        _src.GetLineno(),
        message);
    delete[] message;
}

ErrorType AGS::Parser::Parse_PreAnalyzePhase()
{
    // Needed to partially reset the symbol table later on
    SymbolTable const sym_after_scanning(_sym);

    _pp = PP::kPreAnalyze;
    ErrorType retval = ParseInput();
    if (retval < 0) return retval;

    _fcm.Reset();

    // Keep (just) the headers of functions that have a body to the main symbol table
    // Reset everything else in the symbol table,
    // but keep the entries so that they are guaranteed to have
    // the same index when parsed in phase 2
    return Parse_ReinitSymTable(sym_after_scanning);
}

ErrorType AGS::Parser::Parse_MainPhase()
{
    _pp = PP::kMain;
    return ParseInput();
}

ErrorType AGS::Parser::Parse()
{
    CodeLoc const start_of_input = _src.GetCursor();

    ErrorType retval = Parse_PreAnalyzePhase();
    if (retval < 0) return retval;

    _src.SetCursor(start_of_input);
    retval = Parse_MainPhase();
    if (retval < 0) return retval;

    // If the following functions generate errors, they pertain to the source
    // as a whole. So let's generate them for the last source char. 
    size_t const last_pos = _src.Length() - 1;
    char const *current_section = _src.SectionId2Section(_src.GetSectionIdAt(last_pos)).c_str();
    strncpy(SectionNameBuffer, current_section, sizeof(SectionNameBuffer) / sizeof(char) - 1);
    ccCurScriptName = SectionNameBuffer;
    currentline = _src.GetLinenoAt(last_pos);

    retval = _fcm.CheckForUnresolvedFuncs();
    if (retval < 0) return retval;
    retval = _fim.CheckForUnresolvedFuncs();
    if (retval < 0) return retval;
    return Parse_BlankOutUnusedImports();
}

// Scan inpl into scan tokens, build a symbol table
int cc_scan(std::string const &inpl, SrcList &src, ccCompiledScript &scrip, SymbolTable &symt, MessageHandler &mh)
{
    AGS::Scanner scanner = { inpl, src, scrip, symt, mh };
    return scanner.Scan();
}

int cc_parse(AGS::SrcList &src, ccCompiledScript &scrip, SymbolTable &symt, MessageHandler &mh)
{
    AGS::Parser parser = { src, scrip, symt, mh };
    return parser.Parse();
}

int cc_compile(std::string const &inpl, ccCompiledScript &scrip, MessageHandler &mh)
{
    std::vector<Symbol> symbols;
    LineHandler lh;
    size_t cursor = 0u;
    SrcList src = SrcList(symbols, lh, cursor);
    src.NewSection("UnnamedSection");
    src.NewLine(1u);

    SymbolTable symt;

    ccCurScriptName = nullptr;

    int error_code = cc_scan(inpl, src, scrip, symt, mh);
    if (error_code >= 0)
        error_code = cc_parse(src, scrip, symt, mh);
    return error_code;
}

int cc_compile(std::string const &inpl, ccCompiledScript &scrip)
{
    MessageHandler mh;

    int error_code = cc_compile(inpl, scrip, mh);
    if (error_code >= 0)
    {
        // Here if there weren't any errors.
        return error_code;
    }

    // Here if there was an error. Scaffolding around cc_error()
    ccCurScriptName = SectionNameBuffer;
    MessageHandler::Entry const &err = mh.GetError();
    
    strncpy_s(
        SectionNameBuffer,
        err.Section.c_str(),
        sizeof(SectionNameBuffer) / sizeof(char) - 1);
    currentline = err.Lineno;
    cc_error(err.Message.c_str());
    return error_code;
}
