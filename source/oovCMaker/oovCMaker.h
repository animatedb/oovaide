// oovCMaker.h
// Created on: Jan 23, 2014
// \copyright 2014 DCBlaha.  Distributed under the GPL.

#include "Components.h"
#include "IncludeMap.h"
#include "Packages.h"
#include "Project.h"
#include "BuildConfigReader.h"
#include "OovError.h"


class CMaker
    {
    public:
        CMaker(OovStringRef const projName, bool verbose);
        bool makeFiles(bool writeToProject);

    public:
        std::string mProjectName;

        BuildConfigReader mConfig;
        ComponentTypesFile mCompTypes;
        // First arg is component name, second is path
        // std::map<OovString, OovString> mCachedComponentPaths;
        BuildPackages mBuildPkgs;
        ProjectReader mProject;
        IncDirDependencyMapReader mIncMap;
        bool mVerbose;

        OovStatusReturn makeTopMakelistsFile(OovStringRef const destName);
        OovStatusReturn makeTopLevelFiles(OovStringRef const outDir);
        OovStatusReturn makeToolchainFiles(OovStringRef const outDir);
        OovStatusReturn makeComponentFiles(bool writeToProject, OovStringRef const outDir,
                OovStringVec const &compNames);
        std::string getAnalysisPath() const
            {
            return mConfig.getAnalysisPath();
            }
        OovStringVec getCompSources(OovStringRef const compName);
        OovStringVec getCompLibrariesAndIncs(OovStringRef const compName,
                OovStringVec &extraIncDirs);
        OovStatusReturn writeFile(OovStringRef const destName, OovStringRef const str);
        // A cached version of the function in ComponentTypesFile.
        // This doesn't make a big enough difference at this time.
        // OovString getComponentAbsolutePath(OovStringRef const compName);
        static void appendNames(OovStringVec const &names, char delim,
            OovString &str);
        void addLibsAndIncs(OovStringRef const compName, OovString &str);
        void addPackageDefines(OovStringRef const pkgName, std::string &str);
        void makeDefineName(OovStringRef const pkgName, OovString &defName);
        OovStatusReturn makeToolchainFile(OovStringRef const compilePath, OovStringRef const destName);
        OovStatusReturn makeTopInFile(OovStringRef const destName);
        OovStatusReturn makeTopVerInFile(OovStringRef const destName);
        OovStatusReturn makeComponentFile(OovStringRef const compName,
            ComponentTypesFile::eCompTypes compType,
            OovStringVec const &source, OovStringRef const destName);
        OovString makeJavaComponentFile(OovStringRef const compName,
            ComponentTypesFile::eCompTypes compType,
            OovStringVec const &source, OovStringRef const destName);
    };
