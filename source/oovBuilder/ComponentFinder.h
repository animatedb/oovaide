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

/// This keeps the insertion order, but does not store duplicates.
class InsertOrderedSet:public OovStringVec
{
public:
    void insert(OovStringRef const str)
        {
        if(!exists(str))
            push_back(str);
        }
    bool exists(OovStringRef const &str) const;
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
            OovStringRef const compName, ComponentTypesFile &compFile,
            ScannedComponentInfo &file,
            OovStringRef analysisPath) const;
        static OovString getComponentName(ProjectReader const &proj,
            OovStringRef const filePath);

    private:
        OovStringSet mJavaSourceFiles;
        OovStringSet mCppSourceFiles;
        OovStringSet mCppIncludeFiles;
        void saveComponentFileInfo(ScannedComponentInfo::CompFileTypes cft,
            ProjectReader const &proj, OovStringRef const compName,
            ComponentTypesFile&compFile, ScannedComponentInfo &file,
            OovStringRef analysisPath, OovStringSet const &newFiles) const;
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
            ComponentTypesFile &file, ScannedComponentInfo &scannedFile,
            OovStringRef analysisPath);
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
        enum VariableName
            {
            VN_ConvertTool,     // Uses build config name, comp name, file type, and the platform.
            VN_IntDeps,         // Uses build config name, comp name, file type, and the platform.
            };
        /// @todo - should read these definitions from file
        enum VarFileTypes
            {
            VFT_Cpp,            // For .cpp, .h, ...
            VFT_Java            // For .java, .class
            };

        ComponentFinder():
            mComponentTypesFile(mProject), mProjectBuildArgs(mProject),
            mScanningPackage(nullptr), mAddIncs(false), mAddLibs(false)
            {}
        bool readProject(OovStringRef const oovProjectDir,
            OovStringRef buildMode, OovStringRef const buildConfigName);

        /// Recursively scan a directory for source and include files.
        OovStatus scanProject();

        /// This can accept a rootSrch dir such as "/rootdir/.!/excludedir1/!/excludedir2/"
        void scanExternalProject(OovStringRef const externalRootSrch,
                Package const *pkg=nullptr);

        bool doesBuildPackageExist(OovStringRef pkgName)
            { return getProjectBuildArgs().getBuildPackages().doesPackageExist(pkgName); }
        void addBuildPackage(Package const &pkg);

        /// Adds the components to the project components file.
        void saveProject(OovStringRef analysisPath);

        OovString getComponentName(OovStringRef const filePath) const;

        ProjectReader &getProject()
            { return mProject; }
        ProjectBuildArgs &getProjectBuildArgs()
            { return mProjectBuildArgs; }
        ProjectBuildArgs const &getProjectBuildArgs() const
            { return mProjectBuildArgs; }
        void setCompConfig(OovStringRef filename)
            { mProjectBuildArgs.setCompConfig(filename); }
        const ComponentTypesFile &getComponentTypesFile() const
            { return mComponentTypesFile; }

        const ScannedComponentsInfo &getScannedInfo() const
            { return mScannedInfo; }
        const ScannedComponentInfo &getScannedComponentInfo() const
            { return mScannedComponentInfo; }

        // This returns all external and internal dependencies required for the component.
        OovStringVec getFileIncludeDirs(OovStringRef const srcFile) const;

        OovString makeActualComponentName(OovStringRef const projName) const;
        static OovString getRelCompDir(OovStringRef const projName);
        static void appendArgs(bool appendSwitchArgs, OovStringRef argStr, CppChildArgs &args);

        /// This splits the root search dir into a root dir and exclude directories.
        /// This can accept a rootSrch dir such as "/rootdir/.!/excludedir1/!/excludedir2/"
        /// @param rootSrch The entire search dir including exclude directories.
        /// @param rootDir The returned rootDir part of the search dir.
        /// @param excludes The returned list of exclude directories.
        static void parseProjRefs(OovStringRef const rootSrch, OovString &rootDir,
                OovStringVec &excludes);

        /// Checks to see if the filePath is a substring of any of the exclude
        /// paths.
        /// @param filePath The filePath to search for within the excludes.
        /// @param excludes The list of exclude directories.
        static bool excludesMatch(OovStringRef const filePath,
                OovStringVec const &excludes);

    private:
        // This is overwritten for every external project.
        OovStringVec mExcludeDirs;

        ProjectReader mProject;
        ComponentTypesFile mComponentTypesFile;
        /// @todo - should fix/combine these classes
        ScannedComponentInfo mScannedComponentInfo;     /// This is the file
        ScannedComponentsInfo mScannedInfo;

        OovString mBuildConfigName;
        ProjectBuildArgs mProjectBuildArgs;
        Package *mScanningPackage;
        bool mAddIncs;
        bool mAddLibs;

        /// While searching the directories add C++ source files to the
        /// mComponentNames set, and C++ include files to the mIncludeDirs list.
        virtual bool processFile(OovStringRef const filePath) override;

        /// This returns the external project package dirs, and the internal project
        /// scanned dirs.
        OovStringVec getAllIncludeDirs() const;
        OovStringRef getBuildConfigName() const
            { return mBuildConfigName; }
        enum VarFileTypes getVariableFileType(OovStringRef const filePath) const;
//        OovString getVariableValue(VariableName vn, OovStringRef const srcFile) const;
    };

#endif
