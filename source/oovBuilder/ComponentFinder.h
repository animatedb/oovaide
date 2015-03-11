/*
 * ComponentFinder.h
 *
 *  Created on: Aug 27, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef COMPONENTFINDER_H_
#define COMPONENTFINDER_H_

#include "DirList.h"
#include "Project.h"
#include "Components.h"
#include "OovProcess.h"
#include "Packages.h"
#include "IncludeMap.h"

class FileStat
    {
    public:
	static bool isOutputOld(OovStringRef const outputFn,
		OovStringRef const inputFn);
	static bool isOutputOld(OovStringRef const outputFn,
		OovStringVec const &inputs, int *oldIndex=nullptr);
    };

class ToolPathFile:public NameValueFile
    {
    public:
	void setConfig(OovStringRef const buildConfig)
	    {
	    mBuildConfig = buildConfig;
	    }
	std::string getCompilerPath();
	static std::string getAnalysisToolPath();
	std::string getLibberPath();
	std::string getObjSymbolPath();
	static std::string getCovInstrToolPath();
    private:
	std::string mBuildConfig;
//	std::string mPathAnalysisTool;
	std::string mPathCompiler;
	std::string mPathLibber;
	std::string mPathObjSymbol;
//	std::string mPathCovInstrTool;
	void getPaths();
    };


/// This keeps the insertion order, but does not store duplicates.
class InsertOrderedSet:public OovStringVec
{
public:
    void insert(OovStringRef const str)
	{
	if(!exists(str))
	    push_back(str);
	}
    bool exists(OovStringRef const &str)
	{
	bool exists = false;
	for(const auto &s : *this)
	    {
	    if(s.compare(str) == 0)
		{
		exists = true;
		break;
		}
	    }
	return exists;
	}
};

// Temporary storage while scanning.
class ScannedComponent
    {
    public:
	void addSourceFileName(OovStringRef const objPath)
	    { mSourceFiles.insert(objPath); }
	void addIncludeFileName(OovStringRef const objPath)
	    { mIncludeFiles.insert(objPath); }
	void saveComponentSourcesToFile(OovStringRef const compName,
            ComponentTypesFile &file) const;
    private:
	OovStringSet mSourceFiles;
	OovStringSet mIncludeFiles;
    };

// Temporary storage while scanning.
class ScannedComponentsInfo
    {
    public:
	void addProjectIncludePath(OovStringRef const path)
	    { mProjectIncludeDirs.insert(path); }
	void addSourceFile(OovStringRef const compName, OovStringRef const srcFileName);
	void addIncludeFile(OovStringRef const compName, OovStringRef const srcFileName);
	void initializeComponentTypesFileValues(ComponentTypesFile &file);
	void setProjectComponentsFileValues(ComponentsFile &file);
	InsertOrderedSet const &getProjectIncludeDirs() const
	    { return mProjectIncludeDirs; }

    private:
	std::map<OovString, ScannedComponent> mComponents;
        typedef std::map<OovString, ScannedComponent>::iterator MapIter;

	// Components-init-proj-incs
	InsertOrderedSet mProjectIncludeDirs;

        MapIter addComponents(OovStringRef const compName);
    };

enum eProcessModes { PM_Analyze, PM_Build, PM_CovInstr, PM_CovBuild, PM_CovStats };

/// This recursively searches a directory path and considers each directory that
/// has C++ source files to be a component.  All directories that contain C++
/// include files are saved.
class ComponentFinder:public dirRecurser
    {
    public:
	ComponentFinder():
	mScanningPackage(nullptr)
	    {}
	bool readProject(OovStringRef const oovProjectDir,
		OovStringRef const buildConfigName);

	/// Recursively scan a directory for source and include files.
	void scanProject();

	void scanExternalProject(OovStringRef const externalRootSrch,
		Package const *pkg=nullptr);

	void addBuildPackage(Package const &pkg);

	/// Adds the components and initial includes to the project components
	/// file.
	void saveProject(OovStringRef const compFn);

	OovString getComponentName(OovStringRef const filePath) const;

	const ProjectReader &getProject() const
	    { return mProject; }
	ProjectReader &getProject()
	    { return mProject; }
	const ComponentTypesFile &getComponentTypesFile() const
	    { return mComponentTypesFile; }
	const ComponentsFile &getComponentsFile() const
	    { return mComponentsFile; }
	const ScannedComponentsInfo &getScannedInfo() const
	    { return mScannedInfo; }
	OovStringVec getAllIncludeDirs() const;

	OovString makeFileName(OovStringRef const projName) const
	    {
	    OovString fn = projName;
	    if(strcmp(projName, Project::getRootComponentName()) == 0)
		{ fn = mRootPathName; }
	    return fn;
	    }

    private:
	mutable std::string mRootPathName;
	// This is overwritten for every external project.
	OovStringVec mExcludeDirs;

	ScannedComponentsInfo mScannedInfo;

	ProjectReader mProject;
	ComponentTypesFile mComponentTypesFile;
	ComponentsFile mComponentsFile;
	Package *mScanningPackage;

	/// While searching the directories add C++ source files to the
	/// mComponentNames set, and C++ include files to the mIncludeDirs list.
	virtual bool processFile(OovStringRef const filePath) override;
    };

class CppChildArgs:public OovProcessChildArgs
    {
    public:
	/// add arg list for processing one source file.
	void addCompileArgList(const class ComponentFinder &finder,
		const OovStringVec &incDirs);
    };

#endif
