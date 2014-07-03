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
#include <stdio.h>



// Example test args:
//	../examples/staticlib ../examples/staticlib-oovcde -bld-Debug
int main(int argc, char const * const argv[])
{
    ComponentFinder compFinder;
    char const *oovProjDir = NULL;
    std::string buildConfigName = BuildConfigAnalysis;	// analysis is default.
    bool verbose = false;
    bool success = (argc >= 2);
    if(success)
	{
	oovProjDir = argv[1];
	for(int i=2; i<argc; i++)
	    {
	    std::string testArg = argv[i];
	    if(testArg.find("-bld-", 0, 5) == 0)
		{
		buildConfigName = testArg.substr(5);	// skip "-bld-"
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
        fprintf(stderr, "    -bld-<mode>   mode is Debug or Release, no flag is build analysis docs\n");
        fprintf(stderr, "    -bv           builder verbose - OovBuilder.txt file\n");
	}

    if(success)
	{
	success = compFinder.readProject(oovProjDir, buildConfigName.c_str());
	if(success)
	    {
	    if(compFinder.getProject().getVerbose() || verbose)
		{
		sLog.open(oovProjDir);
		}
	    }
	else
	    {
	    fprintf(stderr, "oovBuilder: Oov project file must exist in %s\n",
		    oovProjDir);
	    }
	}
    if(success)
	{
	BuildConfigWriter cfg;
	cfg.setInitialConfig(buildConfigName.c_str(),
		compFinder.getProject().getExternalArgs(),
		compFinder.getProject().getAllCrcCompileArgs(),
		compFinder.getProject().getAllCrcLinkArgs());

	if(cfg.isConfigDifferent(buildConfigName.c_str(),
		BuildConfig::CT_ExtPathArgsCrc))
	    {
	    /*
	    for(auto const &arg : compFinder.getProject().getExternalArgs())
		{
		printf("Scanning %s\n", &arg[3]);
		compFinder.scanExternalProject(&arg[3]);
		}
		*/
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
	printf("Scanning %s\n", compFinder.getProject().getSrcRootDirectory().c_str());
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

	success = sfp.analyzeSrcFiles(
		compFinder.getProject().getSrcRootDirectory().c_str(),
		analysisPath.c_str());
	if(buildConfigName.compare(BuildConfigAnalysis) != 0)
	    {
	    std::string incDepsPath = cfg.getIncDepsFilePath(buildConfigName.c_str());
	    ComponentBuilder builder(compFinder);
#if(DEBUG_COMMANDS)
	    sLog.logProcess(compFinder.getProject().getSrcRootDirectory().c_str(),
		    Project::getIntermediateDir(buildConfigName.c_str()).c_str(), &argv[3], argc-3);
#endif
	    builder.build(compFinder.getProject().getSrcRootDirectory().c_str()
		    , incDepsPath.c_str(), buildConfigName.c_str());
	    }
	}
    return 0;
}

