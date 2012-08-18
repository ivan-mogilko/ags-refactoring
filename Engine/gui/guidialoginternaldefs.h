//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_GUI__GUIDIALOGINTERNALDEFS_H
#define __AGS_EE_GUI__GUIDIALOGINTERNALDEFS_H

#include "gui/guidialogdefines.h"

#if defined(DJGPP) || defined(LINUX_VERSION) || defined(MAC_VERSION)
#define _getcwd getcwd
#endif

#undef kbhit
extern int rec_getch();
extern int rec_kbhit();
#define kbhit rec_kbhit
#define getch() rec_getch()

#define domouse rec_domouse

#define wbutt __my_wbutt

#define _export
#ifdef WINAPI
#undef WINAPI
#endif
#define WINAPI
#define mbutrelease !rec_misbuttondown
#define TEXT_HT usetup.textheight

#endif // __AGS_EE_GUI__GUIDIALOGINTERNALDEFS_H