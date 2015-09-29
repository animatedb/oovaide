/*
 * OovLibrary.cpp
 *
 *  Created on: Sept. 28, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */
#include "OovLibrary.h"

#ifdef __linux__
#include <dlfcn.h>
#endif

OovLibrary::OovLibrary():
    mLibrary(0)
    {}

OovLibrary::~OovLibrary()
    { close(); }

bool OovLibrary::open(char const *fileName)
    {
    bool success = false;
    close();
#ifdef __linux__
    mLibrary = dlopen(fileName, RTLD_LAZY);
#else
    mLibrary = LoadLibrary(fileName);
    success = (mLibrary != nullptr);
#endif
    return success;
    }

void OovLibrary::close()
    {
    if(mLibrary)
        {
#ifdef __linux__
        dlclose(mLibrary);
#else
        FreeLibrary(mLibrary);
#endif
        }
    }

void OovLibrary::loadModuleSymbol(const char *symbolName, OovProcPtr *symbol) const
    {
#ifdef __linux__
    *symbol = dlsym(mLibrary, symbolName);
#else
    *symbol = GetProcAddress(mLibrary, symbolName);
#endif
    }
