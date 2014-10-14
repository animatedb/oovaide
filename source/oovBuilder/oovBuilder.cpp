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

static bool readProject(ComponentFinder &compFinder, std::string const &buildConfigName,
	char const *oovProjDir, bool verbose)
    {
    bool success = compFinder.readProject(oovProjDir, buildConfigName.c_str());
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
		oovProjDir);
	}
    return success;
    }

static void analyze(ComponentFinder &compFinder, BuildConfigWriter &cfg,
	 std::string const &buildConfigName, std::string const &srcRootDir)
    {
    cfg.setInitialConfig(buildConfigName.c_str(),
	    compFinder.getProject().getExternalArgs(),
	    compFinder.getProject().getAllCrcCompileArgs(),
	    compFinder.getProject().getAllCrcLinkArgs());

    if(cfg.isConfigDifferent(buildConfigName.c_str(),
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
		printf("Scanning %s\n", pkg.getPkgName().c_str());
		/// @todo - only one ext root dir per package currently.
		if(pkg.getExtRefDirs().size() > 0)
		    compFinder.scanExternalProject(pkg.getExtRefDirs()[0].c_str(), &pkg);
		}
	    else
		{
		compFinder.addBuildPackage(pkg);
		}
	    }
	}
    srcFileParser sfp(compFinder);
    printf("Scanning %s\n", srcRootDir.c_str());
    compFinder.scanProject();
    fflush(stdout);

    cfg.setProjectConfig(compFinder.getScannedInfo().getProjectIncludeDirs());
    // Is anything different in the current build configuration?
    if(cfg.isAnyConfigDifferent(buildConfigName.c_str()))
	{
	// Find CRC's that are not used by any build configuration.
	std::vector<std::string> unusedCrcs;
	std::vector<std::string> deleteDirs;
	cfg.getUnusedCrcs(unusedCrcs);
	for(auto const &crcStr : unusedCrcs)
	    {
	    deleteDirs.push_back(cfg.getAnalysisPath(buildConfigName.c_str(),
		    crcStr.c_str()).c_str());
	    }
	if(cfg.isConfigDifferent(buildConfigName.c_str(),
		BuildConfig::CT_ExtPathArgsCrc) ||
	    cfg.isConfigDifferent(buildConfigName.c_str(),
		BuildConfig::CT_ProjPathArgsCrc) ||
	    cfg.isConfigDifferent(buildConfigName.c_str(),
		BuildConfig::CT_OtherArgsCrc))
	    {
	    deleteDirs.push_back(Project::getIntermediateDir(buildConfigName.c_str()
		    ).c_str());
	    }
	else	// Must be only link arguments.
	    {
	    deleteDirs.push_back(Project::getOutputDir(buildConfigName.c_str()
		    ).c_str());
	    }
	for(auto const &dir : deleteDirs)
	    {
	    printf("Deleting %s\n", dir.c_str());
	    if(dir.length() > 5)	// Reduce chance of deleting root
		recursiveDeleteDir(dir.c_str());
	    }
	fflush(stdout);
	cfg.saveConfig(buildConfigName.c_str());
	}

    std::string analysisPath = cfg.getAnalysisPath(buildConfigName.c_str());
    ensurePathExists(analysisPath.c_str());

    FilePath compFn(analysisPath, FP_Dir);
    compFn.appendFile(Project::getAnalysisComponentsFilename());
    compFinder.saveProject(compFn.c_str());

    sfp.analyzeSrcFiles(srcRootDir.c_str(), analysisPath.c_str());
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
		mode.setLowerCase(testArg.substr(6).c_str());
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
		Project::getProjectDirectory().c_str(), verbose);
	}
    if(success)
	{
	if(processMode == PM_CovStats)
	    {
	    if(makeCoverageStats())
		{
		printf("Coverage output: %s",
			Project::getCoverageProjectDirectory().c_str());
		}
	    }
	else
	    {
	    BuildConfigWriter cfg;
	    analyze(compFinder, cfg, buildConfigName, Project::getSrcRootDirectory());

	    std::string configStr = "Configuration: ";
	    configStr += buildConfigName;
	    sVerboseDump.logProgress(configStr.c_str());
	    switch(processMode)
		{
		case PM_Analyze:
		    break;

		default:
		    {
		    std::string incDepsPath = cfg.getIncDepsFilePath(buildConfigName.c_str());
		    ComponentBuilder builder(compFinder);
		    builder.build(processMode, incDepsPath.c_str(), buildConfigName.c_str());
		    }
		    break;
		}
	    }
	}
    return 0;
    }

