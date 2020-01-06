#ifndef __AGS_CN_CORE__DEFVERSION_H
#define __AGS_CN_CORE__DEFVERSION_H

#define ACI_VERSION_STR      "3.4.4.2"
#if defined (RC_INVOKED) // for MSVC resource compiler
#define ACI_VERSION_MSRC_DEF  3,4,4,2
#endif

#ifdef NO_MP3_PLAYER
#define SPECIAL_VERSION "NMP"
#else
#define SPECIAL_VERSION "TRA-HACK"
#endif

#define ACI_COPYRIGHT_YEARS "2011-2019"

#endif // __AGS_CN_CORE__DEFVERSION_H
