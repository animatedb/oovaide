/*
 * IncDirMap.h
 *
 *  Created on: Aug 15, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef INCDIRMAP_H_
#define INCDIRMAP_H_

#include "FilePath.h"
#include "NameValueFile.h"
#include <map>
#include <string>

/// This works to build include paths, and to build include file dependencies
/// This makes a file that keeps a map of paths, and for each path:
///     the last time the path dependencies were updated by the parser,
///     the last time the paths were checked - THIS IS NOT UPDATED!,
///     the included filepath (such as "gtk/gtk.h"), and the search path
///     to get to that file.
class IncDirDependencyMap:public NameValueFile
    {
    public:
        void read(char const * const outDir, char const * const incPath);
        void write();
        void insert(const std::string &includerPath, const FilePath &includedPath);

    private:
        /// This map keeps all included paths for every includer that was parsed.
        /// This map is <includerPath, includedPath's>
        /// The second string is a compound value containing the IncludedPath,
        /// which contains the included filepath, and the search path.
        std::map<std::string, std::set<std::string>> mParsedIncludeDependencies;
        bool includedPathsChanged(OovStringRef includerFn,
                std::set<std::string> const &includedInfoStr) const;
    };


#endif /* INCDIRMAP_H_ */
