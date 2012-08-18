
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__INVENTORYITEM_H
#define __AGS_EE_AC__INVENTORYITEM_H

#include "ac/dynobj/scriptinvitem.h"

void InventoryItem_SetCursorGraphic(ScriptInvItem *iitem, int newSprite);
int  InventoryItem_GetCursorGraphic(ScriptInvItem *iitem);
void InventoryItem_SetGraphic(ScriptInvItem *iitem, int piccy);
void InventoryItem_SetName(ScriptInvItem *scii, const char *newname);
int  InventoryItem_GetID(ScriptInvItem *scii);
ScriptInvItem *GetInvAtLocation(int xx, int yy);
void InventoryItem_GetName(ScriptInvItem *iitem, char *buff);
const char* InventoryItem_GetName_New(ScriptInvItem *invitem);
int  InventoryItem_GetGraphic(ScriptInvItem *iitem);
void InventoryItem_RunInteraction(ScriptInvItem *iitem, int mood);
int  InventoryItem_CheckInteractionAvailable(ScriptInvItem *iitem, int mood);
int  InventoryItem_GetProperty(ScriptInvItem *scii, const char *property);
void InventoryItem_GetPropertyText(ScriptInvItem *scii, const char *property, char *bufer);
const char* InventoryItem_GetTextProperty(ScriptInvItem *scii, const char *property);

void set_inv_item_cursorpic(int invItemId, int piccy);

#endif // __AGS_EE_AC__INVENTORYITEM_H
