/*
 * Components.cpp
 *
 *  Created on: Jan 4, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#include "Components.h"
#include "FilePath.h"
#include "Project.h"
#include "OovString.h"  // For split
#include "OovError.h"
#include "Options.h"
#include <algorithm>
#include <string.h>     // for strcmp


/*
FilePaths getAsmSourceExtensions()
    {
    .S
    .asm
    }
*/

/*
FilePaths getJavaSourceExtension()
    {
    .java
    .class - compiled java
    .dpj - dependency list
    .jar archived classes (similar to lib)
    }
*/

FilePaths getCppHeaderExtensions()
    {
    FilePaths strs;
    strs.push_back(FilePath("h", FP_Ext));
    strs.push_back(FilePath("hh", FP_Ext));
    strs.push_back(FilePath("hpp", FP_Ext));
    strs.push_back(FilePath("hxx", FP_Ext));
    strs.push_back(FilePath("inc", FP_Ext));
    return strs;
    }

FilePaths getCppSourceExtensions()
    {
    FilePaths strs;
    strs.push_back(FilePath("c", FP_Ext));
    strs.push_back(FilePath("cc", FP_Ext));
    strs.push_back(FilePath("c++", FP_Ext));
    strs.push_back(FilePath("cpp", FP_Ext));
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

FilePaths getJavaSourceExtensions()
    {
    FilePaths strs;
    strs.push_back(FilePath("java", FP_Ext));
    return strs;
    }

bool isJavaSource(OovStringRef const file)
    { return(FilePathAnyExtensionMatch(getJavaSourceExtensions(), file)); }

bool isCppHeader(OovStringRef const file)
    { return(FilePathAnyExtensionMatch(getCppHeaderExtensions(), file)); }

bool isCppSource(OovStringRef const file)
    { return(FilePathAnyExtensionMatch(getCppSourceExtensions(), file)); }

bool isLibrary(OovStringRef const file)
    { return(FilePathAnyExtensionMatch(getLibExtensions(), file)); }

OovStatusReturn ScannedComponentInfo::readScannedInfo()
    {
    mCompSourceListFile.setFilename(Project::getComponentSourceListFilePath());
    OovStatus status = mCompSourceListFile.readFile();
    // For new projects, the files are optional.
    /// @todo - should detect the difference between missing files and disk errors.
    if(status.needReport())
        {
        status.clearError();
        }
    return status;
    }

OovStatusReturn ScannedComponentInfo::writeScannedInfo()
    {
    mCompSourceListFile.setFilename(Project::getComponentSourceListFilePath());
    OovStatus status = mCompSourceListFile.writeFile();
    if(status.needReport())
        {
        OovString str = "Unable to write source list file: ";
        str += mCompSourceListFile.getFilename();
        status.report(ET_Error, str);
        }
    return status;
    }

OovStringVec ScannedComponentInfo::getComponentNames() const
    {
    return CompoundValueRef::parseString(
        mCompSourceListFile.getValue("Components"));
    }

void ScannedComponentInfo::setComponentFiles(CompFileTypes cft,
    OovStringRef const compName, OovStringSet const &srcs)
    {
    CompoundValue objArgs;
    for(const auto &src : srcs)
        {
        objArgs.addArg(src);
        }
    OovString tag = getCompTagName(compName, getCompFileTypeTagName(cft));
    mCompSourceListFile.setNameValue(tag, objArgs.getAsString());
    }

OovStringVec ScannedComponentInfo::getComponentDirFiles(OovStringRef compName,
    OovStringRef tagStr) const
    {
    OovString tag = getCompTagName(compName, tagStr);
    OovString val = mCompSourceListFile.getValue(tag);
    return CompoundValueRef::parseString(val);
    }

OovStringRef ScannedComponentInfo::getCompFileTypeTagName(CompFileTypes cft)
    {
    char const *name = nullptr;
    switch(cft)
        {
        case CFT_CppSource:     name = "cppSrc";   break;
        case CFT_CppInclude:    name = "cppInc";   break;
        case CFT_JavaSource:    name = "java";   break;
        }
    return(name);
    }

OovString ScannedComponentInfo::getCompTagName(OovStringRef const compName,
    OovStringRef const tag)
     {
    OovString tagName = "Comp-";
    tagName += tag;
    tagName += "-";
    tagName += compName;
    return tagName;
    }

OovStringVec ScannedComponentInfo::getComponentFiles(ComponentTypesFile compInfo,
    CompFileTypes cft, OovStringRef const compName, bool getNested) const
    {
    return getComponentFiles(compInfo, compName, getCompFileTypeTagName(cft), getNested);
    }

OovStringVec ScannedComponentInfo::getComponentFiles(ComponentTypesFile compInfo,
    OovStringRef const compName, OovStringRef const tagStr, bool getNested) const
    {
    OovStringVec files;
    // This must be allowed for unknown components because:
    // - The component list shows files for unknown components
    // - The subdirectories/nested directories of a component may be unknown.
//    if(getComponentType(compName) != CT_Unknown)
        {
        OovStringVec names = getComponentNames();
        OovString parentName = compName;
        for(auto const &name : names)
            {
            bool match = false;
            if(getNested)
                {
                OovString owner = compInfo.getComponentNameOwner(name);
                match = (owner.compare(compName) == 0);
                }
            else
                {
                match = (parentName == name);
                }
            if(match)
                {
                OovStringVec newFiles = getComponentDirFiles(name, tagStr);
                files.insert(files.end(), newFiles.begin(), newFiles.end());
                }
            }
        }
    return files;
    }


////////////


OovStringVec ComponentTypesFile::getDefinedComponentNames() const
    {
    ComponentDefinitions comps = getDefinedComponents();
    OovStringVec compNames;
    for(auto const &comp : comps)
        {
        compNames.push_back(comp.getCompName());
        }
    return compNames;
    }

OovStringVec ComponentTypesFile::getMatchingCompTypeVariables() const
    {
    OovStringVec compVars;
    if(mBuildEnv)
        {
        compVars = mBuildEnv->getMatchingVariablesIgnoreComp(OptCompType);
        }
    else
        {
        compVars = mProject.getMatchingNames(OptCompType);
        }
    return compVars;
    }

ComponentDefinitions ComponentTypesFile::getDefinedComponents(char const *compName) const
    {
    OovStringVec compVars = getMatchingCompTypeVariables();
    ComponentDefinitions compDefs;
    for(auto const &var : compVars)
        {
        BuildVariable buildVar;
        OovString filterDef = var;
        buildVar.initVarFromString(filterDef, mProject.getValue(var));
        ComponentDefinition def;
        def.mCompName = buildVar.getFilterValue(OptFilterNameComponent);
        def.mCompType = getComponentTypeFromTypeName(mProject.getValue(var));
        if(def.mCompType != CT_Unknown)
            {
            if(compName == nullptr || def.mCompName == compName)
                {
                // If there are multiple defined, then just get the first one.
                // It is expected that component types will only be unknown/none
                // or one other defined type. There should never be conflicting
                // defined types.
                auto it = std::find_if(compDefs.begin(), compDefs.end(),
                    [&def](ComponentDefinition const &item)
                    { return(item.getCompName() == def.getCompName()); });
                if(it == compDefs.end())
                    {
                    compDefs.push_back(def);
                    }
                }
            }
        }
#if(0)
printf("getDefinedComponents %p\n", mBuildEnv);
if(compName)
    { printf("  %s\n", compName); }
for(auto const &def : compDefs)
    {
    printf("    %s %d\n", def.getCompName().getStr(), def.getCompType());
    }
fflush(stdout);
#endif
    return compDefs;
    }

OovString ComponentTypesFile::getComponentDir(OovStringRef relDir,
    OovStringRef compName)
    {
    FilePath outDir(relDir, FP_Dir);
    if(strcmp(compName, Project::getRootComponentName()) != 0)
        { outDir.appendDir(compName); }
    return outDir;
    }

OovString ComponentTypesFile::getComponentBaseFileName(OovStringRef relDir,
    OovStringRef compName)
    {
    FilePath outFileName(getComponentDir(relDir, compName), FP_Dir);

    if(strcmp(compName, Project::getRootComponentName()) != 0)
        { outFileName.appendFile(compName); }
    else
        { outFileName.appendFile(Project::getRootComponentFileName()); }
    return outFileName;
    }

OovString ComponentTypesFile::getComponentFileName(OovStringRef relDir,
    OovStringRef compName, OovStringRef ext)
    {
    FilePath fn(getComponentBaseFileName(relDir, compName), FP_File);
    fn.appendExtension(ext);
    return fn;
    }

OovString ComponentTypesFile::getComponentFileName(OovStringRef relDir,
    OovStringRef compName, OovStringRef basePrefix, OovStringRef ext)
    {
    FilePath fn(getComponentBaseFileName(relDir, compName), FP_File);
    size_t compNamePos = FilePathGetPosEndDir(fn);
    if(compNamePos != std::string::npos)
        { fn.insert(compNamePos+1, basePrefix); }
    fn.appendExtension(ext);
    return fn;
    }

OovStringVec ComponentTypesFile::getDefinedComponentNamesByType(eCompTypes cft) const
    {
    ComponentDefinitions allComps = getDefinedComponents();
    OovStringVec filteredNames;
    for(auto const &compDef : allComps)
        {
        if(compDef.getCompType() == cft)
            {
            filteredNames.push_back(compDef.getCompName());
            }
        }
    return filteredNames;
    }

std::string ComponentTypesFile::getComponentChildName(std::string const &compName)
    {
    OovString child = compName;
    size_t pos = child.rfind('/');
    if(pos != OovString::npos)
        {
        child.erase(0, pos+1);
        }
    return child;
    }

std::string ComponentTypesFile::getComponentParentName(std::string const &compName)
    {
    OovString parent = compName;
    size_t pos = parent.rfind('/');
    if(pos != OovString::npos)
        {
        parent.erase(pos);
        }
    return parent;
    }

bool ComponentTypesFile::anyComponentsDefined() const
    {
    return(getDefinedComponents().size() > 0);
    }

enum eCompTypes ComponentTypesFile::getComponentType(
        OovStringRef const compName) const
    {
    ComponentDefinitions compDefs = getDefinedComponents(compName);
    eCompTypes compType = CT_Unknown;
    if(compDefs.size())
        {
        compType = compDefs[0].getCompType();
        }
    return compType;
    }

void ComponentTypesFile::setComponentType(OovStringRef const compName, eCompTypes ct)
    {
    OovString tag = buildCompTypeVarFilterName(compName);
    OovStringRef const value = getComponentTypeAsFileValue(ct);
    mProject.setNameValue(tag, value);
    }

void ComponentTypesFile::coerceParentComponents(OovStringRef const compName)
    {
    OovString name = compName;
    while(1)
        {
        size_t pos = name.rfind('/');
        if(pos != std::string::npos)
            {
            name.erase(pos);
            }
        if(compName != name.getStr())
            {
            setComponentType(name, CT_Unknown);
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

void ComponentTypesFile::coerceChildComponents(OovStringRef const compName)
    {
    ComponentDefinitions compDefs = getDefinedComponents();
    OovString parentName = compName;
    for(auto const &def : compDefs)
        {
        if(def.getCompName() != parentName)
            {
            if(compareComponentNames(parentName, def.getCompName()))
                {
                setComponentType(def.getCompName(), CT_Unknown);
                }
            }
        }
    }

void ComponentTypesFile::setComponentType(OovStringRef const compName,
    OovStringRef const typeName)
    {
    eCompTypes newType = getComponentTypeFromTypeName(typeName);
    if(newType != CT_Unknown && newType != getComponentType(compName))
        {
        coerceParentComponents(compName);
        coerceChildComponents(compName);
        }
    setComponentType(compName, getComponentTypeFromTypeName(typeName));
    }

enum eCompTypes ComponentTypesFile::getComponentTypeFromTypeName(
        OovStringRef const compTypeName)
    {
    eCompTypes ct = CT_Unknown;
    if(compTypeName.numBytes() != 0)
        {
        if(compTypeName[0] == 'P')
            ct = CT_Program;
        else if(compTypeName[0] == 'U')
            ct = CT_Unknown;
        else if(compTypeName[1] == 't')
            ct = CT_StaticLib;
        else if(compTypeName[1] == 'h')
            ct = CT_SharedLib;
        else if(compTypeName[0] == 'J')
            {
            if(compTypeName[4] == 'L' || compTypeName[5] == 'L')
                { ct = CT_JavaJarLib; }
            else
                { ct = CT_JavaJarProg; }
            }
        }
    return ct;
    }

OovStringRef const ComponentTypesFile::getLongComponentTypeName(eCompTypes ct)
    {
    char const *p = NULL;
    switch(ct)
        {
        case CT_Unknown:        p = "Undefined/Not Applicable";         break;
        case CT_StaticLib:      p = "Static/Compile-time Library";      break;
        case CT_SharedLib:      p = "Shared/Run-time Library";          break;
        case CT_Program:        p = "Program/Executable";               break;
        case CT_JavaJarLib :    p = "Java Library Jar";                 break;
        case CT_JavaJarProg:    p = "Java Executable Jar";              break;
        }
    return p;
    }

OovStringRef const ComponentTypesFile::getComponentTypeAsFileValue(eCompTypes ct)
    {
    char const *p = NULL;
    switch(ct)
        {
        case CT_Unknown:        p = "Unknown";          break;
        case CT_StaticLib:      p = "StaticLib";        break;
        case CT_SharedLib:      p = "SharedLib";        break;
        case CT_Program:        p = "Program";          break;
        case CT_JavaJarLib :    p = "JavaLib";          break;
        case CT_JavaJarProg:    p = "JavaProg";         break;
        }
    return p;
    }

OovString ComponentTypesFile::buildCompTypeVarFilterName(OovStringRef const compName)
    {
    BuildVariable buildVar;
    buildVar.setVarName(OptCompType);
    buildVar.addFilter(OptFilterNameComponent, compName);
    buildVar.setFunction(BuildVariable::F_Assign);
    return(buildVar.getVarFilterName());
    }

OovString ComponentTypesFile::getComponentNameOwner(OovStringRef compName) const
    {
    ComponentDefinition ownerComp;
    ComponentDefinitions definedComps = getDefinedComponents();
    for(auto const &comp : definedComps)
        {
        int len = comp.getCompName().length();
        if(comp.getCompName().compare(0, len, compName, len) == 0)
            {
            if(comp.getCompName().length() > ownerComp.getCompName().length())
                {
                ownerComp = comp;
                }
            }
        }
    OovString ownerCompName;
    if(ownerComp.getCompName().length() == 0)
        {
        OovString rootName = Project::getRootComponentName();
        if(getComponentType(rootName) != CT_Unknown)
            {
            ownerCompName = rootName;
            }
        }
    else
        {
        ownerCompName = ownerComp.getCompName();
        }
    return ownerCompName;
    }

OovString ComponentTypesFile::getComponentAbsolutePath(
        OovStringRef const compName) const
    {
    FilePath path(Project::getSrcRootDirectory(), FP_Dir);
    if(strcmp(compName, Project::getRootComponentName()) != 0)
        {
        path.appendDir(compName);
        }
    return path;
    }


