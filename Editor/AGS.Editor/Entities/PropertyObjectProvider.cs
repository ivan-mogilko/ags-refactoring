using System;

namespace AGS.Editor
{
    /// <summary>
    /// IPropertyObjectProvider is an interface that provides object(s)
    /// for a PropertyGrid.
    /// TODO: consider moving to AGS.Types or AGS.Controls library and
    /// implement this interface in ContentDocument, thus making this
    /// shared by all panes that use PropertyGrid.
    /// Expand with other property-related methods (ObjectList etc).
    /// </summary>
    interface IPropertyObjectProvider
    {
        object PropertyGridObject { get; }
    }
}
