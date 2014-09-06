/*
 * IncludeMap.h
 *
 *  Created on: Jan 6, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#ifndef INCLUDEMAP_H_
#define INCLUDEMAP_H_

#include <string>
#include <set>
#include "NameValueFile.h"
#include "FilePath.h"

struct IncludedPath
    {
    public:
	IncludedPath():
	    mPos(0)
	    {}
	IncludedPath(std::string const &path, std::string const &includedFn):
            mFullPath(path)
	    {
	    mPos = mFullPath.length();
	    mFullPath += includedFn;
	    }
	std::string getIncDir() const
	    {
	    return(mFullPath.substr(0, mPos));
	    }
	const std::string &getFullPath() const
	    { return mFullPath; }
	bool operator<(IncludedPath const &incPath) const
	    { return(mFullPath < incPath.mFullPath);}
    // This saves the included path such as "gtk/gtk.h" and the directory to
    // get to the included path.
    std::string mFullPath;
    size_t mPos;
    };

void discardDirs(std::vector<std::string> &dirs);

/// See the oovCppParser project for a definition of the file that this reads.
class IncDirDependencyMapReader:public NameValueFile
    {
    public:
	void read(char const * const fn);
	void getImmediateIncludeFilesUsedBySourceFile(
		char const * const srcName, std::set<IncludedPath> &incFiles) const;
	// This recursively finds all include files for the specified
	// interface or implementation source file name.
	void getNestedIncludeFilesUsedBySourceFile(
		char const * const srcName, std::set<IncludedPath> &incFiles) const;
	// This only matches exact directories, not nested trees.
	std::vector<std::string> getIncludeFilesDefinedInDirectory(
		char const * const dirName) const;
	// This recursively finds all include directories for the specified
	// interface or implementation source file name.
	std::vector<std::string> getNestedIncludeDirsUsedBySourceFile(
		char const * const srcName) const;
/*
	std::set<std::string> getIncludeDirsUsedByDirectory(
		char const * const compDir);
*/
	bool anyRootDirsUsedBySourceFile(std::vector<std::string> const &incRoots,
		std::set<std::string> &includesUsedSoFar,
		char const * const srcName) const;
	bool anyRootDirsMatch(std::vector<std::string> const &incRoots,
		char const * const dirName) const;
	// Get the include directories sorted by the ordered include root
	// directories for the specified source implementation file name.
	// It does this by finding the longest directory that matches.
	std::vector<std::string> getOrderedIncludeDirsForSourceFile(
		char const * const srcName,
		const std::vector<std::string> &orderedIncRoots) const;
	std::vector<std::string> getFilesDefinedInDirectory(
		char const * const dirName) const;
    };

#endif /* INCLUDEMAP_H_ */
