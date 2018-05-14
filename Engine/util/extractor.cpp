#include "extractor.h"
#include "ac/gamesetupstruct.h"
#include "ac/roomstruct.h"
#include "ac/room.h"
#include "core/assetmanager.h"
#include "gfx/bitmap.h"
#include "util/directory.h"
#include "util/path.h"
#include "util/stream.h"

using namespace AGS::Common;

void DoExtractRoomMessages(const roomstruct &room, const String &dest_file)
{
    if (room.nummes == 0)
        return;
    Stream *out = File::CreateFile(dest_file);
    if (!out)
        return;
    for (int i = 0; i < room.nummes; i++)
    {
        if (!room.message[i])
            return;
        size_t len = strlen(room.message[i]);
        if (len > 0)
        {
            out->Write(room.message[i], len);
            out->Write("\n", 1);
        }
    }
    delete out;
}

void ExtractGlobalMessages(const GameSetupStruct &game, const String &dest_directory)
{
    if (!Path::IsDirectory(dest_directory))
        return;
    Stream *out = File::CreateFile(String::FromFormat("%s/game_messages.txt", dest_directory.GetCStr()));
    if (!out)
        return;
    for (int i = 0; i < MAXGLOBALMES; i++)
    {
        if (!game.messages[i])
            continue;
        size_t len = strlen(game.messages[i]);
        if (len > 0)
        {
            out->Write(game.messages[i], len);
            out->Write("\n", 1);
        }
    }
    delete out;
}

#define NO_GAME_ID_IN_ROOM_FILE 16325

void ExtractRoomMessages(int from, int to, const GameSetupStruct &game, const String &dest_directory)
{
    if (!Path::IsDirectory(dest_directory))
        return;

    roomstruct room;
    String room_filename;
    for (int roomid = from; roomid <= to; ++roomid)
    {
        room_filename.Format("room%d.crm", roomid);
        if (roomid == 0) {
            if (loaded_game_file_version < kGameVersion_270 && Common::AssetManager::DoesAssetExist("intro.crm") ||
                loaded_game_file_version >= kGameVersion_270 && !Common::AssetManager::DoesAssetExist(room_filename))
            {
                room_filename = "intro.crm";
            }
        }
        if (!AssetManager::DoesAssetExist(room_filename))
            continue;

        // TODO: fixme! -- proper room memory deletion

        room.gameId = NO_GAME_ID_IN_ROOM_FILE;
        load_room(room_filename, &room, game.IsHiRes());
        if ((room.gameId != NO_GAME_ID_IN_ROOM_FILE) &&
            (room.gameId != game.uniqueid)) {
                continue;
        }

        if (game.IsHiRes() && (game.options[OPT_NATIVECOORDINATES] == 0))
        {
            convert_room_coordinates_to_low_res(&room);
        }



        // TODO: make this a delegate
        DoExtractRoomMessages(room, String::FromFormat("%s/room%d_messages.txt", dest_directory.GetCStr(), roomid));
    }
}
