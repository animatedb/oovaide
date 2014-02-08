/*
 * Components.h
 *
 *  Created on: Jan 4, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#ifndef COMPONENTS_H_
#define COMPONENTS_H_

#include "NameValueFile.h"
#include "FilePath.h"

FilePaths getHeaderExtensions();
FilePaths getSourceExtensions();
FilePaths getLibExtensions();

bool isHeader(char const * const file);
bool isSource(char const * const file);
bool isLibrary(char const * const file);

// This file is read and written by both oovcde and oovBuilder.
class ComponentTypesFile:public NameValueFile
    {
    public:
	enum CompTypes
	    {
	    CT_Unknown,
	    CT_StaticLib,	// .a or .lib
	    CT_SharedLib,	// .so or .dll
	    CT_Program		// no extension or .exe
	    };

	bool read();
	std::vector<std::string> getComponentNames() const
	    { return CompoundValueRef::parseString(getValue("Components").c_str()); }
	bool anyComponentsDefined() const;
	enum CompTypes getComponentType(char const * const compName) const;
	std::vector<std::string> getComponentSources(char const * const compName) const;
	std::vector<std::string> getComponentIncludes(char const * const compName) const;

	static enum CompTypes getComponentTypeFromTypeName(
		char const * const compTypeName);
	static char const * const getLongComponentTypeName(CompTypes ct);
	static char const * const getShortComponentTypeName(CompTypes ct)
	    { return getComponentTypeAsFileValue(ct); }
	static char const * const getComponentTypeAsFileValue(CompTypes ct);
	static std::string getCompTagName(std::string compName, char const * const tag);
	std::string getComponentAbsolutePath(char const * const compName) const;
    };

/// This file is stored in the analysis directory.
/// This class does not open the file. The read must be done before calling any of
/// the other members.
class ComponentsFile:public NameValueFile
    {
    public:
	void read(char const * const fn);
	/// Get the include paths.
	std::vector<std::string> getProjectIncludeDirs() const
	    {
	    return CompoundValueRef::parseString(
		    getProjectIncludeDirsStr().c_str());
	    }
	std::vector<std::string> getAbsoluteIncludeDirs() const;
	std::vector<std::string> getExternalRootPaths() const
	    {
	    return CompoundValueRef::parseString(
		    getValue("Components-init-ext-roots").c_str());
	    }
	static void parseProjRefs(char const * const arg, std::string &rootDir,
		std::vector<std::string> &excludes);
	static bool excludesMatch(const std::string &filePath,
		const std::vector<std::string> &excludes);

    private:
	std::string getProjectIncludeDirsStr() const;
    };

#endif /* COMPONENTS_H_ */
