using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;

namespace AGS.Types
{
    public enum FontHinting
    {
        Default,
        [Description("Disable hinting")]
        NoHint,
        [Description("Disable auto hinting")]
        NoAutoHint,
        [Description("Force auto hinting")]
        ForceAutoHint
    }
}
