// men.cpp

#include "men.h"
#include <string.h>

static Men sMen;

char const * const Men::getName() const
    { return "Men"; }

bool Men::putTogether()
    { return false; }

extern "C"
{
SHAREDSHARED_EXPORT void getResourceName(char *buf, int maxBytes)
    {
    if(strlen(sMen.getName()) < maxBytes)
        strcpy(buf, sMen.getName());
    }

SHAREDSHARED_EXPORT bool putTogether()
    {
    return sMen.putTogether();
    }
}
