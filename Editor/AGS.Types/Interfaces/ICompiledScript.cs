using System;

namespace AGS.Types
{
    /// <summary>
    /// ICompiledScript is an opaque object, which hides an internal
    /// implementation of a script compilation result.
    /// 
    /// There's not much use for this interface currently, but its contents,
    /// as well as potential use, may be expanded in the future.
    /// 
    /// NOTE: there's a separate ICompiledScriptInternal interface declared in
    /// AGS.Native, used internally by the AGS.Editor. That interface may be
    /// used as a reference for the "fullfilled" ICompiledScript, when we are
    /// sure about what would be required of it.
    /// </summary>
    public interface ICompiledScript : IDisposable
    {
        /// <summary>
        /// Writes this compiled script's data to the file stream.
        /// </summary>
        void Write(System.IO.Stream ostream);
    }
}
