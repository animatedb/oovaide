/***************************************************************************
 *   Copyright (C) 2013 by dcblaha,,,   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include "ComponentFinder.h"
#include "srcFileParser.h"
#include "ComponentBuilder.h"
#include "Version.h"
#include "BuildConfigWriter.h"
#include "Project.h"
#include "Packages.h"
#include "Coverage.h"
#include <stdio.h>


class OovBuilder
    {
    public:
        void build(eProcessModes processMode, OovStringRef oovProjDir,
                OovStringRef buildConfigName, bool verbose);
        ComponentFinder &getComponentFinder()
            { return mCompFinder; }

    private:
        ComponentFinder mCompFinder;

        bool readProject(OovStringRef const buildConfigName,
            OovStringRef const oovProjDir, bool verbose);
        void analyze(BuildConfigWriter &cfg, eProcessModes procMode,
            OovStringRef const buildConfigName, OovStringRef const srcRootDir);
    };


void OovBuilder::build(eProcessModes processMode, OovStringRef oovProjDir,
        OovStringRef buildConfigName, bool verbose)
    {
    bool success = true;
    Project::setProjectDirectory(oovProjDir);
    if(processMode == PM_CovBuild)
        {
        success = makeCoverageBuildProject();
        }
    if(success)
        {
        success = readProject(buildConfigName, Project::getProjectDirectory(),
                verbose);
        }
    if(success)
        {
        if(processMode == PM_CovStats)
            {
            if(makeCoverageStats())
                {
                printf("Coverage output: %s",
                    Project::getCoverageProjectDirectory().getStr());
                }
            }
        else
            {
            BuildConfigWriter cfg;
            analyze(cfg, processMode, buildConfigName, Project::getSrcRootDirectory());

            std::string configStr = "Configuration: ";
            configStr += buildConfigName;
            sVerboseDump.logProgress(configStr);
            switch(processMode)
                {
                case PM_Analyze:
                    break;

                default:
                    {
                    std::string incDepsPath = cfg.getIncDepsFilePath();
                    ComponentBuilder compBuilder(getComponentFinder());
                    compBuilder.build(processMode, incDepsPath, buildConfigName);
                    }
                    break;
                }
            }
        }
    }

bool OovBuilder::readProject(OovStringRef const buildConfigName,
        OovStringRef const oovProjDir, bool verbose)
    {
    bool success = mCompFinder.readProject(oovProjDir, buildConfigName);
    if(success)
        {
        if(mCompFinder.getProjectBuildArgs().getVerbose() || verbose)
            {
            sVerboseDump.open(oovProjDir);
            }
        }
    else
        {
        fprintf(stderr, "oovBuilder: Oov project file must exist in %s\n",
            oovProjDir.getStr());
        }
    return success;
    }

void OovBuilder::analyze(BuildConfigWriter &cfg,
        eProcessModes procMode, OovStringRef const buildConfigName,
        OovStringRef const srcRootDir)
    {
    // @todo - This should probably check the oovcde-pkg.txt include crc to
    // see if it is different than what was used to generate the
    // oovcde-tmp-buildpkg.txt.  This cheat may be ok, but seems to
    // work a bit strange that sometimes the buildpkg file is not recreated
    // immediately after the oovcde-pkg.txt file is updated.  Also, the
    // analysis directory should probably be deleted if the include
    // paths have changed.
    OovString bldPkgFilename = Project::getBuildPackagesFilePath();
    if(FileIsFileOnDisk(bldPkgFilename))
        {
        if(FileStat::isOutputOld(bldPkgFilename, ProjectPackages::getFilename()))
            {
            printf("Deleting build config\n");
            FileDelete(bldPkgFilename.getStr());
            FileDelete(cfg.getBuildConfigFilename().c_str());
            }
        }

    cfg.setInitialConfig(buildConfigName,
        mCompFinder.getProjectBuildArgs().getExternalArgs(),
        mCompFinder.getProjectBuildArgs().getAllCrcCompileArgs(),
        mCompFinder.getProjectBuildArgs().getAllCrcLinkArgs());

    if(cfg.isConfigDifferent(buildConfigName,
            BuildConfig::CT_ExtPathArgsCrc))
        {
        // This is for the -ER switch.
        for(auto const &arg : mCompFinder.getProjectBuildArgs().getExternalArgs())
            {
            printf("Scanning %s\n", &arg[3]);
            fflush(stdout);
            mCompFinder.scanExternalProject(&arg[3]);
            }
        for(auto const &pkg : mCompFinder.getProjectBuildArgs().getProjectPackages().
            getPackages())
            {
            if(!mCompFinder.doesBuildPackageExist(pkg.getPkgName()))
                {
                if(pkg.needDirScan())
                    {
                    printf("Scanning %s\n", pkg.getPkgName().getStr());
                    fflush(stdout);
                    mCompFinder.scanExternalProject(pkg.getRootDir(), &pkg);
                    }
                else
                    {
                    mCompFinder.addBuildPackage(pkg);
                    }
                }
            }
        }
    srcFileParser sfp(mCompFinder);
    printf("Scanning %s\n", srcRootDir.getStr());
    mCompFinder.scanProject();
    fflush(stdout);

    cfg.setProjectConfig(mCompFinder.getScannedInfo().getProjectIncludeDirs());
    // Is anything different in the current build configuration?
    if(cfg.isAnyConfigDifferent(buildConfigName))
        {
        // Find CRC's that are not used by any build configuration.
        OovStringVec unusedCrcs;
        OovStringVec deleteDirs;
        bool deleteAnalysis = false;
        cfg.getUnusedCrcs(unusedCrcs);
        for(auto const &crcStr : unusedCrcs)
            {
            deleteDirs.push_back(cfg.getAnalysisPathUsingCRC(crcStr));
            }
        if(cfg.isConfigDifferent(buildConfigName,
            BuildConfig::CT_ExtPathArgsCrc) ||
            cfg.isConfigDifferent(buildConfigName,
            BuildConfig::CT_ProjPathArgsCrc) ||
            cfg.isConfigDifferent(buildConfigName,
            BuildConfig::CT_OtherArgsCrc))
            {
            deleteAnalysis = true;
            if(procMode != PM_Analyze)
                {
                deleteDirs.push_back(Project::getIntermediateDir(buildConfigName));
                }
            }
        else    // Must be only link arguments.
            {
            deleteDirs.push_back(Project::getOutputDir(buildConfigName));
            }
        cfg.saveConfig(buildConfigName);
        // This must be after the saveConfig, because it uses the new CRC's
        // to delete the new analysis path.
        OovString analysisPath = cfg.getAnalysisPath();
        if(deleteAnalysis)
            {
            deleteDirs.push_back(analysisPath);
            }
        for(auto const &dir : deleteDirs)
            {
            printf("Deleting %s\n", dir.getStr());
            if(dir.length() > 5)        // Reduce chance of deleting root
                {
                recursiveDeleteDir(dir);
                }
            }
        fflush(stdout);

        if(deleteAnalysis)
            {
            // Windows returns before the directory is actually deleted.
            FileWaitForDirDeleted(analysisPath);
            }
        }

    OovString analysisPath = cfg.getAnalysisPath();
    FileEnsurePathExists(analysisPath);

    FilePath compFn(analysisPath, FP_Dir);
    compFn.appendFile(Project::getAnalysisComponentsFilename());
    mCompFinder.saveProject(compFn);

    sfp.analyzeSrcFiles(srcRootDir, analysisPath);
    }

// Example test args:
//      ../examples/staticlib ../examples/staticlib-oovcde -bld-Debug
int main(int argc, char const * const argv[])
    {
    char const *oovProjDir = NULL;
    OovBuilder builder;
    std::string buildConfigName = BuildConfigAnalysis;  // analysis is default.
    eProcessModes processMode = PM_Analyze;
    bool verbose = false;
    bool success = (argc >= 2);
    if(success)
        {
        oovProjDir = argv[1];
        for(int i=2; i<argc; i++)
            {
            std::string testArg = argv[i];
            if(testArg.find("-cfg-", 0, 5) == 0)
                {
                buildConfigName = testArg.substr(5);    // skip "-cfg-"
                }
            else if(testArg.find("-mode-", 0, 5) == 0)
                {
                OovString mode;
                mode.setLowerCase(testArg.substr(6));
                if(mode.find("analyze") == 0)
                    {
                    processMode = PM_Analyze;
                    }
                else if(mode.find("cov-instr") == 0)
                    {
                    processMode = PM_CovInstr;
                    }
                else if(mode.find("cov-build") == 0)
                    {
                    processMode = PM_CovBuild;
                    }
                else if(mode.find("cov-stat") == 0)
                    {
                    processMode = PM_CovStats;
                    }
                else if(mode.find("build") == 0)
                    {
                    processMode = PM_Build;
                    }
                }
            else if(testArg.compare("-bv") == 0)
                {
                verbose = true;
                }
            }
        }
    else
        {
        fprintf(stderr, "OovBuilder version %s\n", OOV_VERSION);
            fprintf(stderr, "Command format:    <oovProjectDir> [args]...\n");
            fprintf(stderr, "  The oovProjectDir can have exclusion paths using <oovProjectDir>!<path> \n");
            fprintf(stderr, "  The args are:\n");
            fprintf(stderr, "    -cfg-<buildconfig>\n");
            fprintf(stderr, "               buildconfig is Debug, Release or any custom name\n");
            fprintf(stderr, "    -mode-<analyze|build|cov-instr|cov-build|cov-stats>\n");
            fprintf(stderr, "               Analyze, build or coverage\n");
            fprintf(stderr, "    -bv         builder verbose - OovBuilder.txt file\n");
        }

    if(success)
        {
        builder.build(processMode, oovProjDir, buildConfigName, verbose);
        }
    return 0;
    }

