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
#include "script/cc_script.h"
#include "script/cc_internal.h"

namespace AGS
{
namespace Native 
{

using namespace System;
using namespace AGS::Types;

public interface class ICompiledScriptInternal
{
    /// <summary>
    /// Returns a list of function names that this script exports.
    /// NOTE: i'd reserve GetFunctionExport method name temporarily,
    /// in case we'd like to return a list of "Export" or "FunctionExport" structs.
    /// </summary>
    IList<String^>^ GetFunctionExportNames();
};

public ref class CompiledScript : public ICompiledScript, public ICompiledScriptInternal
{
private:
    PScript* _compiledScript;
    String ^_scriptFileName;
    IList<String^>^ _cachedFuncExportNames;

    void WriteCStr(System::IO::BinaryWriter ^writer, const std::string &str)
    {
        for (auto c : str)
        {
            writer->Write((System::Byte)c);
        }
        writer->Write((System::Byte)0);
    }

public:
    CompiledScript(PScript script, String ^scriptFileName) 
    {
        _compiledScript = new PScript();
        *_compiledScript = script;
        _scriptFileName = scriptFileName;
    }

    property PScript Data
    {
        PScript get() { return *_compiledScript; }
        void set(PScript newScript)
        {
            *_compiledScript = newScript;
            _cachedFuncExportNames = nullptr;
        }
    }

    ~CompiledScript() 
    {
        this->!CompiledScript();
    }

    !CompiledScript()
    {
        _compiledScript->reset();
        delete _compiledScript;
        _compiledScript = NULL;
        _scriptFileName = nullptr;
        _cachedFuncExportNames = nullptr;
    }

    virtual IList<String^>^ __clrcall GetFunctionExportNames()
    {
        if (*_compiledScript == NULL)
        {
            throw gcnew AGS::Types::CompileError(gcnew System::String("Script has not been compiled: ") + _scriptFileName);
        }

        if (_cachedFuncExportNames != nullptr)
            return _cachedFuncExportNames;

        List<String^>^ exports = gcnew List<String^>();
        const ccScript *cs = _compiledScript->get();
        for (const auto &fe : cs->exports)
        {
            exports->Add(TextHelper::ConvertASCII(fe.c_str()));
        }
        _cachedFuncExportNames = exports;
        return exports;
    }

    virtual void Write(System::IO::Stream ^ostream)
    {
        if (*_compiledScript == NULL)
        {
            throw gcnew AGS::Types::CompileError(gcnew System::String("Script has not been compiled: ") + _scriptFileName);
        }

        System::IO::BinaryWriter ^writer = gcnew System::IO::BinaryWriter(ostream);
        for (int i = 0; i < 4; ++i)
        {
            // the BinaryWriter seems to be treating CHAR as a 4-byte type here?
            writer->Write((System::Byte)scfilesig[i]);
        }
        const ccScript *cs = _compiledScript->get();
        writer->Write((unsigned)SCOM_VERSION_CURRENT);
        writer->Write((unsigned)cs->globaldata.size());
        writer->Write((unsigned)cs->code.size());
        writer->Write((unsigned)cs->strings.size());
        for (size_t i = 0; i < cs->globaldata.size(); ++i)
        {
            writer->Write((System::Byte)cs->globaldata[i]);
        }
        for (size_t i = 0; i < cs->code.size(); ++i)
        {
            writer->Write((int)cs->code[i]);
        }
        for (size_t i = 0; i < cs->strings.size(); ++i)
        {
            writer->Write((System::Byte)cs->strings[i]);
        }
        writer->Write((unsigned)cs->fixups.size());
        for (size_t i = 0; i < cs->fixups.size(); ++i)
        {
            writer->Write((System::Byte)cs->fixuptypes[i]);
        }
        for (size_t i = 0; i < cs->fixups.size(); ++i)
        {
            writer->Write(cs->fixups[i]);
        }
        writer->Write((unsigned)cs->imports.size());
        for (size_t i = 0; i < cs->imports.size(); ++i)
        {
            WriteCStr(writer, cs->imports[i]);
        }
        writer->Write((unsigned)cs->exports.size());
        for (size_t i = 0; i < cs->exports.size(); ++i)
        {
            WriteCStr(writer, cs->exports[i]);
            writer->Write(cs->export_addr[i]);
        }
        writer->Write((unsigned)cs->sectionNames.size());
        for (size_t i = 0; i < cs->sectionNames.size(); ++i)
        {
            WriteCStr(writer, cs->sectionNames[i]);
            writer->Write(cs->sectionOffsets[i]);
        }
        writer->Write((unsigned)ENDFILESIG);
    }
};

} // namespace Native 
} // namespace AGS
