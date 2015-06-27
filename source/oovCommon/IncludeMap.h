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

static const int IncDirMapNumTimeVals = 2;
static const int IncDirMapNumIncPathParts = 2;

/// This keeps two pieces of information. The path to a file, and the filename
/// of the file.  This is used to store the C++ "#include" filepath, and the
/// search path that is appended to the include filepath to find the file.
struct IncludedPath
    {
    public:
        IncludedPath():
            mPos(0)
            {}
        IncludedPath(OovStringRef const path, OovStringRef const includedFn):
            mFullPath(path)
            {
            mPos = mFullPath.length();
            mFullPath += includedFn;
            }
        OovString getIncDir() const
            {
            return(mFullPath.substr(0, mPos));
            }
        const OovString &getFullPath() const
            { return mFullPath; }
        bool operator<(IncludedPath const &incPath) const
            { return(mFullPath < incPath.mFullPath); }
    // This saves the included path such as "gtk/gtk.h" and the directory to
    // get to the included path.
    OovString mFullPath;
    size_t mPos;        /// The position between the path and filename.
    };

void discardDirs(OovStringVec &dirs);

/// See the oovCppParser project for a definition of the file that this reads.
class IncDirDependencyMapReader:public NameValueFile
    {
    public:
        void read(OovStringRef const fn);
        void getImmediateIncludeFilesUsedBySourceFile(
                OovStringRef const srcName, std::set<IncludedPath> &incFiles) const;
        // This recursively finds all include files for the specified
        // interface or implementation source file name.
        void getNestedIncludeFilesUsedBySourceFile(
                OovStringRef const srcName, std::set<IncludedPath> &incFiles) const;
        std::set<IncludedPath> getAllIncludeFiles() const;
        // This only matches exact directories, not nested trees.
        OovStringVec getIncludeFilesDefinedInDirectory(
                OovStringRef const dirName) const;
        // This recursively finds all include directories for the specified
        // interface or implementation source file name.
        OovStringVec getNestedIncludeDirsUsedBySourceFile(
                OovStringRef const srcName) const;
/*
        std::set<std::string> getIncludeDirsUsedByDirectory(
                OovStringRef const compDir);
*/
        bool anyRootDirsUsedBySourceFile(OovStringVec const &incRoots,
                OovStringSet &includesUsedSoFar,
                OovStringRef const srcName) const;
        bool anyRootDirsMatch(OovStringVec const &incRoots,
                OovStringRef const dirName) const;
        // Get the include directories sorted by the ordered include root
        // directories for the specified source implementation file name.
        // It does this by finding the longest directory that matches.
        OovStringVec getOrderedIncludeDirsForSourceFile(
                OovStringRef const srcName,
                OovStringVec const &orderedIncRoots) const;
        OovStringVec getFilesDefinedInDirectory(
                OovStringRef const dirName) const;
    };

#endif /* INCLUDEMAP_H_ */
