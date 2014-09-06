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
	static bool isOutputOld(char const * const outputFn,
		char const * const inputFn);
	static bool isOutputOld(char const * const outputFn,
		const std::vector<std::string> &inputs, int *oldIndex=nullptr);
    };

/// This keeps the insertion order, but does not store duplicates.
class InsertOrderedSet:public std::vector<std::string>
{
public:
    void insert(const std::string &str)
	{
	if(!exists(str))
	    push_back(str);
	}
    bool exists(const std::string &str)
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
	void addSourceFileName(char const * const objPath)
	    { mSourceFiles.insert(objPath); }
	void addIncludeFileName(char const * const objPath)
	    { mIncludeFiles.insert(objPath); }
	void saveComponentSourcesToFile(char const * const compName,
            ComponentTypesFile &file) const;
    private:
	std::set<std::string> mSourceFiles;
	std::set<std::string> mIncludeFiles;
    };

// Temporary storage while scanning.
class ScannedComponentsInfo
    {
    public:
	void addProjectIncludePath(char const * const path)
	    { mProjectIncludeDirs.insert(path); }
	void addSourceFile(char const * const compName, char const * const srcFileName);
	void addIncludeFile(char const * const compName, char const * const srcFileName);
	void initializeComponentTypesFileValues(ComponentTypesFile &file);
	void setProjectComponentsFileValues(ComponentsFile &file);
	InsertOrderedSet const &getProjectIncludeDirs() const
	    { return mProjectIncludeDirs; }

    private:
	std::map<std::string, ScannedComponent> mComponents;
        typedef std::map<std::string, ScannedComponent>::iterator MapIter;

	// Components-init-proj-incs
	InsertOrderedSet mProjectIncludeDirs;

        MapIter addComponents(char const * const compName);
    };


/// This recursively searches a directory path and considers each directory that
/// has C++ source files to be a component.  All directories that contain C++
/// include files are saved.
class ComponentFinder:public dirRecurser
    {
    public:
	ComponentFinder():
	mScanningPackage(nullptr)
	    {}
	bool readProject(char const * const oovProjectDir,
		char const * const buildConfigName);

	/// Recursively scan a directory for source and include files.
	void scanProject();

	void scanExternalProject(char const * const externalRootSrch,
		Package const *pkg=nullptr);

	void addBuildPackage(Package const &pkg);

	/// Adds the components and initial includes to the project components
	/// file.
	void saveProject(char const * const compFn);

	std::string getComponentName(char const * const filePath) const;

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
	std::vector<std::string> getAllIncludeDirs() const;

	std::string makeFileName(char const * const projName) const
	    {
	    std::string fn = projName;
	    if(strcmp(projName, "<Root>") == 0)
		{ fn = mRootPathName; }
	    return fn;
	    }

    private:
	mutable std::string mRootPathName;
	// This is overwritten for every external project.
	std::vector<std::string> mExcludeDirs;

	ScannedComponentsInfo mScannedInfo;

	ProjectReader mProject;
	ComponentTypesFile mComponentTypesFile;
	ComponentsFile mComponentsFile;
	Package *mScanningPackage;

	/// While searching the directories add C++ source files to the
	/// mComponentNames set, and C++ include files to the mIncludeDirs list.
	virtual bool processFile(const std::string &filePath);
    };

class CppChildArgs:public OovProcessChildArgs
    {
    public:
	/// add arg list for processing one source file.
	void addCompileArgList(const class ComponentFinder &finder,
		const std::vector<std::string> &incDirs);
    };

#endif
