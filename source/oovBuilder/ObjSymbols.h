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
#include <string>

class ObjSymbols
    {
    public:
	/// @param clumpName A clump is the collection of symbols from a list of
	///	files. Normally it relates to a library path that may contain
	///	many libraries.
	/// @param libFileNames List of all library file names in a clump.
	/// @param outPath Location of where to put symbol information.
	/// @param objSymbolTool The executable name.
	/// @param sortedLibFileNames Sorted names
	bool makeClumpSymbols(char const * const clumpName,
		const std::vector<std::string> &libFileNames, char const * const outSymPath,
		char const * const objSymbolTool, class ComponentTaskQueue &queue);

	// From the clump, append the ordered libraries with their directories.
	static void appendOrderedLibs(char const * const clumpName,
		char const * const outPath, std::vector<std::string> &libDirs,
		std::vector<std::string> &sortedLibNames);
	static void appendOrderedLibFileNames(char const * const clumpName,
		char const * const outPath,
		std::vector<std::string> &sortedLibFileNames);

    private:
        InProcMutex mListenerStdMutex;
        bool makeObjectSymbols(const std::vector<std::string> &libFiles,
        	char const * const outSymPath, char const * const objSymbolTool,
        	ComponentTaskQueue &queue, class ClumpSymbols &clumpSymbols);
    };


#endif /* OBJSYMBOLS_H_ */
