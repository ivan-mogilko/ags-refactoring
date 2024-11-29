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
#include <stdlib.h>
#include <string.h>
#include "script/systemimports.h"

SystemImports simp;
SystemImports simp_for_plugin;

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

    btree[name] = ixof;
    if (ixof == imports.size())
        imports.push_back(ScriptImport());
    imports[ixof].Name          = name;
    imports[ixof].Value         = value;
    imports[ixof].InstancePtr   = anotherscr;
    return ixof;
}

void SystemImports::remove(const String &name)
{
    uint32_t idx = getIndexOf(name);
    if (idx == UINT32_MAX)
        return;
    btree.erase(imports[idx].Name);
    imports[idx].Name = nullptr;
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

uint32_t SystemImports::getIndexOf(const String &name) const
{
    IndexMap::const_iterator it = btree.find(name);
    if (it != btree.end())
        return it->second;

    // Not found...
    return UINT32_MAX;
}

uint32_t SystemImports::getIndexOfAny(const String &name) const
{
    // Import names may be potentially formed as:
    //
    //     [type::]name[^argnum][^arglist]
    //
    // where "type" is the name of a type, "name" is the name of a function,
    // "argnum" is the number of arguments and "arglist" is a '^' separated
    // list of types of return value and function args.

    const size_t argnum_at = name.FindChar('^');
    const size_t arglist_at = (argnum_at != String::NoIndex) ? name.FindChar('^', argnum_at + 1) : String::NoIndex;
    uint32_t match_only_argnum = UINT32_MAX;
    uint32_t match_only_name = UINT32_MAX;
    // TODO: optimize this by supporting string views! or compare methods which let compare with a substring of input
    const String name_only = name.Left(argnum_at);
    const String argnum_only = name.Mid(argnum_at + 1, arglist_at - argnum_at - 1);
    const String arglist_only = name.Mid(arglist_at + 1);

    // Scan the range of possible matches, starting with pure name without appendages
    for (auto it = btree.lower_bound(name_only); it != btree.end(); ++it)
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
            btree.erase(import.Name);
            import.Name = nullptr;
            import.Value.Invalidate();
            import.InstancePtr = nullptr;
        }
    }
}

void SystemImports::clear()
{
    btree.clear();
    imports.clear();
}
