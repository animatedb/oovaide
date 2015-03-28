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

bool isHeader(OovStringRef const file);
bool isSource(OovStringRef const file);
bool isLibrary(OovStringRef const file);


/// This file is read and written by both oovcde and oovBuilder.
/// This defines each component's source and include files.
/// A component pretty directly maps to a directory in the project.
class ComponentTypesFile
    {
    public:
	enum eCompTypes
	    {
	    CT_Unknown,
	    CT_StaticLib,	// .a or .lib
	    CT_SharedLib,	// .so or .dll
	    CT_Program		// no extension or .exe
	    };

	bool read();
	bool readTypesOnly(OovStringRef const fn);
	void setComponentNames(OovStringRef const compNames)
	    {
	    mCompTypesFile.setNameValue("Components", compNames);
	    }
	OovStringVec getComponentNames() const
	    {
	    return CompoundValueRef::parseString(
		    mCompTypesFile.getValue("Components"));
	    }
	static std::string getComponentChildName(std::string const &compName);
	static std::string getComponentParentName(std::string const &compName);

	bool anyComponentsDefined() const;
	enum eCompTypes getComponentType(OovStringRef const compName) const;
        void setComponentType(OovStringRef const compName, OovStringRef const typeName);
        void setComponentSources(OovStringRef const compName,
        	OovStringSet const &srcs);
        void setComponentIncludes(OovStringRef const compName,
            OovStringSet const &incs);
	OovStringVec getComponentSources(OovStringRef const compName) const;
	OovStringVec getComponentIncludes(OovStringRef const compName) const;
	OovString getComponentBuildArgs(OovStringRef const compName) const;
	void setComponentBuildArgs(OovStringRef const compName, OovStringRef const args);

	static OovStringRef const getLongComponentTypeName(eCompTypes ct);
	static OovStringRef const getShortComponentTypeName(eCompTypes ct)
	    { return getComponentTypeAsFileValue(ct); }
	OovString getComponentAbsolutePath(OovStringRef const compName) const;
	void writeFile()
	    {
	    mCompTypesFile.writeFile();
	    mCompSourceListFile.writeFile();
	    }
	void writeTypesOnly(OovStringRef const fn)
	    {
	    mCompTypesFile.setFilename(fn);
	    mCompTypesFile.writeFile();
	    }

    private:
	NameValueFile mCompTypesFile;
	NameValueFile mCompSourceListFile;

	OovStringVec getComponentFiles(OovStringRef const compName,
		OovStringRef const tagStr) const;
        void setComponentType(OovStringRef const compName, eCompTypes ct);
        // Setting a component below some parent must make sure the parents are unknown
	void coerceParentComponents(OovStringRef const compName);
	// Setting a component above some child must make sure children are unknown
	void coerceChildComponents(OovStringRef const compName);
	static OovString getCompTagName(OovStringRef const compName, OovStringRef const tag);
	static OovStringRef const getComponentTypeAsFileValue(eCompTypes ct);
	static enum eCompTypes getComponentTypeFromTypeName(
		OovStringRef const compTypeName);
    };

/// This file is stored in the analysis directory.
/// This class does not open the file. The read must be done before calling any of
/// the other members.
class ComponentsFile:public NameValueFile
    {
    public:
	void read(OovStringRef const fn);
	/// Get the include paths.
	OovStringVec getProjectIncludeDirs() const
	    {
	    return CompoundValueRef::parseString(
		    getProjectIncludeDirsStr());
	    }
	OovStringVec getAbsoluteIncludeDirs() const;
	OovStringVec getExternalRootPaths() const
	    {
	    return CompoundValueRef::parseString(
		    getValue("Components-init-ext-roots"));
	    }
	static void parseProjRefs(OovStringRef const arg, OovString &rootDir,
		OovStringVec &excludes);
	static bool excludesMatch(OovStringRef const filePath,
		OovStringVec const &excludes);

    private:
	OovString getProjectIncludeDirsStr() const;
    };

#endif /* COMPONENTS_H_ */
