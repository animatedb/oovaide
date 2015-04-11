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

static bool readProject(ComponentFinder &compFinder, OovStringRef const buildConfigName,
	OovStringRef const oovProjDir, bool verbose)
    {
    bool success = compFinder.readProject(oovProjDir, buildConfigName);
    if(success)
	{
	if(compFinder.getProject().getVerbose() || verbose)
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

static void analyze(ComponentFinder &compFinder, BuildConfigWriter &cfg,
	 OovStringRef const buildConfigName, OovStringRef const srcRootDir)
    {
    cfg.setInitialConfig(buildConfigName,
	    compFinder.getProject().getExternalArgs(),
	    compFinder.getProject().getAllCrcCompileArgs(),
	    compFinder.getProject().getAllCrcLinkArgs());

    if(cfg.isConfigDifferent(buildConfigName,
	    BuildConfig::CT_ExtPathArgsCrc))
	{
	// This is for the -ER switch.
	for(auto const &arg : compFinder.getProject().getExternalArgs())
	    {
	    printf("Scanning %s\n", &arg[3]);
	    compFinder.scanExternalProject(&arg[3]);
	    }
	for(auto const &pkg : compFinder.getProject().getProjectPackages().
		getPackages())
	    {
	    if(pkg.needDirScan())
		{
		printf("Scanning %s\n", pkg.getPkgName().getStr());
		/// @todo - only one ext root dir per package currently.
		if(pkg.getExtRefDirs().size() > 0)
		    {
		    if(pkg.getIncludeDirs().size() == 0)
			{
			compFinder.scanExternalProject(pkg.getExtRefDirs()[0], &pkg);
			}
		    else
			{
			compFinder.addBuildPackage(pkg);
			}
		    }
		}
	    else
		{
		compFinder.addBuildPackage(pkg);
		}
	    }
	}
    srcFileParser sfp(compFinder);
    printf("Scanning %s\n", srcRootDir.getStr());
    compFinder.scanProject();
    fflush(stdout);

    cfg.setProjectConfig(compFinder.getScannedInfo().getProjectIncludeDirs());
    // Is anything different in the current build configuration?
    if(cfg.isAnyConfigDifferent(buildConfigName))
	{
	// Find CRC's that are not used by any build configuration.
	OovStringVec unusedCrcs;
	OovStringVec deleteDirs;
	cfg.getUnusedCrcs(unusedCrcs);
	for(auto const &crcStr : unusedCrcs)
	    {
	    deleteDirs.push_back(cfg.getAnalysisPath(buildConfigName, crcStr));
	    }
	if(cfg.isConfigDifferent(buildConfigName,
		BuildConfig::CT_ExtPathArgsCrc) ||
	    cfg.isConfigDifferent(buildConfigName,
		BuildConfig::CT_ProjPathArgsCrc) ||
	    cfg.isConfigDifferent(buildConfigName,
		BuildConfig::CT_OtherArgsCrc))
	    {
	    deleteDirs.push_back(Project::getIntermediateDir(buildConfigName
		    ));
	    }
	else	// Must be only link arguments.
	    {
	    deleteDirs.push_back(Project::getOutputDir(buildConfigName));
	    }
	for(auto const &dir : deleteDirs)
	    {
	    printf("Deleting %s\n", dir.getStr());
	    if(dir.length() > 5)	// Reduce chance of deleting root
		recursiveDeleteDir(dir);
	    }
	fflush(stdout);
	cfg.saveConfig(buildConfigName);
	}

    OovString analysisPath = cfg.getAnalysisPath(buildConfigName);
    FileEnsurePathExists(analysisPath);

    FilePath compFn(analysisPath, FP_Dir);
    compFn.appendFile(Project::getAnalysisComponentsFilename());
    compFinder.saveProject(compFn);

    sfp.analyzeSrcFiles(srcRootDir, analysisPath);
    }

// Example test args:
//	../examples/staticlib ../examples/staticlib-oovcde -bld-Debug
int main(int argc, char const * const argv[])
    {
    ComponentFinder compFinder;
    char const *oovProjDir = NULL;
    std::string buildConfigName = BuildConfigAnalysis;	// analysis is default.
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
		buildConfigName = testArg.substr(5);	// skip "-cfg-"
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
	Project::setProjectDirectory(oovProjDir);
	if(processMode == PM_CovBuild)
	    {
	    success = makeCoverageBuildProject();
	    }
	}
    if(success)
	{
	success = readProject(compFinder, buildConfigName,
		Project::getProjectDirectory(), verbose);
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
	    analyze(compFinder, cfg, buildConfigName, Project::getSrcRootDirectory());

	    std::string configStr = "Configuration: ";
	    configStr += buildConfigName;
	    sVerboseDump.logProgress(configStr);
	    switch(processMode)
		{
		case PM_Analyze:
		    break;

		default:
		    {
		    std::string incDepsPath = cfg.getIncDepsFilePath(buildConfigName);
		    ComponentBuilder builder(compFinder);
		    builder.build(processMode, incDepsPath, buildConfigName);
		    }
		    break;
		}
	    }
	}
    return 0;
    }

