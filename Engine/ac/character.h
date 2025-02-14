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
//
//
//=============================================================================
#ifndef __AGS_EE_AC__CHARACTER_H
#define __AGS_EE_AC__CHARACTER_H

#include <vector>
#include "ac/characterinfo.h"
#include "ac/characterextras.h"
#include "ac/runtime_defines.h"
#include "ac/dynobj/scriptobjects.h"
#include "ac/dynobj/scriptoverlay.h"
#include "game/characterclass.h"
#include "game/viewport.h"
#include "util/geometry.h"

// **** CHARACTER: FUNCTIONS ****

bool    is_valid_character(int char_id);
// Asserts the character ID is valid,
// if not then prints a warning to the log; returns assertion result
bool    AssertCharacter(const char *apiname, int char_id);

void    Character_AddInventory(Character *chaa, ScriptInvItem *invi, int addIndex);
void    Character_AddWaypoint(Character *chaa, int x, int y);
void    Character_Animate(Character *chaa, int loop, int delay, int repeat,
                          int blocking, int direction, int sframe = 0, int volume = 100);
void    Character_Animate5(Character *chaa, int loop, int delay, int repeat, int blocking, int direction);
void    Character_ChangeRoomAutoPosition(Character *chaa, int room, int newPos);
void    Character_ChangeRoom(Character *chaa, int room, int x, int y);
void    Character_ChangeRoomSetLoop(Character *chaa, int room, int x, int y, int direction);
void    Character_ChangeView(Character *chap, int vii);
void    Character_FaceDirection(Character *char1, int direction, int blockingStyle);
void    Character_FaceCharacter(Character *char1, Character *char2, int blockingStyle);
void    Character_FaceLocation(Character *char1, int xx, int yy, int blockingStyle);
void    Character_FaceObject(Character *char1, ScriptObject *obj, int blockingStyle);
void    Character_FollowCharacter(Character *chaa, Character *tofollow, int distaway, int eagerness);
int     Character_IsCollidingWithChar(Character *char1, Character *char2);
int     Character_IsCollidingWithObject(Character *chin, ScriptObject *objid);
bool    Character_IsInteractionAvailable(Character *cchar, int mood);
void    Character_LockView(Character *chap, int vii);
void    Character_LockViewEx(Character *chap, int vii, int stopMoving);
void    Character_LockViewAlignedEx(Character *chap, int vii, int loop, int align, int stopMoving);
void    Character_LockViewFrameEx(Character *chaa, int view, int loop, int frame, int stopMoving);
void    Character_LockViewOffsetEx(Character *chap, int vii, int xoffs, int yoffs, int stopMoving);
void    Character_LoseInventory(Character *chap, ScriptInvItem *invi);
void    Character_PlaceOnWalkableArea(Character *chap);
void    Character_RemoveTint(Character *chaa);
int     Character_GetHasExplicitTint(Character *chaa);
void    Character_Say(Character *chaa, const char *text);
void    Character_SayAt(Character *chaa, int x, int y, int width, const char *texx);
ScriptOverlay* Character_SayBackground(Character *chaa, const char *texx);
void    Character_SetAsPlayer(Character *chaa);
void    Character_SetIdleView(Character *chaa, int iview, int itime);
void    Character_SetOption(Character *chaa, int flag, int yesorno);
void    Character_SetSpeed(Character *chaa, int xspeed, int yspeed);
void    Character_StopMoving(Character *charp);
void    Character_StopMovingEx(Character *charp, bool force_walkable_area);
void    Character_Tint(Character *chaa, int red, int green, int blue, int opacity, int luminance);
void    Character_Think(Character *chaa, const char *text);
void    Character_UnlockView(Character *chaa);
void    Character_UnlockViewEx(Character *chaa, int stopMoving);
void    Character_Walk(Character *chaa, int x, int y, int blocking, int ignwal);
void    Character_Move(Character *chaa, int x, int y, int blocking, int ignwal);
void    Character_WalkStraight(Character *chaa, int xx, int yy, int blocking);

void    Character_RunInteraction(Character *chaa, int mood);

// **** CHARACTER: PROPERTIES ****

int Character_GetProperty(Character *chaa, const char *property);
void Character_GetPropertyText(Character *chaa, const char *property, char *bufer);
const char* Character_GetTextProperty(Character *chaa, const char *property);

ScriptInvItem* Character_GetActiveInventory(Character *chaa);
void    Character_SetActiveInventory(Character *chaa, ScriptInvItem* iit);
void    SetActiveInventory(int iit); //[DEPRECATED] but used from few other functions
int     Character_GetAnimating(Character *chaa);
int     Character_GetAnimationSpeed(Character *chaa);
void    Character_SetAnimationSpeed(Character *chaa, int newval);
int     Character_GetBaseline(Character *chaa);
void    Character_SetBaseline(Character *chaa, int basel);
int     Character_GetBlinkInterval(Character *chaa);
void    Character_SetBlinkInterval(Character *chaa, int interval);
int     Character_GetBlinkView(Character *chaa);
void    Character_SetBlinkView(Character *chaa, int vii);
int     Character_GetBlinkWhileThinking(Character *chaa);
void    Character_SetBlinkWhileThinking(Character *chaa, int yesOrNo);
int     Character_GetBlockingHeight(Character *chaa);
void    Character_SetBlockingHeight(Character *chaa, int hit);
int     Character_GetBlockingWidth(Character *chaa);
void    Character_SetBlockingWidth(Character *chaa, int wid);
int     Character_GetDiagonalWalking(Character *chaa);
void    Character_SetDiagonalWalking(Character *chaa, int yesorno);
int     Character_GetClickable(Character *chaa);
void    Character_SetClickable(Character *chaa, int clik);
int     Character_GetID(Character *chaa);
int     Character_GetFrame(Character *chaa);
void    Character_SetFrame(Character *chaa, int newval);
int     Character_GetIdleView(Character *chaa);
int     Character_GetIInventoryQuantity(Character *chaa, int index);
int     Character_HasInventory(Character *chaa, ScriptInvItem *invi);
void    Character_SetIInventoryQuantity(Character *chaa, int index, int quant);
int     Character_GetIgnoreLighting(Character *chaa);
void    Character_SetIgnoreLighting(Character *chaa, int yesorno);
void    Character_SetManualScaling(Character *chaa, int yesorno);
int     Character_GetMovementLinkedToAnimation(Character *chaa);
void    Character_SetMovementLinkedToAnimation(Character *chaa, int yesorno);
int     Character_GetLoop(Character *chaa);
void    Character_SetLoop(Character *chaa, int newval);
int     Character_GetMoving(Character *chaa);
const char* Character_GetName(Character *chaa);
void    Character_SetName(Character *chaa, const char *newName);
int     Character_GetNormalView(Character *chaa);
int     Character_GetPreviousRoom(Character *chaa);
int     Character_GetRoom(Character *chaa);
int     Character_GetScaleMoveSpeed(Character *chaa);
void    Character_SetScaleMoveSpeed(Character *chaa, int yesorno);
int     Character_GetScaleVolume(Character *chaa);
void    Character_SetScaleVolume(Character *chaa, int yesorno);
int     Character_GetScaling(Character *chaa);
void    Character_SetScaling(Character *chaa, int zoomlevel);
int     Character_GetSolid(Character *chaa);
void    Character_SetSolid(Character *chaa, int yesorno);
int     Character_GetSpeaking(Character *chaa);
int     Character_GetSpeechColor(Character *chaa);
void    Character_SetSpeechColor(Character *chaa, int ncol);
int     Character_GetSpeechAnimationDelay(Character *chaa);
void    Character_SetSpeechAnimationDelay(Character *chaa, int newDelay);
int     Character_GetSpeechView(Character *chaa);
void    Character_SetSpeechView(Character *chaa, int vii);
int     Character_GetThinkView(Character *chaa);
void    Character_SetThinkView(Character *chaa, int vii);
int     Character_GetTransparency(Character *chaa);
void    Character_SetTransparency(Character *chaa, int trans);
int     Character_GetTurnBeforeWalking(Character *chaa);
void    Character_SetTurnBeforeWalking(Character *chaa, int yesorno);
int     Character_GetView(Character *chaa);
int     Character_GetWalkSpeedX(Character *chaa);
int     Character_GetWalkSpeedY(Character *chaa);
int     Character_GetX(Character *chaa);
void    Character_SetX(Character *chaa, int newval);
int     Character_GetY(Character *chaa);
void    Character_SetY(Character *chaa, int newval);
int     Character_GetZ(Character *chaa);
void    Character_SetZ(Character *chaa, int newval);
int     Character_GetSpeakingFrame(Character *chaa);
int     Character_GetBlendMode(Character *chaa);
void    Character_SetBlendMode(Character *chaa, int blendMode);

//=============================================================================

class MoveList;
namespace AGS { namespace Common { class Bitmap; } }
using namespace AGS; // FIXME later

// Configures and starts character animation.
void animate_character(Character *chap, int loopn, int sppd, int rept,
    int direction = 0, int sframe = 0, int volume = 100);
// Clears up animation parameters
void stop_character_anim(Character *chap);
int  find_looporder_index (int curloop);
// returns 0 to use diagonal, 1 to not
int  useDiagonal (Character *char1);
// returns 1 normally, or 0 if they only have horizontal animations
int  hasUpDownLoops(Character *char1);
void start_character_turning (Character *chinf, int useloop, int no_diagonal);
void fix_player_sprite(Character *chinf, const MoveList &cmls);
// Check whether two characters have walked into each other
int  has_hit_another_character(int sourceChar);
int  doNextCharMoveStep(Character *chi, CharacterExtras *chex);
// Tells if character is currently moving, in eWalkableAreas mode
bool is_char_walking_ndirect(Character *chi);
bool FindNearestWalkableAreaForCharacter(const Point &src, Point &dst);
// Start character walk or move; calculate path using destination and optionally "ignore walls" flag 
void move_character(Character *chaa, int tox, int toy, bool ignwal, bool walk_anim);
// Start character walk or move, using a predefined path
void move_character(Character *chaa, const std::vector<Point> &path, bool walk_anim, const RunPathParams &run_params);
// Start character walk or move along the straight line without pathfinding, until any non-passable area is met
void move_character_straight(Character *chaa, int x, int y, bool walk_anim);
// Start character walk; calculate path using destination and optionally "ignore walls" flag
void walk_character(Character *chaa, int tox, int toy, bool ignwal);
// Start character walk the straight line until any non-passable area is met
void walk_character_straight(Character *chaa, int tox, int toy);
void MoveCharacterToHotspot(int chaa, int hotsp);
void FindReasonableLoopForCharacter(Character *chap);
int  wantMoveNow (Character *chi, CharacterExtras *chex);
void setup_player_character(int charid);
int GetCharacterWidth(int charid);
int GetCharacterHeight(int charid);

Common::Bitmap *GetCharacterImage(int charid, bool *is_original = nullptr);
// Gets current source image (untransformed) for the character
Common::Bitmap *GetCharacterSourceImage(int charid);
Character *GetCharacterAtScreen(int xx, int yy);
// Deduces room object's scale, accounting for both manual scaling and the room region effects;
// calculates resulting sprite size.
void update_character_scale(int charid);
// Get character ID at the given room coordinates
int is_pos_on_character(int xx,int yy);
void get_char_blocking_rect(int charid, int *x1, int *y1, int *width, int *y2);
// Check whether the source char has walked onto character ww
int is_char_on_another (int sourceChar, int ww, int*fromxptr, int*cwidptr);
int my_getpixel(Common::Bitmap *blk, int x, int y);
// X and Y co-ordinates must be in 320x200 format
int check_click_on_character(int xx,int yy,int mood);
void _DisplaySpeechCore(int chid, const char *displbuf);
void _DisplayThoughtCore(int chid, const char *displbuf);
void _displayspeech(const char*texx, int aschar, int xx, int yy, int widd, int isThought);
int get_character_currently_talking();
void DisplaySpeech(const char*texx, int aschar);
int update_lip_sync(int talkview, int talkloop, int *talkframeptr);

// Recalculate dynamic character properties, e.g. after restoring a game save
void restore_characters();

// Calculates character's bounding box in room coordinates (takes only in-room transform into account)
// use_frame_0 optionally tells to use frame 0 of current loop instead of current frame.
Rect GetCharacterRoomBBox(int charid, bool use_frame_0 = false);
// Find a closest viewport given character is to. Checks viewports in their order in game's array,
// and returns either first viewport character's bounding box intersects with (or rather with its camera),
// or the one that is least far away from its camera; calculated as a perpendicular distance between two AABBs.
PViewport FindNearestViewport(int charid);

//
// Character update functions
// TODO: move these into a runtime Character class, when there will be a proper one,
// merging CharacterInfo and CharacterExtras.
//
void UpdateCharacterMoveAndAnim(Character *chi, CharacterExtras *chex, std::vector<int> &followingAsSheep);
void UpdateFollowingExactlyCharacter(Character *chi);
bool UpdateCharacterTurning(Character *chi, CharacterExtras *chex);
void UpdateCharacterMoving(Character *chi, CharacterExtras *chex, int &doing_nothing);
bool UpdateCharacterAnimating(Character *chi, CharacterExtras *chex, int &doing_nothing);
void UpdateCharacterIdle(Character *chi, CharacterExtras *chex, int &doing_nothing);
void UpdateCharacterFollower(Character *chi, std::vector<int> &followingAsSheep, int &doing_nothing);
void UpdateInventory();

extern Character *playerchar;
extern int32_t _sc_PlayerCharPtr;

// order of loops to turn character in circle from down to down
extern const int turnlooporder[8];

#endif // __AGS_EE_AC__CHARACTER_H
