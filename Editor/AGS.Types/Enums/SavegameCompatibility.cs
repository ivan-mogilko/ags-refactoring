using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;

namespace AGS.Types
{
    public enum SavegameCompatibility
    {
        [Description("Load saves with 100% matching content")]
        None = 0,
        [Description("Load saves with less or equal content")]
        AllowMissingContent = 1
    }
}
