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
///	the last time the path dependencies changed,
///	the last time the paths were checked,
///	the included filepath (such as "gtk/gtk.h"), and the directory to get to that file.
/// This file does not store paths that the compiler searches by default.
class IncDirDependencyMap:public NameValueFile
    {
    public:
	void read(char const * const outDir, char const * const incPath);
	void write();
	void insert(const std::string &includerPath, const FilePath &includedPath);

    private:
	/// This map keeps all included paths for every includer that was parsed.
	/// This map is <includerPath, includedPath's>
	std::map<std::string, std::set<std::string>> mParsedIncludeDependencies;
    };


#endif /* INCDIRMAP_H_ */
