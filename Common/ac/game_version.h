//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Game version constants and information
//
//=============================================================================
#ifndef __AGS_CN_AC__GAMEVERSION_H
#define __AGS_CN_AC__GAMEVERSION_H

#include "util/version.h"

/*

Game data versions and changes:
-------------------------------

12 : 2.3 + 2.4

Versions above are incompatible at the moment.

18 : 2.5.0
19 : 2.5.1 + 2.52
20 : 2.5.3

Lip sync data added.
21 : 2.5.4
22 : 2.5.5

Variable number of sprites.
24 : 2.5.6
25 : 2.6.0

Encrypted global messages and dialogs.
26 : 2.6.1

Wait() must be called with parameter > 0
GetRegionAt() clips the input values to the screen size
Color 0 now means transparent instead of black for text windows
SetPlayerCharacter() does nothing if the new character is already the player character.
27 : 2.6.2

Script modules. Fixes bug in the inventory display.
Clickable GUI is selected with regard for the drawing order.
Pointer to the "player" variable is now accessed via a dynamic object.
31 : 2.7.0
32 : 2.7.2

35 : 3.0.0

Room names are serialized when game is compiled in "debug" mode.
36 : 3.0.1

Interactions are now scripts. The number for "not set" changed from 0 to -1 for
a lot of variables (views, sounds).
Deprecated switch between low-res and high-res native coordinates.
37 : 3.1.0

Dialogs are now scripts. New character animation speed.
39 : 3.1.1

Individual character speech animation speed.
40 : 3.1.2

Audio clips
41 : 3.2.0
42 : 3.2.1

43 : 3.3.0
Added few more game options.

44 : 3.3.1
Added custom dialog option highlight colour.

45 : 3.4.0.1
Support for custom game resolution.

46 : 3.4.0.2-.3
Audio playback speed.
Custom dialog option rendering extension.

47 : 3.4.0.4
Custom properties changed at runtime.
Ambient lighting

48 : 3.4.1
OPT_RENDERATSCREENRES, extended engine caps check, font vertical offset.

49 : 3.4.1.2
Font custom line spacing.

50 : 3.5.0.8
Sprites have "real" resolution. Expanded FontInfo data format.
Option to allow legacy relative asset resolutions.

3.6.0 :
Format value is defined as AGS version represented as NN,NN,NN,NN.
Fonts have adjustable outline
3.6.0.11:
New font load flags, control backward compatible font behavior
3.6.0.16:
Idle animation speed, modifiable hotspot names, fixed video frame
3.6.0.21:
Some adjustments to gui text alignment.
3.6.1:
In RTL mode all text is reversed, not only wrappable (labels etc).
3.6.1.10:
Disabled automatic SetRestartPoint.
3.6.1.14:
Extended game object names, resolving hard length limits.
3.6.2:
Object Interactions specify script module where functions are located.
OPT_SAVESCREENSHOTLAYER, CHF_TURNWHENFACE. Button's WrapText and padding.
Few minor behavior changes.
3.6.2.3:
Script module names are written in the game data.

3.9.9 :
BlendModes
4.0.0 :
Raised for org purposes without format changes
4.0.0.8:
Palette component range changed from 64 to 256
4.0.0.9:
32-bit color properties
4.0.0.10:
Font file names
4.0.0.11:
Incremented version, marking sync with 3.6.2.3
*/

enum GameDataVersion
{
    kGameVersion_Undefined      = 0,
    kGameVersion_360_21         = 3060021,
    kGameVersion_361            = 3060100,
    kGameVersion_361_10         = 3060110,
    kGameVersion_361_14         = 3060114,
    kGameVersion_362            = 3060200,
    kGameVersion_362_03         = 3060203,
    kGameVersion_399            = 3999999,
    kGameVersion_400            = 4000000,
    kGameVersion_400_08         = 4000008,
    kGameVersion_400_09         = 4000009,
    kGameVersion_400_10         = 4000010,
    kGameVersion_400_11         = 4000011,
    kGameVersion_400_13         = 4000013,
    kGameVersion_LowSupported   = kGameVersion_360_21,
    kGameVersion_Current        = kGameVersion_400_13
};

// Data format version of the loaded game
extern GameDataVersion loaded_game_file_version;
// The version of the engine the loaded game was compiled for (if available)
extern AGS::Common::Version game_compiled_version;

#endif // __AGS_CN_AC__GAMEVERSION_H
