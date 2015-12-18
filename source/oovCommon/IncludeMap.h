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

static const int IncDirMapNumTimeVals = 2;
static const int IncDirMapNumIncPathParts = 2;

/// This keeps two pieces of information. The path to a file, and the filename
/// of the file.  This is used to store the C++ "#include" filepath, and the
/// search path that is appended to the include filepath to find the file.
/// This could be different than a simple path with filename, because sometimes
/// the #include value can be something like #include <gtk/gtk.h>
struct IncludedPath
    {
    public:
        IncludedPath():
            mPos(0)
            {}
        /// Construct a full included path definition
        /// @param path The path to find the file.
        /// @param includedFn The included filepath.
        IncludedPath(OovStringRef const path, OovStringRef const includedFn):
            mFullPath(path)
            {
            mPos = mFullPath.length();
            mFullPath += includedFn;
            }
        /// Get the include search directory
        OovString getIncDir() const
            {
            return(mFullPath.substr(0, mPos));
            }
        /// Get the full path
        const OovString &getFullPath() const
            { return mFullPath; }
        /// Used for sorting
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
        /// Read the include dependency map file
        /// @param fn The file name to read from
        OovStatusReturn read(OovStringRef const fn);
        /// Get the include files that are directly included in the source file.
        /// @param srcName The source file name
        /// @param incFiles The returned list of included files
        void getImmediateIncludeFilesUsedBySourceFile(
                OovStringRef const srcName, std::set<IncludedPath> &incFiles) const;
        /// Get all included files used by a source file.
        /// This recursively finds all include files for the specified
        /// interface or implementation source file name.
        /// @param srcName The source file name
        /// @param incFiles The returned list of included files
        void getNestedIncludeFilesUsedBySourceFile(
                OovStringRef const srcName, std::set<IncludedPath> &incFiles) const;
        /// Get all included files in the project by all source files.
        std::set<IncludedPath> getAllIncludeFiles() const;
        /// Get all source and included files in the project.
        std::set<OovString> getAllFiles() const;
        /// Get the included files that reside in a directory.
        /// This only matches exact directories, not nested trees.
        /// @param dirName The directory name that contains the files.
// DEAD CODE
//        OovStringVec getIncludeFilesDefinedInDirectory(
//                OovStringRef const dirName) const;
        /// Get the nested include directories that are used by a source file.
        /// This recursively finds all include directories for the specified
        /// interface or implementation source file name.
        /// @param srcName The source file name.
        OovStringVec getNestedIncludeDirsUsedBySourceFile(
                OovStringRef const srcName) const;
/*
        std::set<std::string> getIncludeDirsUsedByDirectory(
                OovStringRef const compDir);
*/
        /// Check if any include roots are used by any of the source files in
        /// a directory.
        /// @param incRoots A list of include roots
        /// @param dirName The directory name to check.
        bool anyRootDirsMatch(OovStringVec const &incRoots,
                OovStringRef const dirName) const;
        /// Get the include directories sorted by the ordered include root
        /// directories for the specified source implementation file name.
        /// It does this by finding the longest directory that matches.
        OovStringVec getOrderedIncludeDirsForSourceFile(
                OovStringRef const srcName,
                OovStringVec const &orderedIncRoots) const;
        OovStringVec getFilesDefinedInDirectory(
                OovStringRef const dirName) const;

    private:
        /// Check if any root directories are used by a source file.
        /// @param incRoots A list of include directories to check for use.
        /// @param includesUsedSoFar A cache of includes to speed up searching.
        /// @param srcName The source file name to check.
        bool anyRootDirsUsedBySourceFile(OovStringVec const &incRoots,
            OovStringSet &includesUsedSoFar,
            OovStringRef const srcName) const;

        /// At the moment, this expands directories that have defined project
        /// files. If there are no defined files, then the wildcard remains in
        /// the set.
        void expandJavaFiles(std::set<IncludedPath> &incFiles) const;

        OovStringVec getJavaFilesDefinedInDirectory(
            OovStringRef const dirName) const;
        // Accepts an include path to support a java style wildcard import.
        OovStringVec getJavaExpandedFiles(OovStringRef const incPath) const;
    };

#endif /* INCLUDEMAP_H_ */
