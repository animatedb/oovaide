// oovCMaker.h
// Created on: Jan 23, 2014
// \copyright 2014 DCBlaha.  Distributed under the GPL.

#include "Components.h"
#include "IncludeMap.h"
#include "Packages.h"
#include "Options.h"
#include "BuildConfigReader.h"


class CMaker
    {
    public:
        CMaker(OovStringRef const projName, bool verbose):
            mProjectName(projName), mBuildPkgs(false), mVerbose(verbose)
            {}
        std::string getAnalysisPath() const
            {
            return mConfig.getAnalysisPath(BuildConfigAnalysis);
            }
        OovStringVec getCompSources(OovStringRef const compName);
        OovStringVec getCompLibrariesAndIncs(OovStringRef const compName,
                OovStringVec &extraIncDirs);
        void makeTopMakelistsFile(OovStringRef const destName);
        void makeTopLevelFiles(OovStringRef const outDir);
        void makeToolchainFiles(OovStringRef const outDir);
        void makeComponentFiles(bool writeToProject, OovStringRef const outDir,
                OovStringVec const &compNames);
        void writeFile(OovStringRef const destName, OovStringRef const str);
        // A cached version of the function in ComponentTypesFile.
        // This doesn't make a big enough difference at this time.
        // OovString getComponentAbsolutePath(OovStringRef const compName);

    public:
        std::string mProjectName;

        BuildConfigReader mConfig;
        ComponentTypesFile mCompTypes;
        // First arg is component name, second is path
        // std::map<OovString, OovString> mCachedComponentPaths;
        BuildPackages mBuildPkgs;
        BuildOptions mBuildOptions;
        IncDirDependencyMapReader mIncMap;
        bool mVerbose;

        static void appendNames(OovStringVec const &names, char delim,
            OovString &str);
        void addLibsAndIncs(OovStringRef const compName, OovString &str);
        void addPackageDefines(OovStringRef const pkgName, std::string &str);
        void makeDefineName(OovStringRef const pkgName, OovString &defName);
        void makeToolchainFile(OovStringRef const compilePath, OovStringRef const destName);
        void makeTopInFile(OovStringRef const destName);
        void makeTopVerInFile(OovStringRef const destName);
        void makeComponentFile(OovStringRef const compName,
            ComponentTypesFile::eCompTypes compType,
            OovStringVec const &source, OovStringRef const destName);
    };
