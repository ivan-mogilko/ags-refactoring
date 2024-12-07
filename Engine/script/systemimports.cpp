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


void ScriptSymbolsMap::Add(const String &name, uint32_t index)
{
    _lookup[name] = index;
}

void ScriptSymbolsMap::Remove(const String &name)
{
    _lookup.erase(name);
}

void ScriptSymbolsMap::Clear()
{
    _lookup.clear();
}

uint32_t ScriptSymbolsMap::GetIndexOf(const String &name) const
{
    auto it = _lookup.find(name);
    if (it != _lookup.end())
        return it->second;

    // Not found...
    return UINT32_MAX;
}

uint32_t ScriptSymbolsMap::GetIndexOfAny(const String &name) const
{
    // Import names may be potentially formed as:
    //
    //     [type::]name[^argnum]
    //
    // where "type" is the name of a type, "name" is the name of a function,
    // "argnum" is the number of arguments.

    const size_t argnum_at = name.FindChar(_appendageSeparator);
    uint32_t match_only_name = UINT32_MAX;
    // TODO: optimize this by supporting string views! or compare methods which let compare with a substring of input
    const String name_only = name.Left(argnum_at);
    const String argnum_only = (argnum_at != UINT32_MAX) ? name.Mid(argnum_at + 1) : String();

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

        if (try_sym.CompareMid(argnum_only, argnum_at + 1) != 0)
            continue; // argnum not matching
        return it->second; // matched whole appendage, found exact match
    }
    
    // If no exact match was found, then select the longest match
    if (match_only_name != UINT32_MAX)
        return match_only_name; // have a match with the function name

    // Not found...
    return UINT32_MAX;
}


SystemImports::SystemImports()
    : _lookup('^')
{
}

uint32_t SystemImports::Add(const String &name, const RuntimeScriptValue &value, ccInstance *anotherscr)
{
    uint32_t ixof = GetIndexOf(name);
    // Check if symbol already exists
    if (ixof != UINT32_MAX)
    {
        // Only allow override if not a script-exported function
        if (anotherscr == nullptr)
        {
            _imports[ixof].Value = value;
            _imports[ixof].InstancePtr = anotherscr;
        }
        return ixof;
    }

    ixof = _imports.size();
    for (size_t i = 0; i < _imports.size(); ++i)
    {
        if (_imports[i].Name == nullptr)
        {
            ixof = i;
            break;
        }
    }

    if (ixof == _imports.size())
        _imports.push_back(ScriptImport());
    _imports[ixof].Name          = name;
    _imports[ixof].Value         = value;
    _imports[ixof].InstancePtr   = anotherscr;
    _lookup.Add(name, ixof);
    return ixof;
}

void SystemImports::Remove(const String &name)
{
    uint32_t idx = GetIndexOf(name);
    if (idx == UINT32_MAX)
        return;

    _lookup.Remove(_imports[idx].Name);
    _imports[idx].Name = {};
    _imports[idx].Value.Invalidate();
    _imports[idx].InstancePtr = nullptr;
}

const ScriptImport *SystemImports::GetByName(const String &name) const
{
    uint32_t o = GetIndexOf(name);
    if (o == UINT32_MAX)
        return nullptr;

    return &_imports[o];
}

const ScriptImport *SystemImports::GetByIndex(uint32_t index) const
{
    if (index >= _imports.size())
        return nullptr;

    return &_imports[index];
}

String SystemImports::FindName(const RuntimeScriptValue &value) const
{
    for (const auto &import : _imports)
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

    for (auto &import : _imports)
    {
        if (import.Name == nullptr)
            continue;

        if (import.InstancePtr == inst)
        {
            _lookup.Remove(import.Name);
            import.Name = nullptr;
            import.Value.Invalidate();
            import.InstancePtr = nullptr;
        }
    }
}

void SystemImports::Clear()
{
    _lookup.Clear();
    _imports.clear();
}
