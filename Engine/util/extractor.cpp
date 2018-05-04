#include "extractor.h"
#include "ac/gamesetupstruct.h"
#include "ac/room.h"
#include "core/assetmanager.h"
#include "game/roomstruct.h"
#include "gfx/bitmap.h"
#include "util/directory.h"
#include "util/path.h"
#include "util/stream.h"

using namespace AGS::Common;

void DoExtractRoomMessages(const RoomStruct &room, const String &dest_file)
{
    if (room.MessageCount == 0)
        return;
    Stream *out = File::CreateFile(dest_file);
    for (int i = 0; i < room.MessageCount; i++)
    {
        out->Write(room.Messages[i], room.Messages[i].GetLength());
        out->Write("\n", 1);
    }
    delete out;
}

#define NO_GAME_ID_IN_ROOM_FILE 16325
extern void convert_room_coordinates_to_data_res(RoomStruct *rstruc);

void ExtractRoomMessages(int from, int to, const GameSetupStruct &game, const String &dest_directory)
{
    if (!Path::IsDirectory(dest_directory))
        Directory::CreateDirectory(dest_directory);

    RoomStruct room;
    String room_filename;
    for (int roomid = from; roomid <= to; ++roomid)
    {
        room_filename.Format("room%d.crm", roomid);
        if (roomid == 0) {
            if (loaded_game_file_version < kGameVersion_270 && AssetMgr->DoesAssetExist("intro.crm") ||
                loaded_game_file_version >= kGameVersion_270 && !AssetMgr->DoesAssetExist(room_filename))
            {
                room_filename = "intro.crm";
            }
        }
        if (!AssetMgr->DoesAssetExist(room_filename))
            continue;

        // TODO: fixme! -- proper room memory deletion

        room.GameID = NO_GAME_ID_IN_ROOM_FILE;
        load_room(room_filename, &room, game.IsLegacyHiRes(), game.SpriteInfos);
        if ((room.GameID != NO_GAME_ID_IN_ROOM_FILE) &&
            (room.GameID != game.uniqueid)) {
                continue;
        }

        convert_room_coordinates_to_data_res(&room);



        // TODO: make this a delegate
        DoExtractRoomMessages(room, String::FromFormat("%s/room%d_messages.txt", dest_directory.GetCStr(), roomid));
    }
}
