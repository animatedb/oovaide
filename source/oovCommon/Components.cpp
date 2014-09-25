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
    mCompTypesFile.setFilename(Project::getComponentTypesFilePath());
    mCompSourceListFile.setFilename(Project::getComponentSourceListFilePath());
    // source list file is optional, so ignore return
    mCompSourceListFile.readFile();
    return mCompTypesFile.readFile();
    }

bool ComponentTypesFile::readTypesOnly(char const * const fn)
    {
    mCompTypesFile.setFilename(fn);
    return mCompTypesFile.readFile();
    }

bool ComponentTypesFile::anyComponentsDefined() const
    {
    auto const &names = getComponentNames();
    return std::any_of(names.begin(), names.end(),
	    [=](std::string const &name)
            {return(getComponentType(name.c_str()) != CT_Unknown);} );
    }

enum ComponentTypesFile::eCompTypes ComponentTypesFile::getComponentType(
	char const * const compName) const
    {
    std::string tag = getCompTagName(compName, "type");
    std::string typeStr = mCompTypesFile.getValue(tag.c_str());
    return getComponentTypeFromTypeName(typeStr.c_str());
    }

void ComponentTypesFile::coerceParentComponents(char const * const compName)
    {
    std::string name = compName;
    while(1)
	{
	size_t pos = name.rfind('/');
	if(pos != std::string::npos)
	    {
	    name.erase(pos);
	    }
	if(name != compName)
	    {
	    setComponentType(name.c_str(), CT_Unknown);
	    }
	if(pos == std::string::npos)
	    {
	    break;
	    }
	}
    }

// This should match "Parameter"="Parameter/PLib", but not match "Comm"!="CommSim"
static bool compareComponentNames(std::string const &parentName, std::string const &childName)
    {
    bool child = false;
    if(childName.length() > parentName.length())
	{
	if(childName[parentName.length()] == '/')
	    {
	    child = (parentName.compare(0, parentName.length(), childName, 0,
		    parentName.length()) == 0);
	    }
	}
    return((parentName == childName) || child);
    }

void ComponentTypesFile::coerceChildComponents(char const * const compName)
    {
    std::vector<std::string> names = getComponentNames();
    std::string parentName = compName;
    for(auto const &name : names)
	{
	if(name != parentName)
	    {
	    if(compareComponentNames(parentName, name))
		{
		setComponentType(name.c_str(), CT_Unknown);
		}
	    }
	}
    }

void ComponentTypesFile::setComponentType(char const * const compName, eCompTypes ct)
    {
    std::string tag = getCompTagName(compName, "type");
    char const *value = getComponentTypeAsFileValue(ct);
    mCompTypesFile.setNameValue(tag.c_str(), value);
    }

void ComponentTypesFile::setComponentType(char const * const compName,
    char const * const typeName)
    {
    eCompTypes newType = getComponentTypeFromTypeName(typeName);
    if(newType != CT_Unknown && newType != getComponentType(compName))
	{
	coerceParentComponents(compName);
	coerceChildComponents(compName);
	}
    setComponentType(compName, getComponentTypeFromTypeName(typeName));
    }

enum ComponentTypesFile::eCompTypes ComponentTypesFile::getComponentTypeFromTypeName(
	char const * const compTypeName)
    {
    eCompTypes ct = CT_Unknown;
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

char const * const ComponentTypesFile::getLongComponentTypeName(eCompTypes ct)
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

char const * const ComponentTypesFile::getComponentTypeAsFileValue(eCompTypes ct)
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

std::string ComponentTypesFile::getCompTagName(std::string const &compName,
    char const * const tag)
    {
    return(std::string("Comp-") + tag + "-" + compName);
    }

void ComponentTypesFile::setComponentSources(char const * const compName,
    std::set<std::string> const &srcs)
    {
    CompoundValue objArgs;
    for(const auto &src : srcs)
	{
	objArgs.addArg(src.c_str());
	}
    std::string tag = getCompTagName(compName, "src");
    mCompSourceListFile.setNameValue(tag.c_str(), objArgs.getAsString().c_str());
    }

void ComponentTypesFile::setComponentIncludes(char const * const compName,
    std::set<std::string> const &incs)
    {
    CompoundValue incArgs;
    for(const auto &src : incs)
	{
	incArgs.addArg(src.c_str());
	}
    std::string tag = getCompTagName(compName, "inc");
    mCompSourceListFile.setNameValue(tag.c_str(), incArgs.getAsString().c_str());
    }

std::vector<std::string> ComponentTypesFile::getComponentSources(
	char const * const compName) const
    {
    return getComponentFiles(compName, "src");
    }

std::vector<std::string> ComponentTypesFile::getComponentIncludes(
    char const * const compName) const
    {
    return getComponentFiles(compName, "inc");
    }

std::vector<std::string> ComponentTypesFile::getComponentFiles(char const * const compName,
	char const * const tagStr) const
    {
    std::vector<std::string> files;
    std::vector<std::string> names = getComponentNames();
    std::string parentName = compName;
    for(auto const &name : names)
	{
	if(compareComponentNames(parentName, name))
	    {
	    std::string tag = ComponentTypesFile::getCompTagName(name, tagStr);
	    std::string val = mCompSourceListFile.getValue(tag.c_str());
	    std::vector<std::string> newFiles = CompoundValueRef::parseString(val.c_str());
	    files.insert(files.end(), newFiles.begin(), newFiles.end());
	    }
	}
    return files;
    }

std::string ComponentTypesFile::getComponentBuildArgs(
	char const * const compName) const
    {
    std::string tag = ComponentTypesFile::getCompTagName(compName, "args");
    return mCompTypesFile.getValue(tag.c_str());
    }

void ComponentTypesFile::setComponentBuildArgs(char const * const compName,
	char const * const args)
    {
    std::string tag = ComponentTypesFile::getCompTagName(compName, "args");
    mCompTypesFile.setNameValue(tag.c_str(), args);
    }

std::string ComponentTypesFile::getComponentAbsolutePath(
    char const * const compName) const
    {
    std::string path;
    std::vector<std::string> src = getComponentSources(compName);
    if(src.size() == 0)
	{
	src = getComponentIncludes(compName);
	}
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
    std::vector<OovString> tokens = StringSplit(arg, '!');
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
