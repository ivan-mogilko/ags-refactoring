
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALDRAWINGSURFACE_H
#define __AGS_EE_AC__GLOBALDRAWINGSURFACE_H

void RawSaveScreen ();
// RawRestoreScreen: copy backup bitmap back to screen; we
// deliberately don't free the block cos they can multiple restore
// and it gets freed on room exit anyway
void RawRestoreScreen();
// Restores the backup bitmap, but tints it to the specified level
void RawRestoreScreenTinted(int red, int green, int blue, int opacity);
void RawDrawFrameTransparent (int frame, int translev);
void RawClear (int clr);
void RawSetColor (int clr);
void RawSetColorRGB(int red, int grn, int blu);
void RawPrint (int xx, int yy, char*texx, ...);
void RawPrintMessageWrapped (int xx, int yy, int wid, int font, int msgm);
void RawDrawImageCore(int xx, int yy, int slot);
void RawDrawImage(int xx, int yy, int slot);
void RawDrawImageOffset(int xx, int yy, int slot);
void RawDrawImageTransparent(int xx, int yy, int slot, int trans);
void RawDrawImageResized(int xx, int yy, int gotSlot, int width, int height);
void RawDrawLine (int fromx, int fromy, int tox, int toy);
void RawDrawCircle (int xx, int yy, int rad);
void RawDrawRectangle(int x1, int y1, int x2, int y2);
void RawDrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3);

#endif // __AGS_EE_AC__GLOBALDRAWINGSURFACE_H
