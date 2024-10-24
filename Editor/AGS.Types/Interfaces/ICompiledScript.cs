using System;

namespace AGS.Types
{
    /// <summary>
    /// ICompiledScript is an opaque object, which hides an internal
    /// implementation of a script compilation result.
    /// 
    /// There's not much use for this interface currently, but its contents,
    /// as well as potential use, may be expanded in the future.
    public interface ICompiledScript : IDisposable
    {
        /// <summary>
        /// Writes this compiled script's data to the file stream.
        /// </summary>
        void Write(System.IO.Stream ostream);
    }
}
