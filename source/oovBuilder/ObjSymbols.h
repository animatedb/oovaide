/*
 * ObjSymbols.h
 *
 *  Created on: Sep 13, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef OBJSYMBOLS_H_
#define OBJSYMBOLS_H_

#include "OovProcess.h"
#include <vector>
#include "OovString.h"

class ObjSymbols
    {
    public:
        /// @param clumpName A clump is the collection of symbols from a list of
        ///     files. Normally it relates to a library path that may contain
        ///     many libraries.
        /// @param libFileNames List of all library file names in a clump.
        /// @param outSymPath Location of where to put symbol information.
        /// @param objSymbolTool The executable name.
        /// @param queue The queue of tasks/processes for processing the libs.
        bool makeClumpSymbols(OovStringRef const clumpName,
                OovStringVec const &libFileNames, OovStringRef const outSymPath,
                OovStringRef const objSymbolTool, class ComponentTaskQueue &queue);

        // From the clump, append the ordered libraries with their directories.
        static void appendOrderedLibs(OovStringRef const clumpName,
                OovStringRef const outPath, OovStringVec &libDirs,
                OovStringVec &sortedLibNames);
        static void appendOrderedLibFileNames(OovStringRef const clumpName,
                OovStringRef const outPath,
                OovStringVec &sortedLibFileNames);

    private:
        InProcMutex mListenerStdMutex;
        bool makeObjectSymbols(OovStringVec const &libFiles,
                OovStringRef const outSymPath, OovStringRef const objSymbolTool,
                ComponentTaskQueue &queue, class ClumpSymbols &clumpSymbols);
    };


#endif /* OBJSYMBOLS_H_ */
