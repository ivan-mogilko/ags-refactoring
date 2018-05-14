#include "util/string.h"

struct GameSetupStruct;

// HACK UTILS
void ExtractGlobalMessages(const GameSetupStruct &game, const AGS::Common::String &dest_directory);
void ExtractRoomMessages(int from, int to, const GameSetupStruct &game, const AGS::Common::String &dest_directory);
