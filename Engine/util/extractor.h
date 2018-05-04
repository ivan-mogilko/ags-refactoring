#include "util/string.h"

struct GameSetupStruct;

// HACK UTILS
void ExtractRoomMessages(int from, int to, const GameSetupStruct &game, const AGS::Common::String &dest_directory);
