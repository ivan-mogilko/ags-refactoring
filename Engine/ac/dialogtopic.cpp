
#include "ac/dialogtopic.h"

void DialogTopic::ReadFromFile(FILE *fp)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    fread(optionnames, 150*sizeof(char), MAXTOPICOPTIONS, fp);
    fread(optionflags, sizeof(int), MAXTOPICOPTIONS, fp);
    optionscripts = (unsigned char *) getw(fp);
    fread(entrypoints, sizeof(short), MAXTOPICOPTIONS, fp);
    startupentrypoint = getshort(fp);//__getshort__bigendian(fp);
    codesize = getshort(fp);//__getshort__bigendian(fp);
    numoptions = getw(fp);
    topicFlags = getw(fp);
//#else
//    throw "DialogTopic::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}