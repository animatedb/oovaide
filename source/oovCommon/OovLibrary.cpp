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

bool OovLibrary::open(char const *fileName)
    {
    close();
#if(USE_GLIB)
    mLibrary = g_module_open(fileName, G_MODULE_BIND_LAZY);
#else
#ifdef __linux__
    mLibrary = dlopen(fileName, RTLD_LAZY);
#else
    mLibrary = LoadLibrary(fileName);
#endif
#endif
    return (mLibrary != nullptr);
    }

void OovLibrary::close()
    {
    if(mLibrary)
        {
#if(USE_GLIB)
        g_module_close(mLibrary);
#else
#ifdef __linux__
        dlclose(mLibrary);
#else
        FreeLibrary(mLibrary);
#endif
#endif
        mLibrary = nullptr;
        }
    }

void OovLibrary::loadModuleSymbol(const char *symbolName, OovProcPtr *symbol) const
    {
#if(USE_GLIB)
    if(!g_module_symbol(mLibrary, symbolName, symbol))
        { *symbol = nullptr; }
#else
#ifdef __linux__
    *symbol = dlsym(mLibrary, symbolName);
#else
    *symbol = GetProcAddress(mLibrary, symbolName);
#endif
#endif
    }
