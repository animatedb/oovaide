/*
 * OovLibrary.h
 *
 *  Created on: Sept. 28, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef OOVLIBRARY_H
#define OOVLIBRARY_H

#include "OovLibrary.h"
#ifdef __linux__
#else
#include "windef.h"
#include "WinBase.h"
#endif


#ifdef __linux__
typedef void *OovProcPtr;
#else
typedef FARPROC OovProcPtr;
#endif

/// A class that encapsulates a run time or dynamic library.
/// This encapsulates something similar to GLib's GModule.
// This class was written to avoid the use of GLib.  This only uses Windows functions on
// MS-Windows. It uses the dl... functions on linux.
// undefined reference to symbol 'dlclose@@GLIBC_2.2.5' on Ubuntu 14-04
class OovLibrary
    {
    public:
        OovLibrary();
        ~OovLibrary();
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
#ifdef __linux__
        void *mLibrary;
#else
        HMODULE mLibrary;
#endif
    };

#endif
