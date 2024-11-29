//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "script/systemimports.h"
#include <stdlib.h>
#include <string.h>
#include "script/cc_instance.h"

using namespace AGS::Common;

SystemImports simp;
SystemImports simp_for_plugin;


void ScriptSymbolsMap::add(const String &name, uint32_t index)
{
    _lookup[name] = index;
}

void ScriptSymbolsMap::remove(const String &name)
{
    _lookup.erase(name);
}

void ScriptSymbolsMap::clear()
{
    _lookup.clear();
}

uint32_t ScriptSymbolsMap::getIndexOf(const String &name) const
{
    auto it = _lookup.find(name);
    if (it != _lookup.end())
        return it->second;

    // Not found...
    return UINT32_MAX;
}

uint32_t ScriptSymbolsMap::getIndexOfAny(const String &name) const
{
    // Import names may be potentially formed as:
    //
    //     [type::]name[^argnum][^arglist]
    //
    // where "type" is the name of a type, "name" is the name of a function,
    // "argnum" is the number of arguments and "arglist" is a '^' separated
    // list of types of return value and function args.

    const size_t argnum_at = name.FindChar(_appendageSeparator);
    const size_t arglist_at = (argnum_at != String::NoIndex) ? name.FindChar(_appendageSeparator, argnum_at + 1) : String::NoIndex;
    uint32_t match_only_argnum = UINT32_MAX;
    uint32_t match_only_name = UINT32_MAX;
    // TODO: optimize this by supporting string views! or compare methods which let compare with a substring of input
    const String name_only = name.Left(argnum_at);
    const String argnum_only = (argnum_at != UINT32_MAX) ? name.Mid(argnum_at + 1, arglist_at - argnum_at - 1) : String();
    const String arglist_only = (arglist_at != UINT32_MAX) ? name.Mid(arglist_at + 1) : String();

    // Scan the range of possible matches, starting with pure name without appendages
    for (auto it = _lookup.lower_bound(name_only); it != _lookup.end(); ++it)
    {
        const String &try_sym = it->first;
        if (try_sym.CompareLeft(name, argnum_at) != 0)
            break; // base name not matching, no reason to continue the range
        else if (argnum_at == String::NoIndex)
            return it->second; // match, and request has no further appendage, choose this

        // If the symbol was registered without argnum, arglist, then remember it
        if (try_sym.GetLength() == argnum_at)
            match_only_name = it->second;

        if (try_sym.CompareMid(argnum_only, argnum_at + 1, arglist_at - argnum_at + 1) != 0)
            continue; // argnum not matching
        else if (arglist_at == String::NoIndex)
            return it->second; // match, and request has no further appendage, choose this

        // If the symbol was registered without arglist, then remember it
        if (try_sym.GetLength() == arglist_at)
            match_only_argnum = it->second;

        if (try_sym.CompareMid(arglist_only, arglist_at + 1) == 0)
            return it->second; // matched whole appendage, found exact match
    }
    
    // If no exact match was found, then select the longest match
    if (match_only_argnum != UINT32_MAX)
        return match_only_argnum; // have a match with number of arguments
    if (match_only_name != UINT32_MAX)
        return match_only_name; // have a match with the function name

    // Not found...
    return UINT32_MAX;
}


SystemImports::SystemImports()
    : _lookup('^')
{
}

uint32_t SystemImports::add(const String &name, const RuntimeScriptValue &value, ccInstance *anotherscr)
{
    uint32_t ixof = getIndexOf(name);
    // Check if symbol already exists
    if (ixof != UINT32_MAX)
    {
        // Only allow override if not a script-exported function
        if (anotherscr == nullptr)
        {
            imports[ixof].Value = value;
            imports[ixof].InstancePtr = anotherscr;
        }
        return ixof;
    }

    ixof = imports.size();
    for (size_t i = 0; i < imports.size(); ++i)
    {
        if (imports[i].Name == nullptr)
        {
            ixof = i;
            break;
        }
    }

    if (ixof == imports.size())
        imports.push_back(ScriptImport());
    imports[ixof].Name          = name;
    imports[ixof].Value         = value;
    imports[ixof].InstancePtr   = anotherscr;
    _lookup.add(name, ixof);
    return ixof;
}

void SystemImports::remove(const String &name)
{
    uint32_t idx = getIndexOf(name);
    if (idx == UINT32_MAX)
        return;

    _lookup.remove(imports[idx].Name);
    imports[idx].Name = {};
    imports[idx].Value.Invalidate();
    imports[idx].InstancePtr = nullptr;
}

const ScriptImport *SystemImports::getByName(const String &name) const
{
    uint32_t o = getIndexOf(name);
    if (o == UINT32_MAX)
        return nullptr;

    return &imports[o];
}

const ScriptImport *SystemImports::getByIndex(uint32_t index) const
{
    if (index >= imports.size())
        return nullptr;

    return &imports[index];
}

String SystemImports::findName(const RuntimeScriptValue &value) const
{
    for (const auto &import : imports)
    {
        if (import.Value == value)
        {
            return import.Name;
        }
    }
    return String();
}

void SystemImports::RemoveScriptExports(ccInstance *inst)
{
    if (!inst)
    {
        return;
    }

    for (auto &import : imports)
    {
        if (import.Name == nullptr)
            continue;

        if (import.InstancePtr == inst)
        {
            _lookup.remove(import.Name);
            import.Name = nullptr;
            import.Value.Invalidate();
            import.InstancePtr = nullptr;
        }
    }
}

void SystemImports::clear()
{
    _lookup.clear();
    imports.clear();
}
