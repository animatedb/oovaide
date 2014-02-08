// horses.cpp

#include "horses.h"
#include <string.h>

static Horses sHorses;

char const * const Horses::getName() const
    { return "Horses"; }

bool Horses::putTogether()
    { return false; }

extern "C"
{
SHAREDSHARED_EXPORT void getResourceName(char *buf, int maxBytes)
    {
    if(strlen(sHorses.getName()) < maxBytes)
        strcpy(buf, sHorses.getName());
    }

SHAREDSHARED_EXPORT bool putTogether()
    {
    return sHorses.putTogether();
    }
}
