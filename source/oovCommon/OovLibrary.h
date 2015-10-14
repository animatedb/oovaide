/*
 * OovLibrary.h
 *
 *  Created on: Sept. 28, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef OOVLIBRARY_H
#define OOVLIBRARY_H

#include "OovLibrary.h"
#define USE_GLIB 0
#if(USE_GLIB)
#include <gmodule.h>
#define OOV_MODULE_IMPORT G_MODULE_IMPORT
#define OOV_MODULE_EXPORT G_MODULE_EXPORT
#else

#define OOV_MODULE_IMPORT extern

#ifdef __linux__
#define OOV_MODULE_EXPORT
#else
#define OOV_MODULE_EXPORT __declspec(dllexport)
#include "windef.h"
#include "WinBase.h"
#endif

#endif

#if(USE_GLIB)
typedef gpointer OovProcPtr;
#else
#ifdef __linux__
typedef void *OovProcPtr;
#else
typedef FARPROC OovProcPtr;
#endif
#endif

/// A class that encapsulates a run time or dynamic library.
/// This encapsulates something similar to GLib's GModule.
// This class was originally written to avoid the use of GLib.  This only uses Windows functions on
// MS-Windows. It uses the dl... functions on linux.
// undefined reference to symbol 'dlclose@@GLIBC_2.2.5' on Ubuntu 14-04
class OovLibrary
    {
    public:
        OovLibrary():
            mLibrary(nullptr)
            {}
        ~OovLibrary()
            { close(); }
        /// @param The file name of the run time library. For both Windows and
        /// linux, a relative filename may search multiple directories.
        bool open(char const *fileName);
        void close();
        /// Load a symbol for the module.
        /// @param symbolName The name of the function/symbol.
        /// @param symbol The pointer to the symbol function.
        void loadModuleSymbol(char const *symbolName, OovProcPtr *symbol) const;
        bool isOpen()
            { return(mLibrary != 0); }

    private:
#if(USE_GLIB)
        GModule *mLibrary;
#else
#ifdef __linux__
        void *mLibrary;
#else
        HMODULE mLibrary;
#endif
#endif
    };

#endif
