using System;

namespace AGS.Types
{
    // TODO: this interface is a stub. Consider expanded as necessary.
    public interface IDialog : IComparable<IDialog>, IToXml
    {
        int ID { get; }
        string ScriptName { get; }
        CustomProperties Properties { get; }
    }
}
