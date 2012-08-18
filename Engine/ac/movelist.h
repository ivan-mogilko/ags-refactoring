#ifndef __AC_MOVE_H
#define __AC_MOVE_H

#define MAXNEEDSTAGES 40
struct MoveList {
    int   pos[MAXNEEDSTAGES];
    int   numstage;
    fixed xpermove[MAXNEEDSTAGES], ypermove[MAXNEEDSTAGES];
    int   fromx, fromy;
    int   onstage, onpart;
    int   lastx, lasty;
    char  doneflag;
    char  direct;  // MoveCharDirect was used or not

    void ReadFromFile(FILE *f);
    void WriteToFile(FILE *f);
};
#endif // __AC_MOVE_H