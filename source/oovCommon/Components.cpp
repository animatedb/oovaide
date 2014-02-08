/*
 * Components.cpp
 *
 *  Created on: Jan 4, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#include "Components.h"
#include "FilePath.h"
#include "Project.h"
#include "OovString.h"	// For split
#include <algorithm>

FilePaths getHeaderExtensions()
    {
    FilePaths strs;
    strs.push_back(FilePath("h", FP_Ext));
    strs.push_back(FilePath("hpp", FP_Ext));
    strs.push_back(FilePath("hxx", FP_Ext));
    return strs;
    }

FilePaths getSourceExtensions()
    {
    FilePaths strs;
    strs.push_back(FilePath("cpp", FP_Ext));
    strs.push_back(FilePath("cc", FP_Ext));
    strs.push_back(FilePath("cxx", FP_Ext));
    return strs;
    }

FilePaths getLibExtensions()
    {
    FilePaths strs;
    strs.push_back(FilePath("a", FP_Ext));
    strs.push_back(FilePath("lib", FP_Ext));
    return strs;
    }

bool isHeader(char const * const file)
    { return(anyExtensionMatch(getHeaderExtensions(), file)); }

bool isSource(char const * const file)
    { return(anyExtensionMatch(getSourceExtensions(), file)); }

bool isLibrary(char const * const file)
    { return(anyExtensionMatch(getLibExtensions(), file)); }

bool ComponentTypesFile::read()
    {
    setFilename(Project::getComponentTypesFilePath());
    return readFile();
    }

bool ComponentTypesFile::anyComponentsDefined() const
    {
    auto const &names = getComponentNames();
    return std::any_of(names.begin(), names.end(),
	    [=](std::string const &name){return(getComponentType(name.c_str()) != CT_Unknown);} );
    }

enum ComponentTypesFile::CompTypes ComponentTypesFile::getComponentType(
	char const * const compName) const
    {
    std::string tag = getCompTagName(compName, "type");
    std::string typeStr = getValue(tag.c_str());
    return getComponentTypeFromTypeName(typeStr.c_str());
    }

enum ComponentTypesFile::CompTypes ComponentTypesFile::getComponentTypeFromTypeName(
	char const * const compTypeName)
    {
    CompTypes ct = CT_Unknown;
    if(compTypeName[0] == 'P')
	ct = CT_Program;
    else if(compTypeName[0] == 'U')
	ct = CT_Unknown;
    else if(compTypeName[1] == 't')
	ct = CT_StaticLib;
    else if(compTypeName[1] == 'h')
	ct = CT_SharedLib;
    return ct;
    }

char const * const ComponentTypesFile::getLongComponentTypeName(CompTypes ct)
    {
    char const *p = NULL;
    switch(ct)
	{
	case CT_Unknown:    	p = "Undefined/Not Applicable";		break;
	case CT_StaticLib:	p = "Static/Compile-time Library";	break;
	case CT_SharedLib:	p = "Shared/Run-time Library";		break;
	case CT_Program:	p = "Program/Executable";		break;
	}
    return p;
    }

char const * const ComponentTypesFile::getComponentTypeAsFileValue(CompTypes ct)
    {
    char const *p = NULL;
    switch(ct)
	{
	case CT_Unknown:    	p = "Unknown";		break;
	case CT_StaticLib:	p = "StaticLib";	break;
	case CT_SharedLib:	p = "SharedLib";	break;
	case CT_Program:	p = "Program";		break;
	}
    return p;
    }

std::string ComponentTypesFile::getCompTagName(std::string compName, char const * const tag)
    {
    return(std::string("Comp-") + tag + "-" + compName);
    }

std::vector<std::string> ComponentTypesFile::getComponentSources(
	char const * const compName) const
    {
    std::string tag = ComponentTypesFile::getCompTagName(compName, "src");
    std::string val = getValue(tag.c_str());
    return CompoundValueRef::parseString(val.c_str());
    }

std::vector<std::string> ComponentTypesFile::getComponentIncludes(
    char const * const compName) const
    {
    std::string tag = ComponentTypesFile::getCompTagName(compName, "inc");
    std::string val = getValue(tag.c_str());
    return CompoundValueRef::parseString(val.c_str());
    }

std::string ComponentTypesFile::getComponentAbsolutePath(char const * const compName) const
    {
    std::string path;
    std::vector<std::string> src = getComponentSources(compName);
    if(src.size() > 0)
	{
	FilePath fp;
	fp.getAbsolutePath(src[0].c_str(), FP_File);
	fp.discardFilename();
	path = fp;
	}
    return path;
    }

//////////////

void ComponentsFile::read(char const * const fn)
    {
    setFilename(fn);
    readFile();
    }

void ComponentsFile::parseProjRefs(char const * const arg, std::string &rootDir,
	std::vector<std::string> &excludes)
    {
    excludes.clear();
    std::vector<std::string> tokens = split(std::string(arg), '!');
    if(rootDir.size() == 0)
	rootDir = tokens[0];
    std::copy(tokens.begin()+1, tokens.end(), excludes.begin());
    }

std::string ComponentsFile::getProjectIncludeDirsStr() const
    {
    return getValue("Components-init-proj-incs");
    }

std::vector<std::string> ComponentsFile::getAbsoluteIncludeDirs() const
    {
    std::string val = getProjectIncludeDirsStr();
    std::vector<std::string> incs = CompoundValueRef::parseString(val.c_str());
    std::for_each(incs.begin(), incs.end(), [](std::string &fn)
	{
	FilePath fp;
	fp.getAbsolutePath(fn.c_str(), FP_Dir);
	fn = fp;
	});
    return incs;
    }
