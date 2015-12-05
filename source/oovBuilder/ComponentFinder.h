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


class CppChildArgs:public OovProcessChildArgs
    {
    public:
        /// add arg list for processing one source file.
        void addCompileArgList(const class ComponentFinder &finder,
                const OovStringVec &incDirs);
    };

class ToolPathFile:public NameValueFile
    {
    public:
        void setConfig(OovStringRef const buildConfig)
            {
            mBuildConfig = buildConfig;
            }
        std::string getCompilerPath();
        // See Project.h for how this varies depending on build configuration.
        std::string getJavaCompilerPath();
        static void appendArgs(bool appendSwitchArgs, OovStringRef argStr, CppChildArgs &args);
        std::string getJavaJarToolPath();
        std::string getLibberPath();
        std::string getObjSymbolPath();
        static std::string getCovInstrToolPath();
        OovString makeBuildConfigArgName(OovStringRef const baseName)
            { return ::makeBuildConfigArgName(baseName, mBuildConfig); }
    private:
        std::string mBuildConfig;
        std::string mPathJavaJarTool;
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
        void addJavaSourceFileName(OovStringRef const objPath)
            { mJavaSourceFiles.insert(objPath); }
        void addCppSourceFileName(OovStringRef const objPath)
            { mCppSourceFiles.insert(objPath); }
        void addCppIncludeFileName(OovStringRef const objPath)
            { mCppIncludeFiles.insert(objPath); }
        void saveComponentSourcesToFile(ProjectReader const &proj,
            OovStringRef const compName, ComponentTypesFile &file,
            OovStringRef analysisPath) const;
        static OovString getComponentName(ProjectReader const &proj,
            OovStringRef const filePath, OovString *rootPathName=nullptr);

    private:
        OovStringSet mJavaSourceFiles;
        OovStringSet mCppSourceFiles;
        OovStringSet mCppIncludeFiles;
        void saveComponentFileInfo(ComponentTypesFile::CompFileTypes cft,
            ProjectReader const &proj, OovStringRef const compName,
            ComponentTypesFile &compFile, OovStringRef analysisPath,
            OovStringSet const &newFiles) const;
    };

// Temporary storage while scanning.
class ScannedComponentsInfo
    {
    public:
        void addProjectIncludePath(OovStringRef const path)
            { mProjectIncludeDirs.insert(path); }
        void addJavaSourceFile(OovStringRef const compName, OovStringRef const srcFileName);
        void addCppSourceFile(OovStringRef const compName, OovStringRef const srcFileName);
        void addCppIncludeFile(OovStringRef const compName, OovStringRef const srcFileName);
        void initializeComponentTypesFileValues(ProjectReader const &proj,
            ComponentTypesFile &file, OovStringRef analysisPath);
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

/// This recursively searches a directory path and considers each directory that
/// has C++ source files to be a component.  All directories that contain C++
/// include files are saved.
class ComponentFinder:public dirRecurser
    {
    public:
        ComponentFinder():
            mProjectBuildArgs(mProject), mScanningPackage(nullptr),
            mAddIncs(false), mAddLibs(false)
            {}
        bool readProject(OovStringRef const oovProjectDir,
                OovStringRef const buildConfigName);

        /// Recursively scan a directory for source and include files.
        OovStatus scanProject();

        /// This can accept a rootSrch dir such as "/rootdir/.!/excludedir1/!/excludedir2/"
        void scanExternalProject(OovStringRef const externalRootSrch,
                Package const *pkg=nullptr);

        bool doesBuildPackageExist(OovStringRef pkgName)
            { return getProjectBuildArgs().getBuildPackages().doesPackageExist(pkgName); }
        void addBuildPackage(Package const &pkg);

        /// Adds the components and initial includes to the project components
        /// file.
        void saveProject(OovStringRef const compFn, OovStringRef analysisPath);

        OovString getComponentName(OovStringRef const filePath);
// DEAD CODE
//        const ProjectReader &getProject() const
//            { return mProject; }
//        ProjectReader &getProject()
//            { return mProject; }

        ProjectBuildArgs &getProjectBuildArgs()
            { return mProjectBuildArgs; }
        ProjectBuildArgs const &getProjectBuildArgs() const
            { return mProjectBuildArgs; }
        const ComponentTypesFile &getComponentTypesFile() const
            { return mComponentTypesFile; }

// DEAD CODE
//        const ComponentsFile &getComponentsFile() const
//            { return mComponentsFile; }

        const ScannedComponentsInfo &getScannedInfo() const
            { return mScannedInfo; }
        OovStringVec getAllIncludeDirs() const;

        OovString makeActualComponentName(OovStringRef const projName) const
            {
            OovString fn = projName;
            if(strcmp(projName, Project::getRootComponentName()) == 0)
                { fn = mRootPathName; }
            return fn;
            }
        static OovString getRelCompDir(OovStringRef const projName)
            {
            OovString fn = projName;
            if(strcmp(projName, Project::getRootComponentName()) == 0)
                { fn = ""; }
            return fn;
            }

    private:
        OovString mRootPathName;
        // This is overwritten for every external project.
        OovStringVec mExcludeDirs;

        ScannedComponentsInfo mScannedInfo;

        ProjectReader mProject;
        ProjectBuildArgs mProjectBuildArgs;
        ComponentTypesFile mComponentTypesFile;
        ComponentsFile mComponentsFile;
        Package *mScanningPackage;
        bool mAddIncs;
        bool mAddLibs;

        /// While searching the directories add C++ source files to the
        /// mComponentNames set, and C++ include files to the mIncludeDirs list.
        virtual bool processFile(OovStringRef const filePath) override;
    };

#endif
